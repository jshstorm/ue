// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "ReferenceBuilder.h"

void UMyGameInstance::Init()
{
	UGameInstance::Init();

	_builder = NewObject<UReferenceBuilder>(this);
	_builder->Initialize();
}

void UMyGameInstance::OnStart()
{
	Super::OnStart();
}
