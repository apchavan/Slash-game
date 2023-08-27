// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Components/AttributeComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Characters/CharacterTypes.h"
#include "NiagaraFunctionLibrary.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseCharacter::GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter)
{
	if (IsAlive() && Hitter)
	{
		DirectionalHitReact(Hitter->GetActorLocation());
	}
	else {
		// Since IsAlive() is false, the character will die
		Die();
	}

	PlayHitSound(ImpactPoint);
	SpawnHitParticles(ImpactPoint);

	// Disable weapon collision to ensure `Hitter` will not hit without actual hitting
	// because 'DisableBoxCollision' animation notify may not executed from animation montage.
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseCharacter::Attack()
{
	// If the `CombatTarget` has a tag called 'Dead' then remove it.
	if (CombatTarget && CombatTarget->ActorHasTag(FName("Dead")))
	{
		CombatTarget = nullptr;
	}
}

void ABaseCharacter::Die_Implementation()
{
	// Add a 'Dead' tag to the character.
	Tags.Add(FName("Dead"));

	PlayDeathMontage();
}

int32 ABaseCharacter::PlayAttackMontage()
{
	return PlayRandomMontageSection(AttackMontage, AttackMontageSections);
}

int32 ABaseCharacter::PlayDeathMontage()
{
	const int32 Selection = PlayRandomMontageSection(DeathMontage, DeathMontageSections);

	TEnumAsByte<EDeathPose> Pose(Selection);

	if (Pose < EDeathPose::EDP_MAX)
	{
		DeathPose = Pose;
	}

	return Selection;
}

void ABaseCharacter::PlayDodgeMontage()
{
	if (DodgeMontage)
	{
		PlayMontageSection(DodgeMontage, FName("Default"));
	}
}

void ABaseCharacter::StopAttackMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && AttackMontage)
	{
		AnimInstance->Montage_Stop(0.25f, AttackMontage);
	}
}

FVector ABaseCharacter::GetTranslationWarpTarget()
{
	if (CombatTarget == nullptr) return FVector();

	const FVector CombatTargetLocation = CombatTarget->GetActorLocation();
	const FVector Location = GetActorLocation();

	// Get vector from `CombatTargetLocation` to `Location` of hitting actor (i.e. enemy) or
	// the one who uses the Motion Warping.
	FVector TargetToMe = (Location - CombatTargetLocation).GetSafeNormal();
	
	// Scale the normalized vector by `WarpTargetDistance`.
	TargetToMe *= WarpTargetDistance;

	// Get actual warp target location by doing (CombatTargetLocation + TargetToMe).
	return (CombatTargetLocation + TargetToMe);
}

FVector ABaseCharacter::GetRotationWarpTarget()
{
	if (CombatTarget)
	{
		return CombatTarget->GetActorLocation();
	}
	return FVector();
}

void ABaseCharacter::PlayHitReactMontage(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		AnimInstance->Montage_JumpToSection(SectionName, HitReactMontage);
	}
}

void ABaseCharacter::DirectionalHitReact(const FVector& ImpactPoint)
{
	const FVector Forward = GetActorForwardVector();

	// Lower Impact Point to the Enemy's Actor Location Z
	const FVector ImpactLowered(ImpactPoint.X, ImpactPoint.Y, GetActorLocation().Z);

	const FVector ToHit = (ImpactLowered - GetActorLocation()).GetSafeNormal();

	// For Dot product between Forward & ToHit,
	// Forward * ToHit = |Forward| * |ToHit| * cos(theta)
	// Since |Forward| = 1 and |ToHit| = 1,
	// Forward * ToHit = cos(theta)
	const double CosTheta = FVector::DotProduct(Forward, ToHit);

	// Take the inverse cosine (or arc-cosine) of cos(theta) to get theta, in radians
	double Theta = FMath::Acos(CosTheta);

	// Convert from radians to degrees
	Theta = FMath::RadiansToDegrees(Theta);

	// If CrossProduct points down, Theta should be negative
	const FVector CrossProduct = FVector::CrossProduct(Forward, ToHit);

	if (CrossProduct.Z < 0)
	{
		Theta *= -1;
	}

	// Play montage section depending on the angle of hit.
	FName Section("FromBack");

	if (Theta >= -45.f && Theta < 45.f)
	{
		Section = FName("FromFront");
	}
	else if (Theta >= -135.f && Theta < -45.f)
	{
		Section = FName("FromLeft");
	}
	else if (Theta >= 45.f && Theta < 135.f)
	{
		Section = FName("FromRight");
	}

	PlayHitReactMontage(Section);

	/*
	UKismetSystemLibrary::DrawDebugArrow(
		this,
		GetActorLocation(),
		GetActorLocation() + CrossProduct * 100.f,
		5.f,
		FLinearColor::Blue,
		5.f
	);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			2,
			5.f,
			FColor::Green,
			FString::Printf(TEXT("Theta: %f"), Theta)
		);
	}

	UKismetSystemLibrary::DrawDebugArrow(
		this,
		GetActorLocation(),
		GetActorLocation() + Forward * 60.f,
		5.f,
		FLinearColor::Red,
		5.f
	);
	UKismetSystemLibrary::DrawDebugArrow(
		this,
		GetActorLocation(),
		GetActorLocation() + ToHit * 60.f,
		5.f,
		FLinearColor::Green,
		5.f
	);
	*/
}

void ABaseCharacter::HandleDamage(float DamageAmount)
{
	if (Attributes)
	{
		Attributes->ReceiveDamage(DamageAmount);
	}
}

void ABaseCharacter::PlayMontageSection(UAnimMontage* Montage, const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && Montage)
	{
		AnimInstance->Montage_Play(Montage);
		AnimInstance->Montage_JumpToSection(SectionName, Montage);
	}
}

int32 ABaseCharacter::PlayRandomMontageSection(UAnimMontage* Montage, const TArray<FName>& SectionNames)
{
	if (SectionNames.Num() <= 0) return -1;

	const int32 MaxSectionIndex = SectionNames.Num() - 1;
	const int32 Selection = FMath::RandRange(0, MaxSectionIndex);
	PlayMontageSection(Montage, SectionNames[Selection]);

	return Selection;
}

void ABaseCharacter::DisableCapsule()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseCharacter::PlayHitSound(const FVector& ImpactPoint)
{
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			ImpactPoint
		);
	}
}

void ABaseCharacter::SpawnHitParticles(const FVector& ImpactPoint)
{
	if (HitParticles && GetWorld())
	{
		// Spawn blood particles if set for body.
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			HitParticles,
			ImpactPoint
		);
	}
	else if (FrictionParticles && GetWorld())
	{
		// Spawn friction effect for metallic bodies.
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			FrictionParticles,
			ImpactPoint
		);
	}
}

bool ABaseCharacter::CanAttack()
{
	return false;
}

void ABaseCharacter::AttackEnd()
{
}

void ABaseCharacter::DodgeEnd()
{
}

bool ABaseCharacter::IsAlive()
{
	return Attributes && Attributes->IsAlive();
}

void ABaseCharacter::DisableMeshCollision()
{
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseCharacter::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	if (EquippedWeapon && EquippedWeapon->GetWeaponBox())
	{
		EquippedWeapon->GetWeaponBox()->SetCollisionEnabled(CollisionEnabled);
		EquippedWeapon->IgnoreActors.Empty();
	}
}
