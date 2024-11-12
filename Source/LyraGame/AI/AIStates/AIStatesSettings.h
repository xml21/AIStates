#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"

#include "AIStatesSettings.generated.h"

class UGameplayEffect;

UCLASS(Config = "Game", DefaultConfig, meta = (DisplayName = "AI States Settings"))
class UAIStatesSettings	: public UDeveloperSettings
{
	GENERATED_BODY()

	UAIStatesSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	// Gameplay effect used to add and remove dynamic tags.
	UPROPERTY(Config, EditDefaultsOnly, Category = "Remember Recent Tags")
	TSoftClassPtr<UGameplayEffect> RememberRecentTagGameplayEffect;
	
	static const UAIStatesSettings* Get() { return GetDefault<UAIStatesSettings>(); }
};
