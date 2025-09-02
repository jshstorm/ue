// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LDNode_Base.h"
#include "LDNode_Mission.generated.h"

UCLASS(abstract)
class ANULEVELDESIGNEDITOR_API ULDNode_Mission : public ULDNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() override;
	// End UEdGraphNode interface.

	virtual UEdGraphPin* GetInputPin() const { return Pins[0]; }
	virtual UEdGraphPin* GetOutputPin() const { return nullptr; }

};
