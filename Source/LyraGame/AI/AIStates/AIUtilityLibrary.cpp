#include "AIUtilityLibrary.h"

#include "AIStatesSubsystem.h"
#include "AI/AIStateController.h"

class UAIStatesSubsystem;

float UAIUtilityLibrary::GetDistanceToAITarget(const AAIStateController* SourceAI)
{
	if(!ensureMsgf(SourceAI != nullptr, TEXT("SourceAI is nullptr!")))
	{
		return 0.0f;
	}
	
	if(IsValid(SourceAI))
	{
		const AActor* TargetActor = SourceAI->GetTarget();
		const APawn* SourcePawn = SourceAI->GetPawn();
		
		if(IsValid(TargetActor) && IsValid(SourcePawn))
		{
			return FVector::Distance(TargetActor->GetActorLocation(), SourcePawn->GetActorLocation());
		}	
	}

	UE_LOG(LogTemp, Error, TEXT("UAIUtilityLibrary::GetDistanceToAITarget - Could not calculate distance!"));
	return 0.0f;
}

const TArray<TObjectPtr<UAbilitySystemComponent>>* UAIUtilityLibrary::GetActiveAIActors(const UObject* WorldContext)
{
	if(!ensureMsgf(WorldContext != nullptr && WorldContext->GetWorld() != nullptr, TEXT("WorldContext is nullptr!")))
	{
		return nullptr;
	}
	
	const auto* AISubsystem = WorldContext->GetWorld()->GetSubsystem<UAIStatesSubsystem>();
	if(IsValid(AISubsystem))
	{
		return &AISubsystem->GetActiveAIActors();
	}
	
	return nullptr;
}
