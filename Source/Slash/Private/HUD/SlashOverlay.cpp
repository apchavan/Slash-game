// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SlashOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"


void USlashOverlay::SetHealthBarPercent(float Percent)
{
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(Percent);
	}
}

void USlashOverlay::SetStaminaBarPercent(float Percent)
{
	if (StaminaProgressBar)
	{
		StaminaProgressBar->SetPercent(Percent);
	}
}

void USlashOverlay::SetGold(int32 Gold)
{
	if (GoldText)
	{
		const FString GoldFString = FString::Printf(TEXT("%d"), Gold);
		const FText GoldFText = FText::FromString(GoldFString);
		GoldText->SetText(GoldFText);
	}
}

void USlashOverlay::SetSouls(int32 Souls)
{
	if (SoulsText)
	{
		const FString SoulsFString = FString::Printf(TEXT("%d"), Souls);
		const FText SoulsFText = FText::FromString(SoulsFString);
		SoulsText->SetText(SoulsFText);
	}
}

void USlashOverlay::SetWeaponDamage(float WeaponDamage)
{
	if (WeaponDamageText)
	{
		const FString WeaponDamageFString = FString::Printf(
			TEXT("%d"),
			static_cast<int32>(WeaponDamage)
		);
		const FText WeaponDamageFText = FText::FromString(WeaponDamageFString);
		WeaponDamageText->SetText(WeaponDamageFText);
	}
}
