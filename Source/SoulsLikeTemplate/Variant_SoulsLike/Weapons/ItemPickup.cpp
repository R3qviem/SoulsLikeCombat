// Copyright Epic Games, Inc. All Rights Reserved.

#include "ItemPickup.h"
#include "ItemDataAsset.h"
#include "InventoryComponent.h"
#include "SoulsLikePlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	// Interaction sphere (root)
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(120.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionSphere->SetGenerateOverlapEvents(true);
	RootComponent = InteractionSphere;

	// Small glowing sphere — no collision, purely visual
	OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
	OrbMesh->SetupAttachment(RootComponent);
	OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OrbMesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.08f));
	OrbMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		OrbMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Soft light to illuminate surroundings
	OrbLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OrbLight"));
	OrbLight->SetupAttachment(RootComponent);
	OrbLight->SetLightColor(FLinearColor(1.0f, 0.9f, 0.6f));
	OrbLight->SetIntensity(5000.0f);
	OrbLight->SetAttenuationRadius(200.0f);
	OrbLight->SetCastShadows(false);

	// Stylized fire VFX
	OrbVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OrbVFX"));
	OrbVFX->SetupAttachment(RootComponent);
	OrbVFX->SetAutoActivate(true);
	OrbVFX->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.3f));

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> VFXFinder(
		TEXT("/Game/Vefects/Stylized_Fire/VFX/Pure/Particles/NS_Stylized_Fire_Radial_02_Pure.NS_Stylized_Fire_Radial_02_Pure"));
	if (VFXFinder.Succeeded())
	{
		OrbVFX->SetAsset(VFXFinder.Object);
	}
}

void AItemPickup::BeginPlay()
{
	Super::BeginPlay();
	SpawnLocation = GetActorLocation();

	// Create a bright emissive unlit material so the sphere glows white
	if (OrbMesh)
	{
		UMaterial* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial"));
		if (BaseMat)
		{
			UMaterialInstanceDynamic* GlowMat = UMaterialInstanceDynamic::Create(BaseMat, this);
			// Set base color to very bright white — with bloom this creates a natural glow
			GlowMat->SetVectorParameterValue(FName("BaseColor"), FLinearColor(50.0f, 45.0f, 30.0f));
			OrbMesh->SetMaterial(0, GlowMat);
		}
	}
}

void AItemPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	BobTime += DeltaTime;

	// Gentle bobbing
	const float BobOffset = FMath::Sin(BobTime * 2.0f) * 10.0f;
	SetActorLocation(FVector(SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z + BobOffset));

	// Slow rotation
	OrbMesh->AddLocalRotation(FRotator(0.0f, DeltaTime * 45.0f, 0.0f));

	// Gentle pulsing light
	const float Pulse = 4000.0f + FMath::Sin(BobTime * 3.0f) * 2000.0f;
	OrbLight->SetIntensity(Pulse);
}

void AItemPickup::PickUp(ASoulsLikePlayerCharacter* Character)
{
	if (!Character || !ItemData)
	{
		return;
	}

	UInventoryComponent* Inventory = Character->InventoryComponent;
	if (!Inventory)
	{
		return;
	}

	if (Inventory->AddItem(ItemData, ItemCount))
	{
		Destroy();
	}
}
