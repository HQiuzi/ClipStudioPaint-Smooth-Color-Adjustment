#pragma once

#include "LayerObject.h"

#include <string>
#include <vector>

// Collapses the history entries produced by one plugin action into a single
// named entry, so the whole action is reverted by one undo.
//
// The layer structure changes (copy/move/merge) are host commands and become
// one composite entry. Layer visibility and locking are direct host state
// writes in this host build.  Their pre-action values are therefore recovered
// only when this composite's result layer is actually removed by Undo; no
// synthetic host property command is constructed.
//
// Every host entry point is resolved by version-specific address and checked
// against its known opening instructions.  If any check fails the caller keeps
// its previous behaviour.
class HostUndoTransaction
{
public:
    struct LayerPresentationState
    {
        uintptr_t layerPtr = 0;
        bool visible = true;
        bool locked = false;
    };

    static bool Available();

    explicit HostUndoTransaction(const wchar_t* historyName);
    ~HostUndoTransaction();

    HostUndoTransaction(const HostUndoTransaction&) = delete;
    HostUndoTransaction& operator=(const HostUndoTransaction&) = delete;

    // False means grouping is unavailable; the caller keeps its previous
    // behaviour.
    bool IsActive() const { return active_; }

    // Arm direct presentation-state recovery after Commit.  The installed
    // observer only restores these values when Undo removes resultLayer; this
    // deliberately excludes ordinary undo operations such as brush strokes.
    static void TrackPresentationRecovery(
        const std::vector<LayerPresentationState>& originalStates,
        LayerObject resultLayer,
        LayerObject resultParent);

    // Fold everything recorded since construction into one named entry.
    bool Commit();

    // Discard everything recorded since construction, restoring the document.
    bool Rollback();

private:
    void* undoModel_ = nullptr;
    void* temporaryStackObject_ = nullptr;
    void* temporaryStackControl_ = nullptr;
    std::wstring historyName_;
    bool active_ = false;
};
