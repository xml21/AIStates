
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "AICharacter.generated.h"

class UFloatingWidgetComponent;
class UMultiCharacterAnimSyncConfig;
class UAIStatesSet;
class ULyraAbilitySystemComponent;
class UBehaviorTree;

/**
 * AAICharacter
 *
 *	Base AI character class
 */
UCLASS(config = Game)
class LYRAGAME_API AAICharacter : public ACharacter
{
	GENERATED_BODY()

public:

	AAICharacter() = default;

	// Behavior Tree asset for this character
	UPROPERTY(EditDefaultsOnly, Category = Behavior)
	TObjectPtr<UBehaviorTree> BTAsset;

	// AI states set with behavior setup for this character
	UPROPERTY(EditDefaultsOnly, Category = States)
	UAIStatesSet* AIStatesSetConfig;

	TObjectPtr<ULyraAbilitySystemComponent> GetAbilitySystemComponent() const { return AbilitySystemComponent; }
	
protected:

	// Ability system component cache for this character
	UPROPERTY(VisibleAnywhere, Category = Abilities)
	TObjectPtr<ULyraAbilitySystemComponent> AbilitySystemComponent;
};
