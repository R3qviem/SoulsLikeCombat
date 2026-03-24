// Copyright Epic Games, Inc. All Rights Reserved.

#include "BonfireCheckpoint.h"
#include "SoulsLikePlayerCharacter.h"
#include "SoulsLikePlayerController.h"
#include "InventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

ABonfireCheckpoint::ABonfireCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root scene
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Base mesh — simple cylinder as campfire base
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(Root);
	BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		BaseMesh->SetStaticMesh(CylinderFinder.Object);
		BaseMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.15f));
		BaseMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	}

	// Dark stone material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DarkMatFinder(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (DarkMatFinder.Succeeded())
	{
		BaseMesh->SetMaterial(0, DarkMatFinder.Object);
	}

	// Interaction sphere
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(Root);
	InteractionSphere->SetSphereRadius(200.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionSphere->SetGenerateOverlapEvents(true);

	// Fire VFX (Niagara) — using the stylized fire we already have
	FireVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireVFX"));
	FireVFX->SetupAttachment(Root);
	FireVFX->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	FireVFX->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.8f));

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FireFXFinder(
		TEXT("/Game/Vefects/Stylized_Fire/VFX/Pure/Particles/NS_Stylized_Fire_Radial_02_Pure.NS_Stylized_Fire_Radial_02_Pure"));
	if (FireFXFinder.Succeeded())
	{
		FireVFX->SetAsset(FireFXFinder.Object.Get());
	}

	// Warm fire light
	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(Root);
	FireLight->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	FireLight->SetLightColor(FLinearColor(1.0f, 0.6f, 0.15f));
	FireLight->SetIntensity(8000.0f);
	FireLight->SetAttenuationRadius(500.0f);
	FireLight->SetCastShadows(true);

	Tags.Add(FName("Bonfire"));
}

void ABonfireCheckpoint::BeginPlay()
{
	Super::BeginPlay();

	// Fire is always active (visual campfire in the world)
	if (FireVFX)
	{
		FireVFX->Activate(true);
	}
}

void ABonfireCheckpoint::Rest(ASoulsLikePlayerCharacter* Player)
{
	if (!Player)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Rest: Player is NULL!"));
		return;
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Rest: Healing player..."));

	bIsLit = true;

	// 1. Heal the player to full
	Player->CurrentHealth = Player->MaxHealth;
	Player->OnHealthChanged.Broadcast(Player->GetHealthPercent());

	// 2. Set this bonfire as the respawn point
	ASoulsLikePlayerController* PC = Cast<ASoulsLikePlayerController>(Player->GetController());
	if (PC)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Rest: Controller found! Setting respawn + respawning enemies"));

		FTransform SpawnTransform = GetActorTransform();
		// Offset slightly so the player doesn't spawn inside the bonfire
		SpawnTransform.SetLocation(GetActorLocation() + GetActorForwardVector() * 100.0f);
		PC->SetRespawnTransform(SpawnTransform);

		// 3. Respawn all enemies in the world
		PC->RespawnAllEnemies();

		// 4. Show level up menu
		PC->ShowLevelUpMenu();
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Rest: Controller cast FAILED!"));
	}
}
