// Fill out your copyright notice in the Description page of Project Settings.


#include "LyraBuilding.h"

ALyraBuilding::ALyraBuilding()
{
	PrimaryActorTick.bCanEverTick = true;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	//BuildingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = BuildingMesh;

	//SetActorEnableCollision(false);
}

void ALyraBuilding::BeginPlay()
{
	Super::BeginPlay();
	
}

void ALyraBuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

float ALyraBuilding::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	BuildingHealth -= DamageAmount;
	if (BuildingHealth <= 0.0f)
	{
		OnDestroyed();
		Destroy();
	}
	
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ALyraBuilding::OnDestroyed()
{
	//Play sound
}

