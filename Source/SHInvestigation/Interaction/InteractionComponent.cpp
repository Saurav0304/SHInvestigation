// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/InteractionComponent.h"

#include "Interaction/Interactable.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

UInteractionComponent::UInteractionComponent()
{
	// Focus is updated by a timer, not every frame.
	PrimaryComponentTick.bCanEverTick = false;

	// Without this, Server_Interact called on a client is silently dropped.
	SetIsReplicatedByDefault(true);
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// The timer runs on every machine, but UpdateFocus() returns immediately unless this pawn is
	// locally controlled. Checking inside the function also covers clients where possession
	// happens after BeginPlay.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FocusTimerHandle, this, &UInteractionComponent::UpdateFocus, TraceInterval, true);
	}
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FocusTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

APawn* UInteractionComponent::GetOwnerPawn() const
{
	return Cast<APawn>(GetOwner());
}

void UInteractionComponent::UpdateFocus()
{
	APawn* Pawn = GetOwnerPawn();
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		// Only the player at the keyboard needs focus/prompt. Clear any stale focus (e.g. after unpossess).
		if (FocusedActor.IsValid())
		{
			SetFocus(nullptr);
		}
		return;
	}

	AController* Controller = Pawn->GetController();
	if (!Controller)
	{
		return;
	}

	// Trace from the camera so the player interacts with what is at the screen centre.
	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	// Third-person camera sits behind the pawn, so extend the trace by the camera->pawn distance.
	const FVector PawnLocation = Pawn->GetActorLocation();
	const float CameraToPawn = FVector::Dist(ViewLocation, PawnLocation);
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * (CameraToPawn + InteractionDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionFocusTrace), false, Pawn);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, SHCollision::Interact, Params);

	AActor* NewFocus = nullptr;
	if (bHit && Hit.GetActor())
	{
		AActor* HitActor = Hit.GetActor();

		const bool bInRange = FVector::Dist(PawnLocation, Hit.ImpactPoint) <= InteractionDistance;

		// The Interact channel ignores walls, so make sure nothing solid is in between.
		if (bInRange && HasLineOfSight(ViewLocation, Hit.ImpactPoint, HitActor) && IsInteractable(HitActor, Pawn))
		{
			NewFocus = HitActor;
		}
	}

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), ViewLocation, bHit ? Hit.ImpactPoint : TraceEnd, NewFocus ? FColor::Green : FColor::Red, false, TraceInterval);
	}

	SetFocus(NewFocus);
}

bool UInteractionComponent::HasLineOfSight(const FVector& From, const FVector& To, AActor* Target) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionLOSTrace), false, GetOwner());
	Params.AddIgnoredActor(Target);

	FHitResult Blocker;
	return !GetWorld()->LineTraceSingleByChannel(Blocker, From, To, ECC_Visibility, Params);
}

bool UInteractionComponent::IsInteractable(AActor* Target, APawn* InstigatorPawn) const
{
	return IsValid(Target)
		&& Target->Implements<UInteractable>()
		&& IInteractable::Execute_CanInteract(Target, InstigatorPawn);
}

void UInteractionComponent::SetFocus(AActor* NewFocus)
{
	const FText NewPrompt = NewFocus ? IInteractable::Execute_GetInteractionPrompt(NewFocus) : FText::GetEmpty();

	// Broadcast only on change: different actor, or same actor with a new prompt ("Open" -> "Close").
	const bool bActorChanged = FocusedActor.Get() != NewFocus;
	const bool bPromptChanged = !NewPrompt.EqualTo(FocusedPrompt);

	if (!bActorChanged && !bPromptChanged)
	{
		return;
	}

	FocusedActor = NewFocus;
	FocusedPrompt = NewPrompt;
	OnFocusChanged.Broadcast(NewFocus, NewPrompt);
}

void UInteractionComponent::TryInteract()
{
	AActor* Target = FocusedActor.Get();
	if (!Target)
	{
		return;
	}

	// On a listen server host this executes locally; on a client it is sent to the server.
	Server_Interact(Target);
}

void UInteractionComponent::Server_Interact_Implementation(AActor* Target)
{
	APawn* Pawn = GetOwnerPawn();
	if (!Pawn || !IsValid(Target))
	{
		return;
	}

	// Never trust the client: re-check everything with the server's state.
	if (!IsInteractable(Target, Pawn))
	{
		UE_LOG(LogInteraction, Warning, TEXT("Server rejected interaction: %s is not interactable for %s"), *GetNameSafe(Target), *GetNameSafe(Pawn));
		return;
	}

	// Distance from the pawn to the closest point of the target's bounds (works for big actors like doors).
	const FBox TargetBounds = Target->GetComponentsBoundingBox();
	const float MaxDistance = InteractionDistance + ServerDistanceTolerance;
	const float DistSq = TargetBounds.IsValid
		? TargetBounds.ComputeSquaredDistanceToPoint(Pawn->GetActorLocation())
		: FVector::DistSquared(Target->GetActorLocation(), Pawn->GetActorLocation());

	if (DistSq > FMath::Square(MaxDistance))
	{
		UE_LOG(LogInteraction, Warning, TEXT("Server rejected interaction: %s too far from %s (%.0f > %.0f)"),
			*GetNameSafe(Pawn), *GetNameSafe(Target), FMath::Sqrt(DistSq), MaxDistance);
		return;
	}

	IInteractable::Execute_Interact(Target, Pawn);
}
