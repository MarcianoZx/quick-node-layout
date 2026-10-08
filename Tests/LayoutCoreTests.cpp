#include "../Source/QuickNodeLayout/Private/LayoutStyles.h"
#include <cassert>
#include <chrono>
#include <iostream>

using namespace QuickLayout;
static void Validate(const std::vector<Node>& Nodes, const std::vector<Point>& Plan)
{
    assert(Nodes.size() == Plan.size());
    for (size_t A = 0; A < Plan.size(); ++A)
    {
        assert(std::isfinite(Plan[A].X) && std::isfinite(Plan[A].Y));
        for (size_t B = A + 1; B < Plan.size(); ++B)
            assert(Plan[A].X + Nodes[A].Width <= Plan[B].X ||
                   Plan[B].X + Nodes[B].Width <= Plan[A].X ||
                   Plan[A].Y + Nodes[A].Height <= Plan[B].Y ||
                   Plan[B].Y + Nodes[B].Height <= Plan[A].Y);
    }
}
int main()
{
    assert(Arrange({}, {}).empty());
    std::vector<Node> Nodes{{200,100,0,0}, {320,160,40,50}, {220,80,0,200}, {240,120,50,300}, {180,90,0,0}};
    std::vector<Edge> Edges{{0,1,8},{1,2,8},{1,3,8},{4,1,1}};
    auto Plan = Arrange(Nodes, Edges); Validate(Nodes, Plan);
    for (auto E : Edges) assert(Plan[E.From].X < Plan[E.To].X);
    assert(Plan[2].Y != Plan[3].Y);
    auto Repeat = Arrange(Nodes, Edges);
    for (size_t I = 0; I < Plan.size(); ++I) assert(Repeat[I].X == Plan[I].X && Repeat[I].Y == Plan[I].Y);
    Edges.push_back({3,0,8}); Edges.push_back({1,1,1});
    Validate(Nodes, Arrange(Nodes, Edges)); // Cycle and self-loop terminate without deleting wires.
    Nodes.push_back({600,400,0,900});
    Validate(Nodes, Arrange(Nodes, Edges)); // Disconnected island and differing sizes.
    for (int Seed = 1; Seed <= 30; ++Seed)
    {
        std::vector<Node> RandomNodes; std::vector<Edge> RandomEdges;
        unsigned State = Seed;
        auto Next = [&]() { State = State * 1664525u + 1013904223u; return State; };
        for (int I = 0; I < 80; ++I) RandomNodes.push_back({double(100 + Next()%500),double(60+Next()%400),double(Next()%2000),double(Next()%2000)});
        for (int I = 0; I < 180; ++I) RandomEdges.push_back({int(Next()%80),int(Next()%80),Next()%2 ? 8. : 1.});
        for (auto Mode : {Style::HumanFlow, Style::CompactFlow, Style::Layered, Style::Gentle})
            Validate(RandomNodes, ArrangeStyle(RandomNodes, RandomEdges, Mode));
    }
    std::vector<Node> FlowNodes{{200,100,0,0,true},{240,120,400,0,true},{200,100,800,0,true},
        {200,100,400,400,true},{160,80,200,200},{180,100,0,200}};
    std::vector<Edge> FlowEdges{{0,1,8},{1,2,8},{0,3,8},{4,1,1},{5,4,1}};
    const auto HumanPlan = ArrangeStyle(FlowNodes, FlowEdges, Style::HumanFlow);
    Validate(FlowNodes, HumanPlan);
    assert(HumanPlan[0].Y == HumanPlan[1].Y && HumanPlan[1].Y == HumanPlan[2].Y);
    assert(HumanPlan[3].Y > HumanPlan[0].Y);
    assert(HumanPlan[4].Y > HumanPlan[1].Y && HumanPlan[5].Y > HumanPlan[1].Y);
    const auto CompactPlan = ArrangeStyle(FlowNodes, FlowEdges, Style::CompactFlow);
    Validate(FlowNodes, CompactPlan);
    auto Area = [&](const auto& P) { double X=0,Y=0; for(size_t I=0;I<P.size();++I){ X=std::max(X,P[I].X+FlowNodes[I].Width); Y=std::max(Y,P[I].Y+FlowNodes[I].Height); } return X*Y; };
    assert(Area(CompactPlan) < Area(HumanPlan));
    std::vector<Node> CleanNodes{{100,100,0,0},{100,100,320,0},{100,100,0,320}};
    const auto GentlePlan = ArrangeStyle(CleanNodes, {}, Style::Gentle);
    for(size_t I=0;I<CleanNodes.size();++I) assert(GentlePlan[I].X==CleanNodes[I].X && GentlePlan[I].Y==CleanNodes[I].Y);
    std::vector<Node> Large(2000, {300,120,0,0}); std::vector<Edge> Chain;
    for (int I = 1; I < 2000; ++I) Chain.push_back({I-1,I,8});
    const auto Start = std::chrono::steady_clock::now();
    for (auto Mode : {Style::HumanFlow, Style::CompactFlow, Style::Layered, Style::Gentle})
        Validate(Large, ArrangeStyle(Large, Chain, Mode));
    std::cout << "PASS: four styles; straight execution, local inputs, compactness, gentle preservation, cycles, 30 random graphs, 2000 nodes ("
        << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-Start).count() << " ms).\n";
}
