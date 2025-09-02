// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LDScript.generated.h"

UCLASS(abstract, Blueprintable/*, DefaultToInstanced, EditInlineNew*/)
class ANULEVELDESIGN_API ULDScript : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	uint32 UniqueId = 0;

	ULDScript();

#if WITH_EDITOR
public:
	virtual bool Validate(FString& error);
	virtual void PushData(TSharedPtr<FJsonObject>& json);
#endif
};