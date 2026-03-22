// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeaponBase.h"
#include "WeaponDataAsset.h"
#include "Components/StaticMeshComponent.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create the weapon mesh component as root
	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMeshComponent;

	// Disable collision on the weapon mesh (hit detection is done via traces, not physics)
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);
}

void AWeaponBase::AttachToCharacter(USkeletalMeshComponent* TargetMesh, FName SocketName)
{
	if (!TargetMesh)
	{
		return;
	}

	// Attach to the specified socket
	AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);

	// Apply the mesh offset from the data asset
	if (WeaponData)
	{
		SetActorRelativeTransform(WeaponData->MeshOffset);
	}
}

void AWeaponBase::DetachFromCharacter()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}
