// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/UnrealEd/Classes/Factories/DataTableFactory.h"
#include "AnuDataTableFactory.generated.h"

UCLASS(hidecategories = Object)
class UAnuDataTableFactory : public UDataTableFactory
{
	GENERATED_UCLASS_BODY()

protected:
	virtual UDataTable* MakeNewDataTable(UObject* InParent, FName Name, EObjectFlags Flags) override;
};