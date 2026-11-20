// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableBase.h"
#include "LightSwitchInteractable.generated.h"

class ADoorInteractable;
class UPointLightComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSwitchToggled, bool, bIsOn, APawn*, InstigatorPawn);

/**
 * Power switch.
 *
 * Cosmetic part (every machine): turns the light components of ControlledLights on/off whenever
 * the replicated state changes. Level lights are not replicated actors, so each machine toggles
 * its own copy from the replicated switch state, which keeps everyone in sync, late joiners included.
 *
 * Gameplay event (server only): unlocks DoorsToUnlock when switched on, and optionally re-locks
 * them when switched off. The doors then replicate their own lock/open state.
 */
UCLASS()
class SHINVESTIGATION_API ALightSwitchInteractable : public AInteractableBase
{
	GENERATED_BODY()

public:
	ALightSwitchInteractable();

	// IInteractable
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

	UFUNCTION(BlueprintPure, Category = "Switch")
	bool IsOn() const { return bIsActive; }

	/** Fired on the server after the switch changed state and the gameplay event ran. */
	UPROPERTY(BlueprintAssignable, Category = "Switch")
	FOnSwitchToggled OnSwitchToggled;

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_IsActive() override;

	/** Level actors whose light components follow the switch (point lights, spot lights...). */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Switch")
	TArray<TObjectPtr<AActor>> ControlledLights;

	/** Doors unlocked by turning the power on. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Switch|Gameplay Event")
	TArray<TObjectPtr<ADoorInteractable>> DoorsToUnlock;

	/** If true, turning the power off locks (and closes) DoorsToUnlock again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Switch|Gameplay Event")
	bool bRelockWhenSwitchedOff = true;

	/** Small status light on the switch (red = off, green = on), so the state is readable with any mesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> IndicatorLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Switch|Indicator")
	FLinearColor IndicatorOffColor = FLinearColor(1.f, 0.08f, 0.03f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Switch|Indicator")
	FLinearColor IndicatorOnColor = FLinearColor(0.1f, 1.f, 0.2f);

private:
	void UpdateLights();

	/** Server only: applies the switch state to the linked doors. */
	void ApplyGameplayEvent();
};
