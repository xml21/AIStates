// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "InstancedStruct.h"
#include "EnvironmentQuery/EnvQueryTypes.h"

#include "AIStatesSet.generated.h"

enum class EMovementGait : uint8;
struct FAISatesActorData;
struct FEnvNamedValue;

class AAIStateController;
class UEnvQuery;
class ULyraGameplayAbility;

namespace AIConditions
{
	static FName ConditionLeaderTagName = TEXT("AI.Condition.Leader");
	static FName ConditionRegularTagName = TEXT("AI.Condition.Regular");
	static FName ConditionAnyAITagName = TEXT("AI.Condition.AnyAI");
	static FName ConditionSelfTagName = TEXT("AI.Condition.Self");
	static FName ConditionPlayerTagName = TEXT("AI.Condition.Player");
	static FName RequestMoveToTagName = TEXT("AI.Request.MoveTo");
	static FName ApproachTargetTagName = TEXT("AI.Request.ApproachTarget");
	static FName RequestOrbitTagName = TEXT("AI.Request.Orbit");
	static FName RequestWaitTagName = TEXT("AI.Request.Wait");
	static FName RequestActionTagName = TEXT("AI.Request.Action");
}

UENUM(BlueprintType)
enum class EMovementGait : uint8
{
	Walking,
	Sprinting
};

// Base struct for the rest of conditional data
USTRUCT(BlueprintType)
struct LYRAGAME_API FAIStateConditionData
{
	GENERATED_BODY()
	
	virtual ~FAIStateConditionData() = default;
	
	virtual bool CheckCondition(AAIStateController* SourceAI) const { return false; }

	// Flag inverting AI state condition
	UPROPERTY(EditAnywhere, meta = (DisplayPriority=1))
	bool bInverted = false;

	// Tag for evaluation target (player, AI etc)
	UPROPERTY(EditAnywhere, meta = (DisplayPriority=2))
	FGameplayTag EvaluationTarget;
	
};

USTRUCT(BlueprintType, DisplayName="Max Distance To Chosen Target")
struct LYRAGAME_API FMaxDistanceToCondition : public FAIStateConditionData
{
	GENERATED_BODY()

	virtual bool CheckCondition(AAIStateController* SourceAI) const override;

	// Scalar defining maximum distance to target
	UPROPERTY(EditAnywhere)
	float MaxDistanceTo = 0.0f;
};

// Attribute Conditions
USTRUCT(BlueprintType, DisplayName="Has At Least Attribute Value")
struct LYRAGAME_API FAttributeChangeCondition : public FAIStateConditionData
{
	GENERATED_BODY()

	// Attribute that is a subject to condition change
	UPROPERTY(EditAnywhere)
	FGameplayAttribute Attribute;

	// Minimum value of change condition attribute
	UPROPERTY(EditAnywhere)
	float MinValue = 0.0f;
	
	virtual bool CheckCondition(AAIStateController* SourceAI) const override;
};

// Tag based conditions
//
USTRUCT(BlueprintType, DisplayName="Has Tags")
struct LYRAGAME_API FGameplayTagMultipleBasedCondition : public FAIStateConditionData
{
	GENERATED_BODY()

	// Tag container for AI state conditions
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer ConditionTag;

	// Flag determining if algorithm should check if the condition has any or all tags
	UPROPERTY(EditAnywhere)
	bool bHasAny = false;
	
	virtual bool CheckCondition(AAIStateController* SourceAI) const override;
};

// Counts instances of tags per Target ex. EngagedByEnemy Tags on Player - so we can limit too many AI from attacking
USTRUCT(BlueprintType, DisplayName="Has Tag Count")
struct LYRAGAME_API FTagCountCondition : public FAIStateConditionData
{
	GENERATED_BODY()

	// Counter tag condition
	UPROPERTY(EditAnywhere)
	FGameplayTag ConditionTag;

	// Minimum amount of tags for a specific condition to be true
	UPROPERTY(EditAnywhere)
	int MinCount = 0;
	
	virtual bool CheckCondition(AAIStateController* SourceAI) const override;
};

USTRUCT(BlueprintType, DisplayName="Count Enemies with Tag")
struct LYRAGAME_API FCountEnemiesWithTag : public FAIStateConditionData
{
	GENERATED_BODY()

	// Flag inverting checking condition for enemies counting 
	UPROPERTY(EditAnywhere)
	bool bInvertedHasTag = false;

	// Minimum amount of tags for a specific condition to be true
	UPROPERTY(EditAnywhere)
	int MinCount = 0;
	
	virtual bool CheckCondition(AAIStateController* SourceAI) const override;
};

USTRUCT(BlueprintType, DisplayName="Check Recently Changed Tag")
struct LYRAGAME_API FCheckRecentlyChangedTag : public FAIStateConditionData
{
	GENERATED_BODY()

	// Recently changed tag 
	UPROPERTY(EditAnywhere)
	FGameplayTag RecentTag;

	// Maximum time elapsed since the change
	UPROPERTY(EditAnywhere)
	int MaxTimePassed = 0;
	
	virtual bool CheckCondition(AAIStateController* SourceAI) const override;
};

// InterruptibleActionData used for tags with conditions to interrupt
USTRUCT(BlueprintType)
struct LYRAGAME_API FAIStateConditionsVariant
{
	GENERATED_BODY()

	// Name for variant title
	UPROPERTY(EditAnywhere, meta = (EditCondition = false))
	FName VariantTitle = "OR";

	// Array for AI state conditions variant 
	UPROPERTY(Category="Conditions", EditAnywhere, meta = (BaseStruct = "/Script/LyraGame.AIStateConditionData", ExcludeBaseStruct))
	TArray<FInstancedStruct> ConditionsVariant;
};

// Struct containing variant conditions for AI state interruptions
USTRUCT(BlueprintType)
struct LYRAGAME_API FAIInterruptibleActionData
{
	GENERATED_BODY()

	FAIInterruptibleActionData() {}

	// Array of variant conditions to interrupt
	UPROPERTY(EditAnywhere, meta = (TitleProperty = "VariantTitle"))
	TArray<FAIStateConditionsVariant> ConditionsToInterrupt;

	// Minimum duration of variant conditions to interrupt
	UPROPERTY(EditAnywhere, meta = (DisplayPriority=2))
	float MinDuration = 0.0f;

	// Maximum duration of variant conditions to interrupt
	UPROPERTY(EditAnywhere, meta = (DisplayPriority=3))
	float MaxDuration = 0.0f;
};

// Structure containing data related to character movement and animation systems 
USTRUCT(BlueprintType)
struct LYRAGAME_API FMovementGaitData
{
	GENERATED_BODY()

	// Flag enabling range based gait
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bRangeBasedGait = false;

	// Enum defining type of movement
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "!bRangeBasedGait"))
	EMovementGait MovementGait = EMovementGait::Walking;

	// Maximum distance for a movement to be considered as walking
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bRangeBasedGait"))
	float WalkingMaxDistance = 500.0f;

	// Maximum distance for a movement to be considered as sprint
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bRangeBasedGait"))
	float SprintingMaxDistance = 1000.0f;
};

// Wrapper struct associating MovementGait and Interruptible data. Used to specify approach type for AI
USTRUCT(BlueprintType)
struct LYRAGAME_API FApproachTargetData
{
	GENERATED_BODY()

	// Movement gait data
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FMovementGaitData MovementGaitData;

	// Movement gait data
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAIInterruptibleActionData InterruptibleData;
};

// Abilities - base interface for AI State Ability types
USTRUCT(BlueprintType)
struct LYRAGAME_API FAIStateActionData
{
	GENERATED_BODY()
	
	virtual ~FAIStateActionData() = default;
	
	virtual TSubclassOf<ULyraGameplayAbility> Activate(AAIStateController* SourceAI) { return {}; }
	
	virtual FString GetAbilityName() {return FString();}
	virtual float GetMaxDuration() const { return false; }
	virtual float GetDesiredDistance() const { return false; }
	
	virtual EMovementGait GetDesiredMovementGait() const { return EMovementGait::Walking; }
	virtual FMovementGaitData GetMovementGaitData() const { return {}; }
	
	virtual bool CanAbilityBeActivated(AAIStateController* SourceAI) const { return false; }
	virtual bool CanAbilityBeInterrupted() {return false;}
	virtual bool ShouldAbilityBeInterrupted(AAIStateController* SourceController) { return false; }

	virtual bool ShouldFollowTarget() const { return false; }
	virtual bool ShouldFocusOnTarget() const { return false; }
	
	virtual UEnvQuery* GetEQS() const { return nullptr; }
	virtual TArray<FEnvNamedValue> GetEQSParams() const { return {}; }
	virtual bool GetApproachTargetData(FApproachTargetData& OutApproachTargetData) const { return false; } 

	// AI state probability weight
	UPROPERTY(EditAnywhere, meta = (DisplayPriority=1))
	float ProbabilityWeight = 0.0f;
};

// Struct defining interruptible ability for AI state
USTRUCT(BlueprintType)
struct LYRAGAME_API FAIStateInterruptibleAbility : public FAIStateActionData
{
	GENERATED_BODY()

	virtual ~FAIStateInterruptibleAbility() override = default;

	virtual TSubclassOf<ULyraGameplayAbility> Activate(AAIStateController* SourceAI) override;
	
	virtual bool CanAbilityBeInterrupted() override {return InterruptibleActionData.ConditionsToInterrupt.Num() > 0;}
	virtual bool ShouldAbilityBeInterrupted(AAIStateController* SourceController) override;
	virtual float GetMaxDuration() const override;

	// AI state interruptible action data 
	UPROPERTY(EditAnywhere)
	FAIInterruptibleActionData InterruptibleActionData;

	FName InterruptibleAbilityTagName;
};

// Struct defining weighted ability for AI state
USTRUCT(BlueprintType)
struct LYRAGAME_API FWeightedAbility : public FAIStateActionData
{
	GENERATED_BODY()
	
	virtual TSubclassOf<ULyraGameplayAbility> Activate(AAIStateController* SourceAI) override;
	virtual bool CanAbilityBeActivated(AAIStateController* SourceAI) const override;
	virtual FString GetAbilityName() override;
	virtual bool GetApproachTargetData(FApproachTargetData& OutApproachTargetData) const override;

	// Gameplay ability class
	UPROPERTY(EditAnywhere)
	TSubclassOf<ULyraGameplayAbility> AbilityClass;

	// Flag enabling usage of custom approach target data
	UPROPERTY(EditAnywhere,  meta = (InlineEditConditionToggle))
	bool bCustomApproachTargetData = false;

	// Custom approach target data. Used only if bCustomApproachTargetData is true
	UPROPERTY(EditAnywhere,  meta = (EditCondition = "bCustomApproachTargetData"))
	FApproachTargetData ApproachTargetData;
};

USTRUCT(BlueprintType)
struct LYRAGAME_API FMoveToLocationAbility : public FAIStateInterruptibleAbility
{
	GENERATED_BODY()

	FMoveToLocationAbility() { InterruptibleAbilityTagName = AIConditions::RequestMoveToTagName; }
	
	virtual bool CanAbilityBeActivated(AAIStateController* SourceAI) const override { return true; }
	virtual FString GetAbilityName() override { return TEXT("Move To Location");}
	virtual EMovementGait GetDesiredMovementGait() const override { return MovementGaitData.MovementGait; }
	virtual FMovementGaitData GetMovementGaitData() const override { return MovementGaitData; };
	virtual bool ShouldFollowTarget() const override { return bFollowTarget; }
	virtual bool ShouldFocusOnTarget() const override { return bFocusOnTarget; }

	virtual UEnvQuery* GetEQS() const override { return EnvQuery; };
	virtual TArray<FEnvNamedValue> GetEQSParams() const override { return QueryParams; };

	// Navmesh EnvQuery
	UPROPERTY(EditAnywhere)
	TObjectPtr<UEnvQuery> EnvQuery = nullptr;

	// Data related to character movement and animation systems
	UPROPERTY(EditAnywhere)
	FMovementGaitData MovementGaitData;

	// Additional query params. Used for example for EQS.
	UPROPERTY(EditAnywhere, meta=(ForceInlineRow))
	TArray<FEnvNamedValue> QueryParams;

	// Flag enabling target following
	UPROPERTY(EditAnywhere)
	bool bFollowTarget = false;

	// Flag enabling target focusing
	UPROPERTY(EditAnywhere)
	bool bFocusOnTarget = false;
};

USTRUCT(BlueprintType)
struct LYRAGAME_API FWaitAbility : public FAIStateInterruptibleAbility
{
	GENERATED_BODY()

	FWaitAbility() {InterruptibleAbilityTagName = AIConditions::RequestWaitTagName;}

	virtual bool CanAbilityBeActivated(AAIStateController* SourceAI) const override { return true; }
	virtual FString GetAbilityName() override { return "Wait"; }
};

USTRUCT(BlueprintType)
struct LYRAGAME_API FOrbitAbility : public FAIStateInterruptibleAbility
{
	GENERATED_BODY()

	FOrbitAbility() { InterruptibleAbilityTagName = AIConditions::RequestOrbitTagName; }

	virtual bool CanAbilityBeActivated(AAIStateController* SourceAI) const override { return true; }
	virtual FString GetAbilityName() override {return "Orbit";}
	virtual float GetDesiredDistance() const override {return DesiredDistance;}

	// Orbit ability target distance
	UPROPERTY(EditAnywhere)
	float DesiredDistance = 0.0f;
};

USTRUCT(BlueprintType)
struct LYRAGAME_API FApproachTargetAbility : public FAIStateInterruptibleAbility
{
	GENERATED_BODY()

	FApproachTargetAbility() { InterruptibleAbilityTagName = AIConditions::ApproachTargetTagName; }
	
	virtual FString GetAbilityName() override {return "Approach Target";}
	virtual FMovementGaitData GetMovementGaitData() const override { return MovementGaitData; }

	// Data related to approaching character movement and animation systems 
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FMovementGaitData MovementGaitData;
};

// Wrapper struct for AI state action data ability
USTRUCT(BlueprintType)
struct LYRAGAME_API FAIStateAbilityNamedWrapper
{
	GENERATED_BODY()

	// AI state ability data
	UPROPERTY(EditAnywhere, meta = (BaseStruct = "/Script/LyraGame.AIStateActionData", ExcludeBaseStruct))
	FInstancedStruct Ability;
};

// State Config Data - InstancedStructs for DataAsset
USTRUCT(BlueprintType)
struct LYRAGAME_API FAIStateDataConfig
{
	GENERATED_BODY()

	// AI state name
	UPROPERTY(EditAnywhere)
	FName StateName;

	// AI state weight
	UPROPERTY(EditAnywhere)
	float StateWeight = 0.0f;

	// Flag enabling AI state blocking on exit	
	UPROPERTY(EditAnywhere)
	bool bBlockOnExit = false;

	// Array of AI state entry conditions
	UPROPERTY(Category="Conditions", EditAnywhere, meta = (BaseStruct = "/Script/LyraGame.AIStateConditionData", ExcludeBaseStruct))
	TArray<FInstancedStruct> Conditions;

	// Array of AI state abilities
	UPROPERTY(EditAnywhere)
	TArray<FAIStateAbilityNamedWrapper> Abilities;
};

// State Runtime Data - explicit types - no casting required
USTRUCT(BlueprintType)
struct LYRAGAME_API FAIStateRuntimeData
{
	GENERATED_BODY()
	
	float StateWeight = 0.0f;
	
	bool bStateBlocked = false;
	bool bMakeBlockedOnExit = false;
	
	TArray<TSharedPtr<FAIStateConditionData>> Conditions;
	TArray<TSharedPtr<FAIStateActionData>> Abilities;
};
/**
 *  Data Asset for configuring states
 */
UCLASS()
class LYRAGAME_API UAIStatesSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// AI state update rate used for testing purposes
	UPROPERTY(EditAnywhere)
	float AIStatesUpdateRate_TESTING = 0.15f;

	// Default movement approach data
	UPROPERTY(EditAnywhere)
	FApproachTargetData DefaultApproachData;

	// Array of AI states for this character
	UPROPERTY(EditAnywhere, meta = (TitleProperty = "StateName"))
	TArray<FAIStateDataConfig> States;
};
