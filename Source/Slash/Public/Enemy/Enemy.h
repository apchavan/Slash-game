// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Interfaces/HitInterface.h"
#include "Characters/CharacterTypes.h"
#include "Enemy.generated.h"

class UHealthBarComponent;
class AAIController;
class UPawnSensingComponent;
class ASoul;
class AHealth;
class AWeapon;

UCLASS()
class SLASH_API AEnemy : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AEnemy();

	/** <AActor> */
	virtual void Tick(float DeltaTime) override;
	float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Destroyed() override;
	/** </AActor> */

	/** <IHitInterface> */
	virtual void GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter) override;
	/** </IHitInterface> */

protected:
	/** <AActor> */
	virtual void BeginPlay() override;
	/** </AActor> */

	/** <ABaseCharacter> */
	virtual void Die_Implementation() override;
	void SpawnSoul();
	void SpawnHealth();
	virtual void Attack() override;
	virtual bool CanAttack() override;
	virtual void AttackEnd() override;
	virtual void HandleDamage(float DamageAmount) override;
	/** </ABaseCharacter> */

	UFUNCTION(BlueprintCallable, Category = "Spawnable")
	void TrySpawningRandomWeapon();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	EEnemyState EnemyState = EEnemyState::EES_Patrolling;

private:

	/** AI Behavior */

	void InitializeEnemy();
	void CheckPatrolTarget();
	void CheckCombatTarget();
	void PatrolTimerFinished();  // Callback for `PatrolTimer`
	void HideHealthBar();
	void ShowHealthBar();
	void LoseInterest();
	void StartPatrolling();
	void ChaseTarget();
	bool IsOutsideCombatRadius();
	bool IsOutsideAttackRadius();
	bool IsInsideAttackRadius();
	bool IsChasing();
	bool IsAttacking();
	bool IsDead();
	bool IsEngaged();
	void ClearPatrolTimer();
	void StartAttackTimer();
	void ClearAttackTimer();
	bool InTargetRange(AActor* Target, double Radius);
	void MoveToTarget(AActor* Target);
	AActor* ChoosePatrolTarget();
	void SpawnDefaultWeapon();

	// Callback for OnPawnSeen in UPawnSensingComponent
	UFUNCTION()
	void PawnSeen(APawn* SeenPawn);

	UPROPERTY(VisibleAnywhere)
	UHealthBarComponent* HealthBarComponent;

	UPROPERTY(VisibleAnywhere)
	UPawnSensingComponent* PawnSensing;

	UPROPERTY(EditAnywhere, Category = Combat)
	TSubclassOf<AWeapon> WeaponClass;

	UPROPERTY(EditAnywhere, Category = Combat)
	double CombatRadius = 1000.0f;

	UPROPERTY(EditAnywhere, Category = Combat)
	double AttackRadius = 150.0f;

	UPROPERTY(EditAnywhere, Category = Combat)
	double AcceptanceRadius = 50.0f;

	UPROPERTY()
	AAIController* EnemyController;

	// Current patrol target
	UPROPERTY(EditInstanceOnly, Category = "AI Navigation")
	AActor* PatrolTarget;

	UPROPERTY(EditInstanceOnly, Category = "AI Navigation")
	TArray<AActor*> PatrolTargets;

	UPROPERTY(EditAnywhere)
	double PatrolRadius = 200.0f;

	FTimerHandle PatrolTimer;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float PatrolWaitMin = 5.0f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float PatrolWaitMax = 10.0f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float PatrollingSpeed = 125.0f;

	FTimerHandle AttackTimer;

	UPROPERTY(EditAnywhere, Category = Combat)
	float AttackMin = 0.5f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float AttackMax = 1.0f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ChasingSpeed = 300.0f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float DeathLifeSpan = 8.0f;

	UPROPERTY(EditAnywhere, Category = Combat)
	TSubclassOf<ASoul> SoulClass;

	UPROPERTY(EditAnywhere, Category = Combat)
	TSubclassOf<AHealth> HealthClass;

	UPROPERTY(EditDefaultsOnly, Category = Combat)
	TArray<TSubclassOf<AWeapon>> SpawnableRandomWeapons;
};
