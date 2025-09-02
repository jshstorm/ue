// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LDNode_Base.h"
#include "LDNode_SelectChild.generated.h"

UCLASS()
class ANULEVELDESIGNEDITOR_API ULDNode_SelectChild : public ULDNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() override;
	virtual const FSlateBrush* GetNodeIcon() const override;
	virtual const FSlateBrush* GetNodeBackgroundImage() const override;
	// End UEdGraphNode interface.

	virtual UEdGraphPin* GetInputPin() const { return Pins[0]; }
	virtual UEdGraphPin* GetOutputPin() const { return Pins[1]; }
};