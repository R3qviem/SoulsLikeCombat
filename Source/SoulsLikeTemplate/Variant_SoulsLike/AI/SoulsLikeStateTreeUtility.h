// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"
#include "SoulsLikeTypes.h"
#include "SoulsLikeStateTreeUtility.generated.h"

class ACharacter;
class AAIController;
class ASoulsLikeBaseEnemy;

////////////////////////////////////////////////////////////////////
// CONDITION: Is In Danger
////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the SL Is In Danger condition
 */
USTRUCT()
struct FStateTreeSLIsInDangerConditionInstanceData
{
	GENERATED_BODY()

	/** Enemy character to check danger status on */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ASoulsLikeBaseEnemy> Character;

	/** Minimum time before reacting to the danger event */
	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (Units = "s"))
	float MinReactionTime = 0.35f;

	/** Maximum time before ignoring the danger event */
	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (Units = "s"))
	float MaxReactionTime = 0.75f;

	/** Line of sight half angle for detecting incoming danger, in degrees */
	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (Units = "deg"))
	float DangerSightConeAngle = 120.0f;
};
STATETREE_POD_INSTANCEDATA(FStateTreeSLIsInDangerConditionInstanceData);

/**
 *  StateTree condition to check if the enemy is about to be hit by an attack
 */
USTRUCT(DisplayName = "SL Is In Danger")
struct FStateTreeSLIsInDangerCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeSLIsInDangerConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FStateTreeSLIsInDangerCondition() = default;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// CONDITION: Check Health
////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the SL Check Health condition
 */
USTRUCT()
struct FStateTreeSLCheckHealthConditionInstanceData
{
	GENERATED_BODY()

	/** Enemy character to check health on */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ASoulsLikeBaseEnemy> Character;

	/** Health percentage threshold (0-1) */
	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (ClampMin = 0, ClampMax = 1))
	float HealthThreshold = 0.5f;

	/** If true, condition passes when health is BELOW threshold. Otherwise, ABOVE. */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	bool bCheckBelow = true;
};
STATETREE_POD_INSTANCEDATA(FStateTreeSLCheckHealthConditionInstanceData);

/**
 *  StateTree condition to check enemy health against a threshold
 */
USTRUCT(DisplayName = "SL Check Health")
struct FStateTreeSLCheckHealthCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeSLCheckHealthConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FStateTreeSLCheckHealthCondition() = default;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// TASK: Attack
////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the SL Attack task
 */
USTRUCT()
struct FStateTreeSLAttackInstanceData
{
	GENERATED_BODY()

	/** Enemy character that will perform the attack */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ASoulsLikeBaseEnemy> Character;

	/** Type of attack to perform */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	EAttackType AttackType = EAttackType::Light;

	/** Maximum number of combo hits the AI will perform */
	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (ClampMin = 1, ClampMax = 10))
	int32 MaxComboHits = 3;
};

/**
 *  StateTree task to tell an enemy to perform an attack
 */
USTRUCT(meta = (DisplayName = "SL Attack", Category = "SoulsLike"))
struct FStateTreeSLAttackTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeSLAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// TASK: Wait For Landing
////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the SL Wait For Landing task
 */
USTRUCT()
struct FStateTreeSLWaitForLandingInstanceData
{
	GENERATED_BODY()

	/** Enemy character to wait for landing on */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ASoulsLikeBaseEnemy> Character;
};

/**
 *  StateTree task to wait until the enemy lands on the ground
 */
USTRUCT(meta = (DisplayName = "SL Wait for Landing", Category = "SoulsLike"))
struct FStateTreeSLWaitForLandingTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeSLWaitForLandingInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// TASK: Face Actor
////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the SL Face Actor task
 */
USTRUCT()
struct FStateTreeSLFaceActorInstanceData
{
	GENERATED_BODY()

	/** AI Controller */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> Controller;

	/** Actor to face towards */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> ActorToFaceTowards;
};

/**
 *  StateTree task to face an AI-controlled pawn towards an actor
 */
USTRUCT(meta = (DisplayName = "SL Face Actor", Category = "SoulsLike"))
struct FStateTreeSLFaceActorTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeSLFaceActorInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// TASK: Set Speed
////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the SL Set Speed task
 */
USTRUCT()
struct FStateTreeSLSetSpeedInstanceData
{
	GENERATED_BODY()

	/** Character to affect */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACharacter> Character;

	/** Max ground speed to set */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Speed = 600.0f;
};

/**
 *  StateTree task to set a character's max walk speed
 */
USTRUCT(meta = (DisplayName = "SL Set Speed", Category = "SoulsLike"))
struct FStateTreeSLSetSpeedTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeSLSetSpeedInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

////////////////////////////////////////////////////////////////////
// TASK: Get Player Info
////////////////////////////////////////////////////////////////////

/**
 *  Instance data for the SL Get Player Info task
 */
USTRUCT()
struct FStateTreeSLGetPlayerInfoInstanceData
{
	GENERATED_BODY()

	/** Character that owns this task */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<ACharacter> Character;

	/** Detected player character */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ACharacter> TargetPlayerCharacter;

	/** Last known location for the target */
	UPROPERTY(VisibleAnywhere)
	FVector TargetPlayerLocation = FVector::ZeroVector;

	/** Distance to the target */
	UPROPERTY(VisibleAnywhere)
	float DistanceToTarget = 0.0f;
};

/**
 *  Ticking StateTree task that continuously updates player information
 */
USTRUCT(meta = (DisplayName = "SL Get Player Info", Category = "SoulsLike"))
struct FStateTreeSLGetPlayerInfoTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeSLGetPlayerInfoInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
