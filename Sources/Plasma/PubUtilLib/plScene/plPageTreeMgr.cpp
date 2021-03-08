/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/

#include "plPageTreeMgr.h"

#include "HeadSpin.h"
#include "hsFastMath.h"
#include "plDrawable.h"
#include "plPipeline.h"
#include "plProfile.h"
#include "hsTemplates.h"
#include "plTweak.h"

#include <algorithm>

#include "plCullPoly.h"
#include "plOccluder.h"
#include "plSceneNode.h"
#include "plVisMgr.h"

#include "plDrawable/plSpaceTreeMaker.h"
#include "plDrawable/plSpaceTree.h"
#include "plMath/hsRadixSort.h"

static std::vector<hsRadixSortElem> scratchList;

bool plPageTreeMgr::fDisableVisMgr = false;

plProfile_CreateTimer("Object Sort", "Draw", DrawObjSort);
plProfile_CreateCounter("Objects Sorted", "Draw", DrawObjSorted);
plProfile_CreateTimer("Occluder Sort", "Draw", DrawOccSort);
plProfile_CreateCounter("Occluders Used", "Draw", DrawOccUsed);
plProfile_CreateTimer("Occluder Build", "Draw", DrawOccBuild);
plProfile_CreateCounter("Occluder Polys Processed", "Draw", DrawOccPolyProc);
plProfile_CreateTimer("Occluder Poly Sort", "Draw", DrawOccPolySort);

plPageTreeMgr::plPageTreeMgr()
:   fSpaceTree()
{
    fVisMgr = plGlobalVisMgr::Instance();
}

plPageTreeMgr::~plPageTreeMgr()
{
    delete fSpaceTree;
}

void plPageTreeMgr::AddNode(plSceneNode* node)
{
    ITrashSpaceTree();

    node->Init();

    fNodes.push_back(node);
}

void plPageTreeMgr::RemoveNode(plSceneNode* node)
{
    ITrashSpaceTree();

    auto it = std::find(fNodes.begin(), fNodes.end(), node);
    if (it != fNodes.end())
        fNodes.erase(it);
}

void plPageTreeMgr::Reset()
{
    fNodes.clear();

    ITrashSpaceTree();
}

void plPageTreeMgr::ITrashSpaceTree()
{
    delete fSpaceTree;
    fSpaceTree = nullptr;
}

bool plPageTreeMgr::Harvest(plVolumeIsect* isect, hsTArray<plDrawVisList>& levList)
{
    levList.SetCount(0);
    if( !(GetSpaceTree() || IBuildSpaceTree()) )
        return false;

    static std::vector<int16_t> list;

    GetSpaceTree()->HarvestLeaves(isect, list);

    for (int16_t idx : list)
    {
        fNodes[idx]->Harvest(isect, levList);
    }

    return levList.GetCount() > 0;
}

#include "plProfile.h"
plProfile_CreateTimer("DrawableTime", "Draw", DrawableTime);
plProfile_Extern(RenderScene);

int plPageTreeMgr::Render(plPipeline* pipe)
{
    // If we don't have a space tree and can't make one, just bail
    if( !(GetSpaceTree() || IBuildSpaceTree()) )
        return 0;

    static std::vector<int16_t> list;
    list.clear();

    plProfile_BeginTiming(RenderScene);

    plVisMgr* visMgr = fDisableVisMgr ? nullptr : fVisMgr;

    if( visMgr )
    {
        plProfile_Extern(VisEval);
        plProfile_BeginTiming(VisEval);
        visMgr->Eval(pipe->GetViewPositionWorld());
        plProfile_EndTiming(VisEval);
    }

    pipe->BeginVisMgr(visMgr);

    IRefreshTree(pipe);

    IGetOcclusion(pipe, list);
    pipe->HarvestVisible(GetSpaceTree(), list);

    static hsTArray<plDrawVisList> levList;
    levList.SetCount(0);
    for (int16_t idx : list)
    {
        fNodes[idx]->CollectForRender(pipe, levList, visMgr);
    }
    
    int numDrawn = IRenderVisList(pipe, levList);

    IResetOcclusion(pipe);

    pipe->EndVisMgr(visMgr);

    plProfile_EndTiming(RenderScene);

    return numDrawn;

}

int plPageTreeMgr::IRenderVisList(plPipeline* pipe, hsTArray<plDrawVisList>& levList)
{
    // Sort levList into sortedDrawList, which is just a list
    // of drawable/visList pairs in ascending render priority order.
    // visLists are just lists of span indices, but only of the
    // spans which are visible (on screen and non-occluded and non-disabled).
    static hsTArray<plDrawVisList> sortedDrawList;
    if( !ISortByLevel(pipe, levList, sortedDrawList) )
    {
        return 0;
    }

    int numDrawn = 0;

    plVisMgr* visMgr = fDisableVisMgr ? nullptr : fVisMgr;

    // Going through the list in order, if we hit a drawable which doesn't need
    // its spans sorted, we can just draw it.
    // If we hit a drawable which does need its spans sorted, we could just draw
    // it, but that precludes sorting spans between drawables (like the player avatar
    // sorting with normal scene objects). So when we hit a drawable which needs
    // span sorting, we sort its spans with the spans of the next N drawables in
    // the sorted list which have the same render priority and which also want their
    // spans sorted.
    int i;
    for( i = 0; i < sortedDrawList.GetCount(); i++ )
    {
        plDrawable* p = sortedDrawList[i].fDrawable;


        plProfile_BeginLap(DrawableTime, p->GetKey()->GetUoid().GetObjectName().c_str());
    
        if( sortedDrawList[i].fDrawable->GetNativeProperty(plDrawable::kPropSortSpans) )
        {
            // IPrepForRenderSortingSpans increments "i" to the next index to be drawn (-1 so the i++
            // at the top of the loop is correct.
            numDrawn += IPrepForRenderSortingSpans(pipe, sortedDrawList, i);
        }
        else
        {
            pipe->PrepForRender(sortedDrawList[i].fDrawable, sortedDrawList[i].fVisList, visMgr);

            pipe->Render(sortedDrawList[i].fDrawable, sortedDrawList[i].fVisList);

            numDrawn += sortedDrawList[i].fVisList.size();

        }

        plProfile_EndLap(DrawableTime, p->GetKey()->GetUoid().GetObjectName().c_str());
    }

    return numDrawn;
}

bool plPageTreeMgr::ISortByLevel(plPipeline* pipe, hsTArray<plDrawVisList>& drawList, hsTArray<plDrawVisList>& sortedDrawList)
{
    sortedDrawList.SetCount(0);

    if( !drawList.GetCount() )
        return false;

    scratchList.resize(drawList.GetCount());

    hsRadixSort::Elem* listTrav = nullptr;
    int i;
    for( i = 0; i < drawList.GetCount(); i++ )
    {
        listTrav = &scratchList[i];
        listTrav->fBody = (void*)&drawList[i];
        listTrav->fNext = listTrav+1;
        listTrav->fKey.fULong = drawList[i].fDrawable->GetRenderLevel().Level();
    }
    listTrav->fNext = nullptr;

    hsRadixSort rad;
    hsRadixSort::Elem* sortedList = rad.Sort(scratchList.data(), hsRadixSort::kUnsigned);

    listTrav = sortedList;

    while( listTrav )
    {
        plDrawVisList& drawVis = *(plDrawVisList*)listTrav->fBody;
        sortedDrawList.Append(drawVis);
        
        listTrav = listTrav->fNext;
    }

    return true;
}

// Render from iDrawStart in drawVis list all drawables with the sort by spans property, well, sorting
// by spans.
// Returns the index of the last one drawn.
int plPageTreeMgr::IPrepForRenderSortingSpans(plPipeline* pipe, hsTArray<plDrawVisList>& drawVis, int& iDrawStart)
{
    uint32_t renderLevel = drawVis[iDrawStart].fDrawable->GetRenderLevel().Level();

    static hsTArray<plDrawVisList*> drawables;
    static hsTArray<plDrawSpanPair> pairs;

    // Given the input drawVisList (list of drawable/visList pairs), we make two new
    // lists. The list "drawables" is just the excerpted sub-list from drawVis starting
    // from the input index and going through all compatible drawables (drawables which
    // are appropriate to sort (and hence intermix) with the first drawable in the list.
    // The second list is the drawableIndex/spanIndex pairs convenient for sorting (where
    // drawIndex indexes into drawables and spanIndex indexes into drawVis[iDraw].fVisList.
    // So pairs[i] resolves into 
    // drawables[pairs[i].fDrawable].fDrawable->GetSpan(pairs[i].fSpan)

    drawables.Append(&drawVis[iDrawStart]);
    for (int16_t idx : drawVis[iDrawStart].fVisList)
    {
        plDrawSpanPair* pair = pairs.Push();
        pair->fDrawable = 0;
        pair->fSpan = idx;
    }

    int iDraw;
    for( iDraw = iDrawStart+1; 
        (iDraw < drawVis.GetCount())
        && (drawVis[iDraw].fDrawable->GetRenderLevel().Level() == renderLevel)
        && drawVis[iDraw].fDrawable->GetNativeProperty(plDrawable::kPropSortSpans);
        iDraw++ )
    {
        for (int16_t idx : drawVis[iDraw].fVisList)
        {
            plDrawSpanPair* pair = pairs.Push();
            pair->fDrawable = drawables.GetCount();
            pair->fSpan = idx;
        }
        drawables.Append(&drawVis[iDraw]);
    }

    // Now that we have them in a more convenient format, sort them and render.
    IRenderSortingSpans(pipe, drawables, pairs);

    int numDrawn = pairs.GetCount();

    drawables.SetCount(0);
    pairs.SetCount(0);

    iDrawStart = iDraw - 1;

    return numDrawn;
}

bool plPageTreeMgr::IRenderSortingSpans(plPipeline* pipe, hsTArray<plDrawVisList*>& drawList, hsTArray<plDrawSpanPair>& pairs)
{

    if( !pairs.GetCount() )
        return false;

    hsPoint3 viewPos = pipe->GetViewPositionWorld();

    plProfile_BeginTiming(DrawObjSort);
    plProfile_IncCount(DrawObjSorted, pairs.GetCount());

    hsRadixSort::Elem* listTrav;
    scratchList.resize(pairs.GetCount());

    // First, sort on distance to the camera (squared).
    listTrav = nullptr;
    int iSort = 0;
    int i;
    for( i = 0; i < pairs.GetCount(); i++ )
    {
        plDrawable* drawable = drawList[pairs[i].fDrawable]->fDrawable;

        listTrav = &scratchList[iSort++];
        listTrav->fBody = (void*)&pairs[i];
        listTrav->fNext = listTrav + 1;

        if( drawable->GetNativeProperty(plDrawable::kPropSortAsOne) )
        {
            const hsBounds3Ext& bnd = drawable->GetSpaceTree()->GetNode(drawable->GetSpaceTree()->GetRoot()).fWorldBounds;
            plConst(float) kDistFudge(1.e-1f);
            listTrav->fKey.fFloat = -(bnd.GetCenter() - viewPos).MagnitudeSquared() + float(pairs[i].fSpan) * kDistFudge;
        }
        else
        {
            const hsBounds3Ext& bnd = drawable->GetSpaceTree()->GetNode(pairs[i].fSpan).fWorldBounds;
            listTrav->fKey.fFloat = -(bnd.GetCenter() - viewPos).MagnitudeSquared();
        }
    }
    if( !listTrav )
    {
        plProfile_EndTiming(DrawObjSort);
        return false;
    }
    listTrav->fNext = nullptr;

    hsRadixSort rad;
    hsRadixSort::Elem* sortedList = rad.Sort(scratchList.data(), 0);

    plProfile_EndTiming(DrawObjSort);

    static std::vector<int16_t> visList;
    visList.clear();

    plVisMgr* visMgr = fDisableVisMgr ? nullptr : fVisMgr;

    // Call PrepForRender on each of these bad boys. We only want to call
    // PrepForRender once on each drawable, no matter how many times we're
    // going to pass it off to be rendered (like if we render span 0 from
    // drawable A, span 1 from drawable A, span 0 from drawable B, span 1 from Drawable A, we
    // don't want to PrepForRender twice or three times on drawable A).
    // So we're going to convert our sorted list back into a list of drawable/visList
    // pairs. We could have done this with our original drawable/visList, but we've
    // hopefully trimmed out some spans because of the fades. This drawable/visList
    // isn't appropriate for rendering (because it doesn't let us switch back and forth 
    // from a drawable, but it's right for the PrepForRenderCall (which does things like
    // face sorting).
    for( i = 0; i < drawList.GetCount(); i++ )
        drawList[i]->fVisList.clear();
    listTrav = sortedList;
    while( listTrav )
    {
        plDrawSpanPair& curPair = *(plDrawSpanPair*)listTrav->fBody;
        drawList[curPair.fDrawable]->fVisList.emplace_back(curPair.fSpan);
        listTrav = listTrav->fNext;
    }
    for( i = 0; i < drawList.GetCount(); i++ )
    {
        pipe->PrepForRender(drawList[i]->fDrawable, drawList[i]->fVisList, visMgr);
    }

    // We'd like to call Render once on a drawable for each contiguous
    // set of spans (so we want to render span 0 and span 1 on a single Render
    // of drawable A in the above, then render drawable B, then back to A).
    // So we go through the sorted drawable/spanIndex pairs list, building
    // a visList for as long as the drawable remains the same. When it
    // changes, we render what we have so far, and start again with the
    // next drawable. Repeat until done.

#if 0
    listTrav = sortedList;
    plDrawSpanPair& curPair = *(plDrawSpanPair*)listTrav->fBody;
    int curDraw = curPair.fDrawable;
    visList.emplace_back(curPair.fSpan);
    listTrav = listTrav->fNext;

    while( listTrav )
    {
        curPair = *(plDrawSpanPair*)listTrav->fBody;
        if( curPair.fDrawable != curDraw )
        {
            pipe->Render(drawList[curDraw]->fDrawable, visList);
            curDraw = curPair.fDrawable;
            visList.clear();
            visList.emplace_back(curPair.fSpan);
        }
        else
        {
            visList.emplace_back(curPair.fSpan);
        }
        listTrav = listTrav->fNext;
    }
    pipe->Render(drawList[curDraw]->fDrawable, visList);
#else
    listTrav = sortedList;
    plDrawSpanPair& curPair = *(plDrawSpanPair*)listTrav->fBody;
    int curDraw = curPair.fDrawable;
    listTrav = listTrav->fNext;

    static hsTArray<uint32_t> numDrawn;
    numDrawn.SetCountAndZero(drawList.GetCount());

    visList.emplace_back(drawList[curDraw]->fVisList[numDrawn[curDraw]++]);

    while( listTrav )
    {
        curPair = *(plDrawSpanPair*)listTrav->fBody;
        if( curPair.fDrawable != curDraw )
        {
            pipe->Render(drawList[curDraw]->fDrawable, visList);
            curDraw = curPair.fDrawable;
            visList.clear();
        }
        visList.emplace_back(drawList[curDraw]->fVisList[numDrawn[curDraw]++]);
        listTrav = listTrav->fNext;
    }
    pipe->Render(drawList[curDraw]->fDrawable, visList);
#endif

    return true;
}

bool plPageTreeMgr::IBuildSpaceTree()
{
    if (fNodes.empty())
        return false;

    plSpaceTreeMaker maker;
    maker.Reset();
    for (plSceneNode* node : fNodes)
        maker.AddLeaf(node->GetSpaceTree()->GetWorldBounds(), node->GetSpaceTree()->IsEmpty());
    fSpaceTree = maker.MakeTree();

    return true;
}

bool plPageTreeMgr::IRefreshTree(plPipeline* pipe)
{
    hsAssert(fNodes.size() < std::numeric_limits<uint16_t>::max(), "Too many nodes");
    for (uint16_t i = 0; i < fNodes.size(); ++i)
    {
        plSceneNode* node = fNodes[i];
        if (node->GetSpaceTree()->IsDirty())
        {
            node->GetSpaceTree()->Refresh();

            GetSpaceTree()->MoveLeaf(i, node->GetSpaceTree()->GetWorldBounds());

            if (!node->GetSpaceTree()->IsEmpty() && fSpaceTree->HasLeafFlag(i, plSpaceTreeNode::kDisabled) )
                fSpaceTree->SetLeafFlag(i, plSpaceTreeNode::kDisabled, false);
        }
    }

    GetSpaceTree()->SetViewPos(pipe->GetViewPositionWorld());

    GetSpaceTree()->Refresh();

    return true;
}

void plPageTreeMgr::AddOccluderList(const hsTArray<plOccluder*> occList)
{
    int iStart = fOccluders.GetCount();
    fOccluders.Resize(iStart + occList.GetCount());

    plVisMgr* visMgr = fDisableVisMgr ? nullptr : fVisMgr;

    if( visMgr )
    {
        const hsBitVector& visSet = visMgr->GetVisSet();
        const hsBitVector& visNot = visMgr->GetVisNot();
        int i;
        for( i = 0; i < occList.GetCount(); i++ )
        {
            if( occList[i] && !occList[i]->InVisNot(visNot) && occList[i]->InVisSet(visSet) )
                fOccluders[iStart++] = occList[i];
        }
    }
    else
    {
        int i;
        for( i = 0; i < occList.GetCount(); i++ )
        {
            if( occList[i] )
                fOccluders[iStart++] = occList[i];
        }
    }
    fOccluders.SetCount(iStart);

}

void plPageTreeMgr::IAddCullPolyList(const std::vector<plCullPoly>& polyList)
{
    int iStart = fCullPolys.GetCount();
    fCullPolys.Resize(iStart + polyList.size());
    for (size_t i = 0; i < polyList.size(); i++)
    {
        fCullPolys[i + iStart] = &polyList[i];
    }
}

void plPageTreeMgr::ISortCullPolys(plPipeline* pipe)
{
    fSortedCullPolys.clear();
    if( !fCullPolys.GetCount() )
        return;

    constexpr size_t kMaxCullPolys = 300;
    size_t numSubmit = 0;

    hsPoint3 viewPos = pipe->GetViewPositionWorld();

    hsRadixSort::Elem* listTrav;
    scratchList.resize(fCullPolys.GetCount());
    for (int i = 0; i < fCullPolys.GetCount(); i++)
    {
        bool backFace = fCullPolys[i]->fNorm.InnerProduct(viewPos) + fCullPolys[i]->fDist <= 0;
        if( backFace )
        {
            if( !fCullPolys[i]->IsHole() && !fCullPolys[i]->IsTwoSided() )
                continue;
        }
        else
        {
            if( fCullPolys[i]->IsHole() )
                continue;
        }

        listTrav = &scratchList[numSubmit];
        listTrav->fBody = (void*)fCullPolys[i];
        listTrav->fNext = listTrav + 1;
        listTrav->fKey.fFloat = (fCullPolys[i]->GetCenter() - viewPos).MagnitudeSquared();

        numSubmit++;
    }
    if( !numSubmit )
        return;

    listTrav->fNext = nullptr;

    hsRadixSort rad;
    hsRadixSort::Elem* sortedList = rad.Sort(scratchList.data(), 0);
    listTrav = sortedList;

    if( numSubmit > kMaxCullPolys )
        numSubmit = kMaxCullPolys;

    fSortedCullPolys.resize(numSubmit);

    for (size_t i = 0; i < numSubmit; i++)
    {
        fSortedCullPolys[i] = (const plCullPoly*)listTrav->fBody;
        listTrav = listTrav->fNext;
    }
}

bool plPageTreeMgr::IGetCullPolys(plPipeline* pipe)
{
    if( !fOccluders.GetCount() )
        return false;

    plProfile_BeginTiming(DrawOccSort);

    hsRadixSort::Elem* listTrav = nullptr;
    scratchList.resize(fOccluders.GetCount());

    hsPoint3 viewPos = pipe->GetViewPositionWorld();

    // cull test the occluders submitted
    int numSubmit = 0;
    int i;
    for( i = 0; i < fOccluders.GetCount(); i++ )
    {
        if( pipe->TestVisibleWorld(fOccluders[i]->GetWorldBounds()) )
        {
            float invDist = -hsFastMath::InvSqrtAppr((viewPos - fOccluders[i]->GetWorldBounds().GetCenter()).MagnitudeSquared());
            listTrav = &scratchList[numSubmit++];
            listTrav->fBody = (void*)fOccluders[i];
            listTrav->fNext = listTrav+1;
            listTrav->fKey.fFloat = fOccluders[i]->GetPriority() * invDist;
        }
    }
    if( !listTrav )
    {
        plProfile_EndTiming(DrawOccSort);
        return false;
    }

    listTrav->fNext = nullptr;


    // Sort the occluders by priority
    hsRadixSort rad;
    hsRadixSort::Elem* sortedList = rad.Sort(scratchList.data(), 0);
    listTrav = sortedList;

    const uint32_t kMaxOccluders = 1000;
    if( numSubmit > kMaxOccluders )
        numSubmit = kMaxOccluders;

    plProfile_IncCount(DrawOccUsed, numSubmit);

    // Take the polys from the first N of them
    for( i = 0; i < numSubmit; i++ )
    {
        plOccluder* occ = (plOccluder*)listTrav->fBody;
        IAddCullPolyList(occ->GetWorldPolyList());
        
        listTrav = listTrav->fNext;
    }

    plProfile_EndTiming(DrawOccSort);

    return fCullPolys.GetCount() > 0;
}

bool plPageTreeMgr::IGetOcclusion(plPipeline* pipe, std::vector<int16_t>& list)
{
    plProfile_BeginTiming(DrawOccBuild);

    fCullPolys.SetCount(0);
    fOccluders.SetCount(0);
    for (plSceneNode* node : fNodes)
        node->SubmitOccluders(this);

    if( !IGetCullPolys(pipe) )
    {
        plProfile_EndTiming(DrawOccBuild);
        return false;
    }

    plProfile_IncCount(DrawOccPolyProc, fCullPolys.GetCount());

    plProfile_BeginTiming(DrawOccPolySort);
    ISortCullPolys(pipe);
    plProfile_EndTiming(DrawOccPolySort);

    if (!fSortedCullPolys.empty())
        pipe->SubmitOccluders(fSortedCullPolys);

    plProfile_EndTiming(DrawOccBuild);

    return !fSortedCullPolys.empty();
}

void plPageTreeMgr::IResetOcclusion(plPipeline* pipe)
{
    fCullPolys.SetCount(0);
    fSortedCullPolys.clear();
}
