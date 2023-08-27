// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Enemy.h"
#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/AttributeComponent.h"
#include "HUD/HealthBarComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Soul.h"
#include "Items/Health.h"
#include "Items/Weapons/Weapon.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetGenerateOverlapEvents(true);

	// Create & attach health bar to root component of class
	HealthBarComponent = CreateDefaultSubobject<UHealthBarComponent>(TEXT("HealthBar"));
	HealthBarComponent->SetupAttachment(GetRootComponent());

	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));
	PawnSensing->SightRadius = 4000.0f;
	PawnSensing->SetPeripheralVisionAngle(45.0f);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead()) return;

	if (EnemyState > EEnemyState::EES_Patrolling)
	{
		CheckCombatTarget();
	}
	else
	{
		CheckPatrolTarget();
	}
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	HandleDamage(DamageAmount);

	CombatTarget = EventInstigator->GetPawn();

	if (IsInsideAttackRadius())
	{
		EnemyState = EEnemyState::EES_Attacking;
	}
	else if (IsOutsideAttackRadius())
	{
		ChaseTarget();
	}

	return DamageAmount;
}

void AEnemy::Destroyed()
{
	// Remove the weapon equipped by Enemy
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
}

void AEnemy::GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter)
{
	Super::GetHit_Implementation(ImpactPoint, Hitter);

	if (!IsDead())
	{
		ShowHealthBar();
	}

	ClearPatrolTimer();
	ClearAttackTimer();
	StopAttackMontage();

	if (IsInsideAttackRadius() && !IsDead())
	{
		StartAttackTimer();
	}
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (PawnSensing)
	{
		// Add binding to `PawnSensing` component
		PawnSensing->OnSeePawn.AddDynamic(this, &AEnemy::PawnSeen);
	}

	InitializeEnemy();

	Tags.Add(FName("Enemy"));
}

void AEnemy::Die_Implementation()
{
	Super::Die_Implementation();

	EnemyState = EEnemyState::EES_Dead;
	ClearAttackTimer();
	HideHealthBar();
	DisableCapsule();
	SetLifeSpan(DeathLifeSpan);

	// To avoid rotating towards direction of acceleration after playing death montage.
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Disable weapon collision.
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);

	// Disable mesh collision & overlap events to avoid hitting again
	// and spawning more than one soul/health at once.
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetGenerateOverlapEvents(false);

	// Spawn a Soul once this enemy dies.
	SpawnSoul();

	// Chance of 50% to spawn a health.
	SpawnHealth();
}

void AEnemy::SpawnSoul()
{
	UWorld* World = GetWorld();
	if (World && SoulClass && Attributes)
	{
		const FVector SpawnLocation = GetActorLocation() + FVector(0.0f, 0.0f, 125.0f);
		ASoul* SpawnedSoul = World->SpawnActor<ASoul>(SoulClass, SpawnLocation, GetActorRotation());
		if (SpawnedSoul)
		{
			SpawnedSoul->SetSouls(Attributes->GetSouls());
			SpawnedSoul->SetOwner(this);
		}
	}
}

void AEnemy::SpawnHealth()
{
	UWorld* World = GetWorld();
	if (World && HealthClass && Attributes && Attributes->GetSpawnableHealth() > 0)
	{
		const FVector SpawnLocation = GetActorLocation() + FVector(-100.0f, 80.0f, 0.0f);

		AHealth* SpawnedHealth = World->SpawnActor<AHealth>(HealthClass, SpawnLocation, GetActorRotation());
		if (SpawnedHealth)
		{
			SpawnedHealth->SetHealthCount(Attributes->GetSpawnableHealth());
		}
	}
}

void AEnemy::Attack()
{
	Super::Attack();
	if (CombatTarget == nullptr) return;

	EnemyState = EEnemyState::EES_Engaged;
	PlayAttackMontage();
}

bool AEnemy::CanAttack()
{
	const bool bCanAttack =
		IsInsideAttackRadius() &&
		!IsAttacking() &&
		!IsEngaged() &&
		!IsDead();

	return bCanAttack;
}

void AEnemy::AttackEnd()
{
	EnemyState = EEnemyState::EES_NoState;
	CheckCombatTarget();
}

void AEnemy::HandleDamage(float DamageAmount)
{
	Super::HandleDamage(DamageAmount);

	if (Attributes && HealthBarComponent)
	{
		// Set health to percent in game UI
		HealthBarComponent->SetHealthPercent(Attributes->GetHealthPercent());
	}
}

void AEnemy::TrySpawningRandomWeapon()
{
	if (SpawnableRandomWeapons.Num() <= 0) return;

	const float SpawnChance = FMath::RandRange(0.0f, 100.0f);

	UWorld* World = GetWorld();

	// Spawn weapon only if `SpawnChance` is less than 30%.
	if (World && SpawnChance < 30)
	{
		const FVector SpawnLocation = GetActorLocation() + FVector(100.0f, -80.0f, 0.0f);

		const int32 Selection = FMath::RandRange(0, SpawnableRandomWeapons.Num() - 1);
		World->SpawnActor<AWeapon>(
			SpawnableRandomWeapons[Selection],
			SpawnLocation,
			GetActorRotation()
		);
	}
}

void AEnemy::InitializeEnemy()
{
	EnemyController = Cast<AAIController>(GetController());

	// Move enemy to the `PatrolTarget`
	MoveToTarget(PatrolTarget);

	// Hide health bar in the beginning
	HideHealthBar();

	// Attach weapon to Enemy's right hand socket
	SpawnDefaultWeapon();
}

void AEnemy::CheckPatrolTarget()
{
	if (InTargetRange(PatrolTarget, PatrolRadius))
	{
		// Set new patrol target after reaching the current target
		PatrolTarget = ChoosePatrolTarget();

		// Set timer to wait for random seconds between `WaitMin` & `WaitMax`
		// then use callback to move enemy to the new target after timer elapsed
		const float WaitTime = FMath::RandRange(PatrolWaitMin, PatrolWaitMax);
		GetWorldTimerManager().SetTimer(PatrolTimer, this, &AEnemy::PatrolTimerFinished, WaitTime);
	}
}

void AEnemy::CheckCombatTarget()
{
	if (IsOutsideCombatRadius())
	{
		ClearAttackTimer();
		LoseInterest();
		if (!IsEngaged())
		{
			StartPatrolling();
		}
	}
	else if (IsOutsideAttackRadius() && !IsChasing())
	{
		ClearAttackTimer();
		if (!IsEngaged())
		{
			ChaseTarget();
		}
	}
	else if (CanAttack())
	{
		StartAttackTimer();
	}
}

void AEnemy::PatrolTimerFinished()
{
	MoveToTarget(PatrolTarget);
}

void AEnemy::HideHealthBar()
{
	if (HealthBarComponent)
	{
		HealthBarComponent->SetVisibility(false);
	}
}

void AEnemy::ShowHealthBar()
{
	if (HealthBarComponent)
	{
		HealthBarComponent->SetVisibility(true);
	}
}

void AEnemy::LoseInterest()
{
	// Outside the combat radius, then no combat target
	CombatTarget = nullptr;

	// Hide health bar
	HideHealthBar();
}

void AEnemy::StartPatrolling()
{
	// Reset the enemy state to patrolling
	EnemyState = EEnemyState::EES_Patrolling;

	// Reset the max walk speed to speed of walking as per created in Blend Space 1D Horizontal Axis
	GetCharacterMovement()->MaxWalkSpeed = PatrollingSpeed;

	// Set movement back to `PatrolTarget`
	MoveToTarget(PatrolTarget);
}

void AEnemy::ChaseTarget()
{
	EnemyState = EEnemyState::EES_Chasing;
	GetCharacterMovement()->MaxWalkSpeed = ChasingSpeed;
	MoveToTarget(CombatTarget);
}

bool AEnemy::IsOutsideCombatRadius()
{
	return !InTargetRange(CombatTarget, CombatRadius);
}

bool AEnemy::IsOutsideAttackRadius()
{
	return !InTargetRange(CombatTarget, AttackRadius);
}

bool AEnemy::IsInsideAttackRadius()
{
	return InTargetRange(CombatTarget, AttackRadius);
}

bool AEnemy::IsChasing()
{
	return EnemyState == EEnemyState::EES_Chasing;
}

bool AEnemy::IsAttacking()
{
	return EnemyState == EEnemyState::EES_Attacking;
}

bool AEnemy::IsDead()
{
	return EnemyState == EEnemyState::EES_Dead;
}

bool AEnemy::IsEngaged()
{
	return EnemyState == EEnemyState::EES_Engaged;
}

void AEnemy::ClearPatrolTimer()
{
	GetWorldTimerManager().ClearTimer(PatrolTimer);
}

void AEnemy::StartAttackTimer()
{
	EnemyState = EEnemyState::EES_Attacking;
	const float AttackTime = FMath::RandRange(AttackMin, AttackMax);
	GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
}

void AEnemy::ClearAttackTimer()
{
	GetWorldTimerManager().ClearTimer(AttackTimer);
}

bool AEnemy::InTargetRange(AActor* Target, double Radius)
{
	if (Target == nullptr) return false;

	// Calculate the distance between enemy & specified target
	const double DistanceToTarget = (Target->GetActorLocation() - GetActorLocation()).Size();

	return DistanceToTarget <= Radius;
}

void AEnemy::MoveToTarget(AActor* Target)
{
	if (EnemyController == nullptr || Target == nullptr) return;

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(Target);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);

	EnemyController->MoveTo(MoveRequest);
}

AActor* AEnemy::ChoosePatrolTarget()
{
	// To eliminate possibility of selecting the same target in random selection,
	// create temporary array of valid targets containing all targets except the current one.
	TArray<AActor*> ValidTargets;
	for (AActor* Target : PatrolTargets)
	{
		if (Target != PatrolTarget)
		{
			ValidTargets.AddUnique(Target);
		}
	}

	const int32 NumPatrolTargets = ValidTargets.Num();
	if (NumPatrolTargets > 0)
	{
		const int32 TargetSelection = FMath::RandRange(0, NumPatrolTargets - 1);
		return ValidTargets[TargetSelection];
	}

	return nullptr;
}

void AEnemy::SpawnDefaultWeapon()
{
	UWorld* World = GetWorld();
	if (World && WeaponClass)
	{
		AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>(WeaponClass);
		DefaultWeapon->Equip(GetMesh(), FName("WeaponSocket"), this, this);
		EquippedWeapon = DefaultWeapon;
	}
}

void AEnemy::PawnSeen(APawn* SeenPawn)
{
	const bool bShouldChaseTarget =
		EnemyState != EEnemyState::EES_Dead &&
		EnemyState != EEnemyState::EES_Chasing &&
		EnemyState < EEnemyState::EES_Attacking &&
		SeenPawn->ActorHasTag(FName("EngageableTarget")) &&
		!(SeenPawn->ActorHasTag(FName("Dead")));

	if (bShouldChaseTarget)
	{
		// Set the `CombatTarget` to `SeenPawn`
		CombatTarget = SeenPawn;

		// Clear the `PatrolTimer` once started chasing the character
		ClearPatrolTimer();

		// Chase target.
		ChaseTarget();
	}
}
