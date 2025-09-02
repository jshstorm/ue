// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "SCCOutputPin.h"
#include "Graph/EdGraph_LevelDesignProp.h"
#include "LDEditorStyle.h"
#include "Widgets/Images/SImage.h"

void SCCOutputPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	this->SetCursor(EMouseCursor::Default);

	typedef SCCOutputPin ThisClass;

	bShowLabel = true;
	IsEditable = true;

	GraphPinObj = InPin;
	check(GraphPinObj != NULL);

	const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
	check(Schema);

	// Set up a hover for pins that is tinted the color of the pin.
	SBorder::Construct(SBorder::FArguments()
	[
		SNew(SImage)
		.Image(GetPinImage())
	]
	.BorderImage(nullptr)
	.OnMouseButtonDown(this, &ThisClass::OnPinMouseDown)
	.Cursor(this, &ThisClass::GetPinCursor)
	.Padding(FMargin(0.0f))
	);
}

TSharedRef<SWidget>	SCCOutputPin::GetDefaultValueWidget()
{
	return SNew(STextBlock);
}

const FSlateBrush* SCCOutputPin::GetPinImage() const
{
	if (GetDirection() == EGPD_Input)
	{
		return FLDEditorStyle::GetBrush("LDEditor.arrow_up");
	}
	return FLDEditorStyle::GetBrush("LDEditor.arrow_down");
}

FSlateColor SCCOutputPin::GetPinColor() const
{
	static const FLinearColor MeshPinColor(1.0f, 0.7f, 0.0f);
	static const FLinearColor MarkerPinColor(0.3f, 0.3f, 1.0f);
	static const FLinearColor DarkColor(0.02f, 0.02f, 0.02f);
	if (!IsHovered()) {
		return DarkColor;
	}

	bool IsStyle = true; //(GraphPinObj->PinType.PinCategory == FMissionObjectivesDataTypes::PinType_Style);
	return IsStyle ? MarkerPinColor : MeshPinColor;
}
