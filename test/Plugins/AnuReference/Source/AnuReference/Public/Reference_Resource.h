// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Reference.h"
#include "Engine/DataTable.h"
#include "PaperFlipbook.h"
#include "SlateCore/Public/Styling/SlateColor.h"

#include "Reference_Resource.generated.h"

class ENGINE_API UAnimInstance;

USTRUCT(Blueprintable)
struct ANUREFERENCE_API FEquipmentTranslucencySortPriority : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EAttachmentParts Part;
	
	UPROPERTY(EditAnywhere)
	int32 Priority;
};

USTRUCT(Blueprintable)
struct ANUREFERENCE_API FAnuTableRow : public FTableRowBase
{
	GENERATED_BODY()

	static inline FSoftObjectPath EmptyRoute;
	static inline FString EmptyPath;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere)
	FString Comment;
#endif
	virtual const FSoftObjectPath& GetRoute() const;
	virtual FString GetPath() const;
	virtual UObject* LoadResource();
	virtual UClass* LoadClass();
	virtual UClass* LoadAnimClass(Stance type = Stance::Normal) const { return nullptr; }
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceCharacterClass : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<class AActor> Class;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftClassPtr<class UAnimInstance> AnimationBP;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftClassPtr<class UAnimInstance> BattleAnimationBP;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UClass* LoadClass() override;
	virtual UClass* LoadAnimClass(Stance type = Stance::Normal) const override;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuMeshInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UObject> Mesh;

	UPROPERTY(EditAnywhere)
	EAttachmentParts Parts = EAttachmentParts::AP_Invalid;

	UPROPERTY(EditAnywhere)
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<class UAnimInstance> AnimationBP;

#pragma region ReadOnly
	UPROPERTY(BlueprintReadOnly)
	FName GroupName;

	UPROPERTY(BlueprintReadOnly)
	ESubMeshType SubPartsType = ESubMeshType::None;

	UPROPERTY(BlueprintReadOnly)
	TSet<ESubMeshType> HideSubMeshes;

	UPROPERTY(BlueprintReadOnly)
	float AppendHeight = 0;

	UPROPERTY(BlueprintReadOnly)
	TSet<EAttachmentParts> HideAttachments;
#pragma endregion

	FAnuMeshInfo() {}
	FAnuMeshInfo(TSoftObjectPtr<UObject> inMesh, EAttachmentParts inParts, TSoftClassPtr<UAnimInstance> inAnimationBP)
		: Mesh(inMesh), Parts(inParts), AnimationBP(inAnimationBP) { }

	const FSoftObjectPath& GetRoute() const { return Mesh.ToSoftObjectPath(); }
	const FString GetPath() const { return GetRoute().GetAssetPathString(); }
};

UENUM(BlueprintType)
enum class EResourceModelType : uint8
{
	None,

	Outfit,
	Face,
	Hair,
	Shoes,

	Bracelet,
	Gloves,
	Hat,
	Eyewear,
	Mask,
	Back,
	Tail,

	SwordOfPlayer,
	BowOfPlayer,
	UnitOfPlayer,

	WeaponOfMonster,

	GiftBoxOfFairy,
	HandPhone,

	Ear,
	Necklace,
	Armband,
	Lens,
	MakeUp,
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceModelMesh : public FAnuTableRow
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Main", meta = (AllowedClasses = "SkeletalMesh,StaticMesh"))
	TSoftObjectPtr<UObject> Mesh;

	UPROPERTY(EditAnywhere, Category = "Main")
	TSoftClassPtr<class UAnimInstance> AnimationBP;
#endif

	UPROPERTY(EditAnywhere, Category = "Main")
	EResourceModelType ModelType = EResourceModelType::None;
	UPROPERTY(EditAnywhere, Category = "Main")
	bool UseSocketOverride = false;
	UPROPERTY(EditAnywhere, Category = "Main", meta = (EditCondition = "UseSocketOverride"))
	FName SocketOverride;
	UPROPERTY(EditAnywhere, Category = "Material", meta = (EditCondition = "ModelType==EResourceModelType::Lens"))
	TSoftObjectPtr<class UMaterialInterface> Material;

#pragma region ReadOnly
	UPROPERTY(VisibleAnywhere, Category = "Auto-Generated")
	TArray<FAnuMeshInfo> Meshes;
	UPROPERTY(VisibleAnywhere, Category = "Auto-Generated")
	EAttachmentParts _mainParts;
	UPROPERTY(VisibleAnywhere, Category = "Auto-Generated")
	EAttachmentParts _materialOverrideTargetParts;
#pragma endregion

	FAnuResourceModelMesh() {}
	FAnuResourceModelMesh(const FAnuMeshInfo& mesh);

#pragma region Lagacy
	TSoftObjectPtr<UObject> GetMainMesh();
	void BuildMeshInfo(TArray<FAnuMeshInfo*>& meshes);
#pragma endregion

	EAttachmentParts GetMainParts();
	void VisitMeshInfos(TFunction<void(FAnuMeshInfo*)>&& visitor);

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UObject* LoadResource() override;
	virtual UClass* LoadAnimClass(Stance type = Stance::Normal)  const override;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceIcon : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<class UTexture2D> Texture;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<class UTexture2D> MaskTextureReadOnly;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bDyeingDynamic = false;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UObject* LoadResource() override;

#if WITH_EDITOR
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override;
#endif
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceCamera : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<class ACameraActor> Camera;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UClass* LoadClass() override;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceCameraShake : public FAnuTableRow
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<class UMatineeCameraShake> CameraShake;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UClass* LoadClass() override;
};

UENUM(BlueprintType)
enum class EMeshConstructType : uint8 {
	Construct, Hide,
};

USTRUCT(BlueprintType)
struct  ANUREFERENCE_API FAnuResourceMeshConstruct : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		EMeshConstructType Type;
	UPROPERTY(EditAnywhere)
		const UDataTable* DataTable;

	FAnuTableRow* GetTable(const FName& uid);
};

class USkeletalMesh;

USTRUCT()
struct ANUREFERENCE_API FAnuResourceConstructModel : public FAnuTableRow
{
	GENERATED_USTRUCT_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceShoesConstruct : public FAnuResourceConstructModel
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<USkeletalMesh> Panty;
	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<USkeletalMesh> Thigh;
	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<USkeletalMesh> Thin;
	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<USkeletalMesh> Foot;
	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<USkeletalMesh> Boots;
	UPROPERTY(EditAnywhere)
		float AppendHeight;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceOutfitConstruct : public FAnuResourceConstructModel
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<USkeletalMesh> Outfit;
	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<USkeletalMesh> Hands;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceWeaponConstruct : public FAnuTableRow
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Shield")
	TSoftObjectPtr<USkeletalMesh> Shield;
	UPROPERTY(EditAnywhere, Category = "Shield")
	TSoftClassPtr<UAnimInstance> ShieldAnimBP;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceModelWorld : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<class UWorld> World;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UObject* LoadResource() override;
};

UENUM(BlueprintType)
enum class EParticleOffsetType : uint8 {
	None, Center, Head
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceParticle : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<class UParticleSystem> Particle;
	UPROPERTY(EditAnywhere)
		FName AttachBoneName {"Root"};
	UPROPERTY(EditAnywhere)
		EParticleOffsetType LocationOffsetType;
	UPROPERTY(EditAnywhere)
		FVector Offset;
	UPROPERTY(EditAnywhere)
		FRotator Rotation {FRotator::ZeroRotator };
	UPROPERTY(EditAnywhere)
		FVector Scale { FVector::OneVector };
	UPROPERTY(EditAnywhere)
		bool KeepWorldScale;
	UPROPERTY(EditAnywhere)
		bool Loop = false;
	UPROPERTY(EditAnywhere)
		bool DetachWithAnimEnded = true;
	UPROPERTY(EditAnywhere)
		float DelaySeconds;
	UPROPERTY(EditAnywhere, Category="BuffEffect", meta = (AllowedClasses = "SceneBase"))
		TArray<TSubclassOf<UObject>> PermittedScenes;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UObject* LoadResource() override;
	UParticleSystem* LoadParticle();
};

UCLASS(BlueprintType)
class ANUREFERENCE_API UAnuLevelSequence : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceLevelSequence : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "LevelSequence"))
		TSoftObjectPtr<UObject> LevelSequence;
	UPROPERTY(EditAnywhere)
		ESkipType SkipType = ESkipType::Selectable;
	UPROPERTY(EditAnywhere)
		bool BlockSituation = false;
	UPROPERTY(EditAnywhere)
		bool ShowOnlySequenceActor = false;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UObject* LoadResource() override;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FClassPoseSequences
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "LevelSequence"))
	TArray<TSoftObjectPtr<UObject>> Sequences;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceLevelSequenceClassPose : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "LevelSequence"))
	TArray<FClassPoseSequences> LevelSequenceByGender;
};

#pragma region Snapshot
USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceSnapshot_Pin : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		FVector Location;
	UPROPERTY(EditAnywhere)
		FRotator Rotation;
};

UENUM(BlueprintType)
enum class ESnapshotAttachType : uint8 {
	None, LifeAction, Weapon, Manual, Phone,
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FSnapshotAttachment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	ESnapshotAttachType Type = ESnapshotAttachType::None;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type != ESnapshotAttachType::Manual"))
	FName TypeArg;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == ESnapshotAttachType::LifeAction"), Category = "LifeAction")
	FName SequenceType;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == ESnapshotAttachType::LifeAction"))
	EClassLicense LicenseType = EClassLicense::None;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == ESnapshotAttachType::Manual"))
	FAnuMeshInfo Mesh;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == ESnapshotAttachType::Manual"))
	FTransform Transform;

public:
	void LoadAssets(TArray<FSoftObjectPath>& assets);
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FSnapshotActor
{
	GENERATED_BODY()

	virtual ~FSnapshotActor() { }

	/** if you want to snap npc costume, add npc uid. ex) objt.npc.campy */
	UPROPERTY(EditAnywhere, Category = "Skin|ServerExport")
		FString SnapNpcUID;
	UPROPERTY(EditAnywhere, Category = "Animation")
		TSoftObjectPtr<class UAnimationAsset> AnimInstance;
	UPROPERTY(EditAnywhere, Category = "Animation")
		float FrameTime = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Point")
		FDataTableRowHandle PinPoint;
	UPROPERTY(EditAnywhere, Category = "Point")
		FTransform PinOffset_Transform;
	UPROPERTY(EditAnywhere, Category = "Attach")
		FSnapshotAttachment Attachment;

	const FString GetAnimName() const;
	UAnimationAsset* GetResource();
	FTransform GetPinPoint() const;
	virtual void LoadAssets(TArray<FSoftObjectPath>& assets);
};

UENUM(BlueprintType)
enum class ESnapshotDecoType : uint8 {
	Texture, Particle, Text
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FSnapshotDeco
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FVector2D Position;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float Angle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FVector2D Size;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		ESnapshotDecoType Type;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == ESnapshotDecoType::Texture", AllowedClasses = "Texture,MaterialInterface,SlateTextureAtlasInterface"), BlueprintReadOnly, Category = "Texture")
		TSoftObjectPtr<UObject> Texture;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == ESnapshotDecoType::Particle"), BlueprintReadOnly, Category = "Particle")
		TSoftObjectPtr<class UParticleSystem> Particle;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == ESnapshotDecoType::Particle"), BlueprintReadOnly, Category = "Particle")
		float Particle_PlayTime;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == ESnapshotDecoType::Text"), BlueprintReadOnly, Category = "Text")
		FText Text;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == ESnapshotDecoType::Text"), BlueprintReadOnly, Category = "Text")
		FDataTableRowHandle TextStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FName Tag;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FSnapshotActor_Object : public FSnapshotActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowedClasses = "SkeletalMesh,StaticMesh"))
		TSoftObjectPtr<UObject> Mesh;
	UPROPERTY(EditAnywhere, Category = "Mesh")
		TArray<TSoftObjectPtr<class UMaterialInterface>> OverrideMaterials;
	UPROPERTY(EditAnywhere)
		FDataTableRowHandle AppearancePreset;
};

UENUM(BlueprintType)
enum class ESnapshotCameraPoint : uint8 {
	Up, Down, DayTime,
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceSnapshot : public FAnuTableRow
{
	GENERATED_BODY()

public:
	FAnuResourceSnapshot();

public:
	UPROPERTY(EditAnywhere, Category = "Texture")
		TArray<TSoftObjectPtr<UTexture2D>> ResultTexture;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "EditorOnly", meta = (MetaTag1 = "ClientOnly"))
		bool ExportTexture{ false };
	UPROPERTY(EditAnywhere, Category = "EditorOnly_Object", meta = (MetaTag1 = "ClientOnly"))
		TArray<FSnapshotActor_Object> Object;
	UPROPERTY(EditAnywhere, Category = "EditorOnly_BG", meta = (MetaTag1 = "ClientOnly"))
		TSoftObjectPtr<UTexture2D> BGTexture;
	UPROPERTY(EditAnywhere, Category = "EditorOnly_BG", meta = (MetaTag1 = "ClientOnly"))
		FTransform BGOffset_Transform { FQuat::Identity, FVector::ZeroVector, FVector(0.2f, 0.2f, 0.2f) };

	void DeleteCachingTextures();
#endif

public:
	UPROPERTY(EditAnywhere, Category = "PC")
		TArray<FSnapshotActor> PC;
	UPROPERTY(EditAnywhere, Category = "Runtime_object")
		FSnapshotActor_Object RuntimeObject;
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "SnapshotStudioBase"))
		TSoftClassPtr<AActor> Studio;
	
	UPROPERTY(EditAnywhere, Category = "Camera")
		ESnapshotCameraPoint CameraPoint = (ESnapshotCameraPoint)0;
	UPROPERTY(EditAnywhere, Category = "Camera")
		FDataTableRowHandle PostProcess;
	UPROPERTY(EditAnywhere, Category = "Deco")
		TArray<FSnapshotDeco> Deco;
	UPROPERTY(EditAnywhere, Category = "Runtime_object")
		TSoftClassPtr<AActor> SpawnGroupActor = nullptr;

	void LoadAssets(TArray<FSoftObjectPath>& assets);
	UTexture2D* GetTexture(int32 index);
	UTexture2D* GetBGTexture();
	FSnapshotActor* GetPCData(EGender gender);
};
#pragma endregion Snapshot

UENUM()
enum class EAnuImageType : uint8 {
	Texture, Snapshot, Gms, Product
};

UCLASS()
class ANUREFERENCE_API UAnuLivePageFeed : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceLivePageFeed : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Header")
		TSoftObjectPtr<UTexture2D> HostIcon;
	UPROPERTY(EditAnywhere, Category = "Header")
		FText HostName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Header")
		FText Title;
	/** 0: upload time calculate by feed added time. upper than 0: fixed upload time */
	UPROPERTY(EditAnywhere, Category = "Header")
		int32 UploadTimeBeforeMinute =0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Header")
		FText LocationText;
	UPROPERTY(EditAnywhere, Category = "Thumbnail")
		EAnuImageType ThumbnailType = EAnuImageType::Snapshot;
	UPROPERTY(EditAnywhere, Category = "Thumbnail", meta = (EditCondition = "ThumbnailType == EanuImageType::Snapshot"))
		FDataTableRowHandle ThumbnailSnapshot;
	UPROPERTY(EditAnywhere, Category = "Thumbnail", meta = (EditCondition = "ThumbnailType == EanuImageType::Texture"))
		TSoftObjectPtr<UTexture2D> ThumbnailTexture;
	UPROPERTY(EditAnywhere, Category = "Thumbnail", meta = (EditCondition = "ThumbnailType == EanuImageType::Gms"))
		FString ThumbnailUrl;
	/** show play image at center of thumbnail */
	UPROPERTY(EditAnywhere, Category = "Thumbnail")
		bool IsVideoType = false;
	UPROPERTY(EditAnywhere, Category = "Body")
		FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
		FName ReplyUID { NAME_None };
	UPROPERTY(EditAnywhere, Category = "Body")
		FName ReactionUID {	NAME_None };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
		TArray<FText> Tags;
	/** sorting rule */
	UPROPERTY(EditAnywhere, Category = "Rule")
		int32 Priority = 0;
	UPROPERTY(EditAnywhere, Category = "Rule")
		bool ExposeBySchedule = false;
	UPROPERTY(EditAnywhere, Category = "Rule", meta = (EditCondition = "ExposeBySchedule"))
		FName ScheduleUID {NAME_None};
	/** 0: no expiration. upper than 0: expire after n days from feed added time */
	UPROPERTY(EditAnywhere, Category = "Rule", meta = (EditCondition = "!ExposeBySchedule"))
		int32 ExpirationInDays = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button", meta = (EditCondition = "!ForceHideButton"))
		FText ButtonText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button", meta = (EditCondition = "!ForceHideButton"))
		FSlateColor ButtonColor;
	UPROPERTY(EditAnywhere, Category = "Button", meta = (EditCondition = "!ForceHideButton", AllowedClasses = "Texture,MaterialInterface,SlateTextureAtlasInterface"))
		TSoftObjectPtr<UObject> ButtonIcon;

	int32 _rtPriority = 0;

	UTexture2D* GetHostIconTexture();
	UTexture2D* GetThumbnailTexture();
	UObject* GetButtonIconTexture();
	const FSoftObjectPath& GetThumbnailTextureRoute() const;
};

UCLASS()
class ANUREFERENCE_API UAnuArea : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceArea : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText Description;
	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<UTexture2D> Icon;

	UTexture2D* GetIconTexture();
};

UENUM(BlueprintType)
enum class EElectronicBoardType : uint8 {
	Stream, Sequence, Texture, Video, News,
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceElectronicBoard : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		bool IsActivate = true;
	UPROPERTY(EditAnywhere)
		bool Reactive = false;
	UPROPERTY(EditAnywhere)
		EElectronicBoardType Type = EElectronicBoardType::Sequence;
	UPROPERTY(EditAnywhere, Category= "Stream", meta = (EditCondition = "Type == EElectronicBoardType::Stream || Type == EElectronicBoardType::Video"))
		FString URL;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EElectronicBoardType::Sequence || Type == EElectronicBoardType::Texture|| Type == EElectronicBoardType::News"))
		FDataTableRowHandle TableValue;
	UPROPERTY(EditAnywhere)
		int32 loopCount = 0;

	void LoadAssets(TArray<FSoftObjectPath>& assets) const;
	UObject* LoadResourceObject();
	FAnuTableRow* GetTable() const;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuElectronicBoardNewsFormat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Inner")
		bool UseDataTable = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "UseDataTable"), Category = "Inner")
		FName TextureOrSnapshotUID;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "!UseDataTable"), Category = "Inner")
		TArray<FSnapshotDeco> Deco;
	UPROPERTY(EditAnywhere, Category="Outer")
		FText Text;
	UPROPERTY(EditAnywhere, Category = "Outer")
		FDataTableRowHandle TextStyle;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceElectronicBoardNews : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		FDataTableRowHandle Sequence;
	UPROPERTY(EditAnywhere, Category = "Talk")
		int32 TalkSeqStartFrame = 0;
	UPROPERTY(EditAnywhere, Category = "Talk")
		int32 TalkSeqEndFrame = 0;
	UPROPERTY(EditAnywhere, Category = "Talk")
		int32 TalkSeqLoopStartFrame = 0;
	UPROPERTY(EditAnywhere, Category = "Talk")
		int32 TalkSeqLoopEndFrame = 0;
	UPROPERTY(EditAnywhere)
		TArray<FAnuElectronicBoardNewsFormat> Format;
	UPROPERTY(EditAnywhere)
		FString Schedule;

	virtual UObject* LoadResource() override;
	virtual const FSoftObjectPath& GetRoute() const override;
};

UCLASS()
class ANUREFERENCE_API UAnuPushAlarm : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourcePushAlarm : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FSlateColor IconTint { FLinearColor(1.f, 1.f, 1.f) };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FSlateColor BgTint { FLinearColor(1.f, 1.f, 1.f) };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText Message;

	UTexture2D* GetIconTexture();
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceActor : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AActor> Actor;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UClass* LoadClass() override;
};

UENUM(BlueprintType)
enum class EVoiceHandleType : uint8
{
	Off,
	Skipable,
	NeverSkip,
	AutoNext,
	Forced,
};

UCLASS()
class ANUREFERENCE_API UAnuRunnable : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceRunnable : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<UObject> Runnable;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UClass* LoadClass() override;

	UObject* GetResource(UObject* outer);
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceViewer : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<FName> FixedObjectUID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<FName> SampleObjectUID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 SampleObjectCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<FName> RandomObjectUID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 RandomObjectCount = 0;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuSocialAnimImpl
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FName StartAction = NAME_None;
	UPROPERTY(EditAnywhere)
	FName LoopAction = NAME_None;
	UPROPERTY(EditAnywhere)
	FName EndAction = NAME_None;
	UPROPERTY(EditAnywhere)
	TArray<FName> BoredActions;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceSocialMotion : public FAnuTableRow
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	int32 GUID = 0;
	UPROPERTY(EditAnywhere)
	int32 Order = 0;
	UPROPERTY(EditAnywhere)
	bool ShowMenu;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsSubscription = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsLooping = false;
	UPROPERTY(EditAnywhere)
	FAnuSocialAnimImpl Animation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<class UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<class UTexture2D> OnlySlotIcon;
	UPROPERTY(EditAnywhere)
	FLinearColor IconBeginColor;
	UPROPERTY(EditAnywhere)
	FLinearColor IconEndColor;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USoundBase> Sound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Desc;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<EShortcutType> Shortcut_Type;
	UPROPERTY(EditAnywhere, Category = "Attach")
	FAnuMeshInfo AttachMesh;
	UPROPERTY(EditAnywhere, Category = "Attach")
	int32 AttachOffset;
	UPROPERTY(EditAnywhere, Category = "Attach")
	FName AttachDir = NAME_None;
	UPROPERTY(EditAnywhere, Category = "Quest")
	int32 AnimCheckDelayMs = 2000;
	
public:
	bool IsLoopMotion() const;
};

UCLASS()
class ANUREFERENCE_API UAnuAppearancePreset : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceAppearancePreset : public FAnuTableRow
{
	GENERATED_BODY()

	// body
	UPROPERTY(EditAnywhere, Category = "Body")
		FDataTableRowHandle Face;
	UPROPERTY(EditAnywhere, Category = "Body")
		TArray<FLinearColor> EyeColors;
	UPROPERTY(EditAnywhere, Category = "Body")
		FLinearColor SkinColor { FColor::FromHex("FFDAC4FF") };
	UPROPERTY(EditAnywhere, Category = "Body")
		bool UseOddEyes = false;
	// basic costume
	UPROPERTY(EditAnywhere, Category = "Costume")
		FDataTableRowHandle Hair;
	UPROPERTY(EditAnywhere, Category = "Costume")
		TArray<FLinearColor> HairColors;
	UPROPERTY(EditAnywhere, Category = "Costume")
		FDataTableRowHandle Outfit;
	UPROPERTY(EditAnywhere, Category = "Costume")
		TArray<FLinearColor> OutfitColors;
	UPROPERTY(EditAnywhere, Category = "Costume")
		FDataTableRowHandle Shoes;
	UPROPERTY(EditAnywhere, Category = "Costume")
		TArray<FLinearColor> ShoesColors;
	// optional costume(accessory)
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle Handwear;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle Hat;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle Eyewear;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle Mask;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle Back;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle Tail;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle Ear;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle Upper;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle Lens;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		TArray<FLinearColor> LensColors;
	UPROPERTY(EditAnywhere, Category = "Accessory")
		FDataTableRowHandle MakeUp;

	// equipments
	UPROPERTY(EditAnywhere, Category = "Weapon")
		FDataTableRowHandle Sword;
	UPROPERTY(EditAnywhere, Category = "Weapon")
		FDataTableRowHandle Bow;

	TArray<FDataTableRowHandle*> GetRows();
	TArray<FLinearColor>* GetColors(EResourceModelType modelType);

	void SetDataTable(UDataTable* table);
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceMapPreview : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool AutoGenerated = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TMap<TSoftObjectPtr<class UTexture2D>, FText> MonsterIcons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TMap<TSoftObjectPtr<class UTexture2D>, FText> BossMonsterIcons;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API UAnuWorldServerList : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceWorldServerList : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 Order = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		EClientType type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText RegionCode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSoftObjectPtr<class UTexture2D> Image;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText Desc_1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText Desc_2;

	UPROPERTY(BlueprintReadOnly)
		int32 GUID = 0;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceLifeAction : public FAnuTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		FName ClassUID;
	UPROPERTY(EditAnywhere)
		EClassLicense LicenseType;
	UPROPERTY(EditAnywhere)
		FName Sequencer_Type;
	UPROPERTY(EditAnywhere, Category = "Mesh", meta = (AllowedClasses = "SkeletalMesh,StaticMesh"))
		FAnuMeshInfo ActionTool;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuAnimationAsset : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UAnimSequence> Asset;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceLevelDesign : public FAnuTableRow
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UObject> Asset;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UObject* LoadResource() override;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceBehaviorTree : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UObject> Asset;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UObject* LoadResource() override;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API UAnuOriginals : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuOriginalsPage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Thumbnail")
		EAnuImageType ThumbnailType = EAnuImageType::Snapshot;
	UPROPERTY(EditAnywhere, Category = "Thumbnail", meta = (EditCondition = "ThumbnailType == EAnuImageType::Snapshot"))
		FDataTableRowHandle ThumbnailSnapshot;
	UPROPERTY(EditAnywhere, Category = "Thumbnail", meta = (EditCondition = "ThumbnailType == EAnuImageType::Texture"))
		TSoftObjectPtr<UTexture2D> ThumbnailTexture;
	UPROPERTY(EditAnywhere, Category = "Thumbnail", meta = (EditCondition = "ThumbnailType == EAnuImageType::Gms"))
		FString ThumbnailUrl;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
		FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
		FText Story;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceOriginals : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		TArray<FAnuOriginalsPage> Pages;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceBellSound : public FAnuTableRow
{
	GENERATED_BODY()

public:
	/** this value will be saved in character db */
	UPROPERTY(EditAnywhere)
		int32 GUID = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSoftObjectPtr<USoundBase> Sound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSoftObjectPtr<class UTexture2D> Icon;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API UAnuProfileCard : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceProfileCard : public FAnuTableRow
{
	GENERATED_BODY()

	// card image
	UPROPERTY(EditAnywhere, Category = "CardImage")
		bool UseGMSCardImage{ true };
	UPROPERTY(EditAnywhere, Category = "CardImage", meta = (EditCondition = "!UseGMSCardImage"))
		TSoftObjectPtr<UTexture2D> CardIcon;
	UPROPERTY(EditAnywhere, Category = "CardImage", meta = (EditCondition = "UseGMSCardImage"))
		FString CardImageUrl;
	// circle image
	UPROPERTY(EditAnywhere, Category = "CircleImage")
		TSoftObjectPtr<UTexture2D> CircleImage;
	// text
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
		FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
		FText Crew;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
		FText Introduce;
	// social
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
		int64 FollowerCnt;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
		int64 Popularity;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social", meta = (ClampMin = 0, ClampMax = 100))
		float InterestMatchPercent;
};

UCLASS(BlueprintType)
class ANUREFERENCE_API UAnuNotice : public UObject
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuNoticeButton
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
		FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
		FLinearColor FontColor{ 0.021219f, 0.021219f, 0.021219f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
		FLinearColor BgColor{ 1.f, 1.f, 1.f };
	UPROPERTY(EditAnywhere, Category = "Style")
		FName IconUID;
	TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
		FLinearColor IconColor { 1.f, 1.f, 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runner")
		FName RunType;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runner")
		FString RunTypeValue;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceNotice : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		bool IsMainType;
	UPROPERTY(EditAnywhere)
		int32 Order;
	UPROPERTY(EditAnywhere)
		FString Schedule;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool ShowSchedule;
	UPROPERTY(EditAnywhere)
		TArray<FAnuNoticeButton> LinkButtons;
	UPROPERTY(BlueprintReadOnly)
		FString ImageUrl;

	void Serialize(TSharedPtr<FJsonObject> postMetaObj);
	bool Deserialize(TSharedPtr<FJsonObject> postMetaObj, FDateTime& startDate, FDateTime& endDate);
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceChatTag : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FText Text;
	UPROPERTY(EditAnywhere)
	FName EventKey;
	UPROPERTY(EditAnywhere)
	FString BuildFunName;
	UPROPERTY(EditAnywhere)
	EPermissionType Permission = EPermissionType::None;
};

UENUM(BlueprintType)
enum class EResourceGalleryType : uint8
{
	Like, Report,
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceGallery : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EResourceGalleryType Type;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Like", meta = (EditCondition = "Type == EResourceGalleryType::Like"))
	TSoftObjectPtr<UTexture2D> LikeTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Like", meta = (EditCondition = "Type == EResourceGalleryType::Like"))
	TSoftObjectPtr<UTexture2D> LikeOffTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Report", meta = (EditCondition = "Type == EResourceGalleryType::Report"))
	FText ReportReason;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FMeshTransformation : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<class USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<class UAnimInstance> Animator;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<UObject> Camera;
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UObject> Sequence;
	UPROPERTY(EditAnywhere)
	float TagWidgetHeight = 0.f;
	UPROPERTY(EditAnywhere)
	bool IsBlockControl;
	UPROPERTY(EditAnywhere)
	FString LayerEventWhenEnded;
	UPROPERTY(EditAnywhere)
	bool IsPlayOtherPC = true;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FArbeitNPCDetailRow : public FAnuTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString NPCImageURL;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor NPCRepresentColor;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor DecoratorColor;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceFlipBook : public FAnuTableRow
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UPaperFlipbook* Flipbook = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D ImageSize;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D Scale;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSlateColor Tint{FLinearColor::White};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool Autoplay{true};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool Loop{true};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PlayRate{ 1.0f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StartKeyFrame;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceChatSticker : public FAnuTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName GroupUID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<class UTexture2D> Image;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAnuResourceFlipBook FlipBook;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceSound : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
		TSoftObjectPtr<class USoundWave> Sound;

	virtual const FSoftObjectPath& GetRoute() const override;
	virtual FString GetPath() const override;
	virtual UObject* LoadResource() override;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceSignatureDesc : public FAnuTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine="true"))
	FText HeaderTitle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine="true"))
	FText HeaderDescription;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine="true"))
	FText BodyTitle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine="true"))
	FText BodyDescription;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuLocalPushInfo : public FAnuTableRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText PushStringTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText PushStringBody;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText PushStringAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TimeValue;
};


USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceClassLicense : public FAnuTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	EClassLicense Type;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText EngName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor Color_1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor Color_2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText TagName;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuInteractionNotifyEffect
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FName SocketName{ "Socket_R_Hand" };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FVector Offset{ 5.f, 0.f, 0.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowedClasses = "ParticleSystem"))
		TSoftObjectPtr<UObject> Particle;
};


USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuInteractionNotifyEffect_Schedule
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		FName Schedule;
	UPROPERTY(EditAnywhere)
		FAnuInteractionNotifyEffect Effect;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuResourceInteractionEffect : public FAnuTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CommonNotify")
		FAnuInteractionNotifyEffect NotifyEffect_Default;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CommonNotify")
		TArray<FAnuInteractionNotifyEffect_Schedule> NotifyEffect_Schedule;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requester")
		FName RequesterEffect_Default;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requester")
		TMap<FName, FName> RequesterEffect_Schedule;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accepter")
		FName AccepterEffect_Default;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accepter")
		TMap<FName, FName> AccepterEffect_Schedule;
};

USTRUCT(BlueprintType)
struct ANUREFERENCE_API FAnuDamageText : public FAnuTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FLinearColor FontColor;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSoftObjectPtr<UMaterialInterface> FontMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 FontSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FLinearColor OutlineColor;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float OutlineSize;	

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FLinearColor ShadowColor;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FVector2D ShadowOffset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float ShadowSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText TextValue;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FName AnimationName;
};