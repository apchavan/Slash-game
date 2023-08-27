// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Weapon.h"
#include "Characters/SlashCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/HitInterface.h"
#include "NiagaraComponent.h"
#include "HUD/WeaponDamageComponent.h"

AWeapon::AWeapon()
{
	WeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Weapon Box"));
	WeaponBox->SetupAttachment(GetRootComponent());

	// Set Collision Presets for WeaponBox.
	WeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	WeaponBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	// Configure box trace objects.
	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(GetRootComponent());

	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(GetRootComponent());

	// Create & attach `DamageHUDComponent` to root component of class.
	DamageHUDComponent = CreateDefaultSubobject<UWeaponDamageComponent>(TEXT("DamageHUDComponent"));
	DamageHUDComponent->SetupAttachment(GetRootComponent());

	// Setup the `DamageHUDSphere` to control when to show/hide `DamageHUDComponent`.
	DamageHUDSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DamageHUDSphere"));
	DamageHUDSphere->SetSphereRadius(500.0f);
	DamageHUDSphere->SetupAttachment(GetRootComponent());
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	WeaponBox->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnBoxOverlap);

	if (DamageHUDComponent)
	{
		// Set weapon's damage HUD using `Damage` value.
		DamageHUDComponent->SetWeaponDamage(Damage);

		// Hide the damage HUD in the beginning.
		HideDamageHUDComponent();
	}

	if (DamageHUDSphere)
	{
		DamageHUDSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnDamageHUDSphereOverlap);
		DamageHUDSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnDamageHUDSphereEndOverlap);
	}
}

void AWeapon::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	ItemState = EItemState::EIS_Equipped;

	SetOwner(NewOwner);
	SetInstigator(NewInstigator);

	AttachMeshToSocket(InParent, InSocketName);

	// Disable collision on weapon's outer sphere once it's equipped to avoid playing equip sound again
	DisableSphereCollision();

	// Play sound while equipping/un-equipping weapon
	PlayEquipSound();

	// Disable Niagara particle effect
	DeactivateEmbers();

	// Remove the damage HUD detection sphere & component attached with weapon.
	if (DamageHUDSphere)
	{
		DamageHUDSphere->DestroyComponent();
		DamageHUDSphere = nullptr;
	}
	if (DamageHUDComponent)
	{
		DamageHUDComponent->DestroyComponent();
		DamageHUDComponent = nullptr;
	}
}

void AWeapon::DeactivateEmbers()
{
	if (ItemEffect)
	{
		ItemEffect->Deactivate();
	}
}

void AWeapon::DisableSphereCollision()
{
	if (Sphere)
	{
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWeapon::PlayEquipSound()
{
	if (EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquipSound,
			GetActorLocation()
		);
	}
}

void AWeapon::AttachMeshToSocket(USceneComponent* InParent, const FName& InSocketName)
{
	FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
	ItemMesh->AttachToComponent(InParent, TransformRules, InSocketName);
}

bool AWeapon::ActorIsSameType(AActor* OtherActor)
{
	return 
		(
			GetOwner()->ActorHasTag(FName("Enemy")) && OtherActor->ActorHasTag(FName("Enemy"))
		) ||
		(
			GetOwner()->ActorHasTag(FName("EngageableTarget")) && OtherActor->ActorHasTag(FName("EngageableTarget"))
		);
}

void AWeapon::ExecuteGetHit(FHitResult& BoxHit)
{
	IHitInterface* HitInterface = Cast<IHitInterface>(BoxHit.GetActor());
	if (HitInterface)
	{
		HitInterface->Execute_GetHit(BoxHit.GetActor(), BoxHit.ImpactPoint, GetOwner());
	}
}

void AWeapon::HideDamageHUDComponent()
{
	if (DamageHUDComponent)
	{
		DamageHUDComponent->SetVisibility(false);
	}
}

void AWeapon::ShowDamageHUDComponent()
{
	if (DamageHUDComponent)
	{
		DamageHUDComponent->SetVisibility(true);
	}
}

void AWeapon::BoxTrace(FHitResult& BoxHit)
{
	const FVector Start = BoxTraceStart->GetComponentLocation();
	const FVector End = BoxTraceEnd->GetComponentLocation();

	TArray<AActor*> ActorsToIgnore;

	// Add weapon itself to be ignored.
	ActorsToIgnore.Add(this);

	// Add weapon owner to be ignored.
	ActorsToIgnore.Add(GetOwner());

	for (AActor* Actor : IgnoreActors)
	{
		ActorsToIgnore.AddUnique(Actor);
	}

	UKismetSystemLibrary::BoxTraceSingle(
		this,
		Start,
		End,
		BoxTraceExtent,
		BoxTraceStart->GetComponentRotation(),
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		bShowBoxDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		BoxHit,
		true
	);
	IgnoreActors.AddUnique(BoxHit.GetActor());
}

void AWeapon::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ActorIsSameType(OtherActor)) return;

	FHitResult BoxHit;
	BoxTrace(BoxHit);

	if (BoxHit.GetActor())
	{
		if (ActorIsSameType(BoxHit.GetActor())) return;

		UGameplayStatics::ApplyDamage(BoxHit.GetActor(), Damage, GetInstigator()->GetController(), this, UDamageType::StaticClass());
		ExecuteGetHit(BoxHit);
		CreateFields(BoxHit.ImpactPoint);
	}
}

void AWeapon::OnDamageHUDSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->ActorHasTag(FName("EngageableTarget")))
	{
		// Show damage HUD on the weapon.
		ShowDamageHUDComponent();
	}
}

void AWeapon::OnDamageHUDSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->ActorHasTag(FName("EngageableTarget")))
	{
		// Hide damage HUD from the weapon.
		HideDamageHUDComponent();
	}
}
