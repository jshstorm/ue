// Copyright Epic Games, Inc. All Rights Reserved.

#include "ResourceModelDataTableEditor.h"
#include "ResourceMeshPartialDataTableEditor.h"
#include "Engine/DataTable.h"

#include "ReferenceBuilder.h"
#include "Reference_Resource.h"
#include "Attachment/AttachmentStatic.h"

#define LOCTEXT_NAMESPACE "DataTableEditor"

const FString FResourceModelDataTableEditor::SupportedDataTableName = "DT_ResourceMesh";
FResourceModelDataTableEditor* FResourceModelDataTableEditor::Instance = nullptr;

FResourceModelDataTableEditor::FResourceModelDataTableEditor()
{
	Instance = this;
}

FResourceModelDataTableEditor::~FResourceModelDataTableEditor()
{
	Instance = nullptr;
}

void FResourceModelDataTableEditor::InitDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table)
{
	FAnuDataTableEditor::InitDataTableEditor(Mode, InitToolkitHost, Table);

#define ADD_PARTIAL_TABLE(RowStruct, DataTableName, RowNamePrefix) { \
	FString RowStructName = #RowStruct; \
	RowStructName.RemoveAt(0); \
	PartialTable partial { *RowStructName, RowNamePrefix, LoadPartialTable(DataTableName) }; \
	_partialTables.Emplace(partial); }

	ADD_PARTIAL_TABLE(FAnuResourceWeaponConstruct, "WeaponConstructTable", "res.mesh.objt.equip.weapon.");
	ADD_PARTIAL_TABLE(FAnuResourceOutfitConstruct, "OutfitConstructTable", "res.mesh.costume.");
	ADD_PARTIAL_TABLE(FAnuResourceShoesConstruct, "UnderBodyConstructTable", "res.mesh.costume.");
	ADD_PARTIAL_TABLE(FAnuUnderBodyHide, "UnderBodyHideTable", "res.mesh.costume.");
	ADD_PARTIAL_TABLE(FAnuGlovesMeshHide, "GlovesHideTable", "res.mesh.costume.");
}

void FResourceModelDataTableEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	FAnuDataTableEditor::FillToolbar(ToolbarBuilder);

	ToolbarBuilder.BeginSection("ExtendCommands");
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FResourceModelDataTableEditor::OnSyncClicked)),
			NAME_None,
			FText::FromString("Sync"),
			FText::FromString("Sync the all row data with partial data tables"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Sync"));

		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FResourceModelDataTableEditor::OnQueryClicked)),
			NAME_None,
			FText::FromString("Query(Test)"),
			FText::FromString(""),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DataTableEditor.Query"));
	}
	ToolbarBuilder.EndSection();
}

void FResourceModelDataTableEditor::OnQueryClicked()
{
	if (auto Changed = GetDataTable()) {
		for (auto& pair : Changed->GetRowMap()) {

			auto rowName = pair.Key;

			// query logic...
		}
	}
}

void FResourceModelDataTableEditor::OnSyncClicked()
{
	if (auto Changed = GetDataTable()) {
		FName backupRowName = HighlightedRowName;

		for (auto& pair : Changed->GetRowMap()) {
			HighlightedRowName = pair.Key;

			BuildMainMesh(Changed);
			BuildSubMeshes(Changed);
		}

		HighlightedRowName = backupRowName;
		HandlePostChange();
	}
}

void FResourceModelDataTableEditor::OnAddClicked()
{
	FAnuDataTableEditor::OnAddClicked();

	if (UDataTable* TablePtr = Cast<UDataTable>(GetEditingObject())) {
		BuildMainMesh(TablePtr);
	}
}

void FResourceModelDataTableEditor::SelectionChange(const UDataTable* Changed, FName RowName)
{
	FAnuDataTableEditor::SelectionChange(Changed, RowName);

	if (UDataTable* TablePtr = Cast<UDataTable>(GetEditingObject())) {
		BuildMainMesh(TablePtr);
		BuildSubMeshes(TablePtr);
		HandlePostChange();
	}
}

void FResourceModelDataTableEditor::PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
	if (Changed == GetDataTable()) {
		BuildMainMesh(Changed);
		BuildSubMeshes(Changed);
	}

	FAnuDataTableEditor::PostChange(Changed, Info);
}

void FResourceModelDataTableEditor::BuildMainMesh(const UDataTable* Changed)
{
	static TMap<EResourceModelType, EAttachmentParts> ConvertModelToParts
	{
		{ EResourceModelType::None,				EAttachmentParts::AP_Invalid },
		{ EResourceModelType::Outfit,			EAttachmentParts::AP_Outfit },
		{ EResourceModelType::Face,				EAttachmentParts::AP_Face },
		{ EResourceModelType::Hair,				EAttachmentParts::AP_Hair },
		{ EResourceModelType::Shoes,			EAttachmentParts::AP_Shoes },
		{ EResourceModelType::Bracelet,			EAttachmentParts::AP_LBracelet },
		{ EResourceModelType::Gloves,			EAttachmentParts::AP_Gloves },
		{ EResourceModelType::Hat,				EAttachmentParts::AP_Hat },
		{ EResourceModelType::Eyewear,			EAttachmentParts::AP_Eyewear },
		{ EResourceModelType::Mask,				EAttachmentParts::AP_Mask },
		{ EResourceModelType::Back,				EAttachmentParts::AP_Back },
		{ EResourceModelType::Tail,				EAttachmentParts::AP_Tail },
		{ EResourceModelType::UnitOfPlayer,		EAttachmentParts::AP_RWeapon },
		{ EResourceModelType::SwordOfPlayer,	EAttachmentParts::AP_RWeapon },
		{ EResourceModelType::BowOfPlayer,		EAttachmentParts::AP_LWeapon },
		{ EResourceModelType::WeaponOfMonster,	EAttachmentParts::AP_RWeapon },
		{ EResourceModelType::GiftBoxOfFairy,	EAttachmentParts::AP_RHand },
		{ EResourceModelType::HandPhone,		EAttachmentParts::AP_LHand },
		{ EResourceModelType::Ear,				EAttachmentParts::AP_Ear },
		{ EResourceModelType::Necklace,			EAttachmentParts::AP_Necklace },
		{ EResourceModelType::Armband,			EAttachmentParts::AP_Armband },
		{ EResourceModelType::Lens,				EAttachmentParts::AP_Lens },
		{ EResourceModelType::MakeUp,			EAttachmentParts::AP_MakeUp },
	};

	static TMap<EAttachmentParts, EAttachmentParts> MaterialOverrideTargetParts {
		{ EAttachmentParts::AP_Lens,			EAttachmentParts::AP_Face },
	};

	if (auto row = Changed->FindRow<FAnuResourceModelMesh>(HighlightedRowName, "")) {
		row->_mainParts = ConvertModelToParts[row->ModelType];
		row->Meshes.Empty();

		if (row->Mesh.IsNull() == false) { // generate main mesh
			FAnuMeshInfo mainMesh;
			mainMesh.GroupName = HighlightedRowName;
			mainMesh.Mesh = row->Mesh;
			mainMesh.SubPartsType = ESubMeshType::None;
			mainMesh.Parts = row->_mainParts;
			mainMesh.SocketName = UAttachmentStatic::GetAttachmentTypeBySlot(mainMesh.Parts);
			mainMesh.AnimationBP = row->AnimationBP;

			if (row->UseSocketOverride) {
				mainMesh.SocketName = row->SocketOverride;
			}

			row->Meshes.Emplace(mainMesh);
		}
		else { // link material override
			if (auto matOvParts = MaterialOverrideTargetParts.Find(row->_mainParts)) {
				row->_materialOverrideTargetParts = *matOvParts;
			}
		}
	}
}

void FResourceModelDataTableEditor::BuildSubMeshes(const UDataTable* Changed)
{
	if (auto parentRow = Changed->FindRow<FAnuResourceModelMesh>(HighlightedRowName, "")) {
		if (parentRow->Meshes.Num() == 0) {
			UE_LOG(LogTemp, Warning, TEXT("[mesh] outfit build failed; cannot find valid mesh for [%s]"), *HighlightedRowName.ToString());
			return;
		}

		for (auto& partial : _partialTables) {
			FString partialRowName = HighlightedRowName.ToString().Replace(*partial.RowNamePrefix, TEXT(""));
			if (auto partialRow = partial.DataTable->FindRow<FTableRowBase>(*partialRowName, "")) {
				FResourceMeshPartialDataTableEditor::BuildData(partial.DataTable, partialRow, parentRow);
			}
		}
	}
}

FSlateColor FResourceModelDataTableEditor::GetRowTextColor(FName RowName) const
{
	if (auto row = GetDataTable()->FindRow<FAnuResourceModelMesh>(RowName, "")) {
		for (auto& one : row->Meshes) {
			if (one.Mesh.IsNull()) {
				return FSlateColor(FColorList::Red);
			}
		}
	}

	return FAnuDataTableEditor::GetRowTextColor(RowName);
}

UDataTable* FResourceModelDataTableEditor::LoadPartialTable(FString tableName)
{
	FString path = FResourceMeshPartialDataTableEditor::SupportedDataTablePath + tableName + "." + tableName;
	return LoadObject<UDataTable>(NULL, *path);
}

#undef LOCTEXT_NAMESPACE