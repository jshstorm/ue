// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NavMeshExport.h"
#include "NavMeshExportStyle.h"
#include "NavMeshExportCommands.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Engine.h"
#include "Engine/World.h"
#include "LevelEditor.h"
//#include "AI/Navigation/RecastNavMesh.h"
//#include "AI/Navigation/NavigationData.h"
//#include "AI/Navigation/NavigationSystem.h"
#include "NavigationSystem.h"
#include "AI/NavDataGenerator.h"
#include "Editor.h"
#include "NavMesh/RecastNavMeshGenerator.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "HAL/PlatformString.h"
#include "../../../../server/source/include/AnuNavmeshFile.h"
#include "../Navmesh/Public/Detour/DetourNavMesh.h"
#include "HAL/PlatformFilemanager.h"

static const FName NavMeshExportTabName("NavMeshExport");

#define LOCTEXT_NAMESPACE "FNavMeshExportModule"

void FNavMeshExportModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FNavMeshExportStyle::Initialize();
	FNavMeshExportStyle::ReloadTextures();

	FNavMeshExportCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FNavMeshExportCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FNavMeshExportModule::PluginButtonClicked),
		FCanExecuteAction());
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FNavMeshExportModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	//{
	//	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	//	ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FNavMeshExportModule::AddToolbarExtension));
	//	
	//	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	//}
}

void FNavMeshExportModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FNavMeshExportStyle::Shutdown();

	FNavMeshExportCommands::Unregister();
}

void FNavMeshExportModule::PluginButtonClicked()
{
	UWorld* world = GEditor->GetEditorWorldContext().World();

	TArray<AActor*> volumes;
	UGameplayStatics::GetAllActorsOfClass(world, ANavMeshBoundsVolume::StaticClass(), volumes);
	if (volumes.Num() == 0) {
		return;
	}

	auto navMeshVolume = volumes[0];
	FString levelName = navMeshVolume->GetLevel()->GetOuter()->GetFName().ToString();

	FString contentDirectory = FPaths::ProjectContentDir();
	FString fileName = FString::Printf(TEXT("%s.anm"), *levelName);

	FString filepath = FPaths::Combine(contentDirectory, TEXT("ExportedReferences"), TEXT("NavMesh"), *fileName);

	if (Export(*filepath)) {
		FString Msg = FString::Printf(TEXT("AnuNavmesh export completed. (%s)"), *filepath);
		FText DialogText = FText::FromString(Msg);
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);
	}
	else {
		FText DialogText = FText::FromString("AnuNavmesh export failed.");
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);
	}
}

bool FNavMeshExportModule::Export(const TCHAR* filepath) const
{
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* fileHandle = platformFile.OpenWrite(filepath);
	if (fileHandle == nullptr) return false;

	UWorld* world = GEditor->GetEditorWorldContext().World();
	if (world == nullptr) return false;

	UNavigationSystemV1* navSys = UNavigationSystemV1::GetNavigationSystem(world);
	//UNavigationSystemBase* navSys = world->GetNavigationSystem();
	if (navSys == nullptr) return false;

	// agent data
	const TArray<FNavDataConfig>& supportedAgents = navSys->GetSupportedAgents();
	AnuNavmeshFileHeader fileHeader;
	fileHeader.version_ = ANUNAVMESH_VERSION;
	fileHeader.dataCount_ = supportedAgents.Num();
	AnuNavmeshFileDataInfo* filedatas = (AnuNavmeshFileDataInfo*)malloc(sizeof(AnuNavmeshFileDataInfo) * fileHeader.dataCount_);

	for (int i = 0; i < supportedAgents.Num(); i++) {
		ANavigationData* navData = navSys->GetNavDataForProps(supportedAgents[i]);

		const FBox& bounds = navData->GetBounds();
		const FVector& min = bounds.Min;
		const FVector& max = bounds.Max;

		AnuNavmeshFileDataInfo* info = &filedatas[i];
		info->minBoundingBox[0] = min.X;
		info->minBoundingBox[1] = min.Y;
		info->minBoundingBox[2] = min.Z;
		info->maxBoundingBox[0] = max.X;
		info->maxBoundingBox[1] = max.Y;
		info->maxBoundingBox[2] = max.Z;

		const FNavDataConfig& config = supportedAgents[i];
		ANSICHAR aName[1024];
		config.Name.GetPlainANSIString(aName);
		FPlatformString::Strcpy(info->agentName_, 32, aName);
		info->extend_[0] = config.GetExtent().X;
		info->extend_[1] = config.GetExtent().Y;
		info->extend_[2] = config.GetExtent().Z;
		info->agentRadius_ = config.AgentRadius;
		info->agentHeight_ = config.AgentHeight;
		info->navWalkingSearchHeightScale_ = config.NavWalkingSearchHeightScale;
		info->agentStepHeight_ = config.AgentStepHeight;
		info->bCanCrouch_ = config.bCanCrouch;
		info->bCanFly_ = config.bCanFly;
		info->bCanJump_ = config.bCanJump;
		info->bCanSwim_ = config.bCanSwim;
		info->bCanWalk_ = config.bCanWalk;
		info->dataOffset_ = sizeof(AnuNavmeshFileHeader);
		info->tileCount_ = 0;
		info->dataSize_ = 0;
	}
	fileHandle->Write((uint8*)&fileHeader, sizeof(AnuNavmeshFileHeader));
	fileHandle->Write((uint8*)filedatas, sizeof(AnuNavmeshFileDataInfo) * fileHeader.dataCount_);

	unsigned long offset = sizeof(AnuNavmeshFileHeader) + sizeof(AnuNavmeshFileDataInfo) * fileHeader.dataCount_;
	for (int i = 0; i < supportedAgents.Num(); i++)
	{
		filedatas[i].dataOffset_ = offset;
		ANavigationData* navData = navSys->GetNavDataForProps(supportedAgents[i]);

		unsigned long dataSize = 0;

		ARecastNavMesh* unrealNavMesh = Cast<ARecastNavMesh>(navData);
		const dtNavMesh* navMesh = unrealNavMesh->GetRecastMesh();
		const dtNavMeshParams* params = navMesh->getParams();
		// 

		// Write
		fileHandle->Write((uint8*)params, sizeof(dtNavMeshParams));
		dataSize += sizeof(dtNavMeshParams);
		// Tile
		for (int ti = 0; ti < navMesh->getMaxTiles(); ti++)
		{
			const dtMeshTile* tile = navMesh->getTile(ti);
			if (tile != nullptr && tile->header != nullptr && tile->dataSize > 0)
			{
				dtTileRef tileRef = navMesh->getTileRef(tile);
				unsigned long  tileDataSize = tile->dataSize;
				// Write
				fileHandle->Write((uint8*)&tileRef, sizeof(tileRef));
				fileHandle->Write((uint8*)&tileDataSize, sizeof(tileDataSize));
				dataSize += sizeof(tileRef);
				dataSize += sizeof(tileDataSize);

				unsigned char* tileData = tile->data;
				// Write
				fileHandle->Write(tileData, tileDataSize);
				dataSize += sizeof(tileDataSize);
				filedatas[i].tileCount_++;
				// not tile cache layer and compressed data
			}
		}
		filedatas[i].dataSize_ = dataSize;
	}
	fileHandle->Flush();
	fileHandle->Seek(sizeof(AnuNavmeshFileHeader));
	// rewrite data header
	fileHandle->Write((uint8*)filedatas, sizeof(AnuNavmeshFileDataInfo) * fileHeader.dataCount_);
	fileHandle->SeekFromEnd(0);
	fileHandle->Flush();
	delete fileHandle;

	free(filedatas);

	return true;
}

void FNavMeshExportModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FNavMeshExportCommands::Get().PluginAction);
}

void FNavMeshExportModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FNavMeshExportCommands::Get().PluginAction);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNavMeshExportModule, NavMeshExport)