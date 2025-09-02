// Copyright Epic Games, Inc. All Rights Reserved.

#include "ResourceMeshPartialDataTableEditor.h"
#include "ResourceModelDataTableEditor.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "PackageHelperFunctions.h"

#include "ReferenceBuilder.h"

#define LOCTEXT_NAMESPACE "DataTableEditor"

const FString FResourceMeshPartialDataTableEditor::SupportedDataTablePath = "/Game/Anu/DataTable/Mesh/";
const FString FResourceMeshPartialDataTableEditor::ParentDataTableName = "DT_ResourceMesh";
const FString FResourceMeshPartialDataTableEditor::ParentDataTablePackagePath = "/Game/Anu/DataTable/References/Resource/" + FResourceMeshPartialDataTableEditor::ParentDataTableName;

void FResourceMeshPartialDataTableEditor::InitDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	FAnuDataTableEditor::InitDataTableEditor(Mode, InitToolkitHost, Table);

#define ADD_HANDLER(Struct, Func) { \
	FString RowStructName = #Struct; \
	RowStructName.RemoveAt(0); \
	PreChangeHandler.Emplace(*RowStructName, FOnRowDataChanged::CreateSP(this, &FResourceMeshPartialDataTableEditor::OnPreChange##Func)); \
	PostChangeHandler.Emplace(*RowStructName, FOnRowDataChanged::CreateSP(this, &FResourceMeshPartialDataTableEditor::OnPostChange##Func)); }

	ADD_HANDLER(FAnuResourceWeaponConstruct, WeaponConstruct);
	ADD_HANDLER(FAnuResourceOutfitConstruct, OutfitConstruct);
	ADD_HANDLER(FAnuResourceShoesConstruct, ShoesConstruct);
	ADD_HANDLER(FAnuUnderBodyHide, ShoesHide);
	ADD_HANDLER(FAnuGlovesMeshHide, GlovesHide);
	ADD_HANDLER(FAnuAttachmentHide, AttachmentHide);
	
}

void FResourceMeshPartialDataTableEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	FAnuDataTableEditor::FillToolbar(ToolbarBuilder);

	ToolbarBuilder.BeginSection("ExtendCommands");
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FResourceMeshPartialDataTableEditor::OnSyncClicked)),
			NAME_None,
			FText::FromString("Sync"),
			FText::FromString("Sync the all row data with parent data table"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Sync"));
	}
	ToolbarBuilder.EndSection();
}

void FResourceMeshPartialDataTableEditor::OnSyncClicked()
{
	if (auto Changed = GetDataTable()) {
		FName backupRowName = HighlightedRowName;

		auto removeHandler = PreChangeHandler.Find(Changed->RowStructName);
		auto emplaceHandler = PostChangeHandler.Find(Changed->RowStructName);
		
		for (auto& pair : Changed->GetRowMap()) {
			HighlightedRowName = pair.Key;

			if (removeHandler) {
				removeHandler->Execute(Changed);
			}
			if (emplaceHandler) {
				emplaceHandler->Execute(Changed);
			}
		}

		HighlightedRowName = backupRowName;

		SaveParentDataTable();
	}
}

void FResourceMeshPartialDataTableEditor::PreChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
	FAnuDataTableEditor::PreChange(Changed, Info);

	if (Changed == GetDataTable()) {
		if (auto handler = PreChangeHandler.Find(Changed->RowStructName)) {
			handler->Execute(Changed);
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPreChangeWeaponConstruct(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuResourceWeaponConstruct>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentWeaponRowPtr(HighlightedRowName)) {
			for (int32 i = dstRow->Meshes.Num() - 1; i > 0; --i) {
				dstRow->Meshes.RemoveAt(i);
			}
			dstRow->Meshes.Shrink();
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPreChangeOutfitConstruct(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuResourceOutfitConstruct>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			for (int32 i = dstRow->Meshes.Num() - 1; i > 0; --i) {
				dstRow->Meshes.RemoveAt(i);
			}
			dstRow->Meshes.Shrink();
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPreChangeShoesConstruct(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuResourceShoesConstruct>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			for (int32 i = dstRow->Meshes.Num() - 1; i > 0; --i) {
				dstRow->Meshes.RemoveAt(i);
			}
			dstRow->Meshes.Shrink();
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPreChangeShoesHide(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuUnderBodyHide>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			dstRow->Meshes[0].HideSubMeshes.Empty();
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPreChangeGlovesHide(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuGlovesMeshHide>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			dstRow->Meshes[0].HideSubMeshes.Empty();
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPreChangeAttachmentHide(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuAttachmentHide>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			dstRow->Meshes[0].HideAttachments.Empty();
		}
	}
}

void FResourceMeshPartialDataTableEditor::PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
	FAnuDataTableEditor::PostChange(Changed, Info);

	if (Changed == GetDataTable()) {
		if (auto handler = PostChangeHandler.Find(Changed->RowStructName)) {
			handler->Execute(Changed);
		}
		SaveParentDataTable();
	}
}

void FResourceMeshPartialDataTableEditor::OnPostChangeWeaponConstruct(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuResourceWeaponConstruct>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentWeaponRowPtr(HighlightedRowName)) {
			BuildWeaponConstruct(Changed, srcRow, dstRow);
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPostChangeOutfitConstruct(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuResourceOutfitConstruct>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			BuildOutfitConstruct(Changed, srcRow, dstRow);
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPostChangeShoesConstruct(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuResourceShoesConstruct>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			BuildShoesConstruct(Changed, srcRow, dstRow);
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPostChangeShoesHide(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuUnderBodyHide>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			BuildShoesHide(Changed, srcRow, dstRow);
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPostChangeGlovesHide(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuGlovesMeshHide>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			BuildGlovesHide(Changed, srcRow, dstRow);
		}
	}
}

void FResourceMeshPartialDataTableEditor::OnPostChangeAttachmentHide(const UDataTable* Changed)
{
	if (auto srcRow = Changed->FindRow<FAnuAttachmentHide>(HighlightedRowName, "")) {
		if (auto dstRow = GetParentCostumeRowPtr(HighlightedRowName)) {
			BuildAttachmentHide(Changed, srcRow, dstRow);
		}
	}
}

void FResourceMeshPartialDataTableEditor::BuildData(const UDataTable* Changed, FTableRowBase* srcRow, FAnuResourceModelMesh* dstRow)
{
	if (Changed->RowStructName.IsEqual("AnuResourceWeaponConstruct")) {
		BuildWeaponConstruct(Changed, static_cast<FAnuResourceWeaponConstruct*>(srcRow), dstRow);
		return;
	}

	if (Changed->RowStructName.IsEqual("AnuResourceOutfitConstruct")) {
		BuildOutfitConstruct(Changed, static_cast<FAnuResourceOutfitConstruct*>(srcRow), dstRow);
		return;
	}

	if (Changed->RowStructName.IsEqual("AnuResourceShoesConstruct")) {
		BuildShoesConstruct(Changed, static_cast<FAnuResourceShoesConstruct*>(srcRow), dstRow);
		return;
	}

	if (Changed->RowStructName.IsEqual("AnuUnderBodyHide")) {
		BuildShoesHide(Changed, static_cast<FAnuUnderBodyHide*>(srcRow), dstRow);
		return;
	}

	if (Changed->RowStructName.IsEqual("AnuGlovesMeshHide")) {
		BuildGlovesHide(Changed, static_cast<FAnuGlovesMeshHide*>(srcRow), dstRow);
		return;
	}

	if (Changed->RowStructName.IsEqual("AnuAttachmentHide")) {
		BuildAttachmentHide(Changed, static_cast<FAnuAttachmentHide*>(srcRow), dstRow);
		return;
	}
}

void FResourceMeshPartialDataTableEditor::BuildWeaponConstruct(const UDataTable* Changed, FAnuResourceWeaponConstruct* srcRow, FAnuResourceModelMesh* dstRow)
{
	// SubMeshes
	dstRow->Meshes.Emplace(MakeMeshInfo(dstRow->Meshes[0].GroupName, srcRow->Shield, ESubMeshType::Weapon, EAttachmentParts::AP_RWeapon, "Socket_Shield", srcRow->ShieldAnimBP));
}

void FResourceMeshPartialDataTableEditor::BuildOutfitConstruct(const UDataTable* Changed, FAnuResourceOutfitConstruct* srcRow, FAnuResourceModelMesh* dstRow)
{
	// Preview
	dstRow->Mesh = srcRow->Outfit;

	// MainMesh
	dstRow->Meshes[0].Mesh = srcRow->Outfit;

	// SubMeshe
	dstRow->Meshes.Emplace(MakeMeshInfo(dstRow->Meshes[0].GroupName, srcRow->Hands, ESubMeshType::Hands, EAttachmentParts::AP_Outfit, NAME_None));
}

void FResourceMeshPartialDataTableEditor::BuildShoesConstruct(const UDataTable* Changed, FAnuResourceShoesConstruct* srcRow, FAnuResourceModelMesh* dstRow)
{
	// Preview
	dstRow->Mesh = srcRow->Foot;

	// MainMesh
	dstRow->Meshes[0].Mesh = srcRow->Foot;
	dstRow->Meshes[0].AppendHeight = srcRow->AppendHeight;

	// SubMeshes
	dstRow->Meshes.Emplace(MakeMeshInfo(dstRow->Meshes[0].GroupName, srcRow->Panty, ESubMeshType::Pants, EAttachmentParts::AP_Shoes, NAME_None));
	dstRow->Meshes.Emplace(MakeMeshInfo(dstRow->Meshes[0].GroupName, srcRow->Thigh, ESubMeshType::ThighLeg, EAttachmentParts::AP_Shoes, NAME_None));
	dstRow->Meshes.Emplace(MakeMeshInfo(dstRow->Meshes[0].GroupName, srcRow->Thin, ESubMeshType::ThinLeg, EAttachmentParts::AP_Shoes, NAME_None));
	if (srcRow->Boots.IsNull() == false) {
		dstRow->Meshes.Emplace(MakeMeshInfo(dstRow->Meshes[0].GroupName, srcRow->Boots, ESubMeshType::Boots, EAttachmentParts::AP_Shoes, NAME_None));
	}
}

void FResourceMeshPartialDataTableEditor::BuildShoesHide(const UDataTable* Changed, FAnuUnderBodyHide* srcRow, FAnuResourceModelMesh* dstRow)
{
	// MainMesh
	for (auto type : srcRow->HideTypes) {
		dstRow->Meshes[0].HideSubMeshes.Emplace(type);
	}
}

void FResourceMeshPartialDataTableEditor::BuildGlovesHide(const UDataTable* Changed, FAnuGlovesMeshHide* srcRow, FAnuResourceModelMesh* dstRow)
{
	// MainMesh
	if (srcRow->HideHand) {
		dstRow->Meshes[0].HideSubMeshes.Emplace(ESubMeshType::Hands);
	}
}

void FResourceMeshPartialDataTableEditor::BuildAttachmentHide(const UDataTable* Changed, struct FAnuAttachmentHide* srcRow, struct FAnuResourceModelMesh* dstRow)
{
	for (auto parts : srcRow->HideParts) {
		dstRow->Meshes[0].HideAttachments.Emplace(parts);
	}
}

void FResourceMeshPartialDataTableEditor::PostUndo(bool bSuccess)
{
	FAnuDataTableEditor::PostUndo(bSuccess);

	if (auto Changed = GetDataTable()) {
		auto removeHandler = PreChangeHandler.Find(Changed->RowStructName);
		auto emplaceHandler = PostChangeHandler.Find(Changed->RowStructName);

		if (removeHandler) {
			removeHandler->Execute(Changed);
		}
		if (emplaceHandler) {
			emplaceHandler->Execute(Changed);
		}

		SaveParentDataTable();
	}
}

void FResourceMeshPartialDataTableEditor::SaveParentDataTable()
{
	FString destLongPackageName = ParentDataTablePackagePath;
	FString destRelFileName = FPackageName::LongPackageNameToFilename(destLongPackageName, ".uasset");
	FString destAbsFileName = FPaths::ConvertRelativePathToFull(destRelFileName);
	FPaths::MakePlatformFilename(destAbsFileName);
	if (auto package = LoadPackage(nullptr, *destLongPackageName, LOAD_None)) {
		SavePackageHelper(package, destAbsFileName);
	}

	GetParentDataTable()->Modify(true);

	if (FResourceModelDataTableEditor::Instance) {
		FResourceModelDataTableEditor::Instance->HandlePostChange();
	}
}

UDataTable* FResourceMeshPartialDataTableEditor::GetParentDataTable()
{
	return Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), NULL, *(ParentDataTablePackagePath + "." + ParentDataTableName)));
}

FAnuResourceModelMesh* FResourceMeshPartialDataTableEditor::GetParentCostumeRowPtr(FName rowName)
{
	return GetParentDataTable()->FindRow<FAnuResourceModelMesh>(*("res.mesh.costume." + rowName.ToString()), "");
}

FAnuResourceModelMesh* FResourceMeshPartialDataTableEditor::GetParentWeaponRowPtr(FName rowName)
{
	return GetParentDataTable()->FindRow<FAnuResourceModelMesh>(*("res.mesh.objt.equip.weapon." + rowName.ToString()), "");
}

void FResourceMeshPartialDataTableEditor::RemoveSubMesh(TArray<FAnuMeshInfo>& meshes, TSoftObjectPtr<USkeletalMesh> target)
{
	int32 foundIndex = meshes.IndexOfByPredicate([target](auto& e) { return e.Mesh == target; });
	if (foundIndex >= 0) {
		meshes.RemoveAt(foundIndex);
		meshes.Shrink();
	}
}

FAnuMeshInfo FResourceMeshPartialDataTableEditor::MakeMeshInfo(FName group, TSoftObjectPtr<UObject> mesh, ESubMeshType subMeshType, EAttachmentParts parts, FName socket, TSoftClassPtr<UAnimInstance> animation)
{
	FAnuMeshInfo info;
	info.GroupName = group;
	info.Mesh = mesh;
	info.SubPartsType = subMeshType;
	info.Parts = parts;
	info.SocketName = socket;
	info.AnimationBP = animation;

	return info;
}

#undef LOCTEXT_NAMESPACE