// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnuDatatableBlueprintLibrary.generated.h"

class ENGINE_API Datatable;

UCLASS()
class ANUDATATABLEEDITOR_API UAnuDatatableBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "Anu|Editor|DataTable")
	static void ExportMapPreview(UDataTable* dataTable, TArray<TSoftObjectPtr<UTexture2D>> ignoreTextures);
	UFUNCTION(BlueprintCallable, Category = "Anu|Editor|DataTable")
	static void BuildVisibleActors(TSoftObjectPtr<UWorld> triggerLevel, FString skipLevelWord = "", bool resetTable = true);

	UFUNCTION(BlueprintCallable, Category = "Anu|Editor|DataTable")
	static void BuildVisibleActorsV2(TSoftObjectPtr<UWorld> triggerLevel, TSoftObjectPtr<UWorld> subLevel, FString skipLevelWord = "", bool resetTable = true);

	UFUNCTION(BlueprintCallable, Category = "Anu|Editor|DataTable")
	static void LoadEditorLevel(TSoftObjectPtr<UWorld> level);
};
