// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Health.generated.h"

/**
 * 
 */
UCLASS()
class SLASH_API AHealth : public AItem
{
	GENERATED_BODY()
	
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:

	// Health count to add in Actor's health after pickup.
	UPROPERTY(EditAnywhere, Category = "Spawnable Health Properties")
	float HealthCount;

public:
	FORCEINLINE float GetHealthCount() const { return HealthCount; }
	FORCEINLINE void SetHealthCount(float NewHealthCount) { HealthCount = NewHealthCount; }
};
