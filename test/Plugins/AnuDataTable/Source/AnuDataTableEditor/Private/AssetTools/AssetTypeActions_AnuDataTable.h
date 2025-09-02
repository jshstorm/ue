// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_CSVAssetBase.h"
#include "DataTable/AnuDataTable.h"

struct ANUREFERENCE_API FAnuResourceSnapshot;

class FAssetTypeActions_AnuDataTable : public FAssetTypeActions_CSVAssetBase
{
private:
	EAssetTypeCategories::Type AssetCategory;

public:
	FAssetTypeActions_AnuDataTable(EAssetTypeCategories::Type InAssetCategory);

	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return FText::FromString("AnuDataTable"); }
	virtual FColor GetTypeColor() const override { return FColor::Cyan; }
	virtual UClass* GetSupportedClass() const override { return UAnuDataTable::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetCategory; }
	virtual void GetActions(const TArray<UObject*>& InObjects, struct FToolMenuSection& Section) override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const override;
	virtual void PerformAssetDiff(UObject* Asset1, UObject* Asset2, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const override;
	// End IAssetTypeActions

protected:
	/** Handler for when CSV is selected */
	void ExecuteExportAsCSV(TArray<TWeakObjectPtr<UObject>> Objects);

	/** Handler for when JSON is selected */
	void ExecuteExportAsJSON(TArray<TWeakObjectPtr<UObject>> Objects);

private:
	void BindSnapshotExport(class IDataTableEditor* tableEditor);
	void ExecuteSnapshotTexture(const TMap<FName, FAnuResourceSnapshot*>& exportData);
};