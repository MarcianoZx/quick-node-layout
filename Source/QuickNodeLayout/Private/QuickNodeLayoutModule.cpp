#include "Modules/ModuleManager.h"
#include "BlueprintEditor.h"
#include "BlueprintEditorContext.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphNode_Comment.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "GraphEditor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "Styling/AppStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "LayoutStyles.h"

#define LOCTEXT_NAMESPACE "QuickNodeLayout"

namespace
{
void Notify(const FText& Text)
{
    FNotificationInfo Info(Text); Info.ExpireDuration = 4.f;
    FSlateNotificationManager::Get().AddNotification(Info);
}

bool CanArrange(TWeakPtr<FBlueprintEditor> Editor)
{
    const auto Pinned = Editor.Pin();
    UEdGraph* Graph = Pinned ? Pinned->GetFocusedGraph() : nullptr;
    return Graph && Pinned->GetCurrentMode() != FName("DesignerName") &&
        Pinned->IsEditable(Graph) && !Pinned->IsGraphReadOnly(Graph) && Graph->bEditable && Graph->GetSchema() &&
        Graph->GetSchema()->IsA<UEdGraphSchema_K2>() && GEditor && !GEditor->PlayWorld;
}

FVector2D NodeSize(UEdGraphNode* Node, const TSharedPtr<SGraphEditor>& View)
{
    FSlateRect Bounds;
    if (View && View->GetBoundsForNode(Node, Bounds, 0.f) && Bounds.Right > Bounds.Left && Bounds.Bottom > Bounds.Top)
        return FVector2D(Bounds.Right - Bounds.Left, Bounds.Bottom - Bounds.Top);
    int32 Inputs = 0, Outputs = 0;
    for (const UEdGraphPin* Pin : Node->Pins)
        if (Pin && !Pin->bHidden) { if (Pin->Direction == EGPD_Input) ++Inputs; else ++Outputs; }
    return FVector2D(FMath::Max(280, Node->NodeWidth), FMath::Max(Node->NodeHeight, 72 + 28 * FMath::Max(Inputs, Outputs)));
}

void Arrange(TWeakPtr<FBlueprintEditor> Editor, QuickLayout::Style Mode)
{
    if (!CanArrange(Editor)) return;
    const auto Pinned = Editor.Pin();
    UEdGraph* Graph = Pinned->GetFocusedGraph();
    const auto View = SGraphEditor::FindGraphEditorForGraph(Graph);
    TSet<UEdGraphNode*> Selected;
    for (UObject* Object : Pinned->GetSelectedNodes())
        if (auto* Node = Cast<UEdGraphNode>(Object)) Selected.Add(Node);
    const bool bSelection = Selected.Num() > 0;

    TArray<UEdGraphNode*> All;
    TArray<UEdGraphNode_Comment*> Comments;
    TMap<UEdGraphNode*, FVector2D> Sizes;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node) continue;
        if (auto* Comment = Cast<UEdGraphNode_Comment>(Node)) Comments.Add(Comment);
        else { All.Add(Node); Sizes.Add(Node, NodeSize(Node, View)); }
    }
    // Capture geometric membership before moving anything; this also works for collapsed comments.
    TMap<UEdGraphNode_Comment*, TArray<UEdGraphNode*>> Members;
    for (auto* Comment : Comments)
        for (auto* Node : All)
        {
            const FVector2D Center = FVector2D(Node->NodePosX, Node->NodePosY) + Sizes[Node] * .5;
            if (Center.X >= Comment->NodePosX && Center.X <= Comment->NodePosX + Comment->NodeWidth &&
                Center.Y >= Comment->NodePosY && Center.Y <= Comment->NodePosY + Comment->NodeHeight)
            {
                Members.FindOrAdd(Comment).Add(Node);
                if (Selected.Contains(Comment)) Selected.Add(Node);
            }
        }
    TArray<UEdGraphNode*> Nodes;
    for (auto* Node : All) if (!bSelection || Selected.Contains(Node)) Nodes.Add(Node);
    if (Nodes.Num() < 2) { Notify(LOCTEXT("TooSmall", "Select at least two nodes, or clear the selection to organize the whole graph.")); return; }
    if (Nodes.Num() > 2000) { Notify(LOCTEXT("TooLarge", "Large graph: select a section with up to 2,000 nodes.")); return; }
    Nodes.Sort([](const UEdGraphNode& A, const UEdGraphNode& B) { return A.NodeGuid.ToString() < B.NodeGuid.ToString(); });

    TMap<UEdGraphNode*, int32> Indices;
    std::vector<QuickLayout::Node> Input;
    std::vector<QuickLayout::Edge> Edges;
    double AnchorX = Nodes[0]->NodePosX, AnchorY = Nodes[0]->NodePosY;
    for (int32 I = 0; I < Nodes.Num(); ++I)
    {
        auto* Node = Nodes[I]; Indices.Add(Node, I);
        const auto Size = Sizes[Node];
        bool bExecution = false;
        for (const auto* Pin : Node->Pins) if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) bExecution = true;
        Input.push_back({Size.X, Size.Y, double(Node->NodePosX), double(Node->NodePosY), bExecution});
        AnchorX = FMath::Min(AnchorX, double(Node->NodePosX)); AnchorY = FMath::Min(AnchorY, double(Node->NodePosY));
    }
    for (int32 I = 0; I < Nodes.Num(); ++I)
        for (auto* Pin : Nodes[I]->Pins)
            if (Pin && Pin->Direction == EGPD_Output)
                for (auto* Linked : Pin->LinkedTo)
                    if (Linked)
                        if (const int32* To = Indices.Find(Linked->GetOwningNode()))
                            Edges.push_back({I, *To, Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec ? 8.0 : 1.0});
    const auto Plan = QuickLayout::ArrangeStyle(Input, Edges, Mode);

    // Check actual node rectangles, not the empty space between selected nodes.
    // This keeps Gentle Cleanup from moving an already clean selection needlessly.
    if (bSelection)
    {
        bool Moved;
        do
        {
            Moved = false;
            for (auto* Other : All)
                if (!Indices.Contains(Other))
                {
                    const auto Size = Sizes[Other];
                    for (size_t I = 0; I < Plan.size(); ++I)
                    {
                        const double X = AnchorX + Plan[I].X, Y = AnchorY + Plan[I].Y;
                        if (X < Other->NodePosX + Size.X + 32 && X + Input[I].Width + 32 > Other->NodePosX &&
                            Y < Other->NodePosY + Size.Y + 32 && Y + Input[I].Height + 32 > Other->NodePosY)
                        { AnchorY = Other->NodePosY + Size.Y + 64 - Plan[I].Y; Moved = true; }
                    }
                }
        } while (Moved);
    }
    const FScopedTransaction Transaction(LOCTEXT("Transaction", "Organize Blueprint Nodes"));
    Graph->Modify();
    for (int32 I = 0; I < Nodes.Num(); ++I)
    {
        Nodes[I]->Modify();
        Nodes[I]->NodePosX = FMath::RoundToInt((AnchorX + Plan[I].X) / 16.0) * 16;
        Nodes[I]->NodePosY = FMath::RoundToInt((AnchorY + Plan[I].Y) / 16.0) * 16;
    }
    // Fit inner comments first, then include them inside enclosing comments.
    Comments.Sort([](const UEdGraphNode_Comment& A, const UEdGraphNode_Comment& B) {
        return double(A.NodeWidth) * A.NodeHeight < double(B.NodeWidth) * B.NodeHeight;
    });
    TMap<UEdGraphNode_Comment*, FSlateRect> Fitted;
    for (auto* Comment : Comments)
    {
        const auto* Group = Members.Find(Comment);
        if (!Group || Group->IsEmpty()) continue;
        bool bAffected = false;
        for (auto* Node : *Group) bAffected |= Indices.Contains(Node);
        if (!bAffected) continue;
        double L = TNumericLimits<double>::Max(), T = L, R = -L, B = -L;
        for (auto* Node : *Group)
        {
            L = FMath::Min(L, double(Node->NodePosX)); T = FMath::Min(T, double(Node->NodePosY));
            R = FMath::Max(R, Node->NodePosX + Sizes[Node].X); B = FMath::Max(B, Node->NodePosY + Sizes[Node].Y);
        }
        for (const auto& Inner : Fitted)
        {
            const auto& InnerGroup = Members[Inner.Key];
            bool bSubset = InnerGroup.Num() < Group->Num();
            for (auto* Node : InnerGroup) bSubset &= Group->Contains(Node);
            if (bSubset) { L = FMath::Min(L, double(Inner.Value.Left)); T = FMath::Min(T, double(Inner.Value.Top)); R = FMath::Max(R, double(Inner.Value.Right)); B = FMath::Max(B, double(Inner.Value.Bottom)); }
        }
        const FSlateRect Rect(L - 40, T - 64, R + 40, B + 40);
        Comment->Modify(); Comment->SetBounds(Rect); Fitted.Add(Comment, Rect);
    }
    if (auto* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph)) Blueprint->MarkPackageDirty();
    Graph->NotifyGraphChanged();
    Notify(FText::Format(LOCTEXT("Done", "{0} nodes organized. Ctrl+Z to undo; save when ready."), FText::AsNumber(Nodes.Num())));
}
}

class FQuickNodeLayoutModule : public IModuleInterface
{
public:
    void StartupModule() override
    {
        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FQuickNodeLayoutModule::RegisterMenus));
    }
    void ShutdownModule() override
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
    }
private:
    void RegisterMenus()
    {
        FToolMenuOwnerScoped Owner(this);
        UToolMenus::Get()->ExtendMenu("AssetEditor.DefaultToolBar")->AddDynamicSection("QuickNodeLayout",
            FNewToolMenuDelegate::CreateLambda([](UToolMenu* Menu)
            {
                auto* Context = Menu->FindContext<UBlueprintEditorToolMenuContext>();
                if (!Context || !Context->BlueprintEditor.IsValid()) return;
                const TWeakPtr<FBlueprintEditor> Editor = Context->BlueprintEditor;
                Menu->AddSection("QuickNodeLayout").AddEntry(FToolMenuEntry::InitComboButton(
                    "QuickNodeLayout.Arrange", FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([Editor] { return CanArrange(Editor); })),
                    FNewToolMenuDelegate::CreateLambda([Editor](UToolMenu* Choices)
                    {
                        auto& Section = Choices->AddSection("Styles", LOCTEXT("Styles", "Layout Style"));
                        const auto Add = [&](FName Id, const FText& Label, const FText& Hint, QuickLayout::Style Mode)
                        {
                            Section.AddMenuEntry(Id, Label, Hint, FSlateIcon(), FUIAction(
                                FExecuteAction::CreateLambda([Editor, Mode] { Arrange(Editor, Mode); }),
                                FCanExecuteAction::CreateLambda([Editor] { return CanArrange(Editor); })));
                        };
                        Add("HumanFlow", LOCTEXT("HumanFlow", "Human Flow"),
                            LOCTEXT("HumanHint", "Keep the first execution path straight; group data inputs below their consumer nodes. Recommended."), QuickLayout::Style::HumanFlow);
                        Add("CompactFlow", LOCTEXT("CompactFlow", "Compact Flow"),
                            LOCTEXT("CompactHint", "Use the same execution-focused layout with tighter spacing."), QuickLayout::Style::CompactFlow);
                        Add("Layered", LOCTEXT("Layered", "Layered Layout"),
                            LOCTEXT("LayeredHint", "Arrange all dependencies in columns using the original layout algorithm."), QuickLayout::Style::Layered);
                        Add("Gentle", LOCTEXT("Gentle", "Gentle Cleanup"),
                            LOCTEXT("GentleHint", "Keep the existing arrangement; snap to grid and push overlapping nodes down."), QuickLayout::Style::Gentle);
                    }),
                    LOCTEXT("Button", "Organize Nodes"),
                    LOCTEXT("Tooltip", "Choose a layout for the selected nodes, or the whole graph if nothing is selected. Ctrl+Z to undo."),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.AlignNodesTop")));
            }));
    }
};

IMPLEMENT_MODULE(FQuickNodeLayoutModule, QuickNodeLayout)
#undef LOCTEXT_NAMESPACE
