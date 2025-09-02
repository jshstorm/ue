// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "WorldLookupTarget.h"
#include "Reference_Interactor.generated.h"

using ArchiveReferences = TArray<UObject*>;
using CustomBuilder = TFunction<void(TArray<class FXmlAttribute>&, UObject*)>;
using CustomBuilders = TMap<FName, CustomBuilder>;

USTRUCT()
struct ANUREFERENCE_API FArchiveReferences
{
	GENERATED_USTRUCT_BODY()

	ArchiveReferences references;
	CustomBuilders propertyBuilders;
	CustomBuilders additionalBuilders;
};

UENUM(BlueprintType)
enum class ECharacterGrade : uint8
{
	None,
	Normal,
	Elite,
	Boss,
};

UCLASS(Blueprintable)
class ANUREFERENCE_API ARefInteractor : public AActor
{
	GENERATED_BODY()

public:
	ARefInteractor(const class FObjectInitializer& ObjectInitializer);

	UPROPERTY()
	FString UID;

	UPROPERTY(BlueprintReadOnly)
	int32 GUID = 0;

	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadWrite)
	FString SpawnObject_UID;

	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadOnly)
	FName Group {NAME_None};

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	float Spawn_Rotation;

	UPROPERTY(Category = "Property", VisibleAnywhere)
	FString Spawn_Type;

	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadWrite)
	FString Portal_Target_Point;

	/** if pc stopped in this range, server confirm that "pc is arrived" */
	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadWrite)
	float Arrive_Radius = 200.f;
	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadWrite)
	float Indicator_Offset = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadWrite)
	uint8 Show_MapIcon : 1;
	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadWrite)
	uint8 bFloating : 1;
	/** The option that indicate that restore any context that represents transform or etc... for owner when interaction was ended. */
	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadWrite)
	uint8 bRestoreContext : 1;	
	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadOnly)
	bool Exportable = true;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "EditorViewOnly")
	TSoftObjectPtr<USkeletalMesh> viewOnlyMesh;
	//UPROPERTY(EditAnywhere, Category = "EditorViewOnly")
	//TSoftClassPtr<UAnimInstance> viewOnlyAnim;
	UPROPERTY(EditAnywhere, Category = "EditorViewOnly")
	TSoftObjectPtr<UStaticMesh> viewOnlyStaticMesh;
#endif

public:
	//runtime
	class URefObject* _spawnObjectRef = nullptr;

	virtual void BeginPlay() override;
	virtual bool UsePreSpawn() const { return false; }

	FName GetSpawnObjectUID() const;
	void GetSpawnObjectUIDs(TSet<FName>& output) const;
	UAttachedInteractorComponent* GetAttachedInteractor() const;

	UObject* GetImplementObject() const { return _implementObj; };
	template <class T>
	T* GetImplementObject() const { return Cast<T>(GetImplementObject()); }

	bool IsPortal() const;
	virtual bool IsPatrolActor() const { return false; }

	virtual ECharacterGrade GetLevelGrade() { return ECharacterGrade::Normal; }
	virtual int32 GetOffenseLevel() { return 0; }
	virtual int32 GetDefenseLevel() { return 0; }

	virtual void SetImplementObject(UObject* object);

	UFUNCTION(BlueprintImplementableEvent)
		void ImplementObjectAssigned();
	UFUNCTION(BlueprintImplementableEvent)
		void ImplementObjectLeaved();
	UFUNCTION(BlueprintImplementableEvent)
		void VisibilityChanged(bool resultVisibility);
	UFUNCTION(BlueprintCallable)
		void GenerateCRC();
	UFUNCTION(BlueprintNativeEvent)
		float GetDesiredDistance() const;
	virtual float GetDesiredDistance_Implementation() const { return 0.f; }
	UFUNCTION(BlueprintPure)
		FName GetPinKey() const;
	UFUNCTION()
		void OnImplementObjectDetoryed(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	// debug
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void AddCustomBuilder(struct FArchiveReferences& archive);
	virtual bool EnableExport() const { return Exportable; }
	virtual bool CheckFK(FString& error, const TFunction<bool(ARefInteractor*)>& additionalChecker = nullptr);

	UFUNCTION(CallInEditor)
	void EnsureRootComponentScale();
	UFUNCTION(CallInEditor)
	void SetRootComponentScale();
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UObject* _implementObj;
};

UENUM(BlueprintType)
enum class ETeamTag : uint8
{
	Neutral,
	Monster,
	Team1, // PC
};

UCLASS(Blueprintable)
class ANUREFERENCE_API ARefMonInteractor : public ARefInteractor
{
	GENERATED_BODY()
public:
	ARefMonInteractor(const FObjectInitializer& ObjectInitializer);

	#pragma region TO BE BATTLE COMPONENT
public:
	UPROPERTY(EditAnywhere, Category = "Property|Mon", BlueprintReadOnly)
		int32 Level_Offense;
	UPROPERTY(EditAnywhere, Category = "Property|Mon", BlueprintReadOnly)
		int32 Level_Defense;
	UPROPERTY(EditAnywhere, Category = "Property|Mon", BlueprintReadOnly)
		ETeamTag Team = ETeamTag::Monster;
	UPROPERTY(EditAnywhere, Category = "Property|Mon", BlueprintReadOnly)
		ECharacterGrade Grade = ECharacterGrade::None;
	UPROPERTY(EditAnywhere, Category = "Property|Mon", BlueprintReadOnly)
		float Search_Distance;
	UPROPERTY(EditAnywhere, Category = "Property|Mon", BlueprintReadOnly)
		bool LookAt = true;

	virtual void BeginPlay() override;
	virtual void SetImplementObject(UObject* object) override;

	virtual ECharacterGrade GetLevelGrade() override { return Grade; }
	virtual int32 GetOffenseLevel() override;
	virtual int32 GetDefenseLevel() override;
	float GetSearchDistSquared() { return Search_Distance * Search_Distance; }

	#pragma endregion TO BE BATTLE COMPONENT

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void AddCustomBuilder(FArchiveReferences& archive) override;
	virtual bool CheckFK(FString& error, const TFunction<bool(ARefInteractor*)>& additionalChecker) override;
#endif
};

UCLASS(Blueprintable)
class ANUREFERENCE_API ARefGimmickInteractor : public ARefInteractor
{
	GENERATED_BODY()
public:
	ARefGimmickInteractor(const FObjectInitializer& ObjectInitializer);
};

#pragma region Patrol
UENUM(BlueprintType)
enum class EPatrol : uint8 {
	None, /*Once, */PingPong, Forward/*, Random*/,
};

UENUM(BlueprintType)
enum class EPatrolStaySec : uint8 {
	StopOver = 0,
	Stay_15 = 15,
	Stay_30 = 30,
	Stay_60 = 60,
	Stay_120 = 120
};

UCLASS(Blueprintable)
class ANUREFERENCE_API ARefNPCInteractor : public ARefInteractor
{
	GENERATED_BODY()
public:
	ARefNPCInteractor(const FObjectInitializer& ObjectInitializer);

public:
	UPROPERTY(EditAnywhere, Category = "Property", BlueprintReadOnly)
		bool PreSpawnd = true;
	UPROPERTY(EditAnywhere, Category = "Property|Patrol", BlueprintReadOnly)
		EPatrol Type = EPatrol::None;
	UPROPERTY(EditAnywhere, Category = "Property|Patrol", BlueprintReadOnly)
		EPatrolStaySec startSec = EPatrolStaySec::StopOver;
	UPROPERTY(EditAnywhere, Category = "Property|Patrol", BlueprintReadOnly)
		TMap<ATargetPoint*, EPatrolStaySec> Points;
	UPROPERTY(EditAnywhere, Category = "Property|Dependency", BlueprintReadOnly)
		FName Leader_Spawner = NAME_None;

		//FOR BATTLE NPC
	UPROPERTY(EditAnywhere, Category = "Property|BattleNPC", BlueprintReadOnly)
		ETeamTag Team = ETeamTag::Team1;
	UPROPERTY(EditAnywhere, Category = "Property|BattleNPC", BlueprintReadOnly)
		int32 Level_Offense;
	UPROPERTY(EditAnywhere, Category = "Property|BattleNPC", BlueprintReadOnly)
		int32 Level_Defense;
	UPROPERTY(EditAnywhere, Category = "Property|BattleNPC", BlueprintReadOnly)
		ECharacterGrade Grade = ECharacterGrade::Normal;

	virtual int32 GetOffenseLevel() override;
	virtual int32 GetDefenseLevel() override;
		//FOR BATTLE NPC
	virtual void BeginPlay() override;


	virtual bool UsePreSpawn() const override;
	virtual bool IsPatrolActor() const override;

public:
	// runtime
	UPROPERTY()
	ARefNPCInteractor* _parentInteractor = nullptr;

#if WITH_EDITOR
	virtual bool EnableExport() const override;
#endif
};

UCLASS()
class ANUREFERENCE_API UAttachedInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttachedInteractorComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName Object_UID = NAME_None;
	UPROPERTY(EditAnywhere)
		FString Group = "None";
	UPROPERTY(VisibleAnywhere)
		FString Spawn_Type = "None";

#if WITH_EDITOR
	static void AddCustomBuilder(FArchiveReferences& archive);
#endif
};

UINTERFACE(BlueprintType)
class ANUREFERENCE_API UPinIndexer : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};
inline UPinIndexer::UPinIndexer(const FObjectInitializer& ObjectInitializer) {}
class ANUREFERENCE_API IPinIndexer
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
		void RegisterPin(UPinComponent* pin);
	UFUNCTION(BlueprintNativeEvent)
		void UnregisterPin(UPinComponent* pin);
};

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ANUREFERENCE_API UPinComponent : public USceneComponent, public IWorldLookupTarget
{
	GENERATED_BODY()

public:
	inline static UObject* IndexManager = nullptr;
	inline static FName NAME_Pin{ "Pin" };

public:
	UPinComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FName Pin;
	UPROPERTY(EditAnywhere)
		bool IsMainPin = false;
	/** if this value is true, character on this pin will be adjusted on ground */
	UPROPERTY(EditAnywhere)
		bool AdjustOnGround = true;

	// world lookup target
	virtual FName GetLookupType_Implementation() override;
	virtual FName GetLookupUID_Implementation() override;

#if WITH_EDITOR
	static void AddCustomBuilder(FArchiveReferences& archive);
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
#endif

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITORONLY_DATA
	UArrowComponent* _direction = nullptr;
#endif
};

UCLASS(Blueprintable)
class ANUREFERENCE_API ARefClientInteractor : public ARefInteractor
{
	GENERATED_BODY()
public:
	ARefClientInteractor(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual bool EnableExport() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

UENUM(BlueprintType)
enum class EScheduleActorType : uint8 
{
	LifeObject,
	Streaming,
};

UCLASS(Blueprintable)
class ANUREFERENCE_API ARefScheduleActorInteractor : public ARefInteractor
{
	GENERATED_BODY()
public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FScheduleActorEndPlay, ARefScheduleActorInteractor*);
	inline static FString String_ScheduleActor{"ScheduleActor"};

	static bool IsScheduleActor(ARefInteractor* spawner);

public:
	ARefScheduleActorInteractor(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetActorHiddenInGame(bool bNewHidden) override;

	UPROPERTY(EditAnywhere, Category = "Property")
		FName Schedule;
	UPROPERTY(EditAnywhere, Category = "Property")
		EScheduleActorType Actor_Type;
	UPROPERTY(EditAnywhere, Category = "Property")
		int32 Interaction_Max_Count = 10;
	UPROPERTY(EditAnywhere, Category = "Property")
		TArray<FString> Stat_Buff;
	UPROPERTY(EditAnywhere, Category = "Property")
		FName Challenge_UID;
	UPROPERTY(EditAnywhere, Category = "Property")
		FText ActorName;
	// runtime
	uint32 _remainCount = 0;
	FScheduleActorEndPlay OnEndPlay;

#if WITH_EDITOR
	virtual void AddCustomBuilder(FArchiveReferences& archive) override;
#endif

protected:
	void UpdateVisibility();

	ECollisionEnabled::Type _originCollision;
};