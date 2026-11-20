// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/InteractableBase.h"
#include "DoorInteractable.generated.h"

/**
 * A single hinged door leaf.
 *
 * - Open/closed state is the inherited replicated bIsActive (true = open).
 * - The server decides everything (open, close, lock); clients only animate in OnRep.
 * - The swing animation runs locally on every machine (tick is only enabled while moving),
 *   so no transform replication is needed and late joiners snap to the correct state.
 * - Two leaves can be paired into a double door: opening/locking one does the same to the other.
 *
 * Actor setup: place the actor on the floor at the HINGE position. Local +X runs along the door
 * width, so rotate the actor 180 degrees for the right-hand leaf of a double door.
 */
UCLASS()
class SHINVESTIGATION_API ADoorInteractable : public AInteractableBase
{
	GENERATED_BODY()

public:
	ADoorInteractable();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	// IInteractable
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	/** Server only. Opens/closes this door (and its paired door). InstigatorPawn decides the swing direction. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Door")
	void SetOpen(bool bOpen, APawn* InstigatorPawn = nullptr);

	/** Server only. Locking an open door also closes it. Applied to the paired door too. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Door")
	void SetLocked(bool bNewLocked);

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen() const { return bIsActive; }

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsLocked() const { return bLocked; }

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_IsActive() override;

	/** Pivot the door leaf rotates around. The Mesh is attached to it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Hinge;

	/** Leaf size in cm. The mesh is scaled/offset to fit, so any box-like mesh works. */
	UPROPERTY(EditAnywhere, Category = "Door|Shape", meta = (ClampMin = "1"))
	float DoorWidth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Door|Shape", meta = (ClampMin = "1"))
	float DoorHeight = 280.f;

	UPROPERTY(EditAnywhere, Category = "Door|Shape", meta = (ClampMin = "1"))
	float DoorThickness = 10.f;

	UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = "0", ClampMax = "180"))
	float OpenAngle = 90.f;

	/** Swing speed in degrees per second. */
	UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = "1"))
	float OpenSpeed = 180.f;

	/** If true the door swings away from the player who opened it. */
	UPROPERTY(EditAnywhere, Category = "Door")
	bool bOpenAwayFromInstigator = true;

	/** Locked doors can still be focused (to show LockedPromptText) but the server refuses to open them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Locked, Category = "Door")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	FText LockedPromptText;

	/** Other leaf of a double door. Set it on both leaves. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Door")
	TObjectPtr<ADoorInteractable> PairedDoor;

	/** +1 / -1: which way the door swings. Chosen by the server when opening, replicated with the open state. */
	UPROPERTY(Replicated)
	float OpenDirection = 1.f;

	UFUNCTION()
	void OnRep_Locked();

	/** Blueprint hook (sound, light on the frame...). Runs on server and clients. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Door")
	void OnLockedStateChanged(bool bNewLocked);

	/** Blueprint hook for "rattle" feedback when someone tries a locked door. Server only. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Door")
	void OnTriedLockedDoor(APawn* InstigatorPawn);

private:
	float GetTargetYaw() const;

	/** Scales and offsets the mesh so its edge sits on the hinge with the configured size. */
	void ApplyDoorShape();
};
