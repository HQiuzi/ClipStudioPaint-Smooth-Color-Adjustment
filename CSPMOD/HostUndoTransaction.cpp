#include "HostUndoTransaction.h"

#include "AddressTable.h"
#include "CSPMOD.h"
#include "CspData.h"
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

bool FindLayerRecursive(LayerObject parent, uintptr_t target, LayerObject& result)
{
    const int childCount = parent.GetChildLayerCount();
    for (int index = 0; index < childCount; ++index)
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

struct PresentationRecoveryPlan
{
    std::vector<HostUndoTransaction::LayerPresentationState> originalStates;
    // Every direct child other than the generated result.  These identities
    // let Redo recognize this exact composite without reacting to brush or
    // ordinary layer commands.
    std::vector<uintptr_t> originalDirectChildren;
    uintptr_t resultPtr = 0;
    uintptr_t parentPtr = 0;
    bool resultIsGroup = false;
    bool resultIsAdjust = false;
    bool resultWasUndone = false;
    bool resultPresentBeforeUndo = false;
    bool active = false;
};

std::vector<PresentationRecoveryPlan> g_presentationRecoveryPlans;

using HostUndoFn = void(__fastcall*)(void*);
HostUndoFn g_originalUndo = nullptr;
HostUndoFn g_originalRedo = nullptr;
bool g_presentationHooksUnavailable = false;

bool RestoreOriginalPresentation(const PresentationRecoveryPlan& plan)
{
    std::size_t restored = 0;
    for (const HostUndoTransaction::LayerPresentationState& state : plan.originalStates)
    {
        LayerObject layer;
        if (!FindLayer(state.layerPtr, layer))
            continue;
        // Lock must be cleared before changing a locked layer's state.
        layer.SetLocked(false);
        layer.SetVisibility(state.visible);
        layer.SetLocked(state.locked);
        ++restored;
    }
    Diag("Presentation recovery restored %zu/%zu original layer(s)", restored,
        plan.originalStates.size());
    return restored != 0;
}

bool ApplyCompositePresentation(const PresentationRecoveryPlan& plan)
{
    std::size_t applied = 0;
    for (const HostUndoTransaction::LayerPresentationState& state : plan.originalStates)
    {
        LayerObject layer;
        if (!FindLayer(state.layerPtr, layer))
            continue;
        layer.SetLocked(false);
        layer.SetVisibility(false);
        layer.SetLocked(true);
        ++applied;
    }
    Diag("Presentation recovery reapplied %zu/%zu original layer(s)", applied,
        plan.originalStates.size());
    return applied != 0;
}

void QueueHostRedraw()
{
    const HWND window = reinterpret_cast<HWND>(CspData::GetNativeWindowHandle());
    if (::IsWindow(window))
        ::InvalidateRect(window, nullptr, FALSE);
}

bool IsOriginalDirectChild(const PresentationRecoveryPlan& plan, uintptr_t ptr)
{
    return std::find(plan.originalDirectChildren.begin(),
        plan.originalDirectChildren.end(), ptr) != plan.originalDirectChildren.end();
}

void __fastcall HookUndo(void* undoModel)
{
    // No allocations or host commands here: this is an observer around the
    // host's own undo path, and writes only raw presentation flags afterward.
    for (PresentationRecoveryPlan& plan : g_presentationRecoveryPlans)
    {
        LayerObject result;
        plan.resultPresentBeforeUndo = plan.active &&
            FindLayer(plan.resultPtr, result);
    }

    g_originalUndo(undoModel);

    for (PresentationRecoveryPlan& plan : g_presentationRecoveryPlans)
    {
        if (!plan.resultPresentBeforeUndo || !plan.active)
            continue;

        LayerObject result;
        if (FindLayer(plan.resultPtr, result))
            continue;

        // A brush stroke, a new layer, or a user eye-toggle leaves this
        // composite result in the tree.  Only its removal is the action we
        // are tracking, so those unrelated Undos cannot overwrite the user's
        // presentation choices.
        if (RestoreOriginalPresentation(plan))
            QueueHostRedraw();
        plan.resultWasUndone = true;
    }
}

void __fastcall HookRedo(void* undoModel)
{
    g_originalRedo(undoModel);

    // Redo recreates the result with a new host pointer.  Look for the one new
    // direct child matching this composite's result shape.  Plans are ordered
    // oldest-first, which also matches normal redo order for nested applies.
    for (PresentationRecoveryPlan& plan : g_presentationRecoveryPlans)
    {
        if (!plan.active || !plan.resultWasUndone)
            continue;

        LayerObject parent;
        if (!FindLayer(plan.parentPtr, parent))
            continue;

        LayerObject candidate;
        int candidateCount = 0;
        const int childCount = parent.GetChildLayerCount();
        for (int index = 0; index < childCount; ++index)
        {
            LayerObject child = parent.GetChildAt_TopToBottom(index);
            if (!child.CheckPtr() || IsOriginalDirectChild(plan, child.ptr) ||
                child.IsGroup() != plan.resultIsGroup ||
                child.IsAdjustLayer() != plan.resultIsAdjust)
            {
                continue;
            }
            candidate = child;
            ++candidateCount;
        }

        if (candidateCount != 1)
            continue;

        plan.resultPtr = candidate.ptr;
        if (ApplyCompositePresentation(plan))
            QueueHostRedraw();
        plan.resultWasUndone = false;
        break;
    }
}

bool EnsurePresentationRecoveryHooks()
{
    if (g_originalUndo != nullptr && g_originalRedo != nullptr)
        return true;
    if (g_presentationHooksUnavailable)
        return false;
    if (!ResolveHostApi() || g_api.undo == nullptr || g_api.redo == nullptr)
        return false;

    if (!CSPMOD::Hook(reinterpret_cast<void*>(g_api.undo),
                       reinterpret_cast<void*>(HookUndo),
                       reinterpret_cast<void**>(&g_originalUndo)))
    {
        g_presentationHooksUnavailable = true;
        return false;
    }
    if (!CSPMOD::Hook(reinterpret_cast<void*>(g_api.redo),
                       reinterpret_cast<void*>(HookRedo),
                       reinterpret_cast<void**>(&g_originalRedo)))
    {
        // The undo detour is already installed at this point.  Keep its valid
        // trampoline so it can safely pass through with no plans armed, and
        // prevent any second attempt to patch either host entry point.
        g_presentationHooksUnavailable = true;
        return false;
    }
    Diag("Presentation recovery hooks installed");
    return true;
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

void HostUndoTransaction::TrackPresentationRecovery(
    const std::vector<LayerPresentationState>& originalStates,
    LayerObject resultLayer,
    LayerObject resultParent)
{
    if (originalStates.empty() || !resultLayer.CheckPtr() ||
        !resultParent.CheckPtr() || !EnsurePresentationRecoveryHooks())
    {
        return;
    }

    PresentationRecoveryPlan plan;
    plan.originalStates = originalStates;
    plan.resultPtr = resultLayer.ptr;
    plan.parentPtr = resultParent.ptr;
    plan.resultIsGroup = resultLayer.IsGroup();
    plan.resultIsAdjust = resultLayer.IsAdjustLayer();
    plan.active = true;

    const int childCount = resultParent.GetChildLayerCount();
    plan.originalDirectChildren.reserve(childCount > 0 ? childCount - 1 : 0);
    for (int index = 0; index < childCount; ++index)
    {
        LayerObject child = resultParent.GetChildAt_TopToBottom(index);
        if (child.CheckPtr() && child.ptr != resultLayer.ptr)
            plan.originalDirectChildren.push_back(child.ptr);
    }

    if (plan.originalDirectChildren.empty())
        return;

    g_presentationRecoveryPlans.push_back(std::move(plan));
    Diag("Presentation recovery armed: originals=%zu", originalStates.size());
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
    if (commandCount != undoCount ||
        g_api.multiCount(multi.object) != static_cast<std::int64_t>(commandCount))
    {
        Diag("Commit count mismatch: undo=%zu redo=%zu multi=%lld",
            undoCount, commandCount,
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
