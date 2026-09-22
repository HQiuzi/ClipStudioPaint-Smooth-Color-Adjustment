#pragma once

#include <string>

// Collapses the history entries produced by one plugin action into a single
// named entry, so the whole action is reverted by one undo.
//
// The host already implements this: it can open a named temporary command
// stack, have every command of an action recorded into it, and then settle
// that stack, either discarding it or folding it into one composite command.
// This class only drives those host entry points; it never rewrites host code
// or host data, and it never constructs host command objects itself.
//
// Every entry point is resolved by version-specific address and checked
// against its known opening instructions.  If any check fails, callers keep
// their previous multi-entry behaviour instead of driving an unknown build.
class HostUndoTransaction
{
public:
    // True when the running host build exposes the interfaces used below.
    static bool Available();

    explicit HostUndoTransaction(const wchar_t* historyName);
    ~HostUndoTransaction();

    HostUndoTransaction(const HostUndoTransaction&) = delete;
    HostUndoTransaction& operator=(const HostUndoTransaction&) = delete;

    // False means grouping is unavailable; the caller keeps its previous
    // behaviour.
    bool IsActive() const { return active_; }

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
