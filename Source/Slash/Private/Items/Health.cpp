// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Health.h"
#include "Interfaces/PickupInterface.h"

void AHealth::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Ensure collecting health only when character gets close enough,
	// by confirming that the `OtherComp` is `UCapsuleComponent`.
	// If not, then return without collecting...

	if (!IsCapsuleComponent(OtherComp)) return;

	IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
	if (PickupInterface)
	{
		PickupInterface->AddHealth(this);

		SpawnPickupSystem();
		SpawnPickupSound();
		Destroy();
	}
}
