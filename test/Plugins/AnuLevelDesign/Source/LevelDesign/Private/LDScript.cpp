// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDScript.h"

ULDScript::ULDScript()
{
	UniqueId = GetUniqueID();
}

#if WITH_EDITOR
bool ULDScript::Validate(FString& error)
{
	return true;
}

void ULDScript::PushData(TSharedPtr<FJsonObject>& json)
{
	json->SetNumberField("uid", UniqueId);
	json->SetStringField("type", GetClass()->GetFName().ToString());
}
#endif