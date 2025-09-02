// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LDObjectiveScript.h"
#include "LDConditionScript.generated.h"

UCLASS(abstract, Blueprintable/*, DefaultToInstanced, EditInlineNew*/)
class ANULEVELDESIGN_API ULDConditionScript : public ULDObjectiveScript
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Value)
	bool Inverted = false;

#if WITH_EDITOR
public:
	virtual void PushData(TSharedPtr<FJsonObject>& json) override;
#endif
};