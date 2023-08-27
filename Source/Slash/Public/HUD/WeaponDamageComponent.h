// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "WeaponDamageComponent.generated.h"

/**
 * 
 */
UCLASS()
class SLASH_API UWeaponDamageComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	void SetWeaponDamage(float WeaponDamage);
	
private:
	UPROPERTY()
	class UWeaponDamage* WeaponDamageWidget;
};
