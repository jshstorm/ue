// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved. 

#include "SCCGraphPalette.h"
#include "LevelDesignEditor.h"
#include "Graph/EdGraphSchema_LevelDesignProp.h"
#include "Graph/LDNode_Root.h"

void SCCGraphPalette::Construct(const FArguments& InArgs, TWeakPtr<FLevelDesignEditor> InLevelDesignEditor)
{
	this->LevelDesignEditor = InLevelDesignEditor;

	this->ChildSlot
		[
			SNew(SBorder)
			.Padding(2.0f)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)

				// Content list
				+ SVerticalBox::Slot()
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						//SNullWidget::NullWidget
						SAssignNew(GraphActionMenu, SGraphActionMenu)
						.OnActionDragged(this, &SCCGraphPalette::OnActionDragged)
						.OnCreateWidgetForAction(this, &SCCGraphPalette::OnCreateWidgetForAction)
						.OnCollectAllActions(this, &SCCGraphPalette::CollectAllActions)
						.AutoExpandActionMenu(true)
					]
				]
			]
		];
}

void SCCGraphPalette::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	const UEdLevelDesignSchema* Schema = GetDefault<UEdLevelDesignSchema>();
	TArray<TSharedPtr<FEdGraphSchemaAction> > Actions;
	FGraphActionMenuBuilder ActionMenuBuilder;
	if (LevelDesignEditor.IsValid()) {
		TSharedPtr<SGraphEditor> GraphEditor = LevelDesignEditor.Pin()->GetGraphEditor();
		if (GraphEditor.IsValid()) {
			UEdGraph* Graph = GraphEditor->GetCurrentGraph();

			// !!
			TArray<ULDNode_Root*> RootNodes;
			Graph->GetNodesOfClass(RootNodes);
			if (RootNodes.Num() > 0)
			{
				Schema->GetActionList(Actions, Graph, ELDActionType::All);
				for (TSharedPtr<FEdGraphSchemaAction> Action : Actions) {
					ActionMenuBuilder.AddAction(Action);
				}
			}
		}
	}
	OutAllActions.Append(ActionMenuBuilder);
}

void SCCGraphPalette::Refresh() {
	RefreshActionsList(true);
}

TSharedRef<SWidget> SCCGraphPalette::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return SGraphPalette::OnCreateWidgetForAction(InCreateData);
}

FReply SCCGraphPalette::OnActionDragged(const TArray< TSharedPtr<FEdGraphSchemaAction> >& InActions, const FPointerEvent& MouseEvent)
{
	return SGraphPalette::OnActionDragged(InActions, MouseEvent);
}