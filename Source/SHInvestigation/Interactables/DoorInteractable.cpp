// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/DoorInteractable.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

ADoorInteractable::ADoorInteractable()
{
	// Tick is only switched on while the door is swinging.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(Root);

	// Re-parent the inherited mesh under the hinge so it rotates with it.
	Mesh->SetupAttachment(Hinge);

	PromptText = NSLOCTEXT("Door", "OpenPrompt", "Open Door");
	ActivePromptText = NSLOCTEXT("Door", "ClosePrompt", "Close Door");
	LockedPromptText = NSLOCTEXT("Door", "LockedPrompt", "Locked");
}

void ADoorInteractable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADoorInteractable, bLocked);
	DOREPLIFETIME(ADoorInteractable, OpenDirection);
}

void ADoorInteractable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyDoorShape();
}

void ADoorInteractable::ApplyDoorShape()
{
	if (!Mesh || !Mesh->GetStaticMesh())
	{
		return;
	}

	const FBox Bounds = Mesh->GetStaticMesh()->GetBoundingBox();
	const FVector Size = Bounds.GetSize();
	if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER || Size.Z <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Scale(DoorWidth / Size.X, DoorThickness / Size.Y, DoorHeight / Size.Z);
	Mesh->SetRelativeScale3D(Scale);

	// Mesh min-X edge on the hinge, centred in thickness, bottom on the floor.
	Mesh->SetRelativeLocation(FVector(-Bounds.Min.X * Scale.X, -Bounds.GetCenter().Y * Scale.Y, -Bounds.Min.Z * Scale.Z));
}

void ADoorInteractable::BeginPlay()
{
	Super::BeginPlay();

	// Snap to the current state (covers doors placed open and players joining late).
	Hinge->SetRelativeRotation(FRotator(0.f, GetTargetYaw(), 0.f));
}

void ADoorInteractable::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bLocked)
	{
		UE_LOG(LogInteraction, Log, TEXT("%s tried locked door %s"), *GetNameSafe(InstigatorPawn), *GetName());
		OnTriedLockedDoor(InstigatorPawn);
		return;
	}

	// Not calling Super: the base class toggles bIsActive directly, the door goes through SetOpen.
	SetOpen(!bIsActive, InstigatorPawn);
	OnInteracted(InstigatorPawn);
}

void ADoorInteractable::SetOpen(bool bOpen, APawn* InstigatorPawn)
{
	// Early-out also stops the paired door from bouncing the call back to us.
	if (!HasAuthority() || bIsActive == bOpen || (bOpen && bLocked))
	{
		return;
	}

	if (bOpen && bOpenAwayFromInstigator && InstigatorPawn)
	{
		// Local +Y side of the door? Swing towards local -Y, and the other way round.
		const FVector ToInstigator = InstigatorPawn->GetActorLocation() - GetActorLocation();
		OpenDirection = FVector::DotProduct(ToInstigator, GetActorRightVector()) > 0.f ? -1.f : 1.f;
	}

	bIsActive = bOpen;

	// C++ OnRep does not run on the server automatically.
	OnRep_IsActive();
	ForceNetUpdate();

	UE_LOG(LogInteraction, Log, TEXT("Door %s %s by %s"), *GetName(), bIsActive ? TEXT("opened") : TEXT("closed"), *GetNameSafe(InstigatorPawn));

	if (PairedDoor)
	{
		PairedDoor->SetOpen(bOpen, InstigatorPawn);
	}
}

void ADoorInteractable::SetLocked(bool bNewLocked)
{
	if (!HasAuthority() || bLocked == bNewLocked)
	{
		return;
	}

	if (bNewLocked && bIsActive)
	{
		SetOpen(false);
	}

	bLocked = bNewLocked;
	OnRep_Locked();
	ForceNetUpdate();

	UE_LOG(LogInteraction, Log, TEXT("Door %s %s"), *GetName(), bLocked ? TEXT("locked") : TEXT("unlocked"));

	if (PairedDoor)
	{
		PairedDoor->SetLocked(bNewLocked);
	}
}

FText ADoorInteractable::GetInteractionPrompt_Implementation() const
{
	return bLocked ? LockedPromptText : Super::GetInteractionPrompt_Implementation();
}

void ADoorInteractable::OnRep_IsActive()
{
	Super::OnRep_IsActive();

	// Start the local swing animation.
	SetActorTickEnabled(true);
}

void ADoorInteractable::OnRep_Locked()
{
	OnLockedStateChanged(bLocked);
}

float ADoorInteractable::GetTargetYaw() const
{
	return bIsActive ? OpenAngle * OpenDirection : 0.f;
}

void ADoorInteractable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float TargetYaw = GetTargetYaw();
	const float CurrentYaw = Hinge->GetRelativeRotation().Yaw;
	const float NewYaw = FMath::FInterpConstantTo(CurrentYaw, TargetYaw, DeltaSeconds, OpenSpeed);

	Hinge->SetRelativeRotation(FRotator(0.f, NewYaw, 0.f));

	if (FMath::IsNearlyEqual(NewYaw, TargetYaw, 0.1f))
	{
		Hinge->SetRelativeRotation(FRotator(0.f, TargetYaw, 0.f));
		SetActorTickEnabled(false);
	}
}
