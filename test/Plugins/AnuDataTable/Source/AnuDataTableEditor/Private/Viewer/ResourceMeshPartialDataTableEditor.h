// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnuDataTableEditor.h"
#include "Reference_Resource.h"

DECLARE_DELEGATE_OneParam(FOnRowDataChanged, const UDataTable*);

class FResourceMeshPartialDataTableEditor : public FAnuDataTableEditor
{
public:
	static const FString SupportedDataTablePath;
	static const FString ParentDataTablePackagePath;
	static const FString ParentDataTableName;

private:
	TMap<FName, FOnRowDataChanged> PreChangeHandler;
	TMap<FName, FOnRowDataChanged> PostChangeHandler;

public:
	virtual void InitDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table) override;
	virtual void PreChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info) override;
	virtual void PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info) override;
	virtual void PostUndo(bool bSuccess) override;

	static void BuildData(const UDataTable* Changed, struct FTableRowBase* srcRow, struct FAnuResourceModelMesh* dstRow);

protected:
	virtual void FillToolbar(FToolBarBuilder& ToolbarBuilder) override;

private:
	void OnSyncClicked();

	void OnPreChangeWeaponConstruct(const UDataTable* Changed);
	void OnPreChangeOutfitConstruct(const UDataTable* Changed);
	void OnPreChangeShoesConstruct(const UDataTable* Changed);
	void OnPreChangeShoesHide(const UDataTable* Changed);
	void OnPreChangeGlovesHide(const UDataTable* Changed);
	void OnPreChangeAttachmentHide(const UDataTable* Changed);

	void OnPostChangeWeaponConstruct(const UDataTable* Changed);
	void OnPostChangeOutfitConstruct(const UDataTable* Changed);
	void OnPostChangeShoesConstruct(const UDataTable* Changed);
	void OnPostChangeShoesHide(const UDataTable* Changed);
	void OnPostChangeGlovesHide(const UDataTable* Changed);
	void OnPostChangeAttachmentHide(const UDataTable* Changed);

	static void BuildWeaponConstruct(const UDataTable* Changed, struct FAnuResourceWeaponConstruct* srcRow, struct FAnuResourceModelMesh* dstRow);
	static void BuildOutfitConstruct(const UDataTable* Changed, struct FAnuResourceOutfitConstruct* srcRow, struct FAnuResourceModelMesh* dstRow);
	static void BuildShoesConstruct(const UDataTable* Changed, struct FAnuResourceShoesConstruct* srcRow, struct FAnuResourceModelMesh* dstRow);
	static void BuildShoesHide(const UDataTable* Changed, struct FAnuUnderBodyHide* srcRow, struct FAnuResourceModelMesh* dstRow);
	static void BuildGlovesHide(const UDataTable* Changed, struct FAnuGlovesMeshHide* srcRow, struct FAnuResourceModelMesh* dstRow);
	static void BuildAttachmentHide(const UDataTable* Changed, struct FAnuAttachmentHide* srcRow, struct FAnuResourceModelMesh* dstRow);

	void SaveParentDataTable();
	UDataTable* GetParentDataTable();
	struct FAnuResourceModelMesh* GetParentCostumeRowPtr(FName rowName);
	struct FAnuResourceModelMesh* GetParentWeaponRowPtr(FName rowName);

	static struct FAnuMeshInfo MakeMeshInfo(FName group, TSoftObjectPtr<UObject> mesh, ESubMeshType subMeshType, EAttachmentParts parts, FName socket, TSoftClassPtr<class UAnimInstance> animation = nullptr);

	static void RemoveSubMesh(TArray<struct FAnuMeshInfo>& meshes, TSoftObjectPtr<class USkeletalMesh> target);
};