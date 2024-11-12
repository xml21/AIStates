// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LyraBuilding.generated.h"

// TODO Add saving prevention if properties are not set

UCLASS(BlueprintType, Blueprintable)
class LYRAGAME_API ALyraBuilding : public AActor
{
	GENERATED_BODY()

public:
	ALyraBuilding();

	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator,
			AActor* DamageCauser) override;
	
	void OnDestroyed();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	UStaticMeshComponent* BuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	float BuildingHealth = 100.f;	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;	
};
