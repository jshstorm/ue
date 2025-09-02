// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "SCCStandardNode.h"
#include "GraphEditor.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/SCCOutputPin.h"
#include "LDNode.h"
#include "LDEditorStyle.h"

void SCCStandardNode::Construct(const FArguments& InArgs, ULDNode_Base* InNode)
{
	GraphNode = InNode;

	this->SetCursor(EMouseCursor::CardinalCross);
	this->UpdateGraphNode();
}

const FSlateBrush* SCCStandardNode::GetNameIcon() const
{
	return FEditorStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Task.Wait.Icon"));
}

void SCCStandardNode::OnPropertyChanged(ULDNode_Base* Sender, const FName& PropertyName)
{
	UpdateGraphNode();
}

bool SCCStandardNode::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
{
	OutErrorMessage = FText::FromString(TEXT("Error"));
	if (InText.ToString().Len() == 0) {
		OutErrorMessage = FText::FromString(TEXT("Invalid Name"));
		return false;
	}

	return true;
}

void SCCStandardNode::UpdateGraphNode()
{
	ULDNode_Base* BaseNode = CastChecked<ULDNode_Base>(GraphNode);
	if (BaseNode == nullptr || BaseNode->AssociatedObject == nullptr) {
		return;
	}

	BaseNode->OnChangeAssociatedNode();

	ULDNode* TextNode = CastChecked<ULDNode>(BaseNode->AssociatedObject);

	InputPins.Empty();
	OutputPins.Empty();
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	const FSlateBrush* NodeTypeIcon = BaseNode->GetNodeIcon();
	const FSlateBrush* NodeBackgroundImage = BaseNode->GetNodeBackgroundImage();

	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(NodeBackgroundImage)
			.Padding(0)
			[
				SAssignNew(TopNodeOverlay, SOverlay)

				// NAME AREA
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(6.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							// INPUT PIN AREA
							SAssignNew(LeftNodeBox, SVerticalBox)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(TitleHorizontalBox, SHorizontalBox)

							+ SHorizontalBox::Slot()
							.VAlign(VAlign_Bottom)
							.HAlign(HAlign_Center)
							.AutoWidth()
							[
								SNew(SBox)
								.WidthOverride(20.0f)
								.HeightOverride(20.0f)
								[
									SNew(SImage)
									.Image(NodeTypeIcon)
								]
							]

							+ SHorizontalBox::Slot()
							.Padding(4.0f, 6.0f, 0.0f, 0.0f)
							.AutoWidth()
							[
								SNew(STextBlock)
								.Visibility(EVisibility::HitTestInvisible)
								.TextStyle(FLDEditorStyle::Get(), "LD.NodeTitle")
								.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
							]
						]

						+ SVerticalBox::Slot()
						.Padding(2.f, .0f)
						.AutoHeight()
						[
							SNew(STextBlock)
							.Visibility(EVisibility::HitTestInvisible)
							.TextStyle(FLDEditorStyle::Get(), "LD.NodeText")
							.AutoWrapText(true)
							.WrapTextAt(220.f)
							.Text(TextNode->Text)
						]

						+ SVerticalBox::Slot()
						.Padding(0.f, 0.f, 0.f, 6.f)
						.AutoHeight()
						[
							NodeTitle.ToSharedRef()
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							// OUTPUT PIN AREA
							SAssignNew(RightNodeBox, SVerticalBox)
						]
					]
				]
			]
		];
	CreatePinWidgets();
}

void SCCStandardNode::CreatePinWidgets()
{
	ULDNode_Base* Node = CastChecked<ULDNode_Base>(GraphNode);
	{
		UEdGraphPin* CurPin = Node->GetOutputPin();
		if (CurPin)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SCCOutputPin, CurPin);
			NewPin->SetIsEditable(IsEditable);
			this->AddPin(NewPin.ToSharedRef());
			OutputPins.Add(NewPin.ToSharedRef());
		}
	}
	{
		UEdGraphPin* CurPin = Node->GetInputPin();
		if (CurPin)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SCCOutputPin, CurPin);
			NewPin->SetIsEditable(IsEditable);
			this->AddPin(NewPin.ToSharedRef());
			InputPins.Add(NewPin.ToSharedRef());
		}
	}
}

void SCCStandardNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility(TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced));
	}

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
		[
			PinToAdd
		];
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		RightNodeBox->AddSlot()
		[
			PinToAdd
		];
		OutputPins.Add(PinToAdd);
	}
}
