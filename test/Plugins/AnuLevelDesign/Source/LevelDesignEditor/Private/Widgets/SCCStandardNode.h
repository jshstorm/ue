// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "SGraphNode.h"
#include "../Graph/LDNode_Base.h"

/**
 * Implements the message interaction graph panel.
 */
class ANULEVELDESIGNEDITOR_API SCCStandardNode : public SGraphNode, public FNodePropertyObserver
{
public:
	SLATE_BEGIN_ARGS(SCCStandardNode) { }
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, ULDNode_Base* InNode);

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	// End of SGraphNode interface

	// FPropertyObserver interface
	virtual void OnPropertyChanged(ULDNode_Base* Sender, const FName& PropertyName) override;
	// End of FPropertyObserver interface

	// Called when text is being committed to check for validity
	bool OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage);

protected:
	virtual const FSlateBrush* GetNameIcon() const;

	static FLinearColor InactiveStateColor;
	static FLinearColor ActiveStateColorDim;
	static FLinearColor ActiveStateColorBright;

	TSharedPtr<SOverlay> TopNodeOverlay;
	TSharedPtr<SHorizontalBox> TitleHorizontalBox;

};
