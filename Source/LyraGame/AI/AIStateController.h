
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIStates/AIStatesSet.h"
#include "GameplayEffectTypes.h"

#include "AIStateController.generated.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
struct LYRAGAME_API FAIStatesCVars
{
	static TAutoConsoleVariable<int32> CVarAIStatesDebug;
};
#endif

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnSelectedAbilitySignature_Debug, FName, PreviousStateName, FName, PreviousAbilityName, FName, CurrentStateName, FName, CurrentAbilityName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnChangedActiveAbilityStateSignature_Debug, FName, AbilityState);

struct FAIStateConditionData;
struct FAIStateRuntimeData;
class ULyraGameplayAbility;
class UAIStatesSet;

/*
 *	Default Controller for AI state based bots
 */
UCLASS()
class LYRAGAME_API AAIStateController
	: public AAIController
{
	GENERATED_BODY()

public:

	AAIStateController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()) {}

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetPawn(APawn* InPawn) override;

	// ----------------------------------------------------------------------------------------------------------------
	// IGenericTeamAgentInterface
	// ----------------------------------------------------------------------------------------------------------------
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	// Function setting new target member for this AI controller as well as updating Blackboard entry
	UFUNCTION(BlueprintCallable)
	void SetTarget(AActor* NewTarget);
	AActor* GetTarget() const { return CurrentTarget; };

	// Function clearing current target member for this AI controller as well as removing Blackboard entry
	UFUNCTION(BlueprintCallable)
	void ClearTarget();

	// Function clearing search location in Blackboard
	UFUNCTION(BlueprintCallable)
	void AbortSearch();

	// Function clearing search location and target actor in Blackboard
	UFUNCTION(BlueprintCallable)
	void ReturnHome(const FTransform& HomeTransform);

	// Function retrieving currently used AI states from AIStatesSetConfig and updating member variable  
	UFUNCTION(BlueprintCallable, Category=AI)
	void UpdateAIState();

	// Function enabling AI states updating
	UFUNCTION(BlueprintCallable, Category=AI)
	void ClearActiveInterruptibleAction() { bActiveInterruptibleAction = false; }

	// Getter function retrieving available abilities
	UFUNCTION(BlueprintCallable, Category=AI)
	bool GetWeightedAbility_STATES(TSubclassOf<ULyraGameplayAbility>& OutAbilityClass);

	// Getter function retrieving ready ability by its weight
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category=AI)
	bool GetActivatableAbilityByWeight(const FGameplayTagContainer& AbilityTags, AActor* Target, bool bCheckAffection, FGameplayTag AlwaysCheckAffectionForTag, TSubclassOf<ULyraGameplayAbility>& OutAbility) const;
	
	bool GetWeightedAbility(const TArray<TSubclassOf<ULyraGameplayAbility>>& Abilities, TSubclassOf<ULyraGameplayAbility>& OutAbilityClass) const;
	const TArray<FAIStateRuntimeData>& GetAIStatesData() const { return AIStates ;}
	bool SetupAIStatesFromConfig();
	void StartApproachingTarget();

protected:

	// Helper function determining if there is a line of sight from one location to another
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	bool HasLineOfSightToLocation(const FVector& FromLocation, const FVector& ToLocation, const TArray<AActor*>& ActorsToIgnore, float TraceSphereRadius = 30.f) const;

	// Helper function determining if there is a line of sight from one location to target actor
	UFUNCTION(BlueprintCallable)
	bool HasLineOfSightToActor(AActor* Other, FVector ViewPoint = FVector::ZeroVector, float TraceSphereRadius = 30.f) const;

	// Helper function determining if there is any path on navmesh from one location to another
	UFUNCTION(BlueprintCallable)
	bool HasDirectNavPathToLocation(const FVector& FromLocation, const FVector& ToLocation, float OffsetFromTarget = 0.f) const;

	// Helper function determining if there is direct path on navmesh from this controller to other actor
	UFUNCTION(BlueprintCallable)
	bool HasDirectNavPathToActor(const AActor* Other, float OffsetFromTarget = 0.f) const;

	UFUNCTION()
	void OnDeathStarted();
	
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget = nullptr;

	bool bDebug = false;

public:
	
	// Blackboard Values Key Names
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Blackboard)
	FName TargetActorKeyName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Blackboard)
	FName SelectedAbilityClassKeyName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Blackboard)
	FName SearchLocationKeyName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Blackboard)
	FName RoamLocationKeyName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Blackboard)
	FName DesiredGaitKeyName;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FName ShouldOrbitKeyName;
	//
	
	FApproachTargetData DefaultApproachTargetData;
	FApproachTargetData CurrentAbilityApproachTargetData;

	FApproachTargetAbility DefaultApproachAbility;

	// Debug variables
	UFUNCTION(BlueprintCallable)
	void BroadcastOnInterruptibleAbilityUpdate_Debug(const FName& Text);
	
	UPROPERTY(BlueprintAssignable)
	FOnSelectedAbilitySignature_Debug OnSelectedAbilityDelegate_Debug;
	
	UPROPERTY(BlueprintAssignable)
	FOnChangedActiveAbilityStateSignature_Debug OnChangedActiveAbilityStateDelegate_Debug;
	
	UPROPERTY(BlueprintAssignable)
	FOnChangedActiveAbilityStateSignature_Debug OnInterruptibleAbilityUpdate_Debug;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> PerAIWidgetClass_Debug;
	//

private:

	UPROPERTY()
	TArray<FAIStateRuntimeData> AIStates;

	UPROPERTY()
	TWeakObjectPtr<UAIStatesSet> AIStatesSetConfig;

	FTimerHandle UpdateAIStateTimerHandle;
	// Debug variables
	FName PreviousState_Debug;
	FName PreviousAbility_Debug;
	//
	int CurrentAIStateIndex = 0;
	bool bActiveInterruptibleAction = false;
	bool bStatesUpdateRequested = false;
};
