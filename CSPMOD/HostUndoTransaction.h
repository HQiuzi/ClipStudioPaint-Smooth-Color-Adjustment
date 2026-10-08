#pragma once

#include "LayerObject.h"

#include <string>
#include <vector>

// Collapses the history entries produced by one plugin action into a single
// named entry, so the whole action is reverted by one undo.
//
// The layer structure changes (copy/move/merge) are host commands and become
// one composite entry. Lock is a plain layer flag, so its native property
// command is appended explicitly. Visibility must be restored separately: the
// host does not turn direct visibility writes into a reversible command.
//
// Every host entry point is resolved by version-specific address and checked
// against its known opening instructions.  If any check fails the caller keeps
// its previous behaviour.
class HostUndoTransaction
{
public:
    struct VisibilityBackup
    {
        uintptr_t layerPtr = 0;
        bool visible = true;
    };

    static bool Available();

    explicit HostUndoTransaction(const wchar_t* historyName);
    ~HostUndoTransaction();

    HostUndoTransaction(const HostUndoTransaction&) = delete;
    HostUndoTransaction& operator=(const HostUndoTransaction&) = delete;

    // False means grouping is unavailable; the caller keeps its previous
    // behaviour.
    bool IsActive() const { return active_; }

    // Capture the host property command for a lock change, so the host undoes
    // the lock together with the structural steps.
    bool TrackLockChange(LayerObject layer);

    // Remember a direct visibility change. ActivateVisibilityRecovery must be
    // called after Commit with the re-resolved result layer.
    void TrackVisibility(LayerObject layer);

    // Starts selective visibility recovery for this committed action. Unlike
    // a blanket post-undo fixup, it only changes flags when this action's
    // result layer itself is removed or recreated.
    void ActivateVisibilityRecovery(LayerObject committedResult);

    // Fold everything recorded since construction into one named entry.
    bool Commit();

    // Discard everything recorded since construction, restoring the document.
    bool Rollback();

private:
    struct SharedPtr
    {
        void* object = nullptr;
        void* control = nullptr;
    };

    void* undoModel_ = nullptr;
    void* temporaryStackObject_ = nullptr;
    void* temporaryStackControl_ = nullptr;
    std::vector<SharedPtr> propertyCommands_;
    std::vector<VisibilityBackup> visibilityBackups_;
    std::wstring historyName_;
    bool active_ = false;
};
