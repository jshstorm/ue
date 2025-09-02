// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "Reference_Resource.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraShake.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "Particles/ParticleSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "AnuDyeingIconReferenceStatics.h"
#include "Dom/JsonObject.h"
#include "Sound/SoundWave.h"

const FSoftObjectPath& FAnuTableRow::GetRoute() const
{
	check(false);
	return EmptyRoute;
}

FString FAnuTableRow::GetPath() const
{
	check(false);
	return EmptyPath;
}

UObject* FAnuTableRow::LoadResource()
{
	check(false);
	return nullptr;
}

UClass* FAnuTableRow::LoadClass()
{
	check(false);
	return nullptr;
}

// resource icon
const FSoftObjectPath& FAnuResourceIcon::GetRoute() const
{
	return Texture.ToSoftObjectPath();
}

FString FAnuResourceIcon::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UObject* FAnuResourceIcon::LoadResource()
{
	return Texture.LoadSynchronous();
}

#if WITH_EDITOR
void FAnuResourceIcon::OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
{
	if (UAnuDyeingIconReferenceStatics::IsDyeingIconReference(Texture.ToSoftObjectPath())) {
		FSoftObjectPath maskedPath = UAnuDyeingIconReferenceStatics::GetDyeingMaskIconReference(Texture.ToSoftObjectPath());
		MaskTextureReadOnly = maskedPath;
	}
	else {
		// Set as default.
		MaskTextureReadOnly = {};
	}
}
#endif

// resource camera
const FSoftObjectPath& FAnuResourceCamera::GetRoute() const
{
	return Camera.ToSoftObjectPath();
}

FString FAnuResourceCamera::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UClass* FAnuResourceCamera::LoadClass()
{
	return Camera.LoadSynchronous();
}

// resource camera shake
const FSoftObjectPath& FAnuResourceCameraShake::GetRoute() const
{
	return CameraShake.ToSoftObjectPath();
}

FString FAnuResourceCameraShake::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UClass* FAnuResourceCameraShake::LoadClass()
{
	return CameraShake.LoadSynchronous();
}

////////////////////////////////////////////////////
// resource character
const FSoftObjectPath& FAnuResourceCharacterClass::GetRoute() const
{
	return Class.ToSoftObjectPath();
}

FString FAnuResourceCharacterClass::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UClass* FAnuResourceCharacterClass::LoadClass()
{
	return Class.LoadSynchronous();
}

UClass* FAnuResourceCharacterClass::LoadAnimClass(Stance type/* = Stance::Normal*/) const
{
	return type == Stance::Normal ? AnimationBP.LoadSynchronous() : BattleAnimationBP.LoadSynchronous();
}

////////////////////////////////////////////////////
// resource mash
FAnuResourceModelMesh::FAnuResourceModelMesh(const FAnuMeshInfo& mesh)
{
	Meshes.Emplace(mesh);
}

TSoftObjectPtr<UObject> FAnuResourceModelMesh::GetMainMesh()
{
	return Meshes[0].Mesh;
}

void FAnuResourceModelMesh::BuildMeshInfo(TArray<FAnuMeshInfo*>& meshes)
{
	for (auto& mesh : Meshes) {
		meshes.Emplace(&mesh);
	}
}

void FAnuResourceModelMesh::VisitMeshInfos(TFunction<void(FAnuMeshInfo*)>&& visitor)
{
	for (auto& mesh : Meshes) {
		visitor(&mesh);
	}
}

EAttachmentParts FAnuResourceModelMesh::GetMainParts()
{
	return Meshes.Num() != 0 ? Meshes[0].Parts : _mainParts;
}

const FSoftObjectPath& FAnuResourceModelMesh::GetRoute() const
{
	return Meshes[0].Mesh.ToSoftObjectPath();
}

FString FAnuResourceModelMesh::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UObject* FAnuResourceModelMesh::LoadResource()
{
	return Meshes[0].Mesh.LoadSynchronous();
}

UClass* FAnuResourceModelMesh::LoadAnimClass(Stance type/* = Stance::Normal*/) const
{
	return Meshes[0].AnimationBP.LoadSynchronous();
}

////////////////////////////////////////////////////
// Resource Mesh Construct 
FAnuTableRow* FAnuResourceMeshConstruct::GetTable(const FName& uid)
{
	return DataTable->FindRow<FAnuTableRow>(uid, TEXT(""), false);
}

////////////////////////////////////////////////////
// resource world
const FSoftObjectPath& FAnuResourceModelWorld::GetRoute() const
{
	return World.ToSoftObjectPath();
}

FString FAnuResourceModelWorld::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UObject* FAnuResourceModelWorld::LoadResource()
{
	return World.LoadSynchronous();
}

////////////////////////////////////////////////////
// resource particle
const FSoftObjectPath& FAnuResourceParticle::GetRoute() const
{
	return Particle.ToSoftObjectPath();
}

FString FAnuResourceParticle::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UObject* FAnuResourceParticle::LoadResource()
{
	return LoadParticle();
}

UParticleSystem* FAnuResourceParticle::LoadParticle()
{
	return Particle.LoadSynchronous();
}

// resource level sequence
const FSoftObjectPath& FAnuResourceLevelSequence::GetRoute() const
{
	return LevelSequence.ToSoftObjectPath();
}

FString FAnuResourceLevelSequence::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UObject* FAnuResourceLevelSequence::LoadResource()
{
	return LevelSequence.LoadSynchronous();
}

////////////////////////////////////////////////////
#pragma region Snapshot
void FSnapshotAttachment::LoadAssets(TArray<FSoftObjectPath>& assets)
{
	FSoftObjectPath attachPath{ Mesh.GetRoute() };
	FString strPath{ (attachPath.ToString()) };
	if (strPath.IsEmpty() == false) {
		assets.Emplace(attachPath);
	}
}

const FString FSnapshotActor::GetAnimName() const
{
	return AnimInstance.GetAssetName();
}

UAnimationAsset* FSnapshotActor::GetResource()
{
	return AnimInstance.LoadSynchronous();
}

FTransform FSnapshotActor::GetPinPoint() const
{
	if (PinPoint.RowName == NAME_None) {
		return PinOffset_Transform;
	}

	if (FAnuResourceSnapshot_Pin* table = PinPoint.GetRow<FAnuResourceSnapshot_Pin>("")) {
		return FTransform{ PinOffset_Transform.Rotator() + table->Rotation, PinOffset_Transform.GetLocation() + table->Location, PinOffset_Transform.GetScale3D() };
	}
	return PinOffset_Transform;
}

void FSnapshotActor::LoadAssets(TArray<FSoftObjectPath>& assets)
{
	assets.Emplace(AnimInstance.ToSoftObjectPath());

	Attachment.LoadAssets(assets);
}

UTexture2D* FAnuResourceSnapshot::GetTexture(int32 index)
{
	index = FMath::Clamp(index, 0, ResultTexture.Num() - 1);
	if (ResultTexture.IsValidIndex(index)) {
		return ResultTexture[index].LoadSynchronous();
	}

	return nullptr;
}

UTexture2D* FAnuResourceSnapshot::GetBGTexture()
{
#if WITH_EDITORONLY_DATA
	return BGTexture.LoadSynchronous();
#else
	return nullptr;
#endif
}

FAnuResourceSnapshot::FAnuResourceSnapshot()
{
#if WITH_EDITORONLY_DATA
	ExportTexture = false;
#endif
}

#if WITH_EDITORONLY_DATA
#include "UnrealEd/Public/ObjectTools.h"

void FAnuResourceSnapshot::DeleteCachingTextures()
{
	if (ResultTexture.Num() == 0) {
		return;
	}

	TArray<UObject*> tempTextures;
	for (auto& texture : ResultTexture) {
		auto obj = texture.LoadSynchronous();
		if (obj->IsValidLowLevel()) {
			tempTextures.Emplace(obj);
		}
	}
	ResultTexture.Empty();
	ObjectTools::ForceDeleteObjects(tempTextures, false);
}
#endif

void FAnuResourceSnapshot::LoadAssets(TArray<FSoftObjectPath>& assets)
{
	for (auto& aniData : PC) {
		aniData.LoadAssets(assets);
	}

	for (auto& texture : ResultTexture) {
		assets.Emplace(texture.ToSoftObjectPath());
	}

	if (Studio.IsNull() == false) {
		assets.Emplace(Studio.ToSoftObjectPath());
	}
}

FSnapshotActor* FAnuResourceSnapshot::GetPCData(EGender gender)
{
	uint8 genderIdx = FMath::Clamp(static_cast<uint8>(gender), (uint8)0, (uint8)(PC.Num() - 1));
	return PC.IsValidIndex(genderIdx) ? &PC[genderIdx] : nullptr;
}
#pragma endregion Snapshot

////////////////////////////////////////////////////
UTexture2D* FAnuResourceLivePageFeed::GetHostIconTexture()
{
	return HostIcon.LoadSynchronous();
}

UTexture2D* FAnuResourceLivePageFeed::GetThumbnailTexture()
{
	check(ThumbnailType == EAnuImageType::Snapshot);
	return ThumbnailTexture.LoadSynchronous();
}

UObject* FAnuResourceLivePageFeed::GetButtonIconTexture()
{
	return ButtonIcon.LoadSynchronous();
}

const FSoftObjectPath& FAnuResourceLivePageFeed::GetThumbnailTextureRoute() const
{
	check(ThumbnailType == EAnuImageType::Texture);
	return ThumbnailTexture.ToSoftObjectPath();
}

// resource area
UTexture2D* FAnuResourceArea::GetIconTexture()
{
	return Icon.LoadSynchronous();
}

//////////////////////////////////////////////////////////////////////////
// AnuResourceElectronicBoard
UObject* FAnuResourceElectronicBoard::LoadResourceObject()
{
	if (auto resTable = GetTable())	{
		return resTable->LoadResource();
	}
	
	return nullptr;
}

void FAnuResourceElectronicBoard::LoadAssets(TArray<FSoftObjectPath>& assets) const
{
	if (auto resTable = GetTable()) {
		assets.Emplace(resTable->GetRoute());
	}
}

FAnuTableRow* FAnuResourceElectronicBoard::GetTable() const
{
	if (TableValue.RowName != NAME_None) {
		return TableValue.GetRow<FAnuTableRow>("");
	}
	return nullptr;
}

UObject* FAnuResourceElectronicBoardNews::LoadResource()
{
	if (auto table = Sequence.GetRow<FAnuResourceLevelSequence>("")) {
		return table->LoadResource();
	}
	return nullptr;
}

const FSoftObjectPath& FAnuResourceElectronicBoardNews::GetRoute() const
{
	if (auto table = Sequence.GetRow<FAnuResourceLevelSequence>("")) {
		return table->GetRoute();
	}
	return FAnuTableRow::GetRoute();
}

// resource push alarm
UTexture2D* FAnuResourcePushAlarm::GetIconTexture()
{
	return Icon.LoadSynchronous();
}

// resource actor
const FSoftObjectPath& FAnuResourceActor::GetRoute() const
{
	return Actor.ToSoftObjectPath();
}

FString FAnuResourceActor::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UClass* FAnuResourceActor::LoadClass()
{
	return Actor.LoadSynchronous();
}

// resource runnable
const FSoftObjectPath& FAnuResourceRunnable::GetRoute() const
{
	return Runnable.ToSoftObjectPath();
}

FString FAnuResourceRunnable::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UClass* FAnuResourceRunnable::LoadClass()
{
	return Runnable.LoadSynchronous();
}

UObject* FAnuResourceRunnable::GetResource(UObject* outer)
{
	UClass* runnableClass = LoadClass();
	return NewObject<UObject>(outer, runnableClass);
}

//////////////////////////////////////////////////////
// social
bool FAnuResourceSocialMotion::IsLoopMotion() const
{
	return Animation.LoopAction != NAME_None;
}

//////////////////////////////////////////////////////
// appearance preset
TArray<FDataTableRowHandle*> FAnuResourceAppearancePreset::GetRows()
{
	TArray<FDataTableRowHandle*> rows;
	rows.Emplace(&Face);
	rows.Emplace(&Hair);
	rows.Emplace(&Outfit);
	rows.Emplace(&Shoes);
	rows.Emplace(&Handwear);
	rows.Emplace(&Hat);
	rows.Emplace(&Eyewear);
	rows.Emplace(&Mask);
	rows.Emplace(&Back);
	rows.Emplace(&Tail);
	rows.Emplace(&Ear);
	rows.Emplace(&Upper);
	rows.Emplace(&Lens);
	rows.Emplace(&MakeUp);
	rows.Emplace(&Sword);
	rows.Emplace(&Bow);

	return MoveTemp(rows);
}

TArray<FLinearColor>* FAnuResourceAppearancePreset::GetColors(EResourceModelType modelType)
{
	switch (modelType)
	{
	case EResourceModelType::Outfit:
		return &OutfitColors;
	case EResourceModelType::Hair:
		return &HairColors;
	case EResourceModelType::Shoes:
		return &ShoesColors;
	case EResourceModelType::Lens:
		return &LensColors;
	}
	return nullptr;
}

void FAnuResourceAppearancePreset::SetDataTable(UDataTable* table)
{
	auto rows{ GetRows() };
	for (auto& row : rows) {
		row->DataTable = table;
	}
}

//////////////////////////////////////////////////////
// LevelDesign
const FSoftObjectPath& FAnuResourceLevelDesign::GetRoute() const
{
	return Asset.ToSoftObjectPath();
}

FString FAnuResourceLevelDesign::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UObject* FAnuResourceLevelDesign::LoadResource()
{
	return Asset.LoadSynchronous();
}

//////////////////////////////////////////////////////
// BehaviorTree
const FSoftObjectPath& FAnuResourceBehaviorTree::GetRoute() const
{
	return Asset.ToSoftObjectPath();
}

FString FAnuResourceBehaviorTree::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UObject* FAnuResourceBehaviorTree::LoadResource()
{
	return Asset.LoadSynchronous();
}

//////////////////////////////////////////////////////
// Notice
void FAnuResourceNotice::Serialize(TSharedPtr<FJsonObject> postMetaObj)
{
	postMetaObj->SetNumberField("order", Order);
	postMetaObj->SetStringField("schedule", Schedule);
	postMetaObj->SetBoolField("show_schedule", ShowSchedule);

	{ // image: use gms upload feature
// 		TArray<TSharedPtr<FJsonValue>> jsonValueArray;
// 		jsonValueArray.Emplace(MakeShared<FJsonValueString>(ImageUrl));
// 		postMetaObj->SetArrayField("images", jsonValueArray);
	}
	{ // buttons
		TArray<TSharedPtr<FJsonValue>> jsonValueArray;
		for (auto& button : LinkButtons) {
			TSharedPtr<FJsonObject> linkButtonObj = MakeShareable(new FJsonObject());
			linkButtonObj->SetStringField("text", button.Text.ToString());
			linkButtonObj->SetStringField("font_color", button.FontColor.ToFColor(true).ToHex());
			linkButtonObj->SetStringField("bg_color", button.BgColor.ToFColor(true).ToHex());
			linkButtonObj->SetStringField("icon", button.IconUID.ToString());
			linkButtonObj->SetStringField("run_type", button.RunType.ToString());
			linkButtonObj->SetStringField("run_type_value", button.RunTypeValue);

			jsonValueArray.Emplace(MakeShared<FJsonValueObject>(linkButtonObj));
		}
		postMetaObj->SetArrayField("link_buttons", jsonValueArray);
	}
}

bool FAnuResourceNotice::Deserialize(TSharedPtr<FJsonObject> postMetaObj, FDateTime& startDate, FDateTime& endDate)
{
	FDateTime utcNow{ FDateTime::UtcNow() };
	FPeriodCondition dateParser;
	FString strValue;

	postMetaObj->TryGetNumberField("order", Order);
	postMetaObj->TryGetStringField("schedule", Schedule);
	postMetaObj->TryGetBoolField("show_schedule", ShowSchedule);

	TArray<FString> images;
	postMetaObj->TryGetStringArrayField("images", images);
	if (images.Num() != 0) {
		ImageUrl = images[0];
	}

	const TArray<TSharedPtr<FJsonValue>>* linkButtons;
	if (postMetaObj->TryGetArrayField("link_buttons", linkButtons)) {
		for (auto& linkButtonMeta : *linkButtons) {
			FAnuNoticeButton button;
			TSharedPtr<FJsonObject> linkButtonObj = linkButtonMeta->AsObject();

			if (linkButtonObj->TryGetStringField("text", strValue)) {
				button.Text = FText::FromString(strValue);
			}

			if (linkButtonObj->TryGetStringField("font_color", strValue)) {
				button.FontColor = FLinearColor::FromSRGBColor(FColor::FromHex(strValue));
			}

			if (linkButtonObj->TryGetStringField("bg_color", strValue)) {
				button.BgColor = FLinearColor::FromSRGBColor(FColor::FromHex(strValue));
			}

			if (linkButtonObj->TryGetStringField("icon", strValue)) {
				button.IconUID = *strValue;
			}

			if (linkButtonObj->TryGetStringField("run_type", strValue)) {
				button.RunType = *strValue;
			}

			if (linkButtonObj->TryGetStringField("run_type_value", strValue)) {
				button.RunTypeValue = strValue;
			}

			LinkButtons.Emplace(MoveTemp(button));
		}
	}

	startDate = FDateTime::Now();
	if (postMetaObj->TryGetStringField("start_date", strValue)) {
		strValue = strValue.Replace(TEXT("+"), TEXT(" "));
		if (FDateTime::Parse(strValue, startDate) == false) {
			checkf(false, TEXT("[notice] notice has invalid start_date[%s]"), *strValue);
			return false;
		}
		startDate = URefSchedule::ConvertUtcToLocal(startDate);
	}


	endDate = FDateTime::Now();
	if (postMetaObj->TryGetStringField("end_date", strValue)) {
		strValue = strValue.Replace(TEXT("+"), TEXT(" "));
		if (FDateTime::Parse(strValue, endDate) == false) {
			checkf(false, TEXT("[notice] notice has invalid end_date[%s]"), *strValue);
			return false;
		}
		endDate = URefSchedule::ConvertUtcToLocal(endDate);
	}

	return true;
}

const FSoftObjectPath& FAnuResourceSound::GetRoute() const
{
	return Sound.ToSoftObjectPath();
}

FString FAnuResourceSound::GetPath() const
{
	return GetRoute().GetAssetPathString();
}

UObject* FAnuResourceSound::LoadResource()
{
	return Sound.LoadSynchronous();
}