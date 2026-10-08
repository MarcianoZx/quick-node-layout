#pragma once
#include "LayoutCore.h"
#include <utility>

namespace QuickLayout
{
enum class Style { HumanFlow, CompactFlow, Layered, Gentle };

inline std::vector<Point> Gentle(const std::vector<Node>& Nodes)
{
    std::vector<Point> Result(Nodes.size());
    if (Nodes.empty()) return Result;
    double Left = Nodes[0].X, Top = Nodes[0].Y;
    for (const auto& N : Nodes) { Left = std::min(Left, N.X); Top = std::min(Top, N.Y); }
    std::vector<int> Order(Nodes.size()); std::iota(Order.begin(), Order.end(), 0);
    std::stable_sort(Order.begin(), Order.end(), [&](int A, int B) {
        return Nodes[A].Y == Nodes[B].Y ? Nodes[A].X < Nodes[B].X : Nodes[A].Y < Nodes[B].Y;
    });
    std::vector<int> Placed;
    for (int N : Order)
    {
        auto& P = Result[N];
        P = {std::round((Nodes[N].X - Left) / 16) * 16, std::round((Nodes[N].Y - Top) / 16) * 16};
        bool Moved;
        do
        {
            Moved = false;
            for (int Other : Placed)
            {
                const auto& Q = Result[Other];
                if (P.X < Q.X + Nodes[Other].Width + 32 && P.X + Nodes[N].Width + 32 > Q.X &&
                    P.Y < Q.Y + Nodes[Other].Height + 32 && P.Y + Nodes[N].Height + 32 > Q.Y)
                { P.Y = std::ceil((Q.Y + Nodes[Other].Height + 32) / 16) * 16; Moved = true; }
            }
        } while (Moved);
        Placed.push_back(N);
    }
    return Result;
}

inline std::vector<Point> Human(const std::vector<Node>& Nodes, const std::vector<Edge>& Edges, bool Compact)
{
    const int Count = static_cast<int>(Nodes.size());
    std::vector<bool> IsExecution(Count);
    std::vector<std::vector<int>> Inputs(Count), Flow(Count);
    for (int N = 0; N < Count; ++N) IsExecution[N] = Nodes[N].Execution;
    for (const auto& E : Edges)
    {
        if (E.From < 0 || E.To < 0 || E.From >= Count || E.To >= Count || E.From == E.To) continue;
        Inputs[E.To].push_back(E.From);
        if (E.Weight >= 8) { IsExecution[E.From] = IsExecution[E.To] = true; Flow[E.From].push_back(E.To); }
    }
    std::vector<int> Anchors;
    for (int N = 0; N < Count; ++N) if (IsExecution[N]) Anchors.push_back(N);
    if (Anchors.empty()) return Arrange(Nodes, Edges, Compact ? 64 : 120, Compact ? 32 : 64);
    std::stable_sort(Anchors.begin(), Anchors.end(), [&](int A, int B) {
        return Nodes[A].Y == Nodes[B].Y ? Nodes[A].X < Nodes[B].X : Nodes[A].Y < Nodes[B].Y;
    });

    // Assign each pure dependency to its closest downstream execution node.
    // Shared inputs are placed once, not duplicated or disconnected.
    std::vector<int> Owner(Count, -1), AnchorIndex(Count, -1);
    std::deque<int> Queue;
    for (int I = 0; I < static_cast<int>(Anchors.size()); ++I)
    { Owner[Anchors[I]] = I; AnchorIndex[Anchors[I]] = I; Queue.push_back(Anchors[I]); }
    while (!Queue.empty())
    {
        const int N = Queue.front(); Queue.pop_front();
        for (int Previous : Inputs[N])
            if (!IsExecution[Previous] && Owner[Previous] < 0)
            { Owner[Previous] = Owner[N]; Queue.push_back(Previous); }
    }
    const int Groups = static_cast<int>(Anchors.size());
    std::vector<std::vector<int>> Members(Groups);
    std::vector<std::vector<Point>> Local(Groups);
    std::vector<Node> Blocks;
    std::vector<Edge> BlockEdges;
    std::vector<int> LocalIndex(Count, -1);
    const double Padding = Compact ? 40 : 80;
    for (int N = 0; N < Count; ++N) if (!IsExecution[N] && Owner[N] >= 0) Members[Owner[N]].push_back(N);
    for (int G = 0; G < Groups; ++G)
    {
        std::vector<Node> Subnodes; std::vector<Edge> Subedges;
        for (int N : Members[G]) { LocalIndex[N] = static_cast<int>(Subnodes.size()); Subnodes.push_back(Nodes[N]); }
        for (const auto& E : Edges)
            if (E.From >= 0 && E.To >= 0 && E.From < Count && E.To < Count &&
                Owner[E.From] == G && Owner[E.To] == G && !IsExecution[E.From] && !IsExecution[E.To])
                Subedges.push_back({LocalIndex[E.From], LocalIndex[E.To], E.Weight});
        Local[G] = Arrange(Subnodes, Subedges, Compact ? 40 : 80, Compact ? 32 : 64, Compact ? 64 : 128);
        double Width = Nodes[Anchors[G]].Width, Height = Nodes[Anchors[G]].Height;
        double DataHeight = 0;
        for (size_t I = 0; I < Subnodes.size(); ++I)
        { Width = std::max(Width, Local[G][I].X + Subnodes[I].Width); DataHeight = std::max(DataHeight, Local[G][I].Y + Subnodes[I].Height); }
        if (!Subnodes.empty()) Height += Padding + DataHeight;
        Blocks.push_back({Width, Height, Nodes[Anchors[G]].X, Nodes[Anchors[G]].Y});
    }
    std::vector<int> InDegree(Groups, 0);
    for (int N : Anchors) for (int Next : Flow[N])
    { BlockEdges.push_back({AnchorIndex[N], AnchorIndex[Next], 8}); ++InDegree[AnchorIndex[Next]]; }
    auto BlockPlan = Arrange(Blocks, BlockEdges, Compact ? 80 : 160);
    // First execution output stays on the same horizontal lane. Other outputs
    // branch down in pin order. Visited guards keep loops and shared merges finite.
    std::vector<int> Lane(Groups, -1);
    std::vector<int> VisitOrder;
    int LaneCount = 0;
    std::vector<int> Roots;
    for (int G = 0; G < Groups; ++G) if (!InDegree[G]) Roots.push_back(G);
    for (int G = 0; G < Groups; ++G) if (InDegree[G]) Roots.push_back(G);
    for (int Root : Roots)
    {
        if (Lane[Root] >= 0) continue;
        std::vector<std::pair<int,int>> Stack{{Root, -1}};
        while (!Stack.empty())
        {
            auto Item = Stack.back(); Stack.pop_back();
            const int G = Item.first;
            if (Lane[G] >= 0) continue;
            Lane[G] = Item.second < 0 ? LaneCount++ : Item.second;
            VisitOrder.push_back(G);
            const auto& Next = Flow[Anchors[G]];
            for (int I = static_cast<int>(Next.size()) - 1; I >= 0; --I)
                Stack.push_back({AnchorIndex[Next[I]], I == 0 ? Lane[G] : -1});
        }
    }
    std::vector<double> LaneHeight(LaneCount, 0), LaneTop(LaneCount, 0);
    for (int G = 0; G < Groups; ++G) LaneHeight[Lane[G]] = std::max(LaneHeight[Lane[G]], Blocks[G].Height);
    for (int L = 1; L < LaneCount; ++L) LaneTop[L] = LaneTop[L-1] + LaneHeight[L-1] + (Compact ? 96 : 192);
    std::vector<Point> Result(Count);
    std::vector<double> LaneRight(LaneCount, 0);
    double Bottom = 0;
    // Disconnected blocks can share X; each root has its own lane.
    for (int G : VisitOrder)
    {
        const int N = Anchors[G];
        const double X = std::max(BlockPlan[G].X, LaneRight[Lane[G]]), Y = LaneTop[Lane[G]];
        LaneRight[Lane[G]] = X + Blocks[G].Width + (Compact ? 80 : 160);
        Result[N] = {X + Blocks[G].Width - Nodes[N].Width, Y};
        for (size_t I = 0; I < Members[G].size(); ++I)
            Result[Members[G][I]] = {X + Local[G][I].X, Y + Nodes[N].Height + Padding + Local[G][I].Y};
        Bottom = std::max(Bottom, Y + Blocks[G].Height);
    }
    std::vector<int> Orphans; std::vector<Node> OrphanNodes; std::vector<Edge> OrphanEdges;
    for (int N = 0; N < Count; ++N) if (Owner[N] < 0)
    { LocalIndex[N] = static_cast<int>(OrphanNodes.size()); Orphans.push_back(N); OrphanNodes.push_back(Nodes[N]); }
    for (const auto& E : Edges)
        if (E.From >= 0 && E.To >= 0 && E.From < Count && E.To < Count && Owner[E.From] < 0 && Owner[E.To] < 0)
            OrphanEdges.push_back({LocalIndex[E.From], LocalIndex[E.To], E.Weight});
    const auto OrphanPlan = Arrange(OrphanNodes, OrphanEdges);
    for (size_t I = 0; I < Orphans.size(); ++I) Result[Orphans[I]] = {OrphanPlan[I].X, Bottom + 240 + OrphanPlan[I].Y};
    for (auto& P : Result) { P.X = std::round(P.X / 16) * 16; P.Y = std::round(P.Y / 16) * 16; }
    return Result;
}

inline std::vector<Point> ArrangeStyle(const std::vector<Node>& Nodes, const std::vector<Edge>& Edges, Style Mode)
{
    switch (Mode)
    {
    case Style::HumanFlow: return Human(Nodes, Edges, false);
    case Style::CompactFlow: return Human(Nodes, Edges, true);
    case Style::Gentle: return Gentle(Nodes);
    default: return Arrange(Nodes, Edges);
    }
}
}
