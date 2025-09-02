// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_AnuDataTable.h"
#include "AnuDataTableEditorModule.h"
#include "ToolMenus.h"
#include "Misc/FileHelper.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/MessageDialog.h"
#include "Framework/Application/SlateApplication.h"

#include "Editor/DataTableEditor/Public/DataTableEditorModule.h"
#include "Viewer/ResourceSnapshotDataTableEditor.h"
#include "DesktopPlatformModule.h"
#include "AssetToolsModule.h"
#include "Actor/SnapshotActorBase.h"

#include "Snapshot/SnapshotStudioActor.h"
#include "Common/ObjectStructures.h"
#include "Reference_Resource.h"
#include "UnrealEd/Public/FileHelpers.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FAssetTypeActions_AnuDataTable::FAssetTypeActions_AnuDataTable(EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

void FAssetTypeActions_AnuDataTable::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	auto Tables = GetTypedWeakObjectPtrs<UObject>(InObjects);

	TArray<FString> ImportPaths;
	for (auto TableIter = Tables.CreateConstIterator(); TableIter; ++TableIter)
	{
		const UDataTable* CurTable = Cast<UDataTable>((*TableIter).Get());
		if (CurTable)
		{
			CurTable->AssetImportData->ExtractFilenames(ImportPaths);
		}
	}

	Section.AddMenuEntry(
		"DataTable_ExportAsCSV",
		LOCTEXT("DataTable_ExportAsCSV", "Export as CSV"),
		LOCTEXT("DataTable_ExportAsCSVTooltip", "Export the data table as a file containing CSV data."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_AnuDataTable::ExecuteExportAsCSV, Tables),
			FCanExecuteAction()
		)
	);

	Section.AddMenuEntry(
		"DataTable_ExportAsJSON",
		LOCTEXT("DataTable_ExportAsJSON", "Export as JSON"),
		LOCTEXT("DataTable_ExportAsJSONTooltip", "Export the data table as a file containing JSON data."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_AnuDataTable::ExecuteExportAsJSON, Tables),
			FCanExecuteAction()
		)
	);

	/* Not supported
	TArray<FString> PotentialFileExtensions;
	PotentialFileExtensions.Add(TEXT(".xls"));
	PotentialFileExtensions.Add(TEXT(".xlsm"));
	PotentialFileExtensions.Add(TEXT(".csv"));
	PotentialFileExtensions.Add(TEXT(".json"));
	Section.AddMenuEntry(
		"DataTable_OpenSourceData",
		LOCTEXT("DataTable_OpenSourceData", "Open Source Data"),
		LOCTEXT("DataTable_OpenSourceDataTooltip", "Opens the data table's source data file in an external editor. It will search using the following extensions: .xls/.xlsm/.csv/.json"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_AnuDataTable::ExecuteFindSourceFileInExplorer, ImportPaths, PotentialFileExtensions),
			FCanExecuteAction::CreateSP(this, &FAssetTypeActions_AnuDataTable::CanExecuteFindSourceFileInExplorer, ImportPaths, PotentialFileExtensions)
		)
	);
	*/
}

void FAssetTypeActions_AnuDataTable::ExecuteExportAsCSV(TArray< TWeakObjectPtr<UObject> > Objects)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto DataTable = Cast<UDataTable>((*ObjIt).Get());
		if (DataTable)
		{
			const FText Title = FText::Format(LOCTEXT("DataTable_ExportCSVDialogTitle", "Export '{0}' as CSV..."), FText::FromString(*DataTable->GetName()));
			const FString CurrentFilename = DataTable->AssetImportData->GetFirstFilename();
			const FString FileTypes = TEXT("Data Table CSV (*.csv)|*.csv");

			TArray<FString> OutFilenames;
			DesktopPlatform->SaveFileDialog(
				ParentWindowWindowHandle,
				Title.ToString(),
				(CurrentFilename.IsEmpty()) ? TEXT("") : FPaths::GetPath(CurrentFilename),
				(CurrentFilename.IsEmpty()) ? TEXT("") : FPaths::GetBaseFilename(CurrentFilename) + TEXT(".csv"),
				FileTypes,
				EFileDialogFlags::None,
				OutFilenames
			);

			if (OutFilenames.Num() > 0)
			{
				FFileHelper::SaveStringToFile(DataTable->GetTableAsCSV(), *OutFilenames[0]);
			}
		}
	}
}

void FAssetTypeActions_AnuDataTable::ExecuteExportAsJSON(TArray< TWeakObjectPtr<UObject> > Objects)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto DataTable = Cast<UDataTable>((*ObjIt).Get());
		if (DataTable)
		{
			const FText Title = FText::Format(LOCTEXT("DataTable_ExportJSONDialogTitle", "Export '{0}' as JSON..."), FText::FromString(*DataTable->GetName()));
			const FString CurrentFilename = DataTable->AssetImportData->GetFirstFilename();
			const FString FileTypes = TEXT("Data Table JSON (*.json)|*.json");

			TArray<FString> OutFilenames;
			DesktopPlatform->SaveFileDialog(
				ParentWindowWindowHandle,
				Title.ToString(),
				(CurrentFilename.IsEmpty()) ? TEXT("") : FPaths::GetPath(CurrentFilename),
				(CurrentFilename.IsEmpty()) ? TEXT("") : FPaths::GetBaseFilename(CurrentFilename) + TEXT(".json"),
				FileTypes,
				EFileDialogFlags::None,
				OutFilenames
			);

			if (OutFilenames.Num() > 0)
			{
				FFileHelper::SaveStringToFile(DataTable->GetTableAsJSON(EDataTableExportFlags::UseJsonObjectsForStructs), *OutFilenames[0]);
			}
		}
	}
}

void FAssetTypeActions_AnuDataTable::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	TArray<UDataTable*> DataTablesToOpen;
	TArray<UDataTable*> InvalidDataTables;

	for (UObject* Obj : InObjects)
	{
		UDataTable* Table = Cast<UDataTable>(Obj);
		if (Table)
		{
			if (Table->GetRowStruct())
			{
				DataTablesToOpen.Add(Table);
			}
			else
			{
				InvalidDataTables.Add(Table);
			}
		}
	}

	if (InvalidDataTables.Num() > 0)
	{
		FTextBuilder DataTablesListText;
		DataTablesListText.Indent();
		for (UDataTable* Table : InvalidDataTables)
		{
			const FName ResolvedRowStructName = Table->GetRowStructName();
			DataTablesListText.AppendLineFormat(LOCTEXT("DataTable_MissingRowStructListEntry", "* {0} (Row Structure: {1})"), FText::FromString(Table->GetName()), FText::FromName(ResolvedRowStructName));
		}

		FText Title = LOCTEXT("DataTable_MissingRowStructTitle", "Continue?");
		const EAppReturnType::Type DlgResult = FMessageDialog::Open(
			EAppMsgType::YesNoCancel,
			FText::Format(LOCTEXT("DataTable_MissingRowStructMsg", "The following Data Tables are missing their row structure and will not be editable.\n\n{0}\n\nDo you want to open these data tables?"), DataTablesListText.ToText()),
			&Title
		);

		switch (DlgResult)
		{
		case EAppReturnType::Yes:
			DataTablesToOpen.Append(InvalidDataTables);
			break;
		case EAppReturnType::Cancel:
			return;
		default:
			break;
		}
	}

	FAnuDataTableEditorModule& DataTableEditorModule = FModuleManager::LoadModuleChecked<FAnuDataTableEditorModule>("AnuDataTableEditor");

	for (UDataTable* Table : DataTablesToOpen)
	{
		auto tableEditor = DataTableEditorModule.CreateDataTableEditor(EToolkitMode::Standalone, EditWithinLevelEditor, Table);

		if (FResourceSnapshotDataTableEditor::SupportedDataTableName.Equals(Table->GetName())){
			BindSnapshotExport(&(tableEditor.Get()));
		}
	}
}

void FAssetTypeActions_AnuDataTable::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		const auto DataTable = CastChecked<UDataTable>(Asset);
		DataTable->AssetImportData->ExtractFilenames(OutSourceFilePaths);
	}
}

// Attempts to export temporary CSV files and diff those. If that fails we fall back to diffing the data table assets directly.
void FAssetTypeActions_AnuDataTable::PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision) const
{
	UDataTable* OldDataTable = CastChecked<UDataTable>(OldAsset);
	UDataTable* NewDataTable = CastChecked<UDataTable>(NewAsset);

	// Build names for temp csv files
	FString RelOldTempFileName = FString::Printf(TEXT("%sTemp%s-%s.csv"), *FPaths::DiffDir(), *OldAsset->GetName(), *OldRevision.Revision);
	FString AbsoluteOldTempFileName = FPaths::ConvertRelativePathToFull(RelOldTempFileName);
	FString RelNewTempFileName = FString::Printf(TEXT("%sTemp%s-%s.csv"), *FPaths::DiffDir(), *NewAsset->GetName(), *NewRevision.Revision);
	FString AbsoluteNewTempFileName = FPaths::ConvertRelativePathToFull(RelNewTempFileName);

	// save temp files
	bool OldResult = FFileHelper::SaveStringToFile(OldDataTable->GetTableAsCSV(EDataTableExportFlags::UseSimpleText), *AbsoluteOldTempFileName);
	bool NewResult = FFileHelper::SaveStringToFile(NewDataTable->GetTableAsCSV(EDataTableExportFlags::UseSimpleText), *AbsoluteNewTempFileName);

	if (OldResult && NewResult)
	{
		FString DiffCommand = GetDefault<UEditorLoadingSavingSettings>()->TextDiffToolPath.FilePath;

		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateDiffProcess(DiffCommand, AbsoluteOldTempFileName, AbsoluteNewTempFileName);
	}
	else
	{
		FAssetTypeActions_CSVAssetBase::PerformAssetDiff(OldAsset, NewAsset, OldRevision, NewRevision);
	}
}

void FAssetTypeActions_AnuDataTable::BindSnapshotExport(IDataTableEditor* tableEditor)
{
	FResourceSnapshotDataTableEditor* snapshotTable = static_cast<FResourceSnapshotDataTableEditor*>(tableEditor);
	snapshotTable->BindExport(FResourceSnapshotDataTableEditor::FExportDelegate::CreateLambda([this](const TArray<FSnapshotDataTableParam>& output) {
			TMap<FName, FAnuResourceSnapshot*> exportData;
			for (auto& it : output) {
				exportData.Emplace(it.key, it.resData);
			}
			
			ExecuteSnapshotTexture(exportData);
		}
	));
}

void FAssetTypeActions_AnuDataTable::ExecuteSnapshotTexture(const TMap<FName, FAnuResourceSnapshot*>& exportData)
{
	const FSoftClassPath ActorPath{ TEXT("/Game/Anu/Actor/Snapshot/BP_Snapshot_Base.BP_Snapshot_Base_c") };
	const TSoftClassPtr<ASnapshotStudioBase> ActorCore{ ActorPath };
	auto classObject = ActorCore.LoadSynchronous();

	const FString PerFixName{ TEXT("/Game/Anu/UI/SnapshotStudio/Texture/DataTable/snapshot.") };
	const FTransform Transform{ FVector(-100000.f, -100000.f, -100000.f) };

	for (auto& it : exportData) {
		auto& resData = it.Value;
		resData->DeleteCachingTextures();
		
		TArray<EDayType> dayTypes{ EDayType::Daytime };
		if (resData->CameraPoint == ESnapshotCameraPoint::DayTime) {
			dayTypes.Emplace(EDayType::Night);
		}

		for (auto& type : dayTypes) {
			ASnapshotStudioBase* studioActor = GWorld->SpawnActor<ASnapshotStudioBase>(resData->Studio.IsNull() ? classObject : resData->Studio.LoadSynchronous() , Transform);
			studioActor->Initialize(resData, type);
			studioActor->BuildExportMode(resData);

			ASnapshotActorBase* actor = GWorld->SpawnActor<ASnapshotActorBase>(resData->SpawnGroupActor.LoadSynchronous(), Transform);
			if (actor != nullptr) {
				actor->CheckAndMoveFromCameraPosiotion(studioActor->GetUseCameraComponent());
				
				TArray<UPrimitiveComponent*> skAllCompoents = actor->GetComponentsAs<UPrimitiveComponent>();
				for (UPrimitiveComponent* skmesh : skAllCompoents) {
					skmesh->SetLightingChannels(false, true, false);
				}

				if (resData->PC.Num() == 2) {
					TArray<UActorComponent*> actorCompoents = actor->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), "PC");
					if (actorCompoents.Num() > 0) {
						USkeletalMeshComponent* pcActorComponent = Cast<USkeletalMeshComponent>(actorCompoents[0]);

						for (FSnapshotActor& pcActor : resData->PC) {
							pcActor.PinOffset_Transform = pcActorComponent->GetComponentTransform();
							pcActor.FrameTime = pcActorComponent->AnimationData.SavedPosition;
						}

						pcActorComponent->SetVisibility(false, true);
					}
					else {
						actorCompoents = actor->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), "PCFemale");
						if (actorCompoents.Num() > 0) {
							USkeletalMeshComponent* pcActorComponent = Cast<USkeletalMeshComponent>(actorCompoents[0]);
							resData->PC[0].PinOffset_Transform = pcActorComponent->GetComponentTransform();
							resData->PC[0].FrameTime = pcActorComponent->AnimationData.SavedPosition;
							resData->PC[0].AnimInstance = pcActorComponent->AnimationData.AnimToPlay;
							pcActorComponent->SetVisibility(false, true);
						}
						actorCompoents = actor->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), "PCMale");
						if (actorCompoents.Num() > 0) {
							USkeletalMeshComponent* pcActorComponent = Cast<USkeletalMeshComponent>(actorCompoents[0]);
							resData->PC[1].PinOffset_Transform = pcActorComponent->GetComponentTransform();
							resData->PC[1].FrameTime = pcActorComponent->AnimationData.SavedPosition;
							resData->PC[1].AnimInstance = pcActorComponent->AnimationData.AnimToPlay;
							pcActorComponent->SetVisibility(false, true);
						}
					}
				}
			}

			FString textureName{ PerFixName + it.Key.ToString() + "_" + FString::FromInt(static_cast<int32>(type)) };
			if (UTexture* texture = studioActor->GetTexture2D(textureName)) {
				resData->ResultTexture.Emplace(texture);
			}

			studioActor->Release();
			GWorld->DestroyActor(studioActor);
			if (actor != nullptr) {
				GWorld->DestroyActor(actor);
			}
		}
	}

	//FLevelEditorActionCallbacks::CheckOutModifiedFiles_Clicked();
	TArray<UPackage*> PackagesToSave;
	FEditorFileUtils::GetDirtyWorldPackages(PackagesToSave);
	FEditorFileUtils::GetDirtyContentPackages(PackagesToSave);

	const bool bCheckDirty = true;
	const bool bPromptUserToSave = false;
	FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptUserToSave);
}
#undef LOCTEXT_NAMESPACE
