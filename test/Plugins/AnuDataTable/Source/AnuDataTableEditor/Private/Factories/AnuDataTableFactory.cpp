// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnuDataTableFactory.h"
#include "DataTable/AnuDataTable.h"

#define LOCTEXT_NAMESPACE "AnuDataTableFactory"

UAnuDataTableFactory::UAnuDataTableFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAnuDataTable::StaticClass();
}

UDataTable* UAnuDataTableFactory::MakeNewDataTable(UObject* InParent, FName Name, EObjectFlags Flags)
{
	return NewObject<UAnuDataTable>(InParent, Name, Flags);
}

#undef LOCTEXT_NAMESPACE