// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/LightSwitchInteractable.h"

#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Pawn.h"
#include "Interactables/DoorInteractable.h"

ALightSwitchInteractable::ALightSwitchInteractable()
{
	PromptText = NSLOCTEXT("LightSwitch", "OnPrompt", "Turn Power On");
	ActivePromptText = NSLOCTEXT("LightSwitch", "OffPrompt", "Turn Power Off");

	IndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("IndicatorLight"));
	IndicatorLight->SetupAttachment(Root);
	IndicatorLight->SetMobility(EComponentMobility::Movable);
	IndicatorLight->SetIntensityUnits(ELightUnits::Candelas);
	IndicatorLight->SetIntensity(3.f);
	IndicatorLight->SetAttenuationRadius(80.f);
	IndicatorLight->SetCastShadows(false);
	IndicatorLight->SetLightColor(IndicatorOffColor);
}

void ALightSwitchInteractable::BeginPlay()
{
	Super::BeginPlay();

	// Lights match the starting state on every machine.
	UpdateLights();

	// Server makes the doors match the starting state (e.g. power off -> door locked).
	if (HasAuthority())
	{
		ApplyGameplayEvent();
	}
}

void ALightSwitchInteractable::Interact_Implementation(APawn* InstigatorPawn)
{
	// Base class: authority check, toggles bIsActive, runs OnRep (lights) and OnInteracted.
	Super::Interact_Implementation(InstigatorPawn);

	if (!HasAuthority())
	{
		return;
	}

	ApplyGameplayEvent();
	OnSwitchToggled.Broadcast(bIsActive, InstigatorPawn);
}

void ALightSwitchInteractable::OnRep_IsActive()
{
	Super::OnRep_IsActive();

	UpdateLights();
}

void ALightSwitchInteractable::UpdateLights()
{
	if (IndicatorLight)
	{
		IndicatorLight->SetLightColor(bIsActive ? IndicatorOnColor : IndicatorOffColor);
	}

	for (AActor* LightActor : ControlledLights)
	{
		if (!LightActor)
		{
			continue;
		}

		TInlineComponentArray<ULightComponent*> LightComponents;
		LightActor->GetComponents(LightComponents);

		for (ULightComponent* Light : LightComponents)
		{
			Light->SetVisibility(bIsActive);
		}
	}
}

void ALightSwitchInteractable::ApplyGameplayEvent()
{
	if (!HasAuthority())
	{
		return;
	}

	for (ADoorInteractable* Door : DoorsToUnlock)
	{
		if (!Door)
		{
			continue;
		}

		if (bIsActive)
		{
			Door->SetLocked(false);
		}
		else if (bRelockWhenSwitchedOff)
		{
			Door->SetLocked(true);
		}
	}

	UE_LOG(LogInteraction, Log, TEXT("Switch %s: power %s, %d linked door(s) updated"),
		*GetName(), bIsActive ? TEXT("ON") : TEXT("OFF"), DoorsToUnlock.Num());
}
