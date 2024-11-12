// Fill out your copyright notice in the Description page of Project Settings.


#include "AIStatesSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include "AI/AIStateController.h"

#include "Components/WidgetComponent.h"

void UAIStatesSubsystem::OnAIStatesDebugToggle(IConsoleVariable* Var)
{
	bDebug = Var->GetInt() > 0;

	StaticInstance->UpdateDebugWidgets();
}

void UAIStatesSubsystem::UpdateDebugWidgets()
{
	if(bDebug == false)
	{
		for (auto It = PerAIWidget_Debug.CreateIterator(); It; ++It)
		{
			auto* WidgetComp = PerAIWidget_Debug[It.GetIndex()];
			if(IsValid(WidgetComp) && IsValid(WidgetComp->GetOwner()))
			{
				WidgetComp->GetOwner()->RemoveOwnedComponent(WidgetComp);
				WidgetComp->DestroyComponent();
			}
		}
		PerAIWidget_Debug.Empty();

		return;
	}

	PerAIWidget_Debug.Empty();
	for(const TObjectPtr<UAbilitySystemComponent> AIAsc : ActiveAIActorsASCList)
	{
		AddDebugWidget(AIAsc);
	}
}

void UAIStatesSubsystem::AddDebugWidget(const TWeakObjectPtr<UAbilitySystemComponent>& AIAsc)
{
	if(AIAsc.IsValid() == false)
	{
		return;
	}
	auto* AIPawn = Cast<APawn>(AIAsc->GetOwnerActor());
	if(IsValid(AIPawn) == false)
	{
		return;
	}
	const auto* AIController = Cast<AAIStateController>(AIPawn->GetController());
	if(!AIController)
	{
		return;
	}
	
	// Add widget component for debug to AI Actor
	auto* WidgetComponent = Cast<UWidgetComponent>(
		AIPawn->AddComponentByClass(UWidgetComponent::StaticClass(),false, FTransform::Identity, false));
		
	if(IsValid(WidgetComponent))
	{
		PerAIWidget_Debug.Add(WidgetComponent);
		
		WidgetComponent->SetWidgetClass(AIController->PerAIWidgetClass_Debug);
		WidgetComponent->InitWidget();
		WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	}
}

void UAIStatesSubsystem::RemoveDebugWidget(const TWeakObjectPtr<UAbilitySystemComponent>& AIAsc)
{
	if(AIAsc.IsValid() == false)
	{
		return;
	}
	auto* AIPawn = Cast<APawn>(AIAsc->GetOwnerActor());
	if(IsValid(AIPawn) == false)
	{
		return;
	}
	const AAIController* AIController = Cast<AAIController>(AIPawn->GetController());
	if(!AIController)
	{
		return;
	}
	
	for(auto* DebugComponent : AIPawn->GetComponentsByTag(UWidgetComponent::StaticClass(), "AIStateDebug"))
	{
		AIPawn->RemoveOwnedComponent(DebugComponent);
	}

	const int32 IndexToRemove = ActiveAIActorsASCList.IndexOfByKey(AIAsc);
	if(PerAIWidget_Debug.IsValidIndex(IndexToRemove))
	{
		UWidgetComponent* Widget = PerAIWidget_Debug[IndexToRemove];
		PerAIWidget_Debug.RemoveAt(IndexToRemove);
		
		Widget->DestroyComponent();
	}
}

UAbilitySystemComponent* UAIStatesSubsystem::GetActiveAIActorByIndex(int32 Index) const
{
	if(ActiveAIActorsASCList.IsValidIndex(Index) && ActiveAIActorsASCList[Index])
	{
		return ActiveAIActorsASCList[Index].Get();
	}

	return nullptr;
}

void UAIStatesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
 
	FAIStatesCVars::CVarAIStatesDebug.AsVariable()
		->SetOnChangedCallback(FConsoleVariableDelegate::CreateStatic(&UAIStatesSubsystem::OnAIStatesDebugToggle));

	StaticInstance = this;

}

void UAIStatesSubsystem::RegisterAIActor(const AAIStateController* AIController)
{
	if(IsValid(AIController) == false)
	{
		return;
	}

	UAbilitySystemComponent* RequestingASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AIController->GetPawn());
	if(ActiveAIActorsASCList.Contains(RequestingASC) == false)
	{
		int32 Index = ActiveAIActorsASCList.Add(MoveTemp(RequestingASC));

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (FAIStatesCVars::CVarAIStatesDebug.GetValueOnGameThread() > 0)
		{
			AddDebugWidget(MoveTemp(RequestingASC));
		}
#endif
		
	}
}

void UAIStatesSubsystem::UnregisterAIActor(const AAIStateController* AIController)
{
	if(IsValid(AIController) == false)
	{
		return;
	}
	
	auto* RequestingASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AIController->GetPawn());
	if(ActiveAIActorsASCList.Contains(RequestingASC))
	{
		ActiveAIActorsASCList.Remove(RequestingASC);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (FAIStatesCVars::CVarAIStatesDebug.GetValueOnGameThread() > 0)
		{
			RemoveDebugWidget(RequestingASC);
		}
#endif
	}
}