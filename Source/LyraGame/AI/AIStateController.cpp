#include "AI/AIStateController.h"

#include "AIStates/AIStatesSet.h"
#include "AIStates/AIStatesSubsystem.h"
#include "AICharacter.h"

#include "AbilitySystemGlobals.h"
#include "NavigationSystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "InstancedStruct.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
TAutoConsoleVariable<int32> FAIStatesCVars::CVarAIStatesDebug(
		TEXT("lyra.aistates.debug"),
		0,
		TEXT("Show AI States debug info.\n")
		TEXT("0 = off\n")
		TEXT("1 = on\n"),
		ECVF_Cheat);
#endif

#define BIND_PERCEPTION_ATTRIBUTE_EVENT(AttributeName) \
	FGameplayAttribute AttributeName##Attribute = UPerceptionAttributeSet::Get##AttributeName##Attribute(); \
	Perception->Set##AttributeName(ASC->GetNumericAttribute(AttributeName##Attribute)); \
	ChangeDelegate = &ASC->GetGameplayAttributeValueChangeDelegate(AttributeName##Attribute); \
	if (!ChangeDelegate->IsBoundToObject(this)) \
	{ \
		ChangeDelegate->AddUObject(this, &ThisClass::On##AttributeName##AttributeChanged); \
	}

#define UNBIND_PERCEPTION_ATTRIBUTE_EVENT(AttributeName) \
	FGameplayAttribute AttributeName##Attribute = UPerceptionAttributeSet::Get##AttributeName##Attribute(); \
	ChangeDelegate = &ASC->GetGameplayAttributeValueChangeDelegate(AttributeName##Attribute); \
	if (ChangeDelegate->IsBoundToObject(this)) \
	{ \
		ChangeDelegate->RemoveAll(this); \
	}

void AAIStateController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	const UWorld* const WorldLocal = GetWorld();
	if(ensureMsgf(WorldLocal != nullptr, TEXT("World is nullptr!")))
	{
		WorldLocal->GetTimerManager().ClearTimer(UpdateAIStateTimerHandle);
	}

	for(FAIStateRuntimeData& State : AIStates)
	{
		State.Conditions.Empty();
	}
	AIStates.Empty();
	
	Super::EndPlay(EndPlayReason);
}

void AAIStateController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	const UWorld* const WorldLocal = GetWorld();
	if(ensureMsgf(WorldLocal != nullptr, TEXT("World is nullptr!")))
	{
		return;
	}

	// AI States setup
	auto* AICharacter = Cast<AAICharacter>(GetPawn());
	if (AICharacter && AICharacter->AIStatesSetConfig /* temp testing */)
	{
		if(SetupAIStatesFromConfig())
		{
			// For things like that cant be bound easily to tags - like PlayerDistance
			WorldLocal->GetTimerManager().SetTimer(UpdateAIStateTimerHandle, this, &ThisClass::UpdateAIState, AICharacter->AIStatesSetConfig->AIStatesUpdateRate_TESTING , true);
		}
	}

	auto* AIStatesSubsystem = GetWorld()->GetSubsystem<UAIStatesSubsystem>();
	if(ensureMsgf(AIStatesSubsystem != nullptr, TEXT("AIStatesSubsystem is nullptr!")))
	{
		AIStatesSubsystem->RegisterAIActor(this);
	}
}

void AAIStateController::BroadcastOnInterruptibleAbilityUpdate_Debug(const FName& Text)
{
	OnInterruptibleAbilityUpdate_Debug.Broadcast(Text);
}

bool AAIStateController::HasDirectNavPathToActor(const AActor* Other, const float OffsetFromTarget) const
{
	if (GetPawn() == nullptr || Other == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AAIStateController::HasDirectNavPathToActor - Either owner pawn or target is invalid!"))
		return false;
	}

	const FVector OwnLocation = GetPawn()->GetActorLocation();
	const FVector OtherLocation = Other->GetTargetLocation(GetPawn());

	return HasDirectNavPathToLocation(OwnLocation, OtherLocation, OffsetFromTarget);
}

bool AAIStateController::HasDirectNavPathToLocation(const FVector& FromLocation, const FVector& ToLocation, float OffsetFromTarget) const
{
	const FVector ToTarget = ToLocation - FromLocation;
	if (ToTarget.Size() > OffsetFromTarget)
	{
		const UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		if(ensureMsgf(NavSystem != nullptr, TEXT("NavSystem is nullptr!")))
		{
			return false;
		}

		const FVector NavTargetLocation = ToLocation - ToTarget.GetSafeNormal() * OffsetFromTarget;
		FVector OutHitLocation;

		return !NavSystem->NavigationRaycast(GetPawn(), FromLocation, NavTargetLocation, OutHitLocation, DefaultNavigationFilterClass, GetPawn()->GetController());
	}
	else
	{
		return true;
	}
}

bool AAIStateController::HasLineOfSightToActor(AActor* Other, FVector ViewPoint, const float TraceSphereRadius) const
{
	if (GetPawn() == nullptr || Other == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AAIStateController::HasLineOfSightToActor - Either owner pawn or target is invalid!"))
		return false;
	}

	const FVector TargetLocation = Other->GetTargetLocation(GetPawn());
	if (ViewPoint.IsZero())
	{
		ViewPoint = GetPawn()->GetActorLocation();
	}

	TArray<AActor*> ActorToIgnore;
	ActorToIgnore.Add(Other);

	return HasLineOfSightToLocation(ViewPoint, TargetLocation, ActorToIgnore, TraceSphereRadius);
}

bool AAIStateController::HasLineOfSightToLocation(const FVector& FromLocation, const FVector& ToLocation, const TArray<AActor*>& ActorsToIgnore, float TraceSphereRadius) const
{
	const UWorld* const WorldLocal = GetWorld();
	if(ensureMsgf(WorldLocal != nullptr, TEXT("World is nullptr!")))
	{
		return false;
	}
	
	FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(WeaponLineOfSight), true, this->GetPawn());
	CollisionParams.AddIgnoredActors(ActorsToIgnore);

	bool bHit = WorldLocal->SweepTestByChannel(FromLocation, ToLocation, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(TraceSphereRadius), CollisionParams);
	if (!bHit)
	{
		return true;
	}

	return false;
}

void AAIStateController::ClearTarget()
{
	SetTarget(nullptr);
}

void AAIStateController::SetTarget(AActor* NewTarget)
{
	if (CurrentTarget != NewTarget)
	{
		if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
		{
			if (NewTarget)
			{
				BlackboardComp->SetValueAsObject(TargetActorKeyName, NewTarget);
				BlackboardComp->ClearValue(SearchLocationKeyName);
			}
			else
			{
				BlackboardComp->ClearValue(TargetActorKeyName);
				
				if (CurrentTarget)
				{
					BlackboardComp->SetValueAsVector(SearchLocationKeyName, CurrentTarget->GetActorLocation());
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AAIStateController::SetTarget - Blackboard component for this controller is invalid!"))
		}
		
		CurrentTarget = NewTarget;
	}
}

void AAIStateController::OnDeathStarted()
{
	auto* AIStatesSubsystem = GetWorld()->GetSubsystem<UAIStatesSubsystem>();
	if(ensureMsgf(AIStatesSubsystem != nullptr, TEXT("AI States subsystem is nullptr!")))
	{
		AIStatesSubsystem->UnregisterAIActor(this);
	}
}

void AAIStateController::AbortSearch()
{
	if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
	{
		BlackboardComp->ClearValue(SearchLocationKeyName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AAIStateController::AbortSearch - Blackboard component for this controller is invalid!"))
	}
}

void AAIStateController::ReturnHome(const FTransform& HomeTransform)
{	
	CurrentTarget = nullptr;

	if(UBlackboardComponent* BlackBoard = GetBlackboardComponent())
	{
		BlackBoard->ClearValue(SearchLocationKeyName);
		BlackBoard->ClearValue(TargetActorKeyName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AAIStateController::AbortSearch - Blackboard component for this controller is invalid!"))
	}
}

void AAIStateController::OnPossess(APawn* InPawn)
{
	const AAICharacter* AICharacter = Cast<AAICharacter>(InPawn);
	if (AICharacter && AICharacter->BTAsset)
	{
		RunBehaviorTree(AICharacter->BTAsset);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AAIStateController::AbortSearch - Blackboard component for this controller is invalid!"))
	}

	Super::OnPossess(InPawn);
}

void AAIStateController::SetGenericTeamId(const FGenericTeamId& InTeamID)
{
	IGenericTeamAgentInterface* ControlledPawn = GetPawn<IGenericTeamAgentInterface>();
	if (ControlledPawn != nullptr)
	{
		ControlledPawn->SetGenericTeamId(InTeamID);
	}
}

FGenericTeamId AAIStateController::GetGenericTeamId() const
{
	IGenericTeamAgentInterface* ControlledPawn = GetPawn<IGenericTeamAgentInterface>();
	if (ControlledPawn != nullptr)
	{
		return ControlledPawn->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

void AAIStateController::UpdateAIState()
{
	if(CurrentTarget == nullptr || bActiveInterruptibleAction)
	{
		return;
	}
	
	TArray<TPair<int32, float>> AvailableStateIndexesWithWeight;
	float AccumulatedWeights = 0.0f;

	// Collect Available States
	for(int32 StateIndex = 0; StateIndex < AIStates.Num(); StateIndex++)
	{
		const auto& [StateWeight, bStateBlocked, bMakeBlockedOnExit, Conditions, Abilities] = AIStates[StateIndex];
		if(bStateBlocked)
		{
			continue;
		}

		bool bIsStateAvailable = true;
		
		for(const TSharedPtr<FAIStateConditionData> AIStateCondition : Conditions)
		{
			if(AIStateCondition == nullptr || AIStateCondition->CheckCondition(this) == false)
			{
				bIsStateAvailable = false;
				break;
			}
		}
			
		if(bIsStateAvailable)
		{
			AccumulatedWeights += StateWeight;
			AvailableStateIndexesWithWeight.Add({StateIndex, StateWeight});
		}
	}

	// Setup probability partitions per ability
	// Replace weights with probability thresholds
	for(int32 i = 0; i < AvailableStateIndexesWithWeight.Num(); i++)
	{
		TPair<int32, float>& CurrentStateProbabilityData = AvailableStateIndexesWithWeight[i];
		
		const float PercentageChance =  CurrentStateProbabilityData.Value / AccumulatedWeights;
		if(i > 0)
		{
			const float LastChanceThreshold = AvailableStateIndexesWithWeight[i-1].Value;
			CurrentStateProbabilityData.Value = LastChanceThreshold + PercentageChance;
		}
		else
		{
			CurrentStateProbabilityData.Value = PercentageChance;
		}
	}

	// Pick state with ChanceThreshold greater or equal than random chance
	const float RandomChanceThreshold =  FMath::RandRange(0.0f, 1.0f);

	for(const TPair<int32, float>& AvailableStateData : AvailableStateIndexesWithWeight)
	{
		if(AvailableStateData.Value >= RandomChanceThreshold)
		{
			if(CurrentAIStateIndex != AvailableStateData.Key)
			{
				auto& OldState = AIStates[CurrentAIStateIndex];
				OldState.bStateBlocked = OldState.bMakeBlockedOnExit;
				
				CurrentAIStateIndex = AvailableStateData.Key;
			}
			break;
		}
	}
}

bool AAIStateController::SetupAIStatesFromConfig()
{
	const auto* AICharacter = Cast<AAICharacter>(GetPawn());
	if (!AICharacter || !AICharacter->AIStatesSetConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("AAIStatesController::SetupAIStatesFromConfig - couldn't access AIStatesSet DataAsset."))
		return false;
	}
	
	AIStatesSetConfig = AICharacter->AIStatesSetConfig;

	TObjectPtr<UAbilitySystemComponent> OwnerASC = AICharacter->GetAbilitySystemComponent();
	if (IsValid(OwnerASC) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("AAIStatesController::SetupAIStatesFromConfig - AI Character AbilitySystemComponent is invalid."))
		return false;
	}

	AIStates.Empty();
	
	for(FAIStateDataConfig& AIStateData : AIStatesSetConfig->States)
	{
		auto& [StateWeight, bStateBlocked, bMakeBlockedOnExit, Conditions, Abilities] =
			AIStates.Add_GetRef({ AIStateData.StateWeight, false, AIStateData.bBlockOnExit, {}, {} });
			
		for(FInstancedStruct& ConditionInstancedStruct : AIStateData.Conditions)
		{
			Conditions.Add(MakeShareable(ConditionInstancedStruct.GetMutablePtr<FAIStateConditionData>()));
		}
			
		for(auto& [Ability] : AIStateData.Abilities)
		{
			if(auto* StateActionData = Ability.GetMutablePtr<FAIStateActionData>())
			{
				Abilities.Add(MakeShareable(StateActionData));
			}
		}
	}

	DefaultApproachTargetData = AIStatesSetConfig->DefaultApproachData;
	
	return true;
}

void AAIStateController::StartApproachingTarget()
{
	DefaultApproachAbility.MovementGaitData = CurrentAbilityApproachTargetData.MovementGaitData;
	DefaultApproachAbility.InterruptibleActionData = CurrentAbilityApproachTargetData.InterruptibleData;
	
	DefaultApproachAbility.Activate(this);
	bActiveInterruptibleAction = true;

	// Debug
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (FAIStatesCVars::CVarAIStatesDebug.GetValueOnGameThread() > 0)
	{
		OnChangedActiveAbilityStateDelegate_Debug.Broadcast("Approaching");	
	}
#endif
}

bool AAIStateController::GetWeightedAbility_STATES(TSubclassOf<ULyraGameplayAbility>& OutAbilityClass)
{
	OutAbilityClass = nullptr;
	float AccumulatedWeights = 0.0f;
	TArray<TPair<int32, float>> AvailableAbilitiesChanceThresholds;
	
	if(!AIStates.IsValidIndex(CurrentAIStateIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("AAIStatesController::GetWeightedAbility_STATES - CurrentAIStateIndex out of bounds!."))
		return false;
	}

	// Collect available abilities with weights
	const TArray<TSharedPtr<FAIStateActionData>>& CurrentStateAbilitiesPool = AIStates[CurrentAIStateIndex].Abilities;
	for(int32 i = 0; i < CurrentStateAbilitiesPool.Num(); i++)
	{
		const auto& Ability = CurrentStateAbilitiesPool[i];
		
		if(Ability && Ability->CanAbilityBeActivated(this))
		{
			AccumulatedWeights += Ability->ProbabilityWeight;
			AvailableAbilitiesChanceThresholds.Add({i, Ability->ProbabilityWeight});
		}
	}

	if(AccumulatedWeights == 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("AAIStatesController::GetWeightedAbility_STATES - Accumulated weights were equal ZERO."));
		return false;
	}

	// Setup probability partitions
	// Turn weights into ChanceThresholds
	for(int32 i = 0; i < AvailableAbilitiesChanceThresholds.Num(); i++)
	{
		TPair<int32, float>& AbilityData = AvailableAbilitiesChanceThresholds[i];
		const auto PercentageChance =  AbilityData.Value / AccumulatedWeights;
			
		if(i > 0)
		{
			const float LastChanceThreshold = AvailableAbilitiesChanceThresholds[i-1].Value;
			AbilityData.Value = LastChanceThreshold + PercentageChance;
		}
		else
		{
			AbilityData.Value = PercentageChance;
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (FAIStatesCVars::CVarAIStatesDebug.GetValueOnGameThread() > 0 && GEngine)
		{
			constexpr int32 Key = 100;
			constexpr float TimeToDisplay = 4.0f;
			GEngine->AddOnScreenDebugMessage(i + Key, TimeToDisplay, FColor::Blue,FString::Printf(TEXT("%ls%hs%f"),
				*CurrentStateAbilitiesPool[AbilityData.Key]->GetAbilityName(), ": ", PercentageChance));
		}
#endif
	}
	
	// Pick ability with ChanceThreshold greater or equal than random chance
	constexpr float MinRandomChance = 0.f;
	constexpr float MaxRandomChance = 1.f;
	const float RandomChanceThreshold = FMath::RandRange(MinRandomChance, MaxRandomChance);
	
	for(const TPair<int32, float>& ChanceThreshold : AvailableAbilitiesChanceThresholds)
	{
		if(ChanceThreshold.Value >= RandomChanceThreshold && CurrentStateAbilitiesPool.IsValidIndex(ChanceThreshold.Key))
		{
			const TSharedPtr<FAIStateActionData> AbilityToActivate = CurrentStateAbilitiesPool[ChanceThreshold.Key];
			if(AbilityToActivate == nullptr)
			{
				break;
			}

			OutAbilityClass = AbilityToActivate->Activate(this);
			bActiveInterruptibleAction = AbilityToActivate->CanAbilityBeInterrupted();

			if(AbilityToActivate->GetApproachTargetData(CurrentAbilityApproachTargetData) == false)
			{
				CurrentAbilityApproachTargetData = DefaultApproachTargetData;
			}

			// Debug
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (FAIStatesCVars::CVarAIStatesDebug.GetValueOnGameThread() > 0 && AIStatesSetConfig.IsValid())
			{
				const int32 CurrentIndex = AvailableAbilitiesChanceThresholds.IndexOfByKey(ChanceThreshold);
				const float PercentageChance = CurrentIndex > 0 ? AvailableAbilitiesChanceThresholds[CurrentIndex-1].Value : ChanceThreshold.Value;

				constexpr int32 PercentageMultiplier = 100;
				const FString AbilityToActivateString = FString::Printf(TEXT("%ls%hs%ls"),
				*AbilityToActivate->GetAbilityName(), " | ", *(FString::FromInt(static_cast<int32>(PercentageChance * PercentageMultiplier)) + "%"));
				
				OnSelectedAbilityDelegate_Debug.Broadcast(PreviousState_Debug, PreviousAbility_Debug,
					AIStatesSetConfig->States[CurrentAIStateIndex].StateName,
					*AbilityToActivateString);
					
				PreviousState_Debug = AIStatesSetConfig->States[CurrentAIStateIndex].StateName;
				PreviousAbility_Debug = *AbilityToActivateString;
			}
#endif
			
			return true;
		}
	}

	// Debug
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (FAIStatesCVars::CVarAIStatesDebug.GetValueOnGameThread() > 0 && AIStatesSetConfig.IsValid())
	{
		OnSelectedAbilityDelegate_Debug.Broadcast(PreviousState_Debug, PreviousAbility_Debug,
				AIStatesSetConfig->States[CurrentAIStateIndex].StateName,
				"None");
	}
#endif
	
	UE_LOG(LogTemp, Error, TEXT("AAIStatesController::GetWeightedAbility_STATES - No ability was chosen."));
	return false;
}

bool AAIStateController::GetWeightedAbility(const TArray<TSubclassOf<ULyraGameplayAbility>>& Abilities, TSubclassOf<ULyraGameplayAbility>& OutAbilityClass) const
{
	const ULyraAbilitySystemComponent* ASC = Cast<ULyraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn()));
	if(ensureMsgf(ASC != nullptr, TEXT("Ability system component is nullptr!")))
	{
		return false;
	}

	float WeightSum = 0.f;

	TArray<TPair<TSubclassOf<ULyraGameplayAbility>, float>> AbilityWeights;

	for (const TSubclassOf<ULyraGameplayAbility>& AbilityClass : Abilities)
	{
		const float Weight = ASC->GetAbilityWeight(AbilityClass);
		WeightSum += Weight;

		AbilityWeights.Add(TPair<TSubclassOf<ULyraGameplayAbility>, float>(AbilityClass, Weight));
	}

	const float Rand = FMath::RandRange(0.f, WeightSum);

	float AccWeight = 0.f;
	for (const TPair<TSubclassOf<ULyraGameplayAbility>, float>& AbilityWithWeight : AbilityWeights)
	{
		const float Weight = AbilityWithWeight.Value;
		if (Weight > 0)
		{
			AccWeight += Weight;
			if (AccWeight > Rand)
			{
				OutAbilityClass = AbilityWithWeight.Key;
				return true;
			}
		}
	}

	return false;
}

bool AAIStateController::GetActivatableAbilityByWeight(const FGameplayTagContainer& AbilityTags, AActor* Target, bool bCheckAffection, FGameplayTag AlwaysCheckAffectionForTag, TSubclassOf<ULyraGameplayAbility>& OutAbility) const
{
	ULyraAbilitySystemComponent* ASC = Cast<ULyraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn()));
	if(ensureMsgf(ASC != nullptr, TEXT("Ability system component is nullptr!")))
	{
		return false;
	}

	TArray<FGameplayAbilitySpec*> MatchingGameplayAbilities;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTags, MatchingGameplayAbilities, false);

	TArray<TSubclassOf<ULyraGameplayAbility>> ReadyAbilities;

	FGameplayEventData Payload;
	Payload.Target = Target;
	Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Target);

	for (const FGameplayAbilitySpec* AbilitySpec : MatchingGameplayAbilities)
	{
		const ULyraGameplayAbility* AbilityCDO = Cast<ULyraGameplayAbility>(AbilitySpec->Ability);
		TSubclassOf<ULyraGameplayAbility> AbilityClass = AbilityCDO->GetClass();

		FGameplayTagContainer FailureTags;
		if (!ASC->CanActivateAbilityByClass(AbilityClass, FGameplayTagContainer::EmptyContainer, FailureTags))
		{
			continue;
		}

		if (bCheckAffection || (AlwaysCheckAffectionForTag.IsValid() && AbilityCDO->AbilityTags.HasTag(AlwaysCheckAffectionForTag)))
		{
			float AffectionStrength;
			constexpr float Leeway = 1.f;
			if (!ASC->WillAbilityAffectTarget(AbilityClass, Payload, Leeway, AffectionStrength))
			{
				continue;
			}
		}

		ReadyAbilities.Add(AbilityClass);
	}

	if (ReadyAbilities.Num() == 0)
	{
		return false;
	}

	TSubclassOf<ULyraGameplayAbility> WeightedAbility;
	if (GetWeightedAbility(ReadyAbilities, WeightedAbility))
	{
		OutAbility = WeightedAbility;
		return true;
	}

	OutAbility = nullptr;
	return false;
}
