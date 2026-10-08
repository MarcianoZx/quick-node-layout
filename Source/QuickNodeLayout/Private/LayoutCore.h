#pragma once

// Independent of Unreal: layout planning never modifies the source graph.
#include <algorithm>
#include <cmath>
#include <deque>
#include <numeric>
#include <vector>

namespace QuickLayout
{
struct Node { double Width, Height, X, Y; bool Execution = false; };
struct Edge { int From, To; double Weight; };
struct Point { double X = 0, Y = 0; };

inline std::vector<Point> Arrange(const std::vector<Node>& Nodes, const std::vector<Edge>& Edges,
    double HorizontalGap = 120, double VerticalGap = 80, double IslandGap = 240)
{
    const int Count = static_cast<int>(Nodes.size());
    std::vector<Point> Result(Count);
    std::vector<std::vector<int>> Out(Count), In(Count), Neighbors(Count);
    for (int E = 0; E < static_cast<int>(Edges.size()); ++E)
    {
        const auto& Link = Edges[E];
        if (Link.From < 0 || Link.To < 0 || Link.From >= Count || Link.To >= Count || Link.From == Link.To) continue;
        Out[Link.From].push_back(E); In[Link.To].push_back(E);
        Neighbors[Link.From].push_back(Link.To); Neighbors[Link.To].push_back(Link.From);
    }
    std::vector<int> Seeds(Count);
    std::iota(Seeds.begin(), Seeds.end(), 0);
    auto OriginalOrder = [&](int A, int B) {
        if (Nodes[A].Y != Nodes[B].Y) return Nodes[A].Y < Nodes[B].Y;
        if (Nodes[A].X != Nodes[B].X) return Nodes[A].X < Nodes[B].X;
        return A < B;
    };
    std::stable_sort(Seeds.begin(), Seeds.end(), OriginalOrder);
    std::vector<bool> Seen(Count, false), Done(Count, false);
    std::vector<int> Rank(Count, 0), Degree(Count, 0);
    std::vector<double> Order(Count, 0), Score(Count, 0);
    double Top = 0;
    for (int Seed : Seeds)
    {
        if (Seen[Seed]) continue;
        std::vector<int> Component{Seed}; Seen[Seed] = true;
        for (size_t Q = 0; Q < Component.size(); ++Q)
            for (int Other : Neighbors[Component[Q]])
                if (!Seen[Other]) { Seen[Other] = true; Component.push_back(Other); }
        std::stable_sort(Component.begin(), Component.end(), OriginalOrder);
        std::deque<int> Ready;
        for (int N : Component) { Degree[N] = static_cast<int>(In[N].size()); if (!Degree[N]) Ready.push_back(N); }
        int Processed = 0, MaxRank = 0;
        while (Processed < static_cast<int>(Component.size()))
        {
            // Cycles remain wired as-is. Choose a deterministic cut only for drawing.
            if (Ready.empty())
            {
                int Cut = -1;
                for (int N : Component) if (!Done[N] && (Cut < 0 || Degree[N] < Degree[Cut])) Cut = N;
                Ready.push_back(Cut);
            }
            const int N = Ready.front(); Ready.pop_front();
            if (Done[N]) continue;
            Done[N] = true; ++Processed; MaxRank = std::max(MaxRank, Rank[N]);
            for (int E : Out[N])
            {
                const int Next = Edges[E].To;
                if (Done[Next]) continue;
                Rank[Next] = std::max(Rank[Next], Rank[N] + 1);
                if (--Degree[Next] == 0) Ready.push_back(Next);
            }
        }
        std::vector<std::vector<int>> Columns(MaxRank + 1);
        for (int N : Component) Columns[Rank[N]].push_back(N);
        auto UpdateOrder = [&]() {
            for (const auto& Column : Columns)
                for (int I = 0; I < static_cast<int>(Column.size()); ++I) Order[Column[I]] = I;
        };
        UpdateOrder();
        // Weighted barycenters favor execution flow over auxiliary data wires.
        for (int Pass = 0; Pass < 6; ++Pass)
        {
            const bool Forward = (Pass % 2 == 0);
            for (int Step = 0; Step <= MaxRank; ++Step)
            {
                const int C = Forward ? Step : MaxRank - Step;
                for (int N : Columns[C])
                {
                    double Sum = 0, Weight = 0;
                    for (int E : (Forward ? In[N] : Out[N]))
                    {
                        const int Other = Forward ? Edges[E].From : Edges[E].To;
                        if ((Forward && Rank[Other] >= C) || (!Forward && Rank[Other] <= C)) continue;
                        Sum += Order[Other] * Edges[E].Weight; Weight += Edges[E].Weight;
                    }
                    Score[N] = Weight ? Sum / Weight : Order[N];
                }
                std::stable_sort(Columns[C].begin(), Columns[C].end(), [&](int A, int B) { return Score[A] < Score[B]; });
                UpdateOrder();
            }
        }
        double Left = 0;
        for (const auto& Column : Columns)
        {
            double Y = 0, Width = 0;
            for (int N : Column) { Result[N] = {Left, Y}; Y += Nodes[N].Height + VerticalGap; Width = std::max(Width, Nodes[N].Width); }
            Left += Width + HorizontalGap;
        }
        for (int Pass = 0; Pass < 4; ++Pass)
        {
            const bool Forward = Pass % 2 == 0;
            for (int Step = 0; Step <= MaxRank; ++Step)
            {
                const int C = Forward ? Step : MaxRank - Step;
                double Floor = 0;
                for (int N : Columns[C])
                {
                    double Sum = 0, Weight = 0;
                    for (int E : (Forward ? In[N] : Out[N]))
                    {
                        const int Other = Forward ? Edges[E].From : Edges[E].To;
                        if ((Forward && Rank[Other] >= C) || (!Forward && Rank[Other] <= C)) continue;
                        Sum += Result[Other].Y * Edges[E].Weight; Weight += Edges[E].Weight;
                    }
                    Result[N].Y = std::max(Floor, Weight ? Sum / Weight : Result[N].Y);
                    Floor = Result[N].Y + Nodes[N].Height + VerticalGap;
                }
            }
        }
        double Bottom = 0;
        for (int N : Component)
        {
            Bottom = std::max(Bottom, Result[N].Y + Nodes[N].Height);
            Result[N].X = std::round(Result[N].X / 16) * 16;
            Result[N].Y = std::round((Result[N].Y + Top) / 16) * 16;
        }
        Top += Bottom + IslandGap;
    }
    return Result;
}
}
