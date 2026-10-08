#include "HostUndoTransaction.h"

#include "AddressTable.h"
#include "CSPMOD.h"
#include "LayerObject.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string_view>
#include <vector>

namespace
{
void Diag(const char* format, ...)
{
    char path[MAX_PATH]{};
    if (GetTempPathA(MAX_PATH, path) == 0)
        return;
    strcat_s(path, "CSPMOD-undo.log");

    FILE* file = nullptr;
    if (fopen_s(&file, path, "a") != 0 || file == nullptr)
        return;

    SYSTEMTIME now{};
    GetLocalTime(&now);
    fprintf(file, "[%02d:%02d:%02d.%03d] ", now.wHour, now.wMinute,
        now.wSecond, now.wMilliseconds);

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);
    fputc('\n', file);
    fclose(file);
}

struct HostLayout
{
    const char* version;
    std::uintptr_t beginTemporary;
    std::uintptr_t endTemporary;
    std::uintptr_t getCurrentStack;
    std::uintptr_t canUndoModel;
    std::uintptr_t undo;
    std::uintptr_t redo;
    std::uintptr_t stackStepRedo;
    std::uintptr_t makeMulti;
    std::uintptr_t multiAdd;
    std::uintptr_t multiCount;
    std::uintptr_t setName;
    std::uintptr_t doCommand;
    std::uintptr_t makeLockCommand;
    std::uintptr_t stringCtor;
    std::uintptr_t stringDtor;
    std::uintptr_t releaseShared;
};

const HostLayout kHostLayouts[] = {
    {
        "3.0.7",
        0x1362BD0,
        0x1363450,
        0x1363610,
        0x13631D0,
        0x1365030,
        0x1364B00,
        0x14C3410,
        0x135D720,
        0x135D680,
        0x135D900,
        0x1365990,
        0x1364680,
        0x21498C0,
        0x247D4A0,
        0x247D530,
        0x0166D30,
    },
};

const std::uintptr_t kUndoModelVtables3_0_7[] = {
    0x374E610, 0x374EEB0, 0x3775B38, 0x3780F78, 0x378F0A0,
    0x3791118, 0x37B2788, 0x37D6B80, 0x37DA3F0, 0x37DEA08,
    0x37EDFB8, 0x3803B10, 0x3804260, 0x3804388, 0x3804478,
};

struct HostSharedPtr
{
    void* object = nullptr;
    void* control = nullptr;
};

struct alignas(8) HostString
{
    std::byte storage[0x30]{};
};

using BeginTemporaryFn = bool(__fastcall*)(void*, const HostString*);
using EndTemporaryFn = void(__fastcall*)(void*);
using GetCurrentStackFn = HostSharedPtr* (__fastcall*)(void*, HostSharedPtr*);
using CanUndoModelFn = bool(__fastcall*)(void*);
using UndoFn = void(__fastcall*)(void*);
using RedoFn = void(__fastcall*)(void*);
using StackStepRedoFn = HostSharedPtr* (__fastcall*)(void*, HostSharedPtr*);
using MakeMultiFn = HostSharedPtr* (__fastcall*)(HostSharedPtr*, bool);
using MultiAddFn = void(__fastcall*)(void*, HostSharedPtr*);
using MultiCountFn = std::int64_t(__fastcall*)(const void*);
using SetNameFn = HostString* (__fastcall*)(void*, const HostString*);
using DoCommandFn = void(__fastcall*)(void*, HostSharedPtr*);
using MakePropertyCommandFn = HostSharedPtr* (__fastcall*)(HostSharedPtr*, HostSharedPtr*);
using HostStringCtorFn = HostString* (__fastcall*)(HostString*, const wchar_t*, std::int32_t);
using HostStringDtorFn = void(__fastcall*)(HostString*);
using ReleaseSharedFn = void(__fastcall*)(HostSharedPtr*);

struct HostApi
{
    BeginTemporaryFn beginTemporary = nullptr;
    EndTemporaryFn endTemporary = nullptr;
    GetCurrentStackFn getCurrentStack = nullptr;
    CanUndoModelFn canUndoModel = nullptr;
    UndoFn undo = nullptr;
    RedoFn redo = nullptr;
    StackStepRedoFn stackStepRedo = nullptr;
    MakeMultiFn makeMulti = nullptr;
    MultiAddFn multiAdd = nullptr;
    MultiCountFn multiCount = nullptr;
    SetNameFn setName = nullptr;
    DoCommandFn doCommand = nullptr;
    MakePropertyCommandFn makeLockCommand = nullptr;
    HostStringCtorFn stringCtor = nullptr;
    HostStringDtorFn stringDtor = nullptr;
    ReleaseSharedFn releaseShared = nullptr;
    const std::uintptr_t* undoVtables = nullptr;
    std::size_t undoVtableCount = 0;
    bool ready = false;
};

HostApi g_api;

template<typename T>
T AtRva(std::uintptr_t rva)
{
    return reinterpret_cast<T>(CSPMOD::GetBaseAddr() + rva);
}

bool HasBytes(std::uintptr_t rva, const std::initializer_list<std::uint8_t>& bytes)
{
    const auto* address = AtRva<const std::uint8_t*>(rva);
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0 || mbi.State != MEM_COMMIT)
        return false;

    const DWORD protect = mbi.Protect & 0xFF;
    const bool readable = protect == PAGE_READONLY || protect == PAGE_READWRITE ||
        protect == PAGE_EXECUTE_READ || protect == PAGE_EXECUTE_READWRITE ||
        protect == PAGE_WRITECOPY || protect == PAGE_EXECUTE_WRITECOPY;
    if (!readable)
        return false;

    const auto start = reinterpret_cast<std::uintptr_t>(address);
    const auto regionEnd = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return start + bytes.size() <= regionEnd &&
        std::equal(bytes.begin(), bytes.end(), address);
}

bool ResolveHostApi()
{
    if (g_api.ready)
        return true;
    if (sizeof(void*) != 8 || CSPMOD::GetBaseAddr() == 0)
        return false;

    const std::string_view version = AddressTable::GetCSPVersion();
    const HostLayout* layout = nullptr;
    for (const HostLayout& candidate : kHostLayouts)
    {
        if (version == candidate.version)
        {
            layout = &candidate;
            break;
        }
    }
    if (layout == nullptr)
        return false;

    if (!HasBytes(layout->beginTemporary, { 0x48, 0x89, 0x5C, 0x24, 0x10 }) ||
        !HasBytes(layout->endTemporary, { 0x48, 0x89, 0x5C, 0x24, 0x10 }) ||
        !HasBytes(layout->getCurrentStack, { 0x48, 0x89, 0x5C, 0x24, 0x08 }) ||
        !HasBytes(layout->canUndoModel, { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x18 }) ||
        !HasBytes(layout->undo, { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x57 }) ||
        !HasBytes(layout->redo, { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83, 0xEC, 0x60 }) ||
        !HasBytes(layout->stackStepRedo, { 0x48, 0x89, 0x5C, 0x24, 0x18, 0x56 }) ||
        !HasBytes(layout->makeMulti, { 0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C, 0x24, 0x20 }) ||
        !HasBytes(layout->multiAdd, { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x54, 0x24, 0x10 }) ||
        !HasBytes(layout->multiCount, { 0x48, 0x8B, 0x41, 0x50, 0x48, 0x2B, 0x41, 0x48 }) ||
        !HasBytes(layout->setName, { 0x48, 0x83, 0xC1, 0x10, 0xE9 }) ||
        !HasBytes(layout->doCommand, { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x54, 0x24, 0x10, 0x57 }) ||
        !HasBytes(layout->makeLockCommand, { 0x4C, 0x8B, 0xDC, 0x49, 0x89, 0x5B, 0x18 }) ||
        !HasBytes(layout->stringCtor, { 0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18 }) ||
        !HasBytes(layout->stringDtor, { 0x40, 0x53, 0x48, 0x83, 0xEC, 0x20 }) ||
        !HasBytes(layout->releaseShared, { 0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0x59, 0x08 }))
    {
        SDL_Log("Apply Effects Layer: host undo signatures mismatch");
        return false;
    }

    g_api.beginTemporary = AtRva<BeginTemporaryFn>(layout->beginTemporary);
    g_api.endTemporary = AtRva<EndTemporaryFn>(layout->endTemporary);
    g_api.getCurrentStack = AtRva<GetCurrentStackFn>(layout->getCurrentStack);
    g_api.canUndoModel = AtRva<CanUndoModelFn>(layout->canUndoModel);
    g_api.undo = AtRva<UndoFn>(layout->undo);
    g_api.redo = AtRva<RedoFn>(layout->redo);
    g_api.stackStepRedo = AtRva<StackStepRedoFn>(layout->stackStepRedo);
    g_api.makeMulti = AtRva<MakeMultiFn>(layout->makeMulti);
    g_api.multiAdd = AtRva<MultiAddFn>(layout->multiAdd);
    g_api.multiCount = AtRva<MultiCountFn>(layout->multiCount);
    g_api.setName = AtRva<SetNameFn>(layout->setName);
    g_api.doCommand = AtRva<DoCommandFn>(layout->doCommand);
    g_api.makeLockCommand = AtRva<MakePropertyCommandFn>(layout->makeLockCommand);
    g_api.stringCtor = AtRva<HostStringCtorFn>(layout->stringCtor);
    g_api.stringDtor = AtRva<HostStringDtorFn>(layout->stringDtor);
    g_api.releaseShared = AtRva<ReleaseSharedFn>(layout->releaseShared);
    g_api.undoVtables = kUndoModelVtables3_0_7;
    g_api.undoVtableCount = sizeof(kUndoModelVtables3_0_7) / sizeof(kUndoModelVtables3_0_7[0]);
    g_api.ready = true;
    return true;
}

void AddRef(const HostSharedPtr& value)
{
    if (value.control != nullptr)
    {
        InterlockedIncrement(reinterpret_cast<volatile LONG*>(
            static_cast<std::byte*>(value.control) + 8));
    }
}

void Release(HostSharedPtr& value)
{
    if (value.control != nullptr)
        g_api.releaseShared(&value);
    value = {};
}

HostSharedPtr CopyOf(const HostSharedPtr& value)
{
    HostSharedPtr copy = value;
    AddRef(copy);
    return copy;
}

bool MakeHostString(const std::wstring& text, HostString& result)
{
    if (text.size() > static_cast<std::size_t>(INT32_MAX))
        return false;
    return g_api.stringCtor(&result, text.c_str(),
        static_cast<std::int32_t>(text.size())) != nullptr;
}

bool IsKnownUndoModel(void* candidate)
{
    if (candidate == nullptr || !CSPMOD::IsPtrValid(candidate, sizeof(void*)))
        return false;
    const auto vtable = *reinterpret_cast<const std::uintptr_t*>(candidate);
    const auto base = CSPMOD::GetBaseAddr();
    for (std::size_t i = 0; i < g_api.undoVtableCount; ++i)
    {
        if (vtable == base + g_api.undoVtables[i])
            return true;
    }
    return false;
}
}

bool HostUndoTransaction::Available()
{
    return ResolveHostApi();
}

HostUndoTransaction::HostUndoTransaction(const wchar_t* historyName)
    : historyName_(historyName != nullptr ? historyName : L"")
{
    if (!ResolveHostApi() || historyName_.empty())
        return;

    undoModel_ = LayerObject::GetUndoModel();
    if (!IsKnownUndoModel(undoModel_))
        return;

    HostString hostName;
    if (!MakeHostString(historyName_, hostName))
        return;
    const bool begun = g_api.beginTemporary(undoModel_, &hostName);
    g_api.stringDtor(&hostName);
    if (!begun)
        return;

    HostSharedPtr stack;
    g_api.getCurrentStack(undoModel_, &stack);
    temporaryStackObject_ = stack.object;
    temporaryStackControl_ = stack.control;
    if (temporaryStackObject_ == nullptr)
    {
        Release(stack);
        g_api.endTemporary(undoModel_);
        return;
    }

    // Keep the reference returned by GetCurrentStack until commit/rollback.
    stack = {};
    active_ = true;
}

HostUndoTransaction::~HostUndoTransaction()
{
    Rollback();
}

namespace
{
bool FindLayerRecursive(LayerObject parent, uintptr_t target, LayerObject& result)
{
    for (int index = 0; index < parent.GetChildLayerCount(); ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr())
            continue;
        if (child.ptr == target)
        {
            result = child;
            return true;
        }
        if (child.IsGroup() && FindLayerRecursive(child, target, result))
            return true;
    }
    return false;
}

bool FindLayer(uintptr_t target, LayerObject& result)
{
    LayerObject root = LayerObject::GetCurrentRoot();
    return target != 0 && root.CheckPtr() && FindLayerRecursive(root, target, result);
}

bool FindParentRecursive(LayerObject parent, uintptr_t target, LayerObject& result)
{
    for (int index = 0; index < parent.GetChildLayerCount(); ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr())
            continue;
        if (child.ptr == target)
        {
            result = parent;
            return true;
        }
        if (child.IsGroup() && FindParentRecursive(child, target, result))
            return true;
    }
    return false;
}

bool FindParent(uintptr_t target, LayerObject& result)
{
    LayerObject root = LayerObject::GetCurrentRoot();
    return target != 0 && root.CheckPtr() && FindParentRecursive(root, target, result);
}

void GetDirectChildPointers(LayerObject parent, std::vector<uintptr_t>& result)
{
    result.clear();
    for (int index = 0; index < parent.GetChildLayerCount(); ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (child.CheckPtr())
            result.push_back(child.ptr);
    }
}

struct VisibilityRecoveryPlan
{
    std::vector<HostUndoTransaction::VisibilityBackup> backups;
    uintptr_t resultPtr = 0;
    uintptr_t resultParentPtr = 0;
    bool resultIsGroup = false;
    bool resultIsAdjust = false;
    bool resultWasUndone = false;
    bool active = false;
};

VisibilityRecoveryPlan g_visibilityRecovery;

using HostUndoFn = void(__fastcall*)(void*);
HostUndoFn g_origUndo = nullptr;
HostUndoFn g_origRedo = nullptr;

void RestoreOriginalVisibility()
{
    for (const HostUndoTransaction::VisibilityBackup& backup : g_visibilityRecovery.backups)
    {
        LayerObject layer;
        if (FindLayer(backup.layerPtr, layer))
            layer.SetVisibility(backup.visible);
    }
}

void HideOriginalsForAppliedAction()
{
    for (const HostUndoTransaction::VisibilityBackup& backup : g_visibilityRecovery.backups)
    {
        LayerObject layer;
        if (FindLayer(backup.layerPtr, layer))
            layer.SetVisibility(false);
    }
}

void __fastcall Hook_Undo(void* undoModel)
{
    LayerObject resultBeforeUndo;
    const bool resultExisted = g_visibilityRecovery.active &&
        FindLayer(g_visibilityRecovery.resultPtr, resultBeforeUndo);
    g_origUndo(undoModel);

    if (!resultExisted || !g_visibilityRecovery.active)
        return;

    LayerObject result;
    if (!FindLayer(g_visibilityRecovery.resultPtr, result))
    {
        // Only the composite action removes its result layer. Undoing a later
        // brush stroke or a manual eye-toggle leaves it in place, so neither
        // operation is allowed to overwrite the user's visibility choice.
        RestoreOriginalVisibility();
        g_visibilityRecovery.resultWasUndone = true;
        Diag("Visibility recovery restored %zu original layer(s)",
            g_visibilityRecovery.backups.size());
    }
}

void __fastcall Hook_Redo(void* undoModel)
{
    std::vector<uintptr_t> beforeChildren;
    LayerObject parent;
    const bool watchRedo = g_visibilityRecovery.active &&
        g_visibilityRecovery.resultWasUndone &&
        FindLayer(g_visibilityRecovery.resultParentPtr, parent);
    if (watchRedo)
        GetDirectChildPointers(parent, beforeChildren);

    g_origRedo(undoModel);

    if (!watchRedo)
        return;

    if (!FindLayer(g_visibilityRecovery.resultParentPtr, parent))
        return;
    std::vector<uintptr_t> afterChildren;
    GetDirectChildPointers(parent, afterChildren);
    for (uintptr_t ptr : afterChildren)
    {
        if (std::find(beforeChildren.begin(), beforeChildren.end(), ptr) != beforeChildren.end())
            continue;
        LayerObject candidate;
        if (!FindLayer(ptr, candidate) ||
            candidate.IsGroup() != g_visibilityRecovery.resultIsGroup ||
            candidate.IsAdjustLayer() != g_visibilityRecovery.resultIsAdjust)
        {
            continue;
        }

        g_visibilityRecovery.resultPtr = ptr;
        HideOriginalsForAppliedAction();
        g_visibilityRecovery.resultWasUndone = false;
        Diag("Visibility recovery reapplied to %zu original layer(s)",
            g_visibilityRecovery.backups.size());
        break;
    }
}

void EnsureUndoHookInstalled()
{
    if (g_origUndo != nullptr && g_origRedo != nullptr)
        return;
    if (!ResolveHostApi() || g_api.undo == nullptr || g_api.redo == nullptr)
        return;

    CSPMOD::Hook(reinterpret_cast<void*>(g_api.undo),
                 reinterpret_cast<void*>(Hook_Undo),
                 reinterpret_cast<void**>(&g_origUndo));
    CSPMOD::Hook(reinterpret_cast<void*>(g_api.redo),
                 reinterpret_cast<void*>(Hook_Redo),
                 reinterpret_cast<void**>(&g_origRedo));
}
}

void HostUndoTransaction::TrackVisibility(LayerObject layer)
{
    if (!active_ || !layer.CheckPtr())
        return;
    visibilityBackups_.push_back({ layer.ptr, layer.IsVisible() });
}

void HostUndoTransaction::ActivateVisibilityRecovery(LayerObject committedResult)
{
    if (visibilityBackups_.empty() || !committedResult.CheckPtr())
        return;

    LayerObject parent;
    if (!FindParent(committedResult.ptr, parent))
        return;

    EnsureUndoHookInstalled();
    if (g_origUndo == nullptr || g_origRedo == nullptr)
        return;

    g_visibilityRecovery.backups = visibilityBackups_;
    g_visibilityRecovery.resultPtr = committedResult.ptr;
    g_visibilityRecovery.resultParentPtr = parent.ptr;
    g_visibilityRecovery.resultIsGroup = committedResult.IsGroup();
    g_visibilityRecovery.resultIsAdjust = committedResult.IsAdjustLayer();
    g_visibilityRecovery.resultWasUndone = false;
    g_visibilityRecovery.active = true;
    Diag("Visibility recovery armed: originals=%zu", visibilityBackups_.size());
}

bool HostUndoTransaction::TrackLockChange(LayerObject layer)
{
    if (!active_)
        return true;
    if (!layer.CheckPtr() || layer.ptr_ref == 0)
        return false;

    HostSharedPtr target{
        reinterpret_cast<void*>(layer.ptr),
        reinterpret_cast<void*>(layer.ptr_ref)
    };
    HostSharedPtr command;
    g_api.makeLockCommand(&command, &target);
    if (command.object == nullptr || command.control == nullptr)
    {
        Release(command);
        return false;
    }

    propertyCommands_.push_back({ command.object, command.control });
    command = {};
    return true;
}

bool HostUndoTransaction::Rollback()
{
    if (!active_)
        return true;

    bool complete = true;
    std::size_t guard = 0;
    while (g_api.canUndoModel(undoModel_))
    {
        if (++guard > 100000)
        {
            complete = false;
            break;
        }
        g_api.undo(undoModel_);
    }
    g_api.endTemporary(undoModel_);

    HostSharedPtr stack{ temporaryStackObject_, temporaryStackControl_ };
    Release(stack);
    temporaryStackObject_ = nullptr;
    temporaryStackControl_ = nullptr;
    for (SharedPtr& tracked : propertyCommands_)
    {
        HostSharedPtr command{ tracked.object, tracked.control };
        Release(command);
        tracked = {};
    }
    propertyCommands_.clear();
    visibilityBackups_.clear();
    active_ = false;
    return complete;
}

bool HostUndoTransaction::Commit()
{
    if (!active_)
        return true;

    auto fail = [&]()
    {
        Diag("Commit failed; restoring the document");
        return Rollback();
    };

    // Put the document back to its pre-action state first: the composite
    // command replays every recorded step, so it has to start from the state
    // those steps originally began in.
    std::size_t undoCount = 0;
    while (g_api.canUndoModel(undoModel_))
    {
        if (++undoCount > 100000)
            return fail();
        g_api.undo(undoModel_);
    }
    if (undoCount == 0)
        return fail();

    HostSharedPtr multi;
    g_api.makeMulti(&multi, false);
    if (multi.object == nullptr)
    {
        Release(multi);
        return fail();
    }

    HostString hostName;
    if (!MakeHostString(historyName_, hostName))
    {
        Release(multi);
        return fail();
    }
    g_api.setName(multi.object, &hostName);
    g_api.stringDtor(&hostName);

    // The document is back at its starting state, so the temporary stack cursor
    // sits on the redo side.  Read the commands from there, oldest first, and
    // collect them into one composite command.
    std::size_t commandCount = 0;
    for (;;)
    {
        if (++commandCount > 100000)
        {
            Release(multi);
            return fail();
        }

        HostSharedPtr command;
        g_api.stackStepRedo(temporaryStackObject_, &command);
        if (command.object == nullptr)
        {
            --commandCount;
            break;
        }

        HostSharedPtr argument = CopyOf(command);
        g_api.multiAdd(multi.object, &argument);
        argument = {};
        Release(command);
    }

    if (commandCount == 0)
    {
        Release(multi);
        return fail();
    }
    const std::size_t structuralCommandCount = commandCount;

    // Lock is a direct layer-memory write in the original plugin, so append
    // its native property command explicitly. Visibility is restored only
    // when this composite action itself is undone (see ActivateVisibilityRecovery).
    for (SharedPtr& tracked : propertyCommands_)
    {
        HostSharedPtr command{ tracked.object, tracked.control };
        HostSharedPtr argument = CopyOf(command);
        g_api.multiAdd(multi.object, &argument);
        argument = {};
        Release(command);
        tracked = {};
        ++commandCount;
    }
    propertyCommands_.clear();

    if (structuralCommandCount != undoCount ||
        g_api.multiCount(multi.object) != static_cast<std::int64_t>(commandCount))
    {
        Diag("Commit count mismatch: undo=%zu structural=%zu total=%zu multi=%lld",
            undoCount, structuralCommandCount, commandCount,
            static_cast<long long>(g_api.multiCount(multi.object)));
        Release(multi);
        return fail();
    }

    // EndTemporary removes the temporary stack.  The composite command holds
    // its own references, so the recorded commands survive it.
    g_api.endTemporary(undoModel_);
    HostSharedPtr stack{ temporaryStackObject_, temporaryStackControl_ };
    Release(stack);
    temporaryStackObject_ = nullptr;
    temporaryStackControl_ = nullptr;
    active_ = false;

    // Run the composite: it re-applies every step as a single history entry.
    HostSharedPtr argument = CopyOf(multi);
    g_api.doCommand(undoModel_, &argument);
    argument = {};
    Release(multi);

    // DoCommand records the composite as current, but this host build leaves
    // the document at the state from immediately before that entry.  The same
    // native Undo/Redo pair the History palette uses synchronizes the document
    // with the selected history entry without changing the visible history.
    g_api.undo(undoModel_);
    g_api.redo(undoModel_);

    Diag("Commit succeeded: %zu commands grouped", commandCount);
    return true;
}
