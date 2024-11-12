// Fill out your copyright notice in the Description page of Project Settings.


#include "LyraBuildingManagerComponent.h"

#include "LyraBuilding.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraCharacter.h"
#include "Player/LyraPlayerController.h"
#include "Runtime/PhysicsCore/Public/CollisionShape.h"

ULyraBuildingManagerComponent::ULyraBuildingManagerComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
	bTickInEditor = true;
	
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bCanEverTick = true;	
}

void ULyraBuildingManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Update(DeltaTime);
}

void ULyraBuildingManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	GhostBuilding = nullptr;
}

void ULyraBuildingManagerComponent::ToggleBuildingMode()
{
	ULyraAbilitySystemComponent* LyraASC = GetPlayerAbilitySystemComponent();
	checkf(LyraASC, TEXT("Ability System Component is invalid!"));

	if (BuildModeState == ELyraBuildModeState::Enabled)
	{
		BuildModeState = ELyraBuildModeState::Disabled;
		LyraASC->UnBlockAbilitiesWithTags(AbilitiesToBlock);
	}
	else
	{
		BuildModeState = ELyraBuildModeState::Enabled;
		LyraASC->BlockAbilitiesWithTags(AbilitiesToBlock);
	}
}

void ULyraBuildingManagerComponent::Update(float DeltaTime)
{
	const ALyraPlayerController* PlayerController = GetController<ALyraPlayerController>();
	if(!ensureMsgf(PlayerController, TEXT("Player Controller is invalid!")))
	{
		return;
	}
	
	if (BuildModeState == ELyraBuildModeState::Enabled)
	{
		const FVector CrosshairLocation3D = GetCrosshairLocationIn3D();
		const FVector ProjectedFloorLocation = GetCrosshairLocationProjection(CrosshairLocation3D);
		
		if(GhostBuilding == nullptr)
		{
			constexpr int32 FirstIndex = 0;
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			FHitResult HitResult;
			GetWorld()->LineTraceSingleByChannel(HitResult, CrosshairLocation3D, ProjectedFloorLocation,
				ECC_Visibility, FCollisionQueryParams::DefaultQueryParam);

			const FVector TargetBuildingLocation = HitResult.bBlockingHit ?
				HitResult.ImpactPoint : CrosshairLocation3D;
			GhostBuilding = GetWorld()->SpawnActor<ALyraBuilding>(TargetBuildingLocation, FRotator::ZeroRotator,
				MoveTemp(SpawnParams));
			GhostBuilding->BuildingMesh->SetStaticMesh(Buildings[FirstIndex].Get());
		}

		if(GhostBuilding.Get() && ensureMsgf(GhostBuilding->BuildingMesh, TEXT("Building mesh is not set on Building Actor!")))
		{		
			FHitResult HitResult;
			GetWorld()->LineTraceSingleByChannel(HitResult, CrosshairLocation3D, ProjectedFloorLocation,
				ECC_Visibility, FCollisionQueryParams::DefaultQueryParam);

			TargetGhostBuildingLocation = HitResult.bBlockingHit ?
				HitResult.ImpactPoint : CurrentGhostBuildingLocation;
			constexpr float InterpSpeed = 20.f;
			CurrentGhostBuildingLocation = FMath::VInterpTo(
				CurrentGhostBuildingLocation, TargetGhostBuildingLocation, DeltaTime, InterpSpeed);
			GhostBuilding->SetActorLocation(CurrentGhostBuildingLocation);
			
			// Check if the building can be placed at the current location
			const bool bCanPlaceBuilding = !IsGhostBuildingOverlapping();
			UStaticMesh* BuildingStaticMesh = GhostBuilding->BuildingMesh->GetStaticMesh();
			if(ensureMsgf(BuildingStaticMesh, TEXT("Building static mesh is invalid!")))
			{
				constexpr int32 MaterialIndex = 0;
				BuildingStaticMesh->SetMaterial(
					MaterialIndex,bCanPlaceBuilding ? EnabledGhostBuildingMaterial.Get() : DisabledGhostBuildingMaterial.Get());
			}
		}
	}
}

void ULyraBuildingManagerComponent::PlaceBuilding()
{
	if(GhostBuilding.Get())
	{
		ensureMsgf(GhostBuilding->BuildingMesh, TEXT("Building mesh is not set on Building Actor!"));
	
		if (BuildModeState == ELyraBuildModeState::Enabled && GhostBuilding->BuildingMesh->IsVisible() &&
			!IsGhostBuildingOverlapping())
		{
			constexpr int32 MaterialIndex = 0;
			ALyraBuilding* NewBuilding = GetWorld()->SpawnActor<ALyraBuilding>(
				GhostBuilding->GetActorLocation(), FRotator::ZeroRotator);
			NewBuilding->BuildingMesh->SetStaticMesh(Buildings[CurrentBuildingIndex].Get());
			NewBuilding->BuildingMesh->SetMaterial(
				MaterialIndex, Buildings[CurrentBuildingIndex].Get()->GetMaterial(MaterialIndex));
			GhostBuilding->Destroy();
			OnBuildingPlaced(NewBuilding);
		}
	}
}

void ULyraBuildingManagerComponent::IncrementBuildingType()
{
	if(CurrentBuildingIndex < Buildings.Num() - 1)
	{
		CurrentBuildingIndex++;
	}
	else
	{
		CurrentBuildingIndex = 0;
	}

	if(GhostBuilding.Get() &&
		ensureMsgf(GhostBuilding->BuildingMesh, TEXT("Building mesh is not set on Building Actor!")))
	{
		GhostBuilding->BuildingMesh->SetStaticMesh(Buildings[CurrentBuildingIndex].Get());
	}
}

void ULyraBuildingManagerComponent::DecrementBuildingType()
{
	if(CurrentBuildingIndex > 0)
	{
		CurrentBuildingIndex--;
	}
	else
	{
		CurrentBuildingIndex = Buildings.Num() - 1;
	}

	if(GhostBuilding.Get() &&
		ensureMsgf(GhostBuilding->BuildingMesh, TEXT("Building mesh is not set on Building Actor!")))
	{
		GhostBuilding->BuildingMesh->SetStaticMesh(Buildings[CurrentBuildingIndex].Get());
	}
}

void ULyraBuildingManagerComponent::OnBuildingPlaced(const ALyraBuilding* Building)
{
	// Play a sound effect or animation to indicate the building has been placed
	ULyraAbilitySystemComponent* LyraASC = GetPlayerAbilitySystemComponent();
	checkf(LyraASC, TEXT("Ability System Component is invalid!"));
	checkf(Building, TEXT("Passed building is invalid!"));

	// Event will fire on authority only and will be replicated
	if (Building->GetLocalRole() == ROLE_Authority)
	{
		LyraASC->ExecuteGameplayCue(OnBuildingPlacedSoundCue, FGameplayCueParameters());
	}
}

void ULyraBuildingManagerComponent::OnBuildingModeChanged()
{
	ALyraPlayerController* PlayerController = GetController<ALyraPlayerController>();
	checkf(PlayerController, TEXT("Player Controller is invalid!"))

	PlayerController->SetIgnoreLookInput(BuildModeState == ELyraBuildModeState::Enabled);
}

bool ULyraBuildingManagerComponent::IsGhostBuildingOverlapping() const
{
	checkf(GhostBuilding.Get(), TEXT("There is no ghost building at the moment!"));
	
	// Get the world of the mesh component
	checkf(GhostBuilding->BuildingMesh, TEXT("Building mesh is not set on Building Actor!"))
	const UStaticMeshComponent* MeshComponent = GhostBuilding->BuildingMesh;
	const UWorld* World = MeshComponent->GetWorld();

	// Create a box trace query
	FCollisionShape BoxShape = FCollisionShape::MakeBox(MeshComponent->Bounds.BoxExtent);
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(MyCollisionQuery), true, GhostBuilding.Get());
	TraceParams.AddIgnoredActor(MeshComponent->GetOwner());
	
	// Trace the shape against the world
	FHitResult HitResult;
	if (World->SweepSingleByChannel(HitResult, MeshComponent->Bounds.Origin + FVector::UpVector, MeshComponent->Bounds.Origin + FVector::UpVector,
		FQuat::Identity, ECC_Visibility, BoxShape, TraceParams))
	{
		// Check if the hit result is not the mesh component itself
		if (HitResult.GetComponent() != MeshComponent)
		{
			// There is a collision with another actor
			return true;
		}
	}

	// There is no collision with any other actor
	return false;
}

FVector ULyraBuildingManagerComponent::GetCrosshairLocationIn3D() const
{
	const ALyraPlayerController* PlayerController = GetController<ALyraPlayerController>();
	if(ensureMsgf(PlayerController, TEXT("Player Controller is invalid!")))
	{
		FVector CameraLocation = FVector::ZeroVector;
		FRotator CameraRotation = FRotator::ZeroRotator;
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

		return CameraLocation + (CameraRotation.Vector() * BuildingRange);
	}

	return FVector::ZeroVector;
}

FVector ULyraBuildingManagerComponent::GetCrosshairLocationProjection(FVector CrosshairLocation) const
{
	constexpr float TraceDistance = 4000.f;
	
	FVector Start = CrosshairLocation;
	FVector End = CrosshairLocation + (FVector::DownVector * TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetPawn<ALyraCharacter>());
	CollisionParams.AddIgnoredActor(GhostBuilding.Get());

	DrawDebugLine(GetWorld(), Start, End, FColor::Blue, true);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams))
	{
		return HitResult.Location;
	}

	return FVector::ZeroVector;
}

TObjectPtr<ULyraAbilitySystemComponent> ULyraBuildingManagerComponent::GetPlayerAbilitySystemComponent() const
{
	if (const ALyraPlayerController* PlayerController = GetController<ALyraPlayerController>())
	{
		return PlayerController->GetLyraAbilitySystemComponent();
	}
	
	return nullptr;
}
