// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "Engine/DataAsset.h"
#include "Factories/Factory.h"
#include "LevelDesign_DataFactory.generated.h"

UCLASS()
class ANULEVELDESIGNEDITOR_API ULevelDesign_DataFactory : public UFactory 
{
	GENERATED_UCLASS_BODY()

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	// End of UFactory interface
};