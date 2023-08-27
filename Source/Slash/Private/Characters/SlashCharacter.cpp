// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SlashCharacter.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GroomComponent.h"
#include "Components/SphereComponent.h"
#include "MotionWarpingComponent.h"
#include "Animation/AnimMontage.h"
#include "Items/Item.h"
#include "Items/Weapons/Weapon.h"
#include "HUD/SlashHUD.h"
#include "HUD/SlashOverlay.h"
#include "Components/AttributeComponent.h"
#include "Items/Soul.h"
#include "Items/Treasure.h"
#include "Items/Health.h"

ASlashCharacter::ASlashCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 300.f;

	// To avoid character to be rotated itself when moving mouse:
	// 1. Disable controller rotation across all axes.
	bUseControllerRotationPitch = bUseControllerRotationYaw = bUseControllerRotationRoll = false;

	// 2. Enable pawn control rotation on `USpringArmComponent` to move camera only.
	// This is also editable from Blueprints.
	CameraBoom->bUsePawnControlRotation = true;

	// Enable movement as per the rotation of character
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(CameraBoom);

	Hair = CreateDefaultSubobject<UGroomComponent>(TEXT("Hair"));
	Hair->SetupAttachment(GetMesh());
	Hair->AttachmentName = FString("head");

	Eyebrows = CreateDefaultSubobject<UGroomComponent>(TEXT("Eyebrows"));
	Eyebrows->SetupAttachment(GetMesh());
	Eyebrows->AttachmentName = FString("head");

	CombatTargetDetectorSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CombatTargetDetectorSphere"));
	CombatTargetDetectorSphere->SetSphereRadius(AttackRadius);
	CombatTargetDetectorSphere->SetupAttachment(GetMesh());
	CombatTargetDetectorSphere->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));

	EchoMotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("EchoMotionWarping"));
	EchoMotionWarping->SetAutoActivate(true);
}

void ASlashCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MovementAction, ETriggerEvent::Triggered, this, &ASlashCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASlashCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ASlashCharacter::Jump);
		EnhancedInputComponent->BindAction(EKeyAction, ETriggerEvent::Triggered, this, &ASlashCharacter::EKeyPressed);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &ASlashCharacter::Attack);
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &ASlashCharacter::Dodge);
	}
}

float ASlashCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	HandleDamage(DamageAmount);
	SetHUDHealth();
	return DamageAmount;
}

void ASlashCharacter::Jump()
{
	if (IsUnoccupied())
	{
		Super::Jump();
	}
}

void ASlashCharacter::GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter)
{
	Super::GetHit_Implementation(ImpactPoint, Hitter);

	if (Attributes && Attributes->GetHealthPercent())
	{
		ActionState = EActionState::EAS_HitReaction;
	}
}

void ASlashCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead()) return;

	// Update stamina regeneration.
	if (Attributes && SlashOverlay)
	{
		Attributes->RegenStamina(DeltaTime);
		SlashOverlay->SetStaminaBarPercent(Attributes->GetStaminaPercent());
	}

	// Update the `CombatTarget` if required.
	CheckAndSetCombatTarget();
}

void ASlashCharacter::SetOverlappingItem(AItem* Item)
{
	OverlappingItem = Item;
}

void ASlashCharacter::AddSouls(ASoul* Soul)
{
	if (Attributes && SlashOverlay)
	{
		Attributes->AddSouls(Soul->GetSouls());
		SlashOverlay->SetSouls(Attributes->GetSouls());
	}
}

void ASlashCharacter::AddGold(ATreasure* Treasure)
{
	if (Attributes && SlashOverlay)
	{
		Attributes->AddGold(Treasure->GetGold());
		SlashOverlay->SetGold(Attributes->GetGold());
	}
}

void ASlashCharacter::AddHealth(AHealth* Health)
{
	if (Attributes)
	{
		Attributes->AddHealth(Health->GetHealthCount());
		SetHUDHealth();
	}
}

void ASlashCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		InitializeEnhancedInputSubsystem(PlayerController);
		InitializeSlashOverlay(PlayerController);
	}

	// Add the tag to this character.
	// This can be helpful at some places where we want to check if the AActor is this character or not.
	// This is also used to decide show/hide the damage HUD on weapons.
	// So, instead of expensive type-casting operation, checking the below tag value on AActor is cheaper.
	Tags.Add(FName("EngageableTarget"));

	CombatTargetDetectorSphere->OnComponentBeginOverlap.AddDynamic(this, &ASlashCharacter::OnSphereOverlap);
	CombatTargetDetectorSphere->OnComponentEndOverlap.AddDynamic(this, &ASlashCharacter::OnSphereEndOverlap);
}

void ASlashCharacter::Move(const FInputActionValue& Value)
{
	// If occupied, don't move
	if (IsOccupied()) return;

	const FVector2D MovementVector = Value.Get<FVector2D>();

	/*
	const FVector Forward = GetActorForwardVector();
	AddMovementInput(Forward, MovementVector.Y);

	const FVector Right = GetActorRightVector();
	AddMovementInput(Right, MovementVector.X);
	*/

	// Move in direction depending on the controller
	const FRotator Rotation = GetController()->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, MovementVector.Y);

	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ASlashCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerPitchInput(LookAxisVector.Y);
	AddControllerYawInput(LookAxisVector.X);
}

void ASlashCharacter::EKeyPressed()
{
	AWeapon* OverlappingWeapon = Cast<AWeapon>(OverlappingItem);
	if (OverlappingWeapon)
	{
		if (EquippedWeapon)
		{
			EquippedWeapon->Destroy();
		}
		EquipWeapon(OverlappingWeapon);
	}
	else
	{
		if (CanDisarm())
		{
			Disarm();
		}
		else if (CanArm())
		{
			Arm();
		}
	}
}

void ASlashCharacter::Attack()
{
	Super::Attack();

	if (CanAttack())
	{
		PlayAttackMontage();
		ActionState = EActionState::EAS_Attacking;
	}
}

void ASlashCharacter::Dodge()
{
	if (IsOccupied() || !HasEnoughStamina()) return;

	if (Attributes && SlashOverlay)
	{
		Attributes->UseStamina(Attributes->GetDodgeCost());
		SlashOverlay->SetStaminaBarPercent(Attributes->GetStaminaPercent());
	}

	PlayDodgeMontage();
	ActionState = EActionState::EAS_Dodge;
}

void ASlashCharacter::AttackEnd()
{
	ActionState = EActionState::EAS_Unoccupied;
}

void ASlashCharacter::DodgeEnd()
{
	Super::DodgeEnd();

	ActionState = EActionState::EAS_Unoccupied;
}

bool ASlashCharacter::CanAttack()
{
	return ActionState == EActionState::EAS_Unoccupied &&
		CharacterState != ECharacterState::ECS_Unequipped;
}

void ASlashCharacter::Die_Implementation()
{
	Super::Die_Implementation();

	ActionState = EActionState::EAS_Dead;

	// Do not take any hit once got into dead state.
	DisableMeshCollision();

	// Remove combat detection sphere.
	CombatTargetDetectorSphere = nullptr;

	// Clear the array of possible targets.
	InRangeCombatTargets.Empty();
}

void ASlashCharacter::EquipWeapon(AWeapon* Weapon)
{
	Weapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);
	CharacterState = ECharacterState::ECS_EquippedOneHandedWeapon;
	SetOverlappingItem(nullptr);  // OverlappingItem = nullptr;
	EquippedWeapon = Weapon;

	// Set `WeaponDamageText` in `SlashOverlay`.
	// This will update the weapon's damage amount in overlay HUD.
	if (SlashOverlay)
	{
		SlashOverlay->SetWeaponDamage(EquippedWeapon->GetWeaponDamage());
	}
}

void ASlashCharacter::PlayEquipMontage(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && EquipMontage)
	{
		AnimInstance->Montage_Play(EquipMontage);
		AnimInstance->Montage_JumpToSection(SectionName, EquipMontage);
	}
}

bool ASlashCharacter::CanDisarm()
{
	return ActionState == EActionState::EAS_Unoccupied && 
		CharacterState != ECharacterState::ECS_Unequipped;
}

bool ASlashCharacter::CanArm()
{
	return ActionState == EActionState::EAS_Unoccupied && 
		CharacterState == ECharacterState::ECS_Unequipped && 
		EquippedWeapon;
}

bool ASlashCharacter::HasEnoughStamina()
{
	return Attributes && Attributes->GetStamina() > Attributes->GetDodgeCost();
}

bool ASlashCharacter::IsOccupied()
{
	return ActionState != EActionState::EAS_Unoccupied;
}

void ASlashCharacter::Disarm()
{
	PlayEquipMontage(FName("Unequip"));
	CharacterState = ECharacterState::ECS_Unequipped;
	ActionState = EActionState::EAS_EquippingWeapon;
}

void ASlashCharacter::Arm()
{
	PlayEquipMontage(FName("Equip"));
	CharacterState = ECharacterState::ECS_EquippedOneHandedWeapon;
	ActionState = EActionState::EAS_EquippingWeapon;
}

void ASlashCharacter::AttachWeaponToBack()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachMeshToSocket(GetMesh(), FName("SpineSocket"));
	}
}

void ASlashCharacter::AttachWeaponToHand()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachMeshToSocket(GetMesh(), FName("RightHandSocket"));
	}
}

void ASlashCharacter::FinishEquipping()
{
	ActionState = EActionState::EAS_Unoccupied;
}

void ASlashCharacter::HitReactEnd()
{
	ActionState = EActionState::EAS_Unoccupied;
}

void ASlashCharacter::CheckAndSetCombatTarget()
{
	// If no targets found, then reset `CombatTarget` & return.
	if (InRangeCombatTargets.Num() <= 0)
	{
		CombatTarget = nullptr;
		// GEngine->AddOnScreenDebugMessage(4, 5.0f, FColor::Yellow, FString::Printf(TEXT("Locked CombatTarget = N.A.")));
		return;
	}

	// Initialize closest Enemy distance to maximum value of `double` type.
	double ClosestEnemyDistance = DBL_MAX;

	// Set the `CombatTarget` based on which Enemy is closest to Character.
	for (int32 index = 0; index < InRangeCombatTargets.Num(); index++)
	{
		AActor* Target = InRangeCombatTargets[index];

		// Get `TargetLocation` of Enemy.
		const FVector TargetLocation = (Target) ? Target->GetActorLocation() : FVector();

		// Compute distance between Enemy to Character.
		double EnemyToMeDistance = (GetActorLocation() - TargetLocation).Size();

		// Check & ensure to avoid negative distance.
		EnemyToMeDistance = (EnemyToMeDistance < 0) ? (EnemyToMeDistance * -1) : EnemyToMeDistance;

		// Update the `CombatTarget` if required.
		if (ClosestEnemyDistance > EnemyToMeDistance)
		{
			CombatTarget = Target;
			ClosestEnemyDistance = EnemyToMeDistance;

			// GEngine->AddOnScreenDebugMessage(4, 5.0f, FColor::Green, FString::Printf(TEXT("Locked CombatTarget = %s"), *CombatTarget->GetName()));
		}
	}
}

void ASlashCharacter::RemoveMotionWarping()
{
	// The `WarpTargetName` arguments below must match the warp target names from Echo's Motion Warping anim notifies.
	if (EchoMotionWarping)
	{
		EchoMotionWarping->RemoveWarpTarget(FName("EchoTranslationTarget"));
		EchoMotionWarping->RemoveWarpTarget(FName("EchoRotationTarget"));
	}
}

void ASlashCharacter::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->ActorHasTag(FName("Enemy")))
	{
		// Add `OtherActor` to array.
		InRangeCombatTargets.AddUnique(OtherActor);
	}
}

void ASlashCharacter::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->ActorHasTag(FName("Enemy")))
	{
		// Remove `OtherActor` from array.
		InRangeCombatTargets.Remove(OtherActor);

		// Clear the `CombatTarget` if it's same as `OtherActor` & remove the motion warping targets.
		if (CombatTarget == OtherActor)
		{
			CombatTarget = nullptr;
			RemoveMotionWarping();
		}
	}
}

bool ASlashCharacter::IsUnoccupied()
{
	return ActionState == EActionState::EAS_Unoccupied;
}

bool ASlashCharacter::IsDead()
{
	return ActionState == EActionState::EAS_Dead;
}

void ASlashCharacter::InitializeEnhancedInputSubsystem(APlayerController* PlayerController)
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(SlashContext, 0);
	}
}

void ASlashCharacter::InitializeSlashOverlay(APlayerController* PlayerController)
{
	if (ASlashHUD* SlashHUD = Cast<ASlashHUD>(PlayerController->GetHUD()))
	{
		SlashOverlay = SlashHUD->GetSlashOverlay();
		if (SlashOverlay && Attributes)
		{
			SlashOverlay->SetHealthBarPercent(Attributes->GetHealthPercent());
			SlashOverlay->SetStaminaBarPercent(1.0f);
			SlashOverlay->SetGold(0);
			SlashOverlay->SetSouls(0);
			SlashOverlay->SetWeaponDamage(0.0f);
		}
	}
}

void ASlashCharacter::SetHUDHealth()
{
	if (SlashOverlay && Attributes)
	{
		SlashOverlay->SetHealthBarPercent(Attributes->GetHealthPercent());
	}
}
