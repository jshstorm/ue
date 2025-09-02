// Copyright 2017 CLOVERGAMES Co., Ltd. All right reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnuDyeingIconReferenceStatics.generated.h"

UCLASS()
class ANUREFERENCE_API UAnuDyeingIconReferenceStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category=ObjectPath)
	static bool IsDyeingIconReference(const FSoftObjectPath& softObjectPath);
	UFUNCTION(BlueprintPure, Category=ObjectPath)
	static FSoftObjectPath GetDyeingMaskIconReference(const FSoftObjectPath& softObjectPtr);
};