// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item.generated.h"

class USphereComponent;
class UNiagaraComponent;
class UNiagaraSystem;

enum class EItemState : uint8
{
	EIS_Hovering,
	EIS_Equipped
};

UCLASS()
class SLASH_API AItem : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AItem();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sine Parameters")
	float Amplitude = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sine Parameters")
	float TimeConstant = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* ItemMesh;

	EItemState ItemState = EItemState::EIS_Hovering;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* Sphere;

	UPROPERTY(EditAnywhere)
	UNiagaraComponent* ItemEffect;

	template<typename T>
	T Avg(T first, T second);

	UFUNCTION(BlueprintPure)
	float TransformedSin();

	UFUNCTION(BlueprintPure)
	float TransformedCos();

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void SpawnPickupSystem();
	virtual void SpawnPickupSound();

	// Check & return whether `Component` is of type `UCapsuleComponent`.
	bool IsCapsuleComponent(UPrimitiveComponent* Component);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float RunningTime;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* PickupEffect;

	UPROPERTY(EditAnywhere)
	USoundBase* PickupSound;

	/*
	* UE Property Specifiers: https://docs.unrealengine.com/en-US/unreal-engine-uproperty-specifiers/
	* UPROPERTY(EditDefaultsOnly) macro exposes variable to Default Blueprint.
	* UPROPERTY(EditInstanceOnly) macro exposes variable to Instance Blueprint.
	* UPROPERTY(EditAnywhere) macro exposes variable to both Default & Instance Blueprint.
	* UPROPERTY(VisibleDefaultsOnly) macro make variable visible only/un-editable in Default Blueprint.
	* UPROPERTY(VisibleInstanceOnly) macro make variable visible only/un-editable in Instance Blueprint.
	* UPROPERTY(VisibleAnywhere) macro make variable visible only/un-editable in both Default & Instance Blueprint.
	*/
};

template<typename T>
inline T AItem::Avg(T first, T second)
{
	return (first + second) / 2;
}
