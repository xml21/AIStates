// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/PawnComponent.h"
#include "LyraBuildingManagerComponent.generated.h"

class ALyraPlayerController;
class ALyraBuilding;
class ULyraAbilitySystemComponent;

UENUM(BlueprintType)
enum class ELyraBuildModeState : uint8
{
	Disabled,
	Enabled
};

/**
 * 
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class LYRAGAME_API ULyraBuildingManagerComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	ULyraBuildingManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	void ToggleBuildingMode();
	void Update(float DeltaTime);
	void PlaceBuilding();
	void IncrementBuildingType();	
	void DecrementBuildingType();
	bool IsGhostBuildingOverlapping() const;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE ELyraBuildModeState GetBuildMode() const { return BuildModeState; }

	//Change to soft pointer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TArray<TObjectPtr<UStaticMesh>> Buildings;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TObjectPtr<UMaterial> EnabledGhostBuildingMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TObjectPtr<UMaterial> DisabledGhostBuildingMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building | Abilities")
	FGameplayTagContainer AbilitiesToBlock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building | Sound Cues")
	FGameplayTag OnBuildingPlacedSoundCue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	float BuildingRange = 500.f;

private:
	
	UPROPERTY()
	TWeakObjectPtr<ALyraBuilding> GhostBuilding = nullptr;
	
	void OnBuildingPlaced(const ALyraBuilding* Building);
	void OnBuildingModeChanged();
	FVector GetCrosshairLocationIn3D() const;
	FVector GetCrosshairLocationProjection(FVector CrosshairLocation) const;
	TObjectPtr<ULyraAbilitySystemComponent> GetPlayerAbilitySystemComponent() const;

	FVector CurrentGhostBuildingLocation = FVector::ZeroVector;
	FVector TargetGhostBuildingLocation = FVector::ZeroVector;
	ELyraBuildModeState BuildModeState = ELyraBuildModeState::Disabled;
	int32 CurrentBuildingIndex = 0;
};
