#include "ScriptAction_ApplyEffectLayers.h"

#include "LayerObject.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>
#include <vector>

namespace
{
using LayerPath = std::vector<int>;

struct SourceLayer
{
    uintptr_t ptr = 0;
    LayerPath path;
};

struct GroupState
{
    LayerObject layer;
    bool open = false;
    bool locked = false;
};

struct LayerState
{
    uintptr_t ptr = 0;
    bool visible = false;
    bool maskVisible = false;
    bool clipping = false;
    bool locked = false;
    uint16_t opacity = 0x100;
    uint32_t blendMode = 0;
};

struct TreeShapeEntry
{
    LayerPath path;
    uintptr_t ptr = 0;
    bool isGroup = false;
    bool isAdjust = false;
    bool mutableLeaf = false;
    int childCount = 0;
};

bool GetLayerAtPath(const LayerPath& path, LayerObject& result)
{
    LayerObject current = LayerObject::GetCurrentRoot();
    if (!current.CheckPtr())
        return false;

    for (int index : path)
    {
        const int childCount = current.GetChildLayerCount();
        if (index < 0 || index >= childCount)
            return false;
        current = current.GetChildAt_TopToBottom(index);
        if (!current.CheckPtr())
            return false;
    }

    result = current;
    return true;
}

bool FindLayerPathRecursive(LayerObject parent, uintptr_t targetPtr, LayerPath& path)
{
    const int childCount = parent.GetChildLayerCount();
    for (int index = 0; index < childCount; ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr())
            continue;

        path.push_back(index);
        if (child.ptr == targetPtr)
            return true;

        if (child.IsGroup() && FindLayerPathRecursive(child, targetPtr, path))
            return true;
        path.pop_back();
    }
    return false;
}

bool FindLayerPath(uintptr_t targetPtr, LayerPath& path)
{
    path.clear();
    LayerObject root = LayerObject::GetCurrentRoot();
    if (!root.CheckPtr() || targetPtr == 0)
        return false;
    if (root.ptr == targetPtr)
        return true;
    return FindLayerPathRecursive(root, targetPtr, path);
}

bool FindLayer(uintptr_t targetPtr, LayerObject& result)
{
    LayerPath path;
    if (!FindLayerPath(targetPtr, path))
        return false;
    return GetLayerAtPath(path, result);
}

bool GetParentAndIndex(const LayerPath& childPath, LayerObject& parent, int& index)
{
    if (childPath.empty())
        return false;

    LayerPath parentPath = childPath;
    index = parentPath.back();
    parentPath.pop_back();
    return GetLayerAtPath(parentPath, parent);
}

bool GetDirectChildPointers(LayerObject parent, std::set<uintptr_t>& children)
{
    children.clear();
    if (!parent.CheckPtr())
        return false;

    const int childCount = parent.GetChildLayerCount();
    for (int index = 0; index < childCount; ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr())
            return false;
        children.insert(child.ptr);
    }
    return static_cast<int>(children.size()) == childCount;
}

void CollectSelectedLayersRecursive(LayerObject parent, LayerPath& path,
                                    std::vector<SourceLayer>& selected)
{
    const int childCount = parent.GetChildLayerCount();
    for (int index = 0; index < childCount; ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr())
            continue;

        path.push_back(index);
        if (child.IsSelected())
            selected.push_back({ child.ptr, path });
        if (child.IsGroup())
            CollectSelectedLayersRecursive(child, path, selected);
        path.pop_back();
    }
}

std::vector<SourceLayer> CollectSelectedLayers()
{
    std::vector<SourceLayer> selected;
    LayerObject root = LayerObject::GetCurrentRoot();
    if (!root.CheckPtr())
        return selected;

    LayerPath path;
    CollectSelectedLayersRecursive(root, path, selected);
    return selected;
}

bool FindClippingTarget(const SourceLayer& source, LayerObject& target, LayerPath& targetPath)
{
    LayerObject parent;
    int sourceIndex = -1;
    if (!GetParentAndIndex(source.path, parent, sourceIndex))
        return false;

    const int childCount = parent.GetChildLayerCount();
    for (int index = sourceIndex + 1; index < childCount; ++index)
    {
        LayerObject candidate = parent.GetChildAt_TopToBottom(index);
        if (!candidate.CheckPtr())
            return false;
        if (candidate.HasLayerClippingMask())
            continue;

        target = candidate;
        targetPath = source.path;
        targetPath.back() = index;
        return true;
    }
    return false;
}

void CollectGroupStatesRecursive(LayerObject parent, std::vector<GroupState>& states)
{
    const int childCount = parent.GetChildLayerCount();
    for (int index = 0; index < childCount; ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr() || !child.IsGroup())
            continue;

        states.push_back({ child, child.IsGroupOpen(), child.IsLocked() });
        CollectGroupStatesRecursive(child, states);
    }
}

std::vector<GroupState> CollectGroupStates()
{
    std::vector<GroupState> states;
    LayerObject root = LayerObject::GetCurrentRoot();
    if (root.CheckPtr())
        CollectGroupStatesRecursive(root, states);
    return states;
}

std::vector<GroupState> CollectGroupSubtreeStates(LayerObject group)
{
    std::vector<GroupState> states;
    if (!group.CheckPtr() || !group.IsGroup())
        return states;

    states.push_back({ group, group.IsGroupOpen(), group.IsLocked() });
    CollectGroupStatesRecursive(group, states);
    return states;
}

void CollectLeafLayerPointers(LayerObject group,
                               std::vector<uintptr_t>& leafPtrs)
{
    if (!group.CheckPtr() || !group.IsGroup())
        return;

    const int childCount = group.GetChildLayerCount();
    for (int index = 0; index < childCount; ++index)
    {
        LayerObject child = group.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr())
            continue;
        // Match the original "Apply Effects Group" protection semantics:
        // a locked group protects its complete subtree, and a locked leaf is
        // retained in the copied group without being rendered into.
        if (child.IsLocked())
            continue;
        if (child.IsGroup())
        {
            CollectLeafLayerPointers(child, leafPtrs);
        }
        else if (!child.IsAdjustLayer())
        {
            leafPtrs.push_back(child.ptr);
        }
    }
}

bool CaptureTreeShapeRecursive(LayerObject node, LayerPath& path,
                               bool protectedByLockedAncestor,
                               std::vector<TreeShapeEntry>& shape)
{
    if (!node.CheckPtr())
        return false;

    const bool isGroup = node.IsGroup();
    const bool isAdjust = node.IsAdjustLayer();
    const bool protectedNode = protectedByLockedAncestor || node.IsLocked();
    shape.push_back({
        path,
        node.ptr,
        isGroup,
        isAdjust,
        !isGroup && !isAdjust && !protectedNode,
        isGroup ? node.GetChildLayerCount() : 0
    });

    if (!isGroup)
        return true;

    const int childCount = node.GetChildLayerCount();
    for (int index = 0; index < childCount; ++index)
    {
        LayerObject child = node.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr())
            return false;
        path.push_back(index);
        if (!CaptureTreeShapeRecursive(child, path, protectedNode, shape))
            return false;
        path.pop_back();
    }
    return true;
}

bool CaptureTreeShape(LayerObject group, std::vector<TreeShapeEntry>& shape)
{
    shape.clear();
    if (!group.CheckPtr() || !group.IsGroup())
        return false;
    LayerPath path;
    return CaptureTreeShapeRecursive(group, path, false, shape);
}

bool GetLayerAtRelativePath(LayerObject root, const LayerPath& path,
                            LayerObject& result)
{
    if (!root.CheckPtr())
        return false;
    result = root;
    for (int index : path)
    {
        if (!result.IsGroup() || index < 0 ||
            index >= result.GetChildLayerCount())
        {
            return false;
        }
        result = result.GetChildAt_TopToBottom(index);
        if (!result.CheckPtr())
            return false;
    }
    return true;
}

bool ValidateTreeShape(uintptr_t groupPtr,
                       const std::vector<TreeShapeEntry>& shape)
{
    LayerObject root;
    if (!FindLayer(groupPtr, root) || !root.IsGroup())
        return false;

    for (const TreeShapeEntry& expected : shape)
    {
        LayerObject actual;
        if (!GetLayerAtRelativePath(root, expected.path, actual) ||
            actual.IsGroup() != expected.isGroup ||
            actual.IsAdjustLayer() != expected.isAdjust)
        {
            return false;
        }
        if (expected.isGroup)
        {
            if (actual.ptr != expected.ptr ||
                actual.GetChildLayerCount() != expected.childCount)
            {
                return false;
            }
        }
        else if (!expected.mutableLeaf && actual.ptr != expected.ptr)
        {
            return false;
        }
    }
    return true;
}

void RestoreGroupStates(const std::vector<GroupState>& states)
{
    for (const GroupState& saved : states)
    {
        LayerObject layer = saved.layer;
        if (layer.CheckPtr() && layer.IsGroup())
        {
            layer.SetLocked(false);
            layer.SetGroupOpen(saved.open);
            layer.SetLocked(saved.locked);
        }
    }
}

void CloseAllGroupsRecursive(LayerObject parent)
{
    const int childCount = parent.GetChildLayerCount();
    for (int index = 0; index < childCount; ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr() || !child.IsGroup())
            continue;

        CloseAllGroupsRecursive(child);
        child.SetGroupOpen(false);
    }
}

void CloseAllGroups()
{
    LayerObject root = LayerObject::GetCurrentRoot();
    if (root.CheckPtr())
        CloseAllGroupsRecursive(root);
}

bool OpenAncestors(const LayerPath& path)
{
    LayerObject current = LayerObject::GetCurrentRoot();
    if (!current.CheckPtr())
        return false;

    for (size_t depth = 0; depth + 1 < path.size(); ++depth)
    {
        const int index = path[depth];
        if (index < 0 || index >= current.GetChildLayerCount())
            return false;
        current = current.GetChildAt_TopToBottom(index);
        if (!current.CheckPtr() || !current.IsGroup())
            return false;
        current.SetGroupOpen(true);
    }
    return true;
}

bool AreAdjacent(uintptr_t upperPtr, uintptr_t lowerPtr)
{
    LayerPath upperPath;
    LayerPath lowerPath;
    if (!FindLayerPath(upperPtr, upperPath) || !FindLayerPath(lowerPtr, lowerPath))
        return false;
    if (upperPath.size() != lowerPath.size() || upperPath.empty())
        return false;
    if (!std::equal(upperPath.begin(), upperPath.end() - 1, lowerPath.begin()))
        return false;
    return upperPath.back() + 1 == lowerPath.back();
}

bool PathComesBefore(const LayerPath& left, const LayerPath& right)
{
    return std::lexicographical_compare(
        left.begin(), left.end(), right.begin(), right.end());
}

bool DuplicateLayer(LayerObject original, LayerObject& copy)
{
    copy = LayerObject{};

    LayerPath originalPath;
    if (!FindLayerPath(original.ptr, originalPath) || !OpenAncestors(originalPath))
        return false;

    LayerObject parent;
    int originalIndex = -1;
    if (!GetParentAndIndex(originalPath, parent, originalIndex))
        return false;

    const int oldCount = parent.GetChildLayerCount();
    std::set<uintptr_t> oldChildren;
    if (!GetDirectChildPointers(parent, oldChildren))
        return false;

    const uintptr_t parentPtr = parent.ptr;
    original.SetSelect();
    LayerObject::Duplicate();

    if (!FindLayer(parentPtr, parent) || parent.GetChildLayerCount() != oldCount + 1)
        return false;

    LayerObject current = LayerObject::GetCurrentLayer();
    LayerObject candidate;
    int newLayerCount = 0;
    for (int index = 0; index < parent.GetChildLayerCount(); ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr() || oldChildren.find(child.ptr) != oldChildren.end())
            continue;
        candidate = child;
        ++newLayerCount;
        if (current.ptr == child.ptr)
            copy = child;
    }

    if (newLayerCount != 1)
        return false;
    if (copy.ptr == 0)
        copy = candidate;
    return copy.CheckPtr() && copy.ptr != original.ptr;
}

bool MoveLayerImmediatelyAbove(uintptr_t movingPtr, uintptr_t targetPtr)
{
    for (int attempt = 0; attempt < 4096; ++attempt)
    {
        if (AreAdjacent(movingPtr, targetPtr))
            return true;

        LayerObject moving;
        LayerPath movingPath;
        LayerPath targetPath;
        if (!FindLayer(movingPtr, moving) ||
            !FindLayerPath(movingPtr, movingPath) ||
            !FindLayerPath(targetPtr, targetPath))
            return false;
        if (movingPath.empty() || targetPath.empty())
            return false;
        if (!OpenAncestors(movingPath) || !OpenAncestors(targetPath))
            return false;

        const LayerPath oldPath = movingPath;
        moving.SetSelect();
        const bool sameParent = movingPath.size() == targetPath.size() &&
            std::equal(movingPath.begin(), movingPath.end() - 1, targetPath.begin());
        if (sameParent)
        {
            if (movingPath.back() < targetPath.back())
                LayerObject::MoveDown();
            else
                LayerObject::MoveUp();
        }
        else if (PathComesBefore(movingPath, targetPath))
        {
            // With the target ancestors open, MoveDown follows the visible
            // layer tree and can carry a root-level copy into nested groups.
            LayerObject::MoveDown();
        }
        else
        {
            return false;
        }

        if (!FindLayerPath(movingPtr, movingPath) || movingPath == oldPath)
            return false;
    }
    return false;
}

bool RefreshClippingLayer(uintptr_t layerPtr, uintptr_t targetPtr)
{
    if (!AreAdjacent(layerPtr, targetPtr))
        return false;

    LayerObject layer;
    LayerPath oldPath;
    LayerPath movedPath;
    if (!FindLayer(layerPtr, layer) || !FindLayerPath(layerPtr, oldPath))
        return false;
    if (!OpenAncestors(oldPath))
        return false;

    LayerObject parent;
    int index = -1;
    if (!GetParentAndIndex(oldPath, parent, index) ||
        parent.GetChildLayerCount() < 2)
    {
        return false;
    }

    // Stay inside the same parent.  Moving upward from the first child can
    // pull a clipping layer out of its group, so use the available neighbour.
    const bool moveUpFirst = index > 0;
    layer.SetSelect();
    if (moveUpFirst)
        LayerObject::MoveUp();
    else
        LayerObject::MoveDown();

    LayerPath expectedMovedPath = oldPath;
    expectedMovedPath.back() += moveUpFirst ? -1 : 1;
    if (!FindLayerPath(layerPtr, movedPath) || movedPath != expectedMovedPath)
        return false;

    if (!FindLayer(layerPtr, layer))
        return false;
    layer.SetSelect();
    if (moveUpFirst)
        LayerObject::MoveDown();
    else
        LayerObject::MoveUp();

    LayerPath restoredPath;
    return FindLayerPath(layerPtr, restoredPath) &&
        restoredPath == oldPath && AreAdjacent(layerPtr, targetPtr);
}

bool RefreshLayerState(uintptr_t layerPtr)
{
    LayerObject layer;
    LayerPath originalPath;
    LayerPath movedPath;
    if (!FindLayer(layerPtr, layer) || !FindLayerPath(layerPtr, originalPath))
        return false;
    if (!OpenAncestors(originalPath))
        return false;

    LayerObject parent;
    int index = -1;
    if (!GetParentAndIndex(originalPath, parent, index) ||
        parent.GetChildLayerCount() < 2)
    {
        return false;
    }

    const bool moveUpFirst = index > 0;
    layer.SetSelect();
    if (moveUpFirst)
        LayerObject::MoveUp();
    else
        LayerObject::MoveDown();

    LayerPath expectedMovedPath = originalPath;
    expectedMovedPath.back() += moveUpFirst ? -1 : 1;
    if (!FindLayerPath(layerPtr, movedPath) || movedPath != expectedMovedPath)
        return false;

    if (!FindLayer(layerPtr, layer))
        return false;
    layer.SetSelect();
    if (moveUpFirst)
        LayerObject::MoveDown();
    else
        LayerObject::MoveUp();

    LayerPath restoredPath;
    return FindLayerPath(layerPtr, restoredPath) &&
        restoredPath == originalPath;
}

bool RefreshViaStablePair(uintptr_t upperPtr, uintptr_t lowerPtr)
{
    if (!AreAdjacent(upperPtr, lowerPtr))
        return false;

    LayerObject upper;
    LayerObject lower;
    LayerPath upperPath;
    LayerPath lowerPath;
    if (!FindLayer(upperPtr, upper) || !FindLayer(lowerPtr, lower) ||
        !upper.IsGroup() || !lower.IsGroup() ||
        !FindLayerPath(upperPtr, upperPath) ||
        !FindLayerPath(lowerPtr, lowerPath))
    {
        return false;
    }

    const bool upperWasOpen = upper.IsGroupOpen();
    const bool lowerWasOpen = lower.IsGroupOpen();
    upper.SetGroupOpen(false);
    lower.SetGroupOpen(false);

    upper.SetSelect();
    LayerObject::MoveDown();

    LayerPath movedUpperPath;
    LayerPath movedLowerPath;
    const bool swapped = FindLayerPath(upperPtr, movedUpperPath) &&
        FindLayerPath(lowerPtr, movedLowerPath) &&
        movedUpperPath == lowerPath && movedLowerPath == upperPath;

    if (swapped && FindLayer(upperPtr, upper))
    {
        upper.SetSelect();
        LayerObject::MoveUp();
    }

    LayerPath restoredUpperPath;
    LayerPath restoredLowerPath;
    const bool restored = swapped &&
        FindLayerPath(upperPtr, restoredUpperPath) &&
        FindLayerPath(lowerPtr, restoredLowerPath) &&
        restoredUpperPath == upperPath && restoredLowerPath == lowerPath;

    if (FindLayer(upperPtr, upper) && upper.IsGroup())
        upper.SetGroupOpen(upperWasOpen);
    if (FindLayer(lowerPtr, lower) && lower.IsGroup())
        lower.SetGroupOpen(lowerWasOpen);
    return restored;
}

bool ValidateVisibleClippingSuffix(LayerObject parent, int targetIndex,
                                   const std::vector<SourceLayer>& sources)
{
    std::set<uintptr_t> selectedPointers;
    int topSelectedIndex = std::numeric_limits<int>::max();
    for (const SourceLayer& source : sources)
    {
        selectedPointers.insert(source.ptr);
        topSelectedIndex = std::min(topSelectedIndex, source.path.back());
    }

    if (topSelectedIndex < 0 || topSelectedIndex >= targetIndex)
        return false;

    for (int index = topSelectedIndex; index < targetIndex; ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (!child.CheckPtr() || !child.HasLayerClippingMask())
            return false;
        if (selectedPointers.find(child.ptr) == selectedPointers.end() && child.IsVisible())
            return false;
    }
    return true;
}

LayerState CaptureLayerState(LayerObject layer)
{
    return {
        layer.ptr,
        layer.IsVisible(),
        layer.IsLayerMaskVisible(),
        layer.HasLayerClippingMask(),
        layer.IsLocked(),
        layer.GetOpacity(),
        layer.GetBlendMode()
    };
}

void RestoreLayerStates(const std::vector<LayerState>& states)
{
    for (const LayerState& saved : states)
    {
        LayerObject layer;
        if (!FindLayer(saved.ptr, layer))
            continue;

        layer.SetLocked(false);
        layer.SetVisibility(saved.visible);
        layer.SetLayerMaskVisibility(saved.maskVisible);
        layer.SetLayerClippingMask(saved.clipping);
        layer.SetOpacity(saved.opacity);
        layer.SetBlendMode(saved.blendMode);
        layer.SetLocked(saved.locked);
    }
}

bool ApplyLayerState(uintptr_t layerPtr, const LayerState& saved,
                     uintptr_t refreshUpperPtr, uintptr_t refreshLowerPtr)
{
    LayerObject layer;
    if (!FindLayer(layerPtr, layer))
        return false;

    layer.SetLocked(false);
    layer.SetVisibility(saved.visible);
    layer.SetLayerMaskVisibility(saved.maskVisible);
    layer.SetLayerClippingMask(saved.clipping);
    layer.SetOpacity(saved.opacity);
    layer.SetBlendMode(saved.blendMode);

    if (!RefreshViaStablePair(refreshUpperPtr, refreshLowerPtr))
        return false;
    if (!FindLayer(layerPtr, layer))
        return false;

    layer.SetLocked(saved.locked);
    return layer.IsVisible() == saved.visible &&
        layer.IsLayerMaskVisible() == saved.maskVisible &&
        layer.HasLayerClippingMask() == saved.clipping &&
        layer.IsLocked() == saved.locked &&
        layer.GetOpacity() == saved.opacity &&
        layer.GetBlendMode() == saved.blendMode;
}

void RestoreSourceSelection(const std::vector<SourceLayer>& sources)
{
    bool first = true;
    for (const SourceLayer& sourceInfo : sources)
    {
        LayerObject source;
        if (!FindLayer(sourceInfo.ptr, source))
            continue;
        if (first)
        {
            source.SetSelect();
            first = false;
        }
        else
        {
            source.AddSelect();
        }
    }
}

void QuarantineNewChildren(uintptr_t parentPtr,
                           const std::set<uintptr_t>& originalChildren)
{
    LayerObject parent;
    if (!FindLayer(parentPtr, parent))
        return;

    std::vector<uintptr_t> newChildren;
    const int childCount = parent.GetChildLayerCount();
    for (int index = 0; index < childCount; ++index)
    {
        LayerObject child = parent.GetChildAt_TopToBottom(index);
        if (child.CheckPtr() && originalChildren.find(child.ptr) == originalChildren.end())
            newChildren.push_back(child.ptr);
    }

    for (uintptr_t childPtr : newChildren)
    {
        LayerObject child;
        if (!FindLayer(childPtr, child))
            continue;
        child.SetLocked(false);
        child.SetVisibility(false);
        child.SetLayerClippingMask(false);
        if (child.IsGroup())
            child.SetGroupOpen(false);
    }

    for (uintptr_t childPtr : newChildren)
    {
        if (!RefreshLayerState(childPtr))
            SDL_Log("Apply Effects Layer could not refresh a quarantined work layer");
    }

    for (uintptr_t childPtr : newChildren)
    {
        LayerObject child;
        if (!FindLayer(childPtr, child))
            continue;
        child.SetLocked(true);
    }
}

bool HideAndLockLayers(const std::vector<uintptr_t>& layerPtrs,
                       uintptr_t refreshLayerPtr,
                       uintptr_t stableLowerPtr = 0)
{
    for (uintptr_t layerPtr : layerPtrs)
    {
        LayerObject layer;
        if (!FindLayer(layerPtr, layer))
            return false;
        layer.SetLocked(false);
        layer.SetVisibility(false);
    }

    // Commit direct state writes through a reversible, path-checked host move.
    const bool refreshed = stableLowerPtr != 0
        ? RefreshViaStablePair(refreshLayerPtr, stableLowerPtr)
        : RefreshLayerState(refreshLayerPtr);
    if (!refreshed)
        return false;

    for (uintptr_t layerPtr : layerPtrs)
    {
        LayerObject layer;
        if (!FindLayer(layerPtr, layer) || layer.IsVisible())
            return false;
        layer.SetLocked(true);
        if (!layer.IsLocked())
            return false;
    }
    return true;
}

bool PrepareTargetCopy(uintptr_t copyPtr, uintptr_t originalTargetPtr)
{
    if (!MoveLayerImmediatelyAbove(copyPtr, originalTargetPtr))
        return false;

    LayerObject copy;
    if (!FindLayer(copyPtr, copy))
        return false;
    copy.SetLocked(false);
    copy.SetVisibility(true);
    copy.SetLayerClippingMask(false);
    if (copy.IsGroup())
        copy.SetGroupOpen(false);

    if (!RefreshLayerState(copyPtr) || !FindLayer(copyPtr, copy))
        return false;
    return !copy.IsLocked() && copy.IsVisible() && !copy.HasLayerClippingMask();
}

bool PrepareSourceCopy(uintptr_t copyPtr, uintptr_t lowerWorkPtr,
                       uintptr_t refreshUpperPtr = 0,
                       uintptr_t refreshLowerPtr = 0)
{
    LayerObject copy;
    if (!FindLayer(copyPtr, copy))
        return false;
    copy.SetLocked(false);
    copy.SetVisibility(true);
    // A duplicated source inherits its clipping flag.  Clear it before a
    // cross-parent move so the host moves only this layer, not a clipping chain.
    copy.SetLayerClippingMask(false);

    if (!MoveLayerImmediatelyAbove(copyPtr, lowerWorkPtr) || !FindLayer(copyPtr, copy))
        return false;
    copy.SetLayerClippingMask(true);
    const bool refreshed = refreshUpperPtr != 0 && refreshLowerPtr != 0
        ? RefreshViaStablePair(refreshUpperPtr, refreshLowerPtr)
        : RefreshClippingLayer(copyPtr, lowerWorkPtr);
    if (!refreshed || !FindLayer(copyPtr, copy))
        return false;
    return !copy.IsLocked() && copy.IsVisible() && copy.HasLayerClippingMask();
}

bool ValidateWorkStack(uintptr_t targetCopyPtr,
                       const std::vector<uintptr_t>& sourceCopyPtrs)
{
    uintptr_t lowerPtr = targetCopyPtr;
    for (uintptr_t sourcePtr : sourceCopyPtrs)
    {
        if (!AreAdjacent(sourcePtr, lowerPtr))
            return false;
        lowerPtr = sourcePtr;
    }
    return true;
}

bool MergeWorkStack(uintptr_t parentPtr, uintptr_t targetCopyPtr,
                    const std::vector<uintptr_t>& sourceCopyPtrs,
                    uintptr_t& resultPtr)
{
    resultPtr = 0;
    LayerObject parent;
    if (!FindLayer(parentPtr, parent))
        return false;

    std::set<uintptr_t> beforeChildren;
    if (!GetDirectChildPointers(parent, beforeChildren))
        return false;
    const int beforeCount = parent.GetChildLayerCount();
    const int workCount = static_cast<int>(sourceCopyPtrs.size()) + 1;

    std::set<uintptr_t> workPtrs;
    workPtrs.insert(targetCopyPtr);
    workPtrs.insert(sourceCopyPtrs.begin(), sourceCopyPtrs.end());
    if (static_cast<int>(workPtrs.size()) != workCount)
        return false;
    for (uintptr_t workPtr : workPtrs)
    {
        if (beforeChildren.find(workPtr) == beforeChildren.end())
            return false;
    }

    LayerObject targetCopy;
    if (!FindLayer(targetCopyPtr, targetCopy))
        return false;
    targetCopy.SetSelect();
    for (uintptr_t sourcePtr : sourceCopyPtrs)
    {
        LayerObject sourceCopy;
        if (!FindLayer(sourcePtr, sourceCopy))
            return false;
        sourceCopy.AddSelect();
    }

    LayerObject::MergeSelect();

    if (!FindLayer(parentPtr, parent) ||
        parent.GetChildLayerCount() != beforeCount - workCount + 1)
        return false;

    std::set<uintptr_t> afterChildren;
    if (!GetDirectChildPointers(parent, afterChildren))
        return false;

    // Depending on the target type, the host either creates a new layer or
    // reuses one of the selected work layers for the merge result.  In both
    // cases every untouched sibling must survive and exactly one output layer
    // must remain in place of the complete work stack.
    for (uintptr_t beforePtr : beforeChildren)
    {
        if (workPtrs.find(beforePtr) == workPtrs.end() &&
            afterChildren.find(beforePtr) == afterChildren.end())
            return false;
    }

    std::vector<uintptr_t> resultCandidates;
    for (uintptr_t afterPtr : afterChildren)
    {
        if (workPtrs.find(afterPtr) != workPtrs.end() ||
            beforeChildren.find(afterPtr) == beforeChildren.end())
        {
            resultCandidates.push_back(afterPtr);
        }
    }
    if (resultCandidates.size() != 1)
        return false;

    resultPtr = resultCandidates.front();
    LayerObject current = LayerObject::GetCurrentLayer();
    if (!current.CheckPtr() || current.ptr != resultPtr)
        SDL_Log("Apply Effects Layer merge result was not the current layer");

    LayerObject result;
    return FindLayer(resultPtr, result);
}

bool MergeLayerDown(uintptr_t parentPtr, uintptr_t upperPtr,
                    uintptr_t lowerPtr, uintptr_t& resultPtr)
{
    resultPtr = 0;
    if (!AreAdjacent(upperPtr, lowerPtr))
        return false;

    LayerObject parent;
    if (!FindLayer(parentPtr, parent))
        return false;

    std::set<uintptr_t> beforeChildren;
    if (!GetDirectChildPointers(parent, beforeChildren))
        return false;
    const int beforeCount = parent.GetChildLayerCount();

    std::set<uintptr_t> workPtrs = { upperPtr, lowerPtr };
    if (workPtrs.size() != 2)
        return false;
    for (uintptr_t workPtr : workPtrs)
    {
        if (beforeChildren.find(workPtr) == beforeChildren.end())
            return false;
    }

    LayerObject upper;
    if (!FindLayer(upperPtr, upper))
        return false;
    upper.SetSelect();
    LayerObject::MergeDown();

    if (!FindLayer(parentPtr, parent) ||
        parent.GetChildLayerCount() != beforeCount - 1)
        return false;

    std::set<uintptr_t> afterChildren;
    if (!GetDirectChildPointers(parent, afterChildren))
        return false;

    for (uintptr_t beforePtr : beforeChildren)
    {
        if (workPtrs.find(beforePtr) == workPtrs.end() &&
            afterChildren.find(beforePtr) == afterChildren.end())
        {
            return false;
        }
    }

    std::vector<uintptr_t> resultCandidates;
    for (uintptr_t afterPtr : afterChildren)
    {
        if (workPtrs.find(afterPtr) != workPtrs.end() ||
            beforeChildren.find(afterPtr) == beforeChildren.end())
        {
            resultCandidates.push_back(afterPtr);
        }
    }
    if (resultCandidates.size() != 1)
        return false;

    resultPtr = resultCandidates.front();
    LayerObject result;
    return FindLayer(resultPtr, result) &&
        !result.IsGroup() && !result.IsAdjustLayer();
}

bool PrepareLeafForEffect(uintptr_t layerPtr, LayerState& saved,
                          uintptr_t refreshUpperPtr,
                          uintptr_t refreshLowerPtr)
{
    LayerObject layer;
    if (!FindLayer(layerPtr, layer) || layer.IsGroup() || layer.IsAdjustLayer())
        return false;

    saved = CaptureLayerState(layer);
    layer.SetLocked(false);
    layer.SetVisibility(true);
    layer.SetLayerMaskVisibility(false);
    layer.SetLayerClippingMask(false);
    layer.SetOpacity(0x100);
    layer.SetBlendMode(0);

    if (!RefreshViaStablePair(refreshUpperPtr, refreshLowerPtr) ||
        !FindLayer(layerPtr, layer))
        return false;
    return !layer.IsLocked() && layer.IsVisible() &&
        !layer.IsLayerMaskVisible() && !layer.HasLayerClippingMask() &&
        layer.GetOpacity() == 0x100 && layer.GetBlendMode() == 0;
}

bool ApplySourceToLeaf(uintptr_t sourcePtr, uintptr_t& targetPtr,
                       uintptr_t workGroupPtr, uintptr_t originalGroupPtr)
{
    LayerObject source;
    LayerObject target;
    LayerPath originalTargetPath;
    if (!FindLayer(sourcePtr, source) || source.IsGroup() || source.IsLocked() ||
        !FindLayer(targetPtr, target) || target.IsGroup() ||
        target.IsAdjustLayer() || !FindLayerPath(targetPtr, originalTargetPath))
    {
        return false;
    }

    LayerState targetState;
    if (!PrepareLeafForEffect(targetPtr, targetState,
                              workGroupPtr, originalGroupPtr))
        return false;

    LayerObject sourceCopy;
    if (!DuplicateLayer(source, sourceCopy))
    {
        ApplyLayerState(targetPtr, targetState,
                        workGroupPtr, originalGroupPtr);
        return false;
    }

    CloseAllGroups();
    if (!PrepareSourceCopy(sourceCopy.ptr, targetPtr,
                           workGroupPtr, originalGroupPtr))
    {
        ApplyLayerState(targetPtr, targetState,
                        workGroupPtr, originalGroupPtr);
        return false;
    }

    LayerPath targetPath;
    LayerObject targetParent;
    int targetIndex = -1;
    if (!FindLayerPath(targetPtr, targetPath) ||
        !GetParentAndIndex(targetPath, targetParent, targetIndex))
    {
        return false;
    }

    uintptr_t resultPtr = 0;
    if (!MergeLayerDown(targetParent.ptr, sourceCopy.ptr, targetPtr, resultPtr))
        return false;

    LayerPath resultPath;
    if (!FindLayerPath(resultPtr, resultPath) || resultPath != originalTargetPath)
        return false;
    if (!ApplyLayerState(resultPtr, targetState,
                         workGroupPtr, originalGroupPtr))
        return false;
    if (!FindLayerPath(resultPtr, resultPath) || resultPath != originalTargetPath)
        return false;

    targetPtr = resultPtr;
    return true;
}

bool NormalizeResult(uintptr_t resultPtr)
{
    LayerObject result;
    if (!FindLayer(resultPtr, result) || result.IsGroup() || result.IsAdjustLayer())
        return false;

    result.SetLocked(false);
    result.SetVisibility(true);
    result.SetLayerClippingMask(false);
    result.SetOpacity(0x100);
    if (!RefreshLayerState(resultPtr) || !FindLayer(resultPtr, result))
        return false;

    return !result.IsGroup() && !result.IsAdjustLayer() &&
        !result.IsLocked() && result.IsVisible() &&
        !result.HasLayerClippingMask() && result.GetOpacity() == 0x100 &&
        result.GetBlendMode() == 0;
}
}

bool ScriptAction_ApplyEffectLayers::Available()
{
    return LayerObject::ApplyEffectAvailable();
}

ScriptAction_ApplyEffectLayers::Result ScriptAction_ApplyEffectLayers::Run()
{
    if (!Available())
        return Result::Unavailable;

    LayerObject root = LayerObject::GetCurrentRoot();
    if (!root.CheckPtr())
        return Result::InvalidSelection;

    std::vector<SourceLayer> sources = CollectSelectedLayers();
    if (sources.empty())
        return Result::InvalidSelection;

    uintptr_t clippingTargetPtr = 0;
    LayerPath clippingTargetPath;
    LayerPath sourceParentPath;

    for (const SourceLayer& sourceInfo : sources)
    {
        LayerObject source;
        if (!FindLayer(sourceInfo.ptr, source) || source.IsGroup() ||
            source.IsLocked() ||
            !source.HasLayerClippingMask() ||
            sourceInfo.path.empty())
            return Result::InvalidSelection;

        LayerObject target;
        LayerPath targetPath;
        if (!FindClippingTarget(sourceInfo, target, targetPath))
            return Result::InvalidSelection;

        LayerPath parentPath = sourceInfo.path;
        parentPath.pop_back();
        if (clippingTargetPtr == 0)
        {
            clippingTargetPtr = target.ptr;
            clippingTargetPath = targetPath;
            sourceParentPath = parentPath;
        }
        else if (target.ptr != clippingTargetPtr || parentPath != sourceParentPath)
        {
            return Result::InvalidSelection;
        }
    }

    LayerObject clippingTarget;
    if (!FindLayer(clippingTargetPtr, clippingTarget))
        return Result::NoTargetLayers;
    const bool targetIsGroup = clippingTarget.IsGroup();
    const bool unsupportedTargetBlend = !targetIsGroup &&
        clippingTarget.GetBlendMode() != 0;
    if (clippingTarget.IsLocked() || !clippingTarget.IsVisible() ||
        clippingTarget.IsAdjustLayer() || unsupportedTargetBlend)
    {
        SDL_Log("Apply Effects Layer rejected target: locked=%d visible=%d adjust=%d blend=%u",
            clippingTarget.IsLocked() ? 1 : 0,
            clippingTarget.IsVisible() ? 1 : 0,
            clippingTarget.IsAdjustLayer() ? 1 : 0,
            static_cast<unsigned int>(clippingTarget.GetBlendMode()));
        return Result::NoTargetLayers;
    }

    if (targetIsGroup)
    {
        std::vector<uintptr_t> originalLeafPtrs;
        CollectLeafLayerPointers(clippingTarget, originalLeafPtrs);
        if (originalLeafPtrs.empty())
            return Result::NoTargetLayers;
    }

    LayerObject sourceParent;
    int clippingTargetIndex = -1;
    if (!GetParentAndIndex(clippingTargetPath, sourceParent, clippingTargetIndex) ||
        !ValidateVisibleClippingSuffix(sourceParent, clippingTargetIndex, sources))
        return Result::InvalidSelection;

    std::sort(sources.begin(), sources.end(),
        [](const SourceLayer& left, const SourceLayer& right)
        {
            return left.path.back() > right.path.back();
        });

    const uintptr_t parentPtr = sourceParent.ptr;
    std::set<uintptr_t> originalChildren;
    if (!GetDirectChildPointers(sourceParent, originalChildren))
        return Result::OperationFailed;

    std::vector<LayerState> originalStates;
    originalStates.reserve(sources.size() + 1);
    for (const SourceLayer& sourceInfo : sources)
    {
        LayerObject source;
        if (!FindLayer(sourceInfo.ptr, source))
            return Result::OperationFailed;
        originalStates.push_back(CaptureLayerState(source));
    }
    originalStates.push_back(CaptureLayerState(clippingTarget));

    const std::vector<GroupState> groupStates = CollectGroupStates();
    std::vector<GroupState> workGroupStates;
    const bool targetGroupWasOpen = targetIsGroup &&
        clippingTarget.IsGroupOpen();
    bool originalsChanged = false;
    auto failOperation = [&](const char* stage)
    {
        if (originalsChanged)
            RestoreLayerStates(originalStates);
        RestoreGroupStates(workGroupStates);
        QuarantineNewChildren(parentPtr, originalChildren);
        RestoreGroupStates(groupStates);
        RestoreSourceSelection(sources);
        SDL_Log("Apply Effects Layer failed during %s", stage);
        return Result::OperationFailed;
    };

    if (!FindLayer(clippingTargetPtr, clippingTarget))
        return failOperation("target lookup");
    LayerObject targetCopy;
    if (!DuplicateLayer(clippingTarget, targetCopy) ||
        !PrepareTargetCopy(targetCopy.ptr, clippingTargetPtr))
        return failOperation("target copy preparation");

    const uintptr_t targetCopyPtr = targetCopy.ptr;
    uintptr_t resultPtr = 0;
    if (targetIsGroup)
    {
        workGroupStates = CollectGroupSubtreeStates(targetCopy);
        if (workGroupStates.empty())
            return failOperation("work group state capture");
        workGroupStates.front().open = targetGroupWasOpen;

        std::vector<TreeShapeEntry> workGroupShape;
        if (!CaptureTreeShape(targetCopy, workGroupShape))
            return failOperation("work group topology capture");

        std::vector<uintptr_t> leafPtrs;
        CollectLeafLayerPointers(targetCopy, leafPtrs);
        if (leafPtrs.empty())
            return failOperation("work group leaf collection");

        for (const SourceLayer& sourceInfo : sources)
        {
            for (uintptr_t& leafPtr : leafPtrs)
            {
                if (!ApplySourceToLeaf(sourceInfo.ptr, leafPtr,
                                       targetCopyPtr, clippingTargetPtr))
                    return failOperation("group leaf merge");
            }
        }

        if (!ValidateTreeShape(targetCopyPtr, workGroupShape))
            return failOperation("group topology validation");

        RestoreGroupStates(workGroupStates);
        resultPtr = targetCopyPtr;
    }
    else
    {
        CloseAllGroups();

        uintptr_t lowerWorkPtr = targetCopyPtr;
        std::vector<uintptr_t> sourceCopyPtrs;
        sourceCopyPtrs.reserve(sources.size());
        for (const SourceLayer& sourceInfo : sources)
        {
            LayerObject source;
            LayerObject sourceCopy;
            if (!FindLayer(sourceInfo.ptr, source) ||
                !DuplicateLayer(source, sourceCopy) ||
                !PrepareSourceCopy(sourceCopy.ptr, lowerWorkPtr))
                return failOperation("source copy preparation");

            sourceCopyPtrs.push_back(sourceCopy.ptr);
            lowerWorkPtr = sourceCopy.ptr;
        }

        if (!ValidateWorkStack(targetCopyPtr, sourceCopyPtrs))
            return failOperation("work stack validation");

        if (!MergeWorkStack(parentPtr, targetCopyPtr, sourceCopyPtrs, resultPtr))
            return failOperation("native merge");
    }

    LayerObject parentAfterMerge;
    std::set<uintptr_t> afterMergeChildren;
    if (!FindLayer(parentPtr, parentAfterMerge) ||
        !GetDirectChildPointers(parentAfterMerge, afterMergeChildren))
        return failOperation("merge result verification");
    for (uintptr_t originalPtr : originalChildren)
    {
        if (afterMergeChildren.find(originalPtr) == afterMergeChildren.end())
            return failOperation("original sibling verification");
    }

    if (!targetIsGroup && !NormalizeResult(resultPtr))
        return failOperation("result normalization");

    std::vector<uintptr_t> backupPtrs;
    backupPtrs.reserve(sources.size() + 1);
    backupPtrs.push_back(clippingTargetPtr);
    for (const SourceLayer& sourceInfo : sources)
        backupPtrs.push_back(sourceInfo.ptr);

    // Restore the document's presentation state before committing.  The
    // original target group's lock is then deliberately overridden below,
    // because it is retained as a hidden, locked recovery copy.
    RestoreGroupStates(groupStates);
    RestoreGroupStates(workGroupStates);

    originalsChanged = true;
    if (!HideAndLockLayers(backupPtrs, resultPtr,
                           targetIsGroup ? clippingTargetPtr : 0))
        return failOperation("backup commit");

    LayerObject result;
    if (!FindLayer(resultPtr, result))
        return failOperation("final result lookup");
    result.SetSelect();

    SDL_Log("Apply Effects Layer completed: %llu sources, target mode=%s",
        static_cast<unsigned long long>(sources.size()),
        targetIsGroup ? "preserved group" : "single layer");
    return Result::Success;
}
