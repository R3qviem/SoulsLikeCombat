// Copyright Epic Games, Inc. All Rights Reserved.

#include "SoulsLikePlayerCharacter.h"
#include "CharacterStateComponent.h"
#include "StaminaComponent.h"
#include "LockOnComponent.h"
#include "WeaponManagerComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "WeaponDataAsset.h"
#include "InputBufferComponent.h"
#include "InventoryComponent.h"
#include "CameraOcclusionComponent.h"
#include "ItemDataAsset.h"
#include "ItemPickup.h"
#include "BonfireCheckpoint.h"
#include "SoulsLikePlayerController.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"

ASoulsLikePlayerCharacter::ASoulsLikePlayerCharacter()
{
	// Set collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// Set default skeletal mesh (Manny mannequin)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MeshFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// Set default animation blueprint (Combat ABP for warrior-like locomotion)
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBPFinder(
		TEXT("/Game/Variant_Combat/Anims/ABP_Manny_Combat"));
	if (AnimBPFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimBPFinder.Class);
	}

	// Default greatsword weapon data (can be overridden in the editor via WeaponManager->UnarmedWeaponData)
	{
		UWeaponDataAsset* GreatswordData = CreateDefaultSubobject<UWeaponDataAsset>(TEXT("DefaultGreatswordData"));
		GreatswordData->WeaponName = FText::FromString(TEXT("Moon Sword"));
		GreatswordData->WeaponType = EWeaponType::Greatsword;
		GreatswordData->BaseDamage = 25.0f;
		GreatswordData->DodgeStaminaCost = 20.0f;

		// Load light attack combo montage (GSCombo — Mixamo sword combo)
		static ConstructorHelpers::FObjectFinder<UAnimMontage> LightComboMontageFinder(
			TEXT("/Game/Animations/AttackSword/GSCombo_Anim_Montage.GSCombo_Anim_Montage"));
		if (LightComboMontageFinder.Succeeded())
		{
			GreatswordData->LightAttackMontage.AttackMontage = LightComboMontageFinder.Object;
			GreatswordData->LightAttackMontage.PlayRate = 1.0f;
		}

		// Load heavy attack montage (GSHeavy — Mixamo heavy strike)
		static ConstructorHelpers::FObjectFinder<UAnimMontage> HeavyMontageFinder(
			TEXT("/Game/Animations/AttackSword/GSHeavy_Anim_Montage.GSHeavy_Anim_Montage"));
		if (HeavyMontageFinder.Succeeded())
		{
			GreatswordData->HeavyAttackMontage.AttackMontage = HeavyMontageFinder.Object;
			GreatswordData->HeavyAttackMontage.PlayRate = 1.0f;
		}

		// Load parry montage (Blocking — Mixamo block animation)
		static ConstructorHelpers::FObjectFinder<UAnimMontage> ParryMontageFinder(
			TEXT("/Game/Animations/AttackSword/Blocking_Anim_Montage.Blocking_Anim_Montage"));
		if (ParryMontageFinder.Succeeded())
		{
			GreatswordData->ParryMontage = ParryMontageFinder.Object;
		}

		// Load sword mesh
		static ConstructorHelpers::FObjectFinder<UStaticMesh> SwordMeshFinder(
			TEXT("/Game/Fab/Moon_Sword/moon_sword/StaticMeshes/moon_sword.moon_sword"));
		if (SwordMeshFinder.Succeeded())
		{
			GreatswordData->WeaponMesh = SwordMeshFinder.Object;
		}

		// Light combo chain — 3 hits using montage sections
		FComboAttack LightAttack1;
		LightAttack1.MontageSectionName = NAME_None; // Default section = first attack
		LightAttack1.DamageMultiplier = 1.0f;
		LightAttack1.StaminaCost = 15.0f;
		LightAttack1.HitReaction = EHitReactionType::Light;
		GreatswordData->LightComboChain.Add(LightAttack1);

		FComboAttack LightAttack2;
		LightAttack2.MontageSectionName = FName("Attack2");
		LightAttack2.DamageMultiplier = 1.2f;
		LightAttack2.StaminaCost = 18.0f;
		LightAttack2.HitReaction = EHitReactionType::Light;
		GreatswordData->LightComboChain.Add(LightAttack2);

		FComboAttack LightAttack3;
		LightAttack3.MontageSectionName = FName("Attack3");
		LightAttack3.DamageMultiplier = 1.5f;
		LightAttack3.StaminaCost = 22.0f;
		LightAttack3.HitReaction = EHitReactionType::Heavy;
		LightAttack3.KnockbackImpulse = 300.0f;
		GreatswordData->LightComboChain.Add(LightAttack3);

		// Heavy combo — single powerful strike
		FComboAttack HeavyAttack;
		HeavyAttack.MontageSectionName = NAME_None;
		HeavyAttack.DamageMultiplier = 2.5f;
		HeavyAttack.StaminaCost = 35.0f;
		HeavyAttack.HitReaction = EHitReactionType::Heavy;
		HeavyAttack.KnockbackImpulse = 500.0f;
		GreatswordData->HeavyComboChain.Add(HeavyAttack);

		WeaponManager->UnarmedWeaponData = GreatswordData;
	}

	// Load death animation (plays before ragdoll)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathAnimFinder(
		TEXT("/Game/Animations/AttackSword/GSDeath_Anim.GSDeath_Anim"));
	if (DeathAnimFinder.Succeeded())
	{
		DeathAnimSequence = DeathAnimFinder.Object;
	}

	// Load directional dodge montages
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DodgeFMontage(
		TEXT("/Game/Animations/Dodge_F_Montage.Dodge_F_Montage"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DodgeBMontage(
		TEXT("/Game/Animations/Dodge_B_Montage.Dodge_B_Montage"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DodgeLMontage(
		TEXT("/Game/Animations/Dodge_L_Montage.Dodge_L_Montage"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DodgeRMontage(
		TEXT("/Game/Animations/Dodge_R_Montage.Dodge_R_Montage"));

	if (DodgeFMontage.Succeeded()) { DodgeForwardMontage = DodgeFMontage.Object; }
	if (DodgeBMontage.Succeeded()) { DodgeBackMontage = DodgeBMontage.Object; }
	if (DodgeLMontage.Succeeded()) { DodgeLeftMontage = DodgeLMontage.Object; }
	if (DodgeRMontage.Succeeded()) { DodgeRightMontage = DodgeRMontage.Object; }

	// Create camera boom — same as 3rd person but higher angle
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 800.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraLagSpeed = 10.0f;
	CameraBoom->CameraRotationLagSpeed = 10.0f;

	DesiredZoomDistance = 800.0f;

	// Create follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Create lock-on component
	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));

	// Create input buffer component
	InputBuffer = CreateDefaultSubobject<UInputBufferComponent>(TEXT("InputBuffer"));

	// Create inventory component
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	// Camera occlusion — fades objects that block the camera view
	CameraOcclusionComponent = CreateDefaultSubobject<UCameraOcclusionComponent>(TEXT("CameraOcclusionComponent"));

	// Create weapon mesh component attached to weapon socket
	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComponent->SetupAttachment(GetMesh(), FName("weapon_socket"));
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Set default sword mesh and transform
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SwordMeshFinder(
		TEXT("/Game/Fab/Moon_Sword/moon_sword/StaticMeshes/moon_sword.moon_sword"));
	if (SwordMeshFinder.Succeeded())
	{
		WeaponMeshComponent->SetStaticMesh(SwordMeshFinder.Object);
		WeaponMeshComponent->SetRelativeLocation(FVector(-10.0f, -17.1f, 47.0f));
		WeaponMeshComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, -110.0f));
		WeaponMeshComponent->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.2f));
	}

	// Default socket name
	WeaponManager->WeaponSocketName = FName("weapon_socket");

	// Player tag
	Tags.Add(FName("Player"));
}

void ASoulsLikePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Set initial controller pitch for top-down view (mouse yaw-only keeps this locked)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(FRotator(-55.0f, PC->GetControlRotation().Yaw, 0.0f));
	}

	// Weapon mesh is attached via SetupAttachment in constructor — BP transform overrides apply automatically

	// Bind the input buffer delegate
	InputBuffer->OnBufferedInputConsumed.AddDynamic(this, &ASoulsLikePlayerCharacter::OnBufferedInputConsumed);

	// Listen for state changes to consume buffer when returning to an actionable state
	StateComponent->OnStateChanged.AddDynamic(this, &ASoulsLikePlayerCharacter::OnStateChangedHandler);
}

void ASoulsLikePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateMovementOrientation();

	// Auto-fire heavy attack at max charge
	if (bIsChargingHeavy)
	{
		const float ChargeTime = GetWorld()->GetTimeSeconds() - HeavyChargeStartTime;
		if (ChargeTime >= MaxChargeTime)
		{
			bIsChargingHeavy = false;
			// Force jump to attack section
			if (bHasLoopedChargedAttack)
			{
				CheckChargedAttack();
			}
		}
	}

	// Smooth zoom interpolation
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DesiredZoomDistance, DeltaTime, ZoomInterpSpeed);
	}
}

void ASoulsLikePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, TEXT("SetupPlayerInputComponent CALLED!"));
	}

	// Always create input actions fresh (avoids stale BP serialization issues)
	IA_Move = NewObject<UInputAction>(this, TEXT("IA_Move"));
	IA_Move->ValueType = EInputActionValueType::Axis2D;

	IA_Look = NewObject<UInputAction>(this, TEXT("IA_Look"));
	IA_Look->ValueType = EInputActionValueType::Axis2D;

	IA_MouseLook = NewObject<UInputAction>(this, TEXT("IA_MouseLook"));
	IA_MouseLook->ValueType = EInputActionValueType::Axis2D;

	IA_LightAttack = NewObject<UInputAction>(this, TEXT("IA_LightAttack"));
	IA_LightAttack->ValueType = EInputActionValueType::Boolean;

	IA_HeavyAttack = NewObject<UInputAction>(this, TEXT("IA_HeavyAttack"));
	IA_HeavyAttack->ValueType = EInputActionValueType::Boolean;
	IA_Dodge = NewObject<UInputAction>(this, TEXT("IA_Dodge"));
	IA_Dodge->ValueType = EInputActionValueType::Boolean;

	IA_Block = NewObject<UInputAction>(this, TEXT("IA_Block"));
	IA_Block->ValueType = EInputActionValueType::Boolean;

	IA_LockOn = NewObject<UInputAction>(this, TEXT("IA_LockOn"));
	IA_LockOn->ValueType = EInputActionValueType::Boolean;

	IA_SwitchTarget = NewObject<UInputAction>(this, TEXT("IA_SwitchTarget"));
	IA_SwitchTarget->ValueType = EInputActionValueType::Axis1D;

	IA_Zoom = NewObject<UInputAction>(this, TEXT("IA_Zoom"));
	IA_Zoom->ValueType = EInputActionValueType::Axis1D;

	IA_Interact = NewObject<UInputAction>(this, TEXT("IA_Interact"));
	IA_Interact->ValueType = EInputActionValueType::Boolean;

	IA_ToggleInventory = NewObject<UInputAction>(this, TEXT("IA_ToggleInventory"));
	IA_ToggleInventory->ValueType = EInputActionValueType::Boolean;

	IA_QuickSlot1 = NewObject<UInputAction>(this, TEXT("IA_QuickSlot1"));
	IA_QuickSlot1->ValueType = EInputActionValueType::Boolean;

	IA_QuickSlot2 = NewObject<UInputAction>(this, TEXT("IA_QuickSlot2"));
	IA_QuickSlot2->ValueType = EInputActionValueType::Boolean;

	IA_QuickSlot3 = NewObject<UInputAction>(this, TEXT("IA_QuickSlot3"));
	IA_QuickSlot3->ValueType = EInputActionValueType::Boolean;

	IA_QuickSlot4 = NewObject<UInputAction>(this, TEXT("IA_QuickSlot4"));
	IA_QuickSlot4->ValueType = EInputActionValueType::Boolean;

	// Create and register default input mapping context
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			UInputMappingContext* IMC = NewObject<UInputMappingContext>(this, TEXT("DefaultMappingContext"));

			// WASD Movement (IA_Move is Axis2D: X = right/left, Y = forward/back)
			{
				// W - Forward (+Y): swizzle key press from X to Y
				FEnhancedActionKeyMapping& WMapping = IMC->MapKey(IA_Move, EKeys::W);
				UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
				Swizzle->Order = EInputAxisSwizzle::YXZ;
				WMapping.Modifiers.Add(Swizzle);
			}
			{
				// S - Backward (-Y): swizzle to Y then negate
				FEnhancedActionKeyMapping& SMapping = IMC->MapKey(IA_Move, EKeys::S);
				UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(this);
				Swizzle->Order = EInputAxisSwizzle::YXZ;
				UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
				SMapping.Modifiers.Add(Swizzle);
				SMapping.Modifiers.Add(Negate);
			}
			{
				// A - Left (-X): negate
				FEnhancedActionKeyMapping& AMapping = IMC->MapKey(IA_Move, EKeys::A);
				UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
				AMapping.Modifiers.Add(Negate);
			}
			// D - Right (+X): no modifiers
			IMC->MapKey(IA_Move, EKeys::D);

			// Mouse Look (Mouse2D gives Axis2D with X=yaw, Y=pitch)
			{
				FEnhancedActionKeyMapping& MouseMapping = IMC->MapKey(IA_MouseLook, EKeys::Mouse2D);
				// Negate Y axis only for standard pitch direction (mouse up = look up)
				UInputModifierNegate* PitchNegate = NewObject<UInputModifierNegate>(this);
				PitchNegate->bX = false;
				PitchNegate->bY = true;
				PitchNegate->bZ = false;
				MouseMapping.Modifiers.Add(PitchNegate);
			}

			// Combat actions
			IMC->MapKey(IA_LightAttack, EKeys::LeftMouseButton);
			IMC->MapKey(IA_HeavyAttack, EKeys::RightMouseButton);
			IMC->MapKey(IA_Dodge, EKeys::SpaceBar);
			IMC->MapKey(IA_Block, EKeys::LeftControl);
			IMC->MapKey(IA_LockOn, EKeys::MiddleMouseButton);

			// Target switching
			{
				// Q - Switch left (-1)
				FEnhancedActionKeyMapping& QMapping = IMC->MapKey(IA_SwitchTarget, EKeys::Q);
				UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
				QMapping.Modifiers.Add(Negate);
			}
			// E - Switch right (+1)
			IMC->MapKey(IA_SwitchTarget, EKeys::E);

			// Mouse wheel zoom
			IMC->MapKey(IA_Zoom, EKeys::MouseScrollUp);
			{
				FEnhancedActionKeyMapping& ScrollDownMapping = IMC->MapKey(IA_Zoom, EKeys::MouseScrollDown);
				UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(this);
				ScrollDownMapping.Modifiers.Add(Negate);
			}

			// Interact and inventory
			IMC->MapKey(IA_Interact, EKeys::F);
			IMC->MapKey(IA_ToggleInventory, EKeys::I);

			// Quick slots (1, 2, 3, 4)
			IMC->MapKey(IA_QuickSlot1, EKeys::One);
			IMC->MapKey(IA_QuickSlot2, EKeys::Two);
			IMC->MapKey(IA_QuickSlot3, EKeys::Three);
			IMC->MapKey(IA_QuickSlot4, EKeys::Four);

			Subsystem->AddMappingContext(IMC, 0);
		}
	}

	// Bind input actions
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	// Movement
	EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASoulsLikePlayerCharacter::HandleMove);
	EnhancedInput->BindAction(IA_Move, ETriggerEvent::Completed, this, &ASoulsLikePlayerCharacter::HandleMove);

	// Camera look
	EnhancedInput->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ASoulsLikePlayerCharacter::HandleLook);
	EnhancedInput->BindAction(IA_MouseLook, ETriggerEvent::Triggered, this, &ASoulsLikePlayerCharacter::HandleLook);

	// Combat
	EnhancedInput->BindAction(IA_LightAttack, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleLightAttack);
	EnhancedInput->BindAction(IA_HeavyAttack, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleHeavyAttackStart);
	EnhancedInput->BindAction(IA_HeavyAttack, ETriggerEvent::Completed, this, &ASoulsLikePlayerCharacter::HandleHeavyAttackRelease);
	EnhancedInput->BindAction(IA_Dodge, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleDodge);

	// Block (hold)
	EnhancedInput->BindAction(IA_Block, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleBlockStart);
	EnhancedInput->BindAction(IA_Block, ETriggerEvent::Completed, this, &ASoulsLikePlayerCharacter::HandleBlockEnd);

	// Lock-on
	EnhancedInput->BindAction(IA_LockOn, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleLockOn);
	EnhancedInput->BindAction(IA_SwitchTarget, ETriggerEvent::Triggered, this, &ASoulsLikePlayerCharacter::HandleSwitchTarget);

	// Zoom
	EnhancedInput->BindAction(IA_Zoom, ETriggerEvent::Triggered, this, &ASoulsLikePlayerCharacter::HandleZoom);

	// Interact and inventory
	EnhancedInput->BindAction(IA_Interact, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleInteract);
	EnhancedInput->BindAction(IA_ToggleInventory, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleToggleInventory);

	// Quick slots (1-4)
	EnhancedInput->BindAction(IA_QuickSlot1, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleQuickSlot1);
	EnhancedInput->BindAction(IA_QuickSlot2, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleQuickSlot2);
	EnhancedInput->BindAction(IA_QuickSlot3, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleQuickSlot3);
	EnhancedInput->BindAction(IA_QuickSlot4, ETriggerEvent::Started, this, &ASoulsLikePlayerCharacter::HandleQuickSlot4);
}

// ===== INPUT HANDLERS =====

void ASoulsLikePlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	FVector2D MoveInput = Value.Get<FVector2D>();
	LastMoveInput = MoveInput;

	if (StateComponent->IsInputBlocked())
	{
		return;
	}

	// Update state to Moving if we have input, Idle if released
	if (!MoveInput.IsNearlyZero())
	{
		StateComponent->RequestStateChange(ECharacterState::Moving);
	}
	else
	{
		if (StateComponent->GetCurrentState() == ECharacterState::Moving)
		{
			StateComponent->RequestStateChange(ECharacterState::Idle);
		}
		return;
	}

	if (LockOnComponent->IsLockedOn())
	{
		// Strafe movement relative to locked target
		const FVector MoveDir = LockOnComponent->GetMoveDirectionRelativeToTarget(MoveInput);
		AddMovementInput(MoveDir, 1.0f);
	}
	else
	{
		// Standard camera-relative movement
		if (GetController())
		{
			const FRotator Rotation = GetController()->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			AddMovementInput(ForwardDirection, MoveInput.Y);
			AddMovementInput(RightDirection, MoveInput.X);
		}
	}
}

void ASoulsLikePlayerCharacter::HandleLook(const FInputActionValue& Value)
{
	// When locked on, camera is controlled by the LockOnComponent
	if (LockOnComponent->IsLockedOn())
	{
		return;
	}

	FVector2D LookInput = Value.Get<FVector2D>();

	if (GetController())
	{
		// Only apply yaw (X axis rotation) — top-down pitch is fixed by the boom
		AddControllerYawInput(LookInput.X);
	}
}

void ASoulsLikePlayerCharacter::HandleLightAttack()
{
	// During Attacking state, let the combo system handle queuing
	if (StateComponent->GetCurrentState() == ECharacterState::Attacking)
	{
		AttemptAttack(EAttackType::Light);
		return;
	}

	// Buffer during other blocked states (Dodging, Stunned)
	if (StateComponent->IsInputBlocked())
	{
		InputBuffer->BufferInput(EBufferedInput::LightAttack);
		return;
	}

	AttemptAttack(EAttackType::Light);
}

void ASoulsLikePlayerCharacter::HandleHeavyAttackStart()
{
	// If already attacking, queue heavy as combo
	if (StateComponent->GetCurrentState() == ECharacterState::Attacking)
	{
		AttemptAttack(EAttackType::Heavy);
		return;
	}

	if (StateComponent->IsInputBlocked())
	{
		InputBuffer->BufferInput(EBufferedInput::HeavyAttack);
		return;
	}

	// Start charging — set flag BEFORE playing the montage so CheckChargedAttack loops
	bIsChargingHeavy = true;
	bHasLoopedChargedAttack = false;
	HeavyChargeStartTime = GetWorld()->GetTimeSeconds();

	// Start the charged attack montage (it will loop via CheckChargedAttack)
	AttemptAttack(EAttackType::Heavy);
}

void ASoulsLikePlayerCharacter::HandleHeavyAttackRelease()
{
	if (!bIsChargingHeavy)
	{
		return;
	}

	bIsChargingHeavy = false;

	// If the charge loop has already started, CheckChargedAttack will jump to attack section on next notify
	// If the montage hasn't looped yet, force jump to attack section now
	if (bHasLoopedChargedAttack)
	{
		CheckChargedAttack();
	}
}

void ASoulsLikePlayerCharacter::HandleDodge()
{
	if (StateComponent->IsInputBlocked())
	{
		InputBuffer->BufferInput(EBufferedInput::Dodge, LastMoveInput);
		return;
	}
	AttemptDodge(LastMoveInput);
}

void ASoulsLikePlayerCharacter::HandleBlockStart()
{
	if (StateComponent->IsInputBlocked())
	{
		InputBuffer->BufferInput(EBufferedInput::Block);
		return;
	}
	AttemptBlock();
}

void ASoulsLikePlayerCharacter::HandleBlockEnd()
{
	EndBlock();
}

void ASoulsLikePlayerCharacter::HandleLockOn()
{
	LockOnComponent->ToggleLockOn();
}

void ASoulsLikePlayerCharacter::HandleSwitchTarget(const FInputActionValue& Value)
{
	float Direction = Value.Get<float>();
	LockOnComponent->SwitchTarget(Direction);
}

void ASoulsLikePlayerCharacter::HandleZoom(const FInputActionValue& Value)
{
	const float ZoomInput = Value.Get<float>();
	DesiredZoomDistance = FMath::Clamp(DesiredZoomDistance - ZoomInput * ZoomStep, MinZoomDistance, MaxZoomDistance);
}

void ASoulsLikePlayerCharacter::OnActionEnd()
{
	Super::OnActionEnd();
	// Buffer consumption happens in OnStateChangedHandler when state transitions to Idle
}

void ASoulsLikePlayerCharacter::OnAttackHitConfirmed(AActor* HitActor, const FVector& ImpactPoint)
{
	// Light camera shake when dealing damage
	ApplyCameraImpact(4.0f);
}

void ASoulsLikePlayerCharacter::ProcessDamage(FDamageInfo DamageInfo)
{
	ASoulsLikeBaseCharacter::ProcessDamage(DamageInfo);

	// Camera shake on taking damage (stronger than dealing)
	if (!IsDead() && !DamageInfo.bWasParried)
	{
		const float Intensity = DamageInfo.bWasBlocked ? 5.0f : 10.0f;
		ApplyCameraImpact(Intensity);
	}
}

void ASoulsLikePlayerCharacter::ApplyCameraImpact(float Intensity)
{
	if (!CameraBoom)
	{
		return;
	}

	// Apply random offset to spring arm socket
	const FVector RandomOffset(
		FMath::FRandRange(-Intensity, Intensity),
		FMath::FRandRange(-Intensity, Intensity),
		FMath::FRandRange(-Intensity * 0.5f, Intensity * 0.5f)
	);
	CameraBoom->SocketOffset += RandomOffset;

	// Restore original offset after a short delay
	const FVector RestoreOffset = RandomOffset;
	GetWorld()->GetTimerManager().SetTimer(CameraShakeTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, RestoreOffset]()
	{
		if (CameraBoom)
		{
			CameraBoom->SocketOffset -= RestoreOffset;
		}
	}), 0.08f, false);
}

void ASoulsLikePlayerCharacter::HandleDeath()
{
	// Release lock-on before dying
	if (LockOnComponent->IsLockedOn())
	{
		LockOnComponent->ToggleLockOn();
	}

	// Call base death (animation, ragdoll, etc.)
	ASoulsLikeBaseCharacter::HandleDeath();

	// Disable player input
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// Destroy the pawn after a delay so the controller can respawn us
	SetLifeSpan(5.0f);
}

void ASoulsLikePlayerCharacter::OnStateChangedHandler(ECharacterState NewState)
{
	if (NewState == ECharacterState::Idle || NewState == ECharacterState::Moving)
	{
		InputBuffer->ConsumeBuffer();
	}
	else if (NewState == ECharacterState::Dead)
	{
		InputBuffer->ClearBuffer();
		bIsChargingHeavy = false;
	}

	// Cancel charge if interrupted (stunned, etc.)
	if (NewState == ECharacterState::Stunned || NewState == ECharacterState::Dodging)
	{
		bIsChargingHeavy = false;
	}
}

void ASoulsLikePlayerCharacter::OnBufferedInputConsumed(EBufferedInput InputType, FVector2D Direction)
{
	switch (InputType)
	{
	case EBufferedInput::LightAttack:
		AttemptAttack(EAttackType::Light);
		break;
	case EBufferedInput::HeavyAttack:
		AttemptAttack(EAttackType::Heavy);
		break;
	case EBufferedInput::Dodge:
		AttemptDodge(Direction);
		break;
	case EBufferedInput::Block:
		AttemptBlock();
		break;
	default:
		break;
	}
}

void ASoulsLikePlayerCharacter::UpdateMovementOrientation()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!CMC)
	{
		return;
	}

	// Don't override rotation settings during dodge (we disable orient-to-movement for directional dodge)
	if (StateComponent->GetCurrentState() == ECharacterState::Dodging)
	{
		return;
	}

	if (LockOnComponent->IsLockedOn())
	{
		// Strafe mode: face the locked target, disable orient-to-movement
		CMC->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
	}
	else
	{
		// Free movement: orient to movement direction
		CMC->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;
	}
}

// ===== INTERACT & INVENTORY =====

void ASoulsLikePlayerCharacter::HandleInteract()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("HandleInteract called!"));
	}

	if (StateComponent->IsInputBlocked())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Input is BLOCKED!"));
		}
		return;
	}

	const float InteractRange = 350.0f;
	const FVector MyLocation = GetActorLocation();

	// Check for bonfire checkpoints by distance (most reliable)
	int32 BonfireCount = 0;
	for (TActorIterator<ABonfireCheckpoint> It(GetWorld()); It; ++It)
	{
		ABonfireCheckpoint* Bonfire = *It;
		BonfireCount++;
		float Dist = FVector::Dist(MyLocation, Bonfire->GetActorLocation());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow,
				FString::Printf(TEXT("Bonfire found! Dist: %.0f (need < %.0f)"), Dist, InteractRange));
		}
		if (Bonfire && Dist <= InteractRange)
		{
			Bonfire->Rest(this);
			return;
		}
	}
	if (BonfireCount == 0 && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("No bonfires found in level!"));
	}

	// Sphere overlap check for item pickups
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(200.0f);
	if (GetWorld()->OverlapMultiByChannel(Overlaps, MyLocation, FQuat::Identity, ECC_WorldDynamic, Sphere))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (AItemPickup* Pickup = Cast<AItemPickup>(Overlap.GetActor()))
			{
				Pickup->PickUp(this);
				ASoulsLikePlayerController* PC = Cast<ASoulsLikePlayerController>(GetController());
				if (PC)
				{
					PC->RefreshQuickItemHUD();
				}
				return;
			}
		}
	}
}

void ASoulsLikePlayerCharacter::HandleToggleInventory()
{
	ASoulsLikePlayerController* PC = Cast<ASoulsLikePlayerController>(GetController());
	if (PC)
	{
		PC->ToggleInventory();
	}
}

void ASoulsLikePlayerCharacter::HandleQuickSlot(int32 SlotIndex)
{
	if (StateComponent->IsInputBlocked())
	{
		return;
	}

	if (!InventoryComponent)
	{
		return;
	}

	// Set this as the active slot and use it
	InventoryComponent->SetActiveQuickSlot(SlotIndex);

	const UItemDataAsset* UsedItem = InventoryComponent->UseQuickSlot(SlotIndex);
	if (UsedItem)
	{
		// Apply item effects
		if (UsedItem->HealAmount > 0.0f)
		{
			CurrentHealth = FMath::Min(CurrentHealth + UsedItem->HealAmount, MaxHealth);
			OnHealthChanged.Broadcast(GetHealthPercent());
		}
	}

	// Refresh the quick item HUD
	ASoulsLikePlayerController* PC = Cast<ASoulsLikePlayerController>(GetController());
	if (PC)
	{
		PC->RefreshQuickItemHUD();
	}
}
