// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorldLookupTarget.generated.h"

UINTERFACE(BlueprintType)
class ANUREFERENCE_API UWorldLookupTarget : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};
inline UWorldLookupTarget::UWorldLookupTarget(const FObjectInitializer& ObjectInitializer) {}
class ANUREFERENCE_API IWorldLookupTarget
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
		FName GetLookupType();
	UFUNCTION(BlueprintNativeEvent)
		FName GetLookupUID();
};