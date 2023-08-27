// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Treasure.h"
#include "Interfaces/PickupInterface.h"

void ATreasure::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// Ensure collecting soul only when character gets close enough,
	// by confirming that the `OtherComp` is `UCapsuleComponent`.
	// If not, then return without collecting...

	if (!IsCapsuleComponent(OtherComp)) return;

	IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
	if (PickupInterface)
	{
		PickupInterface->AddGold(this);

		SpawnPickupSound();
		Destroy();
	}
}
