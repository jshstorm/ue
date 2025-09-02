// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDConditionScript.h"

#if WITH_EDITOR

void ULDConditionScript::PushData(TSharedPtr<FJsonObject>& json)
{
	Super::PushData(json);
	json->SetBoolField(TEXT("inverted"), Inverted);
}

#endif