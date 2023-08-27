// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/WeaponDamageComponent.h"
#include "HUD/WeaponDamage.h"
#include "Components/TextBlock.h"

void UWeaponDamageComponent::SetWeaponDamage(float WeaponDamage)
{
	if (!IsValid(WeaponDamageWidget))
	{
		WeaponDamageWidget = Cast<UWeaponDamage>(GetUserWidgetObject());
	}

	if (WeaponDamageWidget && WeaponDamageWidget->WeaponDamageText)
	{
		const FString WeaponDamageFString = FString::Printf(
			TEXT("%d"),
			static_cast<int32>(WeaponDamage)
		);
		const FText WeaponDamageFText = FText::FromString(WeaponDamageFString);

		WeaponDamageWidget->WeaponDamageText->SetText(WeaponDamageFText);
	}
}
