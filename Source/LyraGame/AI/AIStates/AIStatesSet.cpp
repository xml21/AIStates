// Fill out your copyright notice in the Description page of Project Settings.


#include "AIStatesSet.h"

#include "AIStatesSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"

#include "LyraGame/AI/AIStates/AIUtilityLibrary.h"
#include "LyraGame/AI/AIStateController.h"

bool FMaxDistanceToCondition::CheckCondition(AAIStateController* SourceAI) const
{
	if(ensureMsgf(SourceAI != nullptr, TEXT("SourceAI is nullptr!")))
	{
		return false;
	}

	if(ensureMsgf(SourceAI->GetWorld() != nullptr, TEXT("World is nullptr!")))
	{
		return false;
	}

	const auto* AIStatesSubsystem = SourceAI->GetWorld()->GetSubsystem<UAIStatesSubsystem>();
	if(ensureMsgf(AIStatesSubsystem != nullptr, TEXT("AI states subsystem is nullptr!")))
	{
		return false;
	}
	
	bool bResult = false;
	if(EvaluationTarget == FGameplayTag::RequestGameplayTag(AIConditions::ConditionPlayerTagName))
	{
		bResult = UAIUtilityLibrary::GetDistanceToAITarget(SourceAI) < this->MaxDistanceTo;	
	}
	else
	{
		const TArray<TObjectPtr<UAbilitySystemComponent>>& AIsASCList = AIStatesSubsystem->GetActiveAIActors();
		for(const TObjectPtr<UAbilitySystemComponent>& ASC : AIsASCList)
		{
			if(!ASC == false)
			{
				continue;
			}
			
			const AActor* AIActor = ASC->GetOwnerActor();
			const APawn* SourcePawn = SourceAI->GetPawn();

			// Check for specific AI Tag Identifiers
			if(EvaluationTarget.IsValid() == false || ASC.Get()->HasMatchingGameplayTag(EvaluationTarget)
				&& ASC->GetOwnerActor() && SourceAI->GetPawn())
			{
				bResult = FVector::Distance(AIActor->GetActorLocation(), SourcePawn->GetActorLocation()) < this->MaxDistanceTo; 
				break;
			}
		}
	}

	return (bResult && bInverted == false) || (bResult == false && bInverted);
}

// Tags - check either all matching tags or any matching tag, can be used with single/multiple tags
bool FGameplayTagMultipleBasedCondition::CheckCondition(AAIStateController* SourceAI) const
{
	const auto* AIStatesSubsystem = SourceAI->GetWorld()->GetSubsystem<UAIStatesSubsystem>();
	if(ensureMsgf(AIStatesSubsystem != nullptr, TEXT("AI states subsystem is nullptr!")))
	{
		return false;
	}

	bool bResult = false;
	if(EvaluationTarget == FGameplayTag::RequestGameplayTag(AIConditions::ConditionPlayerTagName))
	{
		auto* PlayerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetTarget());
		if(IsValid(PlayerASC))
		{
			bResult = bHasAny ? PlayerASC->HasAnyMatchingGameplayTags(ConditionTag) : PlayerASC->HasAllMatchingGameplayTags(ConditionTag);
		}
	}
	else if(EvaluationTarget == FGameplayTag::RequestGameplayTag(AIConditions::ConditionSelfTagName))
	{
		const auto* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetPawn());
		if(IsValid(SourceASC))
		{
			bResult = bHasAny ? SourceASC->HasAnyMatchingGameplayTags(ConditionTag) : SourceASC->HasAllMatchingGameplayTags(ConditionTag);
		}
	}
	else
	{
		const auto& AIsASCList = AIStatesSubsystem->GetActiveAIActors();
		for(const auto& ASC : AIsASCList)
		{
			if(!ASC == false)
			{
				continue;
			}

			// Check for specific AI Tag Identifiers
			if(EvaluationTarget.IsValid() == false || ASC.Get()->HasMatchingGameplayTag(EvaluationTarget))
			{
				bResult = bHasAny ? ASC.Get()->HasAnyMatchingGameplayTags(ConditionTag) : ASC.Get()->HasAllMatchingGameplayTags(ConditionTag);
			}
		}
	}
	
	return (bResult && bInverted == false) || (bResult == false && bInverted);
}

bool FTagCountCondition::CheckCondition(AAIStateController* SourceAI) const
{
	const auto* AIStatesSubsystem = SourceAI->GetWorld()->GetSubsystem<UAIStatesSubsystem>();
	if(ensureMsgf(AIStatesSubsystem != nullptr, TEXT("AI states subsystem is nullptr!")))
	{
		return false;
	}

	bool bResult = false;
	if(EvaluationTarget == FGameplayTag::RequestGameplayTag(AIConditions::ConditionPlayerTagName))
	{
		const auto* PlayerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetTarget());
		if(IsValid(PlayerASC) && PlayerASC->GetTagCount(ConditionTag) >= MinCount)
		{
			bResult = true;
		}
	}
	else if(EvaluationTarget == FGameplayTag::RequestGameplayTag(AIConditions::ConditionSelfTagName))
	{
		const auto* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetPawn());
		if(SourceASC && SourceASC->GetTagCount(ConditionTag) >= MinCount)
		{
			bResult = true;
		}
	}
	else
	{
		const TArray<TObjectPtr<UAbilitySystemComponent>>& AIASCList = AIStatesSubsystem->GetActiveAIActors();
		for(const TObjectPtr<UAbilitySystemComponent>& ASC : AIASCList)
		{
			if(!ASC)
			{
				continue;
			}

			// Check for specific AI Tag Identifiers
			if(EvaluationTarget.IsValid() == false || ASC.Get()->HasMatchingGameplayTag(EvaluationTarget)
				&& ASC.Get()->GetTagCount(ConditionTag) >= MinCount)
			{
				bResult = true;
				break;
			}
		}
	}
	
	return (bResult && bInverted == false) || (bResult == false && bInverted);
}

bool FCountEnemiesWithTag::CheckCondition(AAIStateController* SourceAI) const
{
	const auto* AIStatesSubsystem = SourceAI->GetWorld()->GetSubsystem<UAIStatesSubsystem>();
	if(ensureMsgf(AIStatesSubsystem != nullptr, TEXT("AI states subsystem is nullptr!")))
	{
		return false;
	}

	int32 TagsCount = 0;
	const TArray<TObjectPtr<UAbilitySystemComponent>>& AIASCList = AIStatesSubsystem->GetActiveAIActors();
	for(const TObjectPtr<UAbilitySystemComponent>& ASC : AIASCList)
	{
		if(!ASC == false)
		{
			continue;
		}

		// Check for specific AI Tag Identifiers
		if(EvaluationTarget.IsValid() == false || ASC.Get()->HasMatchingGameplayTag(EvaluationTarget) != bInvertedHasTag)
		{
			TagsCount++;
		}
	}
	
	const bool bResult = TagsCount >= MinCount;
	
	return (bResult && bInverted == false) || (bResult == false && bInverted);
}

bool FCheckRecentlyChangedTag::CheckCondition(AAIStateController* SourceAI) const
{
	const auto* AIStatesSubsystem = SourceAI->GetWorld()->GetSubsystem<UAIStatesSubsystem>();
	if(ensureMsgf(AIStatesSubsystem != nullptr, TEXT("AI states subsystem is nullptr!")))
	{
		return false;
	}

	bool bResult = false;
	
	float bHasRememberedTag = false;
	float RecentTagPassedTime = 0.0f;
	if(EvaluationTarget == FGameplayTag::RequestGameplayTag(AIConditions::ConditionPlayerTagName))
	{
		const auto* PlayerASC = Cast<ULyraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetTarget()));
		if(PlayerASC)
		{
			bHasRememberedTag = PlayerASC->GetRecentTagTimePassed(RecentTag, RecentTagPassedTime);
		}
	}
	else if(EvaluationTarget == FGameplayTag::RequestGameplayTag(AIConditions::ConditionSelfTagName))
	{
		const auto* SourceASC = Cast<ULyraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetTarget()));
		if(SourceASC)
		{
			bHasRememberedTag = SourceASC->GetRecentTagTimePassed(RecentTag, RecentTagPassedTime);
		}
	}
	else
	{
		const auto& AIsASCList = AIStatesSubsystem->GetActiveAIActors();
		for(const auto& ASC : AIsASCList)
		{
			if(!ASC)
			{
				continue;
			}
			const auto* AbilitySystemComp = Cast<ULyraAbilitySystemComponent>(ASC.Get());
			
			// Check for specific AI Tag Identifiers
			if(EvaluationTarget.IsValid() == false || (IsValid(AbilitySystemComp) && AbilitySystemComp->HasMatchingGameplayTag(EvaluationTarget)))
			{
				bHasRememberedTag = AbilitySystemComp->GetRecentTagTimePassed(RecentTag, RecentTagPassedTime);
				break;
			}
		}
	}

	if(bHasRememberedTag)
	{
		bResult = RecentTagPassedTime <= MaxTimePassed;
		bResult = (bResult && bInverted == false) || (bResult == false && bInverted);	
	}
	
	return bResult;
}

TSubclassOf<ULyraGameplayAbility> FAIStateInterruptibleAbility::Activate(AAIStateController* SourceAI)
{
	if(ensureMsgf(SourceAI != nullptr, TEXT("Source AI is nullptr!")))
	{
		return nullptr;
	}
	
	auto* AbilitySystemComp = Cast<ULyraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetPawn()));
	if(ensureMsgf(AbilitySystemComp != nullptr, TEXT("Ability system component is nullptr!")))
	{
		return nullptr;
	}

	const FGameplayTag InterruptibleAbilityTag = FGameplayTag::RequestGameplayTag(InterruptibleAbilityTagName);
	const FGameplayAbilitySpec* AbilitySpec = AbilitySystemComp->GetActivatableGameplayAbilitySpecByTag(InterruptibleAbilityTag);

	// Activate ability sending Interruptible data within payload
	if(ensureMsgf(AbilitySpec != nullptr, TEXT("AbilitySpec is nullptr!")))
	{
		FGameplayEventData Payload = {};
		Payload.Target = SourceAI->GetTarget();
			
		AbilitySystemComp->TriggerAbilityFromGameplayEvent(
		AbilitySpec->Handle, nullptr,
		InterruptibleAbilityTag, &Payload,*AbilitySystemComp);	
	}

	return nullptr;
}

bool FAIStateInterruptibleAbility::ShouldAbilityBeInterrupted(AAIStateController* SourceController)
{
	for(const auto& [VariantTitle, ConditionsVariant] : InterruptibleActionData.ConditionsToInterrupt)
	{
		bool IsConditionsVariantValid = true;

		for(const FInstancedStruct& ConditionInstancedStruct : ConditionsVariant)
		{
			const auto* Condition = ConditionInstancedStruct.GetPtr<FAIStateConditionData>();
			if(Condition == nullptr || Condition->CheckCondition(SourceController) == false)
			{
				IsConditionsVariantValid = false;
				break;
			}
		}

		// If any group was valid return true
		if(IsConditionsVariantValid)
		{
			return true;
		}
	}
	
	return false;
}

float FAIStateInterruptibleAbility::GetMaxDuration() const
{
	return FMath::RandRange(InterruptibleActionData.MinDuration, InterruptibleActionData.MaxDuration);
}

TSubclassOf<ULyraGameplayAbility> FWeightedAbility::Activate(AAIStateController* SourceAI)
{
	return AbilityClass.Get();
}

bool FWeightedAbility::CanAbilityBeActivated(AAIStateController* SourceAI) const
{
	if(ensureMsgf(SourceAI != nullptr, TEXT("SourceAI is nullptr!")))
	{
		return false;
	}

	const ULyraAbilitySystemComponent* ASC = Cast<ULyraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetPawn()));
	if(ensureMsgf(ASC != nullptr, TEXT("Ability system component is nullptr!")))
	{
		return false;
	}

	FGameplayTagContainer FailureTagsEmpty;
	FGameplayTagContainer OwnedGameplayTagContainer;
	ASC->GetOwnedGameplayTags(OwnedGameplayTagContainer);
	
	return ASC->CanActivateAbilityByClass(AbilityClass, OwnedGameplayTagContainer, FailureTagsEmpty);
}

FString FWeightedAbility::GetAbilityName()
{
	return AbilityClass ? AbilityClass->GetName().LeftChop(2) : FString("None");
}

bool FWeightedAbility::GetApproachTargetData(FApproachTargetData& OutApproachTargetData) const
{
	OutApproachTargetData = ApproachTargetData;
	return bCustomApproachTargetData;
}

// Attributes
bool FAttributeChangeCondition::CheckCondition(AAIStateController* SourceAI) const
{
	const auto* AIStatesSubsystem = SourceAI->GetWorld()->GetSubsystem<UAIStatesSubsystem>();
	if(ensureMsgf(AIStatesSubsystem != nullptr, TEXT("AI states subsystem is nullptr!")))
	{
		return false;
	}

	// Lambda to check attribute value
	auto CheckAttributeValue = [&](const auto* ASC) -> TOptional<bool> {
		if (!IsValid(ASC)) return FNullOpt{false};

		bool bHasAttribute;
		const float CurrentValue = ASC->GetGameplayAttributeValue(Attribute, bHasAttribute);
		return bHasAttribute ? CurrentValue >= MinValue : false;
	};

	TOptional bResult = false;
	if(EvaluationTarget == FGameplayTag::RequestGameplayTag(AIConditions::ConditionPlayerTagName))
	{
		const UAbilitySystemComponent* PlayerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetTarget());
		bResult = CheckAttributeValue(PlayerASC);
	}
	else if(EvaluationTarget == FGameplayTag::RequestGameplayTag(AIConditions::ConditionSelfTagName))
	{
		const UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAI->GetPawn());
		bResult = CheckAttributeValue(SourceASC);
	}
	else
	{
		const TArray<TObjectPtr<UAbilitySystemComponent>>& AIASCList = AIStatesSubsystem->GetActiveAIActors();
		for(const TObjectPtr<UAbilitySystemComponent>& ASC : AIASCList)
		{
			if(!ASC)
			{
				continue;
			}

			// Check for specific AI Tag Identifiers
			if(EvaluationTarget.IsValid() == false || ASC.Get()->HasMatchingGameplayTag(EvaluationTarget))
			{
				bool bHasAttribute;
				const auto CurrentValue = ASC.Get()->GetGameplayAttributeValue(Attribute,bHasAttribute);
			
				bResult = bHasAttribute ? CurrentValue >= MinValue : false;
				break;
			}
		}
	}
	
	return (bResult && bInverted == false) || (bResult == false && bInverted);
}