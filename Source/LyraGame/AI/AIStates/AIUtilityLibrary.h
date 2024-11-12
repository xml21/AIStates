#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AIUtilityLibrary.generated.h"

class AAIStateController;
class UAbilitySystemComponent;

/*
 *	Library with helper functions for AI states system
 */
UCLASS()
class LYRAGAME_API UAIUtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	// Getter function retrieving distance from given AI controller to its target 
	UFUNCTION(BlueprintPure, Category = "AIUtilities")
	static float GetDistanceToAITarget(const AAIStateController* SourceAI);
	
	static const TArray<TObjectPtr<UAbilitySystemComponent>>* GetActiveAIActors(const UObject* WorldContext);

};
