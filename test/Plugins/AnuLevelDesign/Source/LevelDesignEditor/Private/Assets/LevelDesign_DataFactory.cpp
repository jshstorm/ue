// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LevelDesign_DataFactory.h"
#include "LevelDesign.h"

ULevelDesign_DataFactory::ULevelDesign_DataFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) 
{
	SupportedClass = ULevelDesign::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

bool ULevelDesign_DataFactory::CanCreateNew() const {
	return true;
}

UObject* ULevelDesign_DataFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) 
{
	return NewObject<ULevelDesign>(InParent, Class, Name, Flags | RF_Transactional);
}
