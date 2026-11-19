// Fill out your copyright notice in the Description page of Project Settings.


#include "Interaction/InteractableBase.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"

AInteractableBase::AInteractableBase()
{
	// No per-frame logic; state changes are event driven.
	PrimaryActorTick.bCanEverTick = false;

	// Required so server-side state changes reach clients.
	bReplicates = true;
	SetReplicateMovement(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	// The Interact channel defaults to Ignore, so opt this mesh in explicitly.
	Mesh->SetCollisionResponseToChannel(SHCollision::Interact, ECR_Block);

	PromptText = NSLOCTEXT("Interaction", "DefaultPrompt", "Interact");
}

void AInteractableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInteractableBase, bInteractionEnabled);
	DOREPLIFETIME(AInteractableBase, bIsActive);
}

void AInteractableBase::Interact_Implementation(APawn* InstigatorPawn)
{
	// Gameplay state is only ever changed by the server.
	if (!HasAuthority())
	{
		return;
	}

	bIsActive = !bIsActive;

	// OnRep is not called automatically on the server in C++, so call it for the host.
	OnRep_IsActive();

	// Push the change out now instead of waiting for the next net update.
	ForceNetUpdate();

	UE_LOG(LogInteraction, Log, TEXT("%s interacted with %s -> bIsActive=%s (Authority=%s)"),
		*GetNameSafe(InstigatorPawn), *GetName(),
		bIsActive ? TEXT("true") : TEXT("false"),
		HasAuthority() ? TEXT("true") : TEXT("false"));

	OnInteracted(InstigatorPawn);
}

bool AInteractableBase::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return bInteractionEnabled;
}

FText AInteractableBase::GetInteractionPrompt_Implementation() const
{
	if (bIsActive && !ActivePromptText.IsEmpty())
	{
		return ActivePromptText;
	}
	return PromptText;
}

void AInteractableBase::SetInteractionEnabled(bool bEnabled)
{
	if (!HasAuthority() || bInteractionEnabled == bEnabled)
	{
		return;
	}

	bInteractionEnabled = bEnabled;
	ForceNetUpdate();
}

void AInteractableBase::BeginPlay()
{
	Super::BeginPlay();

	// Show the correct look for the initial state (e.g. an instance placed with bIsActive = true).
	UpdateStateMaterial();
}

void AInteractableBase::UpdateStateMaterial()
{
	UMaterialInterface* StateMaterial = bIsActive ? ActiveMaterial : InactiveMaterial;
	if (Mesh && StateMaterial)
	{
		Mesh->SetMaterial(0, StateMaterial);
	}
}

void AInteractableBase::OnRep_IsActive()
{
	UpdateStateMaterial();
	OnActiveStateChanged(bIsActive);
}
