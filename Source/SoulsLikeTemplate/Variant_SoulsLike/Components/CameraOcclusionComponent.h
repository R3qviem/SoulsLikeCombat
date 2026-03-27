// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraOcclusionComponent.generated.h"

class UPrimitiveComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Info about a mesh currently being faded due to camera occlusion.
 */
USTRUCT()
struct FOccludedMeshInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> Component = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FadeMaterials;

	bool bOriginalCastShadow = true;
	float CurrentOpacity = 1.0f;
	bool bIsOccluding = false;
};

/**
 * When a static mesh blocks the camera view of the player,
 * it fades to a low opacity so the player is always visible.
 * Simple whole-object transparency — no holes, no texture swap.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UCameraOcclusionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCameraOcclusionComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Target opacity when faded (0.0 = invisible, 0.15 = barely visible) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float FadedOpacity = 0.15f;

	/** How fast the fade happens */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = 1.0))
	float FadeSpeed = 6.0f;

	/** Trace radius for detecting blocking meshes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = 5.0, Units = "cm"))
	float TraceRadius = 50.0f;

	/** Object types to check */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
	TArray<TEnumAsByte<EObjectTypeQuery>> OcclusionObjectTypes;

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY()
	TMap<UPrimitiveComponent*, FOccludedMeshInfo> OccludedMeshes;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> FadeMaterial;

	void UpdateOccludingMeshes();
	void UpdateFading(float DeltaTime);
	void ApplyFadeMaterials(FOccludedMeshInfo& Info);
	void RestoreOriginalMaterials(FOccludedMeshInfo& Info);

#if WITH_EDITOR
	void EnsureFadeMaterialAsset();
#endif
};
