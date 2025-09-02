// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "Reference_Interactor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "DrawDebugHelpers.h"
#include "KeyGenerator.h"
#include "XmlNode.h"
#include "LogAnuReference.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Particles/ParticleSystemComponent.h"

ARefInteractor::ARefInteractor(const class FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
	, Show_MapIcon(true)
	, bFloating(false)
	, bRestoreContext(true)
{
}

void ARefInteractor::BeginPlay()
{
	Super::BeginPlay();

	if (auto component = FindComponentByClass<USkeletalMeshComponent>()) {
		component->DestroyComponent();
	}
}

UAttachedInteractorComponent* ARefInteractor::GetAttachedInteractor() const
{
	auto component = FindComponentByClass<UAttachedInteractorComponent>();
	return component;
}

FName ARefInteractor::GetPinKey() const
{
	TArray<UActorComponent*> pins;
	GetComponents(UPinComponent::StaticClass(), pins);
	if (pins.Num() == 0) {
		return NAME_None;
	}

	for (auto& it : pins) {
		UPinComponent* pin = static_cast<UPinComponent*>(it);
		if (pin->IsMainPin) {
			return pin->Pin;
		}
	}
	return static_cast<UPinComponent*>(pins[0])->Pin;
}

void ARefInteractor::OnImplementObjectDetoryed(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	SetImplementObject(nullptr);
}

bool ARefInteractor::IsPortal() const
{
	return Portal_Target_Point.IsEmpty() == false;
}

void ARefInteractor::SetImplementObject(UObject* object)
{
	checkf(object == nullptr || _implementObj == nullptr, TEXT("spawner[%s] implement object assigned duplicately"), *GetName());

	_implementObj = object;

	if (_implementObj) {
		ImplementObjectAssigned();
	}
	else {
		ImplementObjectLeaved();
	}

	if (AActor* actor = Cast<AActor>(object)) {
		actor->OnEndPlay.AddDynamic(this, &ARefInteractor::OnImplementObjectDetoryed);
	}
}

FName ARefInteractor::GetSpawnObjectUID() const
{
	if (UAttachedInteractorComponent* attachedInteractor = GetAttachedInteractor()) {
		return attachedInteractor->Object_UID;
	}
 	return *SpawnObject_UID;
}

void ARefInteractor::GetSpawnObjectUIDs(TSet<FName>& output) const
{
	output.Emplace(*SpawnObject_UID);

	auto attachedInteractors = GetComponentsAs<UAttachedInteractorComponent>();
	for (auto& attached : attachedInteractors) {
		output.Emplace(attached->Object_UID);
	}
}

void ARefInteractor::GenerateCRC()
{
	FString actorName;
#if WITH_EDITOR
	actorName = GetActorLabel();
#endif

	if (UID == actorName) {
		return;
	}

	UID = TCHAR_TO_UTF8(*actorName);
	check(UID.IsEmpty() == false);

	GUID = UCRC32::GetPtr()->Generate32(*UID);
}

#if WITH_EDITOR
void ARefInteractor::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent);

	if (GetWorld() == nullptr || GetWorld()->WorldType != EWorldType::Editor) {
		return;
	}

	FProperty* memberPropertyThatChanged = propertyChangedEvent.MemberProperty;
	if (memberPropertyThatChanged == nullptr) {
		return;
	}

	static FName NAME_AUTO_MOVE_DISTANCE{ "AutoMoveDistance" };
	static TMap<FName, FColor> DebugSphereColors;
	if (DebugSphereColors.Num() == 0) {
		DebugSphereColors.Emplace("Arrive_Radius", FColor::Cyan);
		DebugSphereColors.Emplace(NAME_AUTO_MOVE_DISTANCE, FColor::Yellow);
	}

	const FName memberPropertyName = memberPropertyThatChanged != NULL ? memberPropertyThatChanged->GetFName() : NAME_None;
	auto colorIter = DebugSphereColors.Find(memberPropertyName);
	if (colorIter == nullptr) {
		return;
	}

	void* memberPropPtr = memberPropertyThatChanged->ContainerPtrToValuePtr<void>(this);
	check(memberPropPtr != nullptr);

	FFloatProperty* floatProperty = static_cast<FFloatProperty*>(memberPropertyThatChanged);
	float value = floatProperty->GetPropertyValue(memberPropPtr);

	if (memberPropertyName == NAME_AUTO_MOVE_DISTANCE) {
		if (UStaticMeshComponent* meshComponent = Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass()))) {
			UCapsuleComponent* interactionCollision = Cast<UCapsuleComponent>(GetComponentByClass(UCapsuleComponent::StaticClass()));
			check(interactionCollision);
			DrawDebugSphere(GetWorld(), meshComponent->GetComponentTransform().GetTranslation(), interactionCollision->GetScaledCapsuleRadius() + value, 12, *colorIter, false, -1.f);
			return;
		}
	}

	DrawDebugSphere(GetWorld(), GetActorLocation(), value, 12, *colorIter, false, 1.f);
}

bool ARefInteractor::CheckFK(FString& error, const TFunction<bool(ARefInteractor*)>& additionalChecker /*= nullptr*/)
{
	if (additionalChecker != nullptr && additionalChecker(this) == false) {
		return false;
	}

	if (Spawn_Type != "SpawnPoint" && SpawnObject_UID.IsEmpty()) {
		error = FString::Printf(TEXT("[exporter] spawner[%s] SpawnObjectUID is empty"), *GetName());
		return false;
	}

	return true;
}

void ARefInteractor::EnsureRootComponentScale()
{
	TArray<AActor*> actors;
	UGameplayStatics::GetAllActorsOfClass(this, StaticClass(), actors);

	int32 errors = 0;
	for (auto& actor : actors) {
		auto interactor = Cast<ARefInteractor>(actor);
		USceneComponent* root = interactor->GetRootComponent();
		if (!root->GetComponentScale().Equals(FVector::OneVector)) {
			UE_LOG(LogAnuReference, Error, TEXT("Object that component scale is not one has detected.\nObjectName: %s, Scale: %s, SpawnUID: %s"), *interactor->GetName(), *root->GetComponentScale().ToString(), *interactor->SpawnObject_UID);
			++errors;
		}
	}

	if (errors == 0) {
		UE_LOG(LogAnuReference, Verbose, TEXT("%d objects found, 0 errors."), actors.Num());
	}
	else {
		UE_LOG(LogAnuReference, Warning, TEXT("%d objects found, %d errors."), actors.Num(), errors);
	}
}

void ARefInteractor::SetRootComponentScale()
{
	FVector scale = RootComponent->GetComponentScale();
	if (scale.Equals(FVector::OneVector)) {
		UE_LOG(LogAnuReference, Verbose, TEXT("The component scale of object '%s' is nearly one! Validate is not needed."), *GetName());
		return;
	}

	RootComponent->SetRelativeScale3D(FVector::OneVector);

	TArray<USceneComponent*> childs;
	RootComponent->GetChildrenComponents(false, childs);

	for (auto& child : childs) {
		FVector relativeLocation = child->GetRelativeLocation();
		FVector relativeScale = child->GetRelativeScale3D();

		child->SetRelativeLocation(relativeLocation * scale);
		child->SetRelativeScale3D(relativeScale * scale);
	}

	UE_LOG(LogAnuReference, Verbose, TEXT("Validation completed for '%s' object! Scale changed from %s to %s."), *GetName(), *scale.ToString(), *FVector::OneVector.ToString());
}

void ARefInteractor::AddCustomBuilder(struct FArchiveReferences& archive)
{
	archive.propertyBuilders.Add("Location", [](TArray<FXmlAttribute>& attrs, UObject* actor) {
		ARefInteractor* spawner = CastChecked<ARefInteractor>(actor);
		const FVector& pos = spawner->GetActorTransform().GetLocation();
		attrs.Add(FXmlAttribute(TEXT("LocationX"), FString::SanitizeFloat(pos.X)));
		attrs.Add(FXmlAttribute(TEXT("LocationY"), FString::SanitizeFloat(pos.Y)));
		attrs.Add(FXmlAttribute(TEXT("LocationZ"), FString::SanitizeFloat(pos.Z)));
	});

	UAttachedInteractorComponent::AddCustomBuilder(archive);
	UPinComponent::AddCustomBuilder(archive);
}
#endif

/////////////////////////////////////////////////////////////////
ARefMonInteractor::ARefMonInteractor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

void ARefMonInteractor::BeginPlay()
{
	Super::BeginPlay();
}

void ARefMonInteractor::SetImplementObject(UObject* object)
{
	// not supported
}

int32 ARefMonInteractor::GetOffenseLevel()
{
	return Level_Offense;
}

int32 ARefMonInteractor::GetDefenseLevel()
{
	return Level_Defense;
}

#if WITH_EDITOR
void ARefMonInteractor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName propertyName = PropertyChangedEvent.GetPropertyName();
	if (propertyName.IsEqual("Search_Distance")) {
		static FName MeshTag("SearchDist");
		const auto& staticMesh = GetComponentsByTag(USphereComponent::StaticClass(), MeshTag);
		if (staticMesh.IsValidIndex(0)) {
			if (USphereComponent* component = Cast<USphereComponent>(staticMesh[0])) {
				component->SetSphereRadius(Search_Distance);
			}
		}
	}
}

void ARefMonInteractor::AddCustomBuilder(FArchiveReferences& archive)
{
	ARefInteractor::AddCustomBuilder(archive);

	archive.propertyBuilders.Add("Grade", [](TArray<FXmlAttribute>& attrs, UObject* actor) {
		if (auto spawner = CastChecked<ARefMonInteractor>(actor)) {
			auto EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECharacterGrade"), true);
			attrs.Add(FXmlAttribute(TEXT("Grade"), EnumPtr->GetNameStringByIndex((int32)spawner->Grade)));
		}
	});

	archive.propertyBuilders.Add("Team", [](TArray<FXmlAttribute>& attrs, UObject* actor) {
		if (auto spawner = CastChecked<ARefMonInteractor>(actor)) {
			auto EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETeamTag"), true);
			attrs.Add(FXmlAttribute(TEXT("Team"), EnumPtr->GetNameStringByIndex((int32)spawner->Team)));
		}
	});
}

bool ARefMonInteractor::CheckFK(FString& error, const TFunction<bool(ARefInteractor*)>& additionalChecker)
{
	if (ARefInteractor::CheckFK(error, additionalChecker) == false) {
		return false;
	}

	if (Grade == ECharacterGrade::None) {
		error = FString::Printf(TEXT("[exporter] mon spawner[%s] has no grade"), *GetName());
		return false;
	}

	return true;
}
#endif
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
ARefGimmickInteractor::ARefGimmickInteractor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Spawn_Type = "Gimmick";
}


/////////////////////////////////////////////////////////////////
ARefNPCInteractor::ARefNPCInteractor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

int32 ARefNPCInteractor::GetOffenseLevel()
{
	return Level_Offense;
}

int32 ARefNPCInteractor::GetDefenseLevel()
{
	return Level_Defense;
}

void ARefNPCInteractor::BeginPlay()
{
	Super::BeginPlay();
}

bool ARefNPCInteractor::UsePreSpawn() const
{ 
	return PreSpawnd && Leader_Spawner.IsNone();
}

bool ARefNPCInteractor::IsPatrolActor() const
{
	if (_parentInteractor) {
		return _parentInteractor->IsPatrolActor();
	}

	if (Type == EPatrol::None) {
		return false;
	}

	checkf(Points.Num() > 0, TEXT("check point element actorname : %s"), *GetName());
	return Points.Num() > 0;
}

#if WITH_EDITOR
bool ARefNPCInteractor::EnableExport() const
{
	if (ARefInteractor::EnableExport() == false) {
		return false;
	}

	if (Leader_Spawner != NAME_None) {
		return false;
	}
	return true;
}
#endif
/////////////////////////////////////////////////////////////////

UAttachedInteractorComponent::UAttachedInteractorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void UAttachedInteractorComponent::AddCustomBuilder(FArchiveReferences& archive)
{
	// ObjectUID
	archive.additionalBuilders.Add("AttachedObject", [](TArray<FXmlAttribute>& attrs, UObject* actor) {
		ARefInteractor* interactor = Cast<ARefInteractor>(actor);
		if (interactor == nullptr) {
			return;
		}

		FString value;

		auto components = interactor->GetComponentsAs<UAttachedInteractorComponent>();
		for (auto component : components) {
			if (value.IsEmpty() == false) {
				value += '|';
			}
			value += component->Object_UID.ToString();
		}

		attrs.Add(FXmlAttribute(TEXT("AttachedObject"), *value));
	});

	// ObjectGroup
	archive.additionalBuilders.Add("AttachedObjectGroup", [](TArray<FXmlAttribute>& attrs, UObject* actor) {
		ARefInteractor* interactor = Cast<ARefInteractor>(actor);
		if (interactor == nullptr) {
			return;
		}

		FString value;

		auto components = interactor->GetComponentsAs<UAttachedInteractorComponent>();
		for (auto component : components) {
			if (value.IsEmpty() == false) {
				value += '|';
			}
			value += component->Group;
		}

		attrs.Add(FXmlAttribute(TEXT("AttachedObjectGroup"), *value));
	});

	// ObjectType
	archive.additionalBuilders.Add("AttachedObjectType", [](TArray<FXmlAttribute>& attrs, UObject* actor) {
		ARefInteractor* interactor = Cast<ARefInteractor>(actor);
		if (interactor == nullptr) {
			return;
		}

		FString value;

		auto components = interactor->GetComponentsAs<UAttachedInteractorComponent>();
		for (auto component : components) {
			if (value.IsEmpty() == false) {
				value += '|';
			}
			value += component->Spawn_Type;
		}

		attrs.Add(FXmlAttribute(TEXT("AttachedObjectType"), *value));
	});
}
#endif

UPinComponent::UPinComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPinComponent::OnRegister()
{
#if WITH_EDITORONLY_DATA
	if(_direction == nullptr) {
		_direction = NewObject<UArrowComponent>(this, NAME_None);
		_direction->SetIsVisualizationComponent(true);
		_direction->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
#endif
	Super::OnRegister();
}

void UPinComponent::BeginPlay()
{
	Super::BeginPlay();
	if (IndexManager) {
		IPinIndexer::Execute_RegisterPin(IndexManager, this);
	}
}

void UPinComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IndexManager) {
		IPinIndexer::Execute_UnregisterPin(IndexManager, this);
	}
	Super::EndPlay(EndPlayReason);
}

FName UPinComponent::GetLookupType_Implementation()
{
	return NAME_Pin;
}

FName UPinComponent::GetLookupUID_Implementation()
{
	return Pin;
}

#if WITH_EDITOR
void UPinComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);

#if WITH_EDITORONLY_DATA
	if (_direction) {
		_direction->DestroyComponent();
	}
#endif
}

void UPinComponent::AddCustomBuilder(FArchiveReferences& archive)
{
	archive.additionalBuilders.Add("Pin", [](TArray<FXmlAttribute>& attrs, UObject* actor) {
		ARefInteractor* interactor = Cast<ARefInteractor>(actor);
		if (interactor == nullptr) {
			return;
		}

		FName pinKey{interactor->GetPinKey()};
		attrs.Add(FXmlAttribute(TEXT("Spawn_Pin_Key"), pinKey.IsNone() ? "" : pinKey.ToString()));
	});
}
#endif

ARefClientInteractor::ARefClientInteractor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
bool ARefClientInteractor::EnableExport() const
{
	return false;
}

void ARefClientInteractor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	TArray<UActorComponent*> pins;
	GetComponents(UPinComponent::StaticClass(), pins);
	for (auto& it : pins) {
		UPinComponent* pin = static_cast<UPinComponent*>(it);
		pin->IsMainPin = false;
	}
}
#endif

bool ARefScheduleActorInteractor::IsScheduleActor(ARefInteractor* spawner)
{
	return spawner && spawner->Spawn_Type == ARefScheduleActorInteractor::String_ScheduleActor;
}

ARefScheduleActorInteractor::ARefScheduleActorInteractor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Spawn_Type = String_ScheduleActor;
}

void ARefScheduleActorInteractor::BeginPlay()
{
	Super::BeginPlay();
	if (auto sm = FindComponentByClass<UStaticMeshComponent>()) {
		_originCollision = sm->GetCollisionEnabled();
	}
	UpdateVisibility();
}

void ARefScheduleActorInteractor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	OnEndPlay.Broadcast(this);
}

void ARefScheduleActorInteractor::SetActorHiddenInGame(bool bNewHidden)
{
	bool prevHidden = IsActorHidden();
	Super::SetActorHiddenInGame(bNewHidden);

	if (prevHidden && !bNewHidden) { // change to be visible
		const auto& particles = GetComponentsAs<UParticleSystemComponent>();
		for (auto& particle : particles) {
			particle->Activate(true);
		}
	}
	else if (!prevHidden && bNewHidden) { // change to be invisible
		const auto& particles = GetComponentsAs<UParticleSystemComponent>();
		for (auto& particle : particles) {
			particle->Activate(false);
		}
	}

	UpdateVisibility();
}

void ARefScheduleActorInteractor::UpdateVisibility()
{
	if (auto sm = FindComponentByClass<UStaticMeshComponent>()) {
		sm->SetCollisionEnabled(IsHidden() ? ECollisionEnabled::Type::NoCollision : _originCollision);
	}
}

#if WITH_EDITOR
void ARefScheduleActorInteractor::AddCustomBuilder(FArchiveReferences& archive)
{
	Super::AddCustomBuilder(archive);

	archive.propertyBuilders.Add("Actor_Type", [](TArray<FXmlAttribute>& attrs, UObject* actor) {
		if (ARefScheduleActorInteractor* spawner = CastChecked<ARefScheduleActorInteractor>(actor)) {
			const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EScheduleActorType"), true);
			attrs.Add(FXmlAttribute(TEXT("Actor_Type"), EnumPtr->GetNameStringByIndex((int32)spawner->Actor_Type)));
		}
	});

	archive.propertyBuilders.Add("Stat_Buff", [](TArray<FXmlAttribute>& attrs, UObject* actor) {
		if (ARefScheduleActorInteractor* spawner = CastChecked<ARefScheduleActorInteractor>(actor)) {
			for(int32 i = 0; i < spawner->Stat_Buff.Num(); i++) {
				attrs.Add(FXmlAttribute(FString::Printf(TEXT("Stat_Buff_%d"), i + 1), spawner->Stat_Buff[i]));
			}
		}
	});
}
#endif