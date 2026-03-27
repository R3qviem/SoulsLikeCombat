// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameraOcclusionComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

#if WITH_EDITOR
#include "Materials/Material.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"
#endif

UCameraOcclusionComponent::UCameraOcclusionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	OcclusionObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FadeMaterialFinder(
		TEXT("/Game/SoulsLike/Materials/M_OcclusionFade.M_OcclusionFade"));
	if (FadeMaterialFinder.Succeeded())
	{
		FadeMaterial = FadeMaterialFinder.Object;
	}
}

void UCameraOcclusionComponent::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	if (!FadeMaterial)
	{
		EnsureFadeMaterialAsset();
	}
#endif
}

void UCameraOcclusionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateOccludingMeshes();
	UpdateFading(DeltaTime);
}

void UCameraOcclusionComponent::UpdateOccludingMeshes()
{
	for (auto& Pair : OccludedMeshes)
	{
		Pair.Value.bIsOccluding = false;
	}

	APlayerCameraManager* CamManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	AActor* Owner = GetOwner();
	if (!CamManager || !Owner) return;

	FCollisionObjectQueryParams ObjectParams;
	for (const auto& ObjType : OcclusionObjectTypes)
	{
		ObjectParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(ObjType));
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	QueryParams.bTraceComplex = false;

	FCollisionShape SphereShape;
	SphereShape.SetSphere(TraceRadius);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits,
		CamManager->GetCameraLocation(), Owner->GetActorLocation(),
		FQuat::Identity, ObjectParams, SphereShape, QueryParams);

	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* HitComp = Hit.GetComponent();
		if (!HitComp || HitComp->Mobility == EComponentMobility::Movable)
		{
			continue;
		}

		if (FOccludedMeshInfo* Existing = OccludedMeshes.Find(HitComp))
		{
			Existing->bIsOccluding = true;
		}
		else
		{
			FOccludedMeshInfo NewInfo;
			NewInfo.Component = HitComp;
			NewInfo.bIsOccluding = true;
			NewInfo.CurrentOpacity = 1.0f;
			NewInfo.bOriginalCastShadow = HitComp->CastShadow;

			for (int32 i = 0; i < HitComp->GetNumMaterials(); ++i)
			{
				NewInfo.OriginalMaterials.Add(HitComp->GetMaterial(i));
			}

			ApplyFadeMaterials(NewInfo);
			OccludedMeshes.Add(HitComp, MoveTemp(NewInfo));
		}
	}
}

void UCameraOcclusionComponent::UpdateFading(float DeltaTime)
{
	TArray<UPrimitiveComponent*> ToRemove;

	for (auto& Pair : OccludedMeshes)
	{
		FOccludedMeshInfo& Info = Pair.Value;

		if (!Info.Component || !IsValid(Info.Component))
		{
			ToRemove.Add(Pair.Key);
			continue;
		}

		const float TargetOpacity = Info.bIsOccluding ? FadedOpacity : 1.0f;
		Info.CurrentOpacity = FMath::FInterpTo(Info.CurrentOpacity, TargetOpacity, DeltaTime, FadeSpeed);

		if (!Info.bIsOccluding && Info.CurrentOpacity > 0.99f)
		{
			RestoreOriginalMaterials(Info);
			ToRemove.Add(Pair.Key);
			continue;
		}

		// Update opacity on all MIDs
		for (UMaterialInstanceDynamic* MID : Info.FadeMaterials)
		{
			if (MID)
			{
				MID->SetScalarParameterValue(FName("Opacity"), Info.CurrentOpacity);
			}
		}

		// Disable shadows when heavily faded
		Info.Component->SetCastShadow(Info.CurrentOpacity > 0.5f);
	}

	for (UPrimitiveComponent* Key : ToRemove)
	{
		OccludedMeshes.Remove(Key);
	}
}

void UCameraOcclusionComponent::ApplyFadeMaterials(FOccludedMeshInfo& Info)
{
	if (!Info.Component || !FadeMaterial) return;

	const int32 NumMaterials = Info.Component->GetNumMaterials();
	Info.FadeMaterials.SetNum(NumMaterials);

	for (int32 i = 0; i < NumMaterials; ++i)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(FadeMaterial, this);
		if (!MID) continue;

		MID->SetScalarParameterValue(FName("Opacity"), Info.CurrentOpacity);

		// Try to match original material color
		UMaterialInterface* OrigMat = Info.OriginalMaterials.IsValidIndex(i)
			? Info.OriginalMaterials[i].Get() : nullptr;
		if (OrigMat)
		{
			FLinearColor BaseColor = FLinearColor(0.5f, 0.5f, 0.5f);
			static const FName ColorNames[] = {
				FName("BaseColor"), FName("Base Color"), FName("Color"),
				FName("Tint"), FName("DiffuseColor"),
			};
			for (const FName& Name : ColorNames)
			{
				if (OrigMat->GetVectorParameterValue(FHashedMaterialParameterInfo(Name), BaseColor))
				{
					break;
				}
			}
			MID->SetVectorParameterValue(FName("BaseColor"), BaseColor);
		}

		Info.FadeMaterials[i] = MID;
		Info.Component->SetMaterial(i, MID);
	}
}

void UCameraOcclusionComponent::RestoreOriginalMaterials(FOccludedMeshInfo& Info)
{
	if (!Info.Component || !IsValid(Info.Component)) return;

	for (int32 i = 0; i < Info.OriginalMaterials.Num(); ++i)
	{
		if (Info.OriginalMaterials[i])
		{
			Info.Component->SetMaterial(i, Info.OriginalMaterials[i]);
		}
	}

	Info.Component->SetCastShadow(Info.bOriginalCastShadow);
	Info.FadeMaterials.Empty();
}

// =============================================================================
// Simple translucent material: BaseColor * Opacity
// =============================================================================

#if WITH_EDITOR
void UCameraOcclusionComponent::EnsureFadeMaterialAsset()
{
	const FString FullPath = TEXT("/Game/SoulsLike/Materials/M_OcclusionFade");

	FadeMaterial = LoadObject<UMaterialInterface>(nullptr, *FullPath);
	if (FadeMaterial) return;

	UE_LOG(LogTemp, Log, TEXT("CameraOcclusionComponent: Creating M_OcclusionFade..."));

	UPackage* Package = CreatePackage(*FullPath);
	Package->FullyLoad();

	UMaterial* Mat = NewObject<UMaterial>(Package, TEXT("M_OcclusionFade"), RF_Public | RF_Standalone);
	Mat->BlendMode = BLEND_Translucent;
	Mat->SetShadingModel(MSM_DefaultLit);
	Mat->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;

	UMaterialEditorOnlyData* EditorData = Mat->GetEditorOnlyData();
	if (!EditorData) return;

	// --- BaseColor parameter (try to match original object color) ---
	UMaterialExpressionVectorParameter* ColorParam =
		NewObject<UMaterialExpressionVectorParameter>(Mat);
	ColorParam->ParameterName = FName("BaseColor");
	ColorParam->DefaultValue = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	Mat->GetExpressionCollection().AddExpression(ColorParam);

	EditorData->BaseColor.Expression = ColorParam;
	EditorData->BaseColor.OutputIndex = 0;

	// --- Opacity parameter ---
	UMaterialExpressionScalarParameter* OpacityParam =
		NewObject<UMaterialExpressionScalarParameter>(Mat);
	OpacityParam->ParameterName = FName("Opacity");
	OpacityParam->DefaultValue = 0.15f;
	Mat->GetExpressionCollection().AddExpression(OpacityParam);

	EditorData->Opacity.Expression = OpacityParam;
	EditorData->Opacity.OutputIndex = 0;

	// Compile and save
	Mat->PreEditChange(nullptr);
	Mat->PostEditChange();

	const FString FilePath = FPackageName::LongPackageNameToFilename(
		FullPath, FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Mat, *FilePath, SaveArgs);

	FadeMaterial = Mat;
	UE_LOG(LogTemp, Log, TEXT("CameraOcclusionComponent: M_OcclusionFade created."));
}
#endif
