// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LDNode_Base.h"
#include "LDNode_Root.generated.h"

class ULevelDesign;

UCLASS()
class ANULEVELDESIGNEDITOR_API ULDNode_Root : public ULDNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY()
	ULevelDesign* LevelDesignAsset;

public:
	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() override;
	virtual const FSlateBrush* GetNodeIcon() const override;
	virtual UEdGraphPin* GetInputPin() const { return nullptr; }
	virtual UEdGraphPin* GetOutputPin() const { return Pins[0]; }
	// End UEdGraphNode interface.

	void SetupDataAsset(class ULevelDesign* asset);
};