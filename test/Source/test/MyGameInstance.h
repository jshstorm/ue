// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class TEST_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()
	

private:
	UPROPERTY()
	class UReferenceBuilder* _builder = nullptr;

private:
	virtual void Init() override;
	virtual void OnStart() override;
};
