// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Public/Subsystems/WorldSubsystem.h"

#include "AIStatesSubsystem.generated.h"

class UWidgetComponent;
class UAbilitySystemComponent;
class AAIStateController;

USTRUCT()
struct FAIActorsData
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAbilitySystemComponent>> ASCList;
};

/**
 * Subsystem for AI States
 */
UCLASS()
class LYRAGAME_API UAIStatesSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	inline static UAIStatesSubsystem* StaticInstance = nullptr;
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void RegisterAIActor(const AAIStateController* AIController);
	void UnregisterAIActor(const AAIStateController* AIController);

	// Debug
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	
	static void OnAIStatesDebugToggle(IConsoleVariable* Var);
	inline static bool bDebug = false;

	void UpdateDebugWidgets();
	void AddDebugWidget(const TWeakObjectPtr<UAbilitySystemComponent>& AIAsc);
	void RemoveDebugWidget(const TWeakObjectPtr<UAbilitySystemComponent>& AIAsc);
	
#endif

	// Array containing widget components for AI Actor debugging 
	UPROPERTY(BlueprintReadOnly)
	TArray<UWidgetComponent*> PerAIWidget_Debug;

	// Getter function retrieving active AI actor by index
	UFUNCTION(BlueprintCallable)
	UAbilitySystemComponent* GetActiveAIActorByIndex(int32 Index) const;

	// Getter function retrieving active AI actor count
	UFUNCTION(BlueprintCallable)
	int32 GetActiveAIActorCount() const { return ActiveAIActorsASCList.Num(); }
	
	const TArray<TObjectPtr<UAbilitySystemComponent>>& GetActiveAIActors() const { return ActiveAIActorsASCList; }
	
private:

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAbilitySystemComponent>> ActiveAIActorsASCList;
};
