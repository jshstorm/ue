// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "SGraphPin.h"
#include "AssetThumbnail.h"

class ANULEVELDESIGNEDITOR_API SCCOutputPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SCCOutputPin){}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	// End SGraphPin interface

	/** @return The color that we should use to draw this pin */
	virtual FSlateColor GetPinColor() const override;

	// End SGraphPin interface

	const FSlateBrush* GetPinImage() const;
};
