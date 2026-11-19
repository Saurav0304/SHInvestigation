// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionPromptWidget.generated.h"

class UTextBlock;
class UInteractionComponent;

/**
 * Interaction prompt ("[E] Open Door").
 *
 * Works out of the box: if no layout is provided, RebuildWidget() builds a default one
 * (a centred TextBlock near the bottom of the screen).
 * To restyle it, create a Widget Blueprint child (e.g. WBP_InteractionPrompt) and add a
 * TextBlock named exactly "PromptText"; the C++ logic then drives that TextBlock instead.
 *
 * The widget only listens to UInteractionComponent::OnFocusChanged; the component knows
 * nothing about UI, which keeps gameplay and presentation decoupled.
 */
UCLASS()
class SHINVESTIGATION_API UInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds to a pawn's interaction component. Pass nullptr to unbind (e.g. pawn died / unpossessed). */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionComponent(UInteractionComponent* NewComponent);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Bound to a TextBlock named "PromptText" in a Blueprint layout, or created in C++ if there is none. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;

	/** Font size used by the default C++ layout. */
	UPROPERTY(EditAnywhere, Category = "Interaction|Default Layout", meta = (ClampMin = "8"))
	int32 DefaultFontSize = 22;

	/** Distance from the bottom of the screen used by the default C++ layout. */
	UPROPERTY(EditAnywhere, Category = "Interaction|Default Layout")
	float DefaultBottomOffset = 150.f;

	/** Format for the prompt. {Prompt} is replaced by the interactable's prompt text. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText PromptFormat = NSLOCTEXT("Interaction", "PromptFormat", "[E] {Prompt}");

	/** Optional Blueprint hook (fade animation, sound...). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnPromptVisibilityChanged(bool bVisible);

private:
	UFUNCTION()
	void HandleFocusChanged(AActor* NewFocus, const FText& Prompt);

	TWeakObjectPtr<UInteractionComponent> BoundComponent;

	bool bPromptVisible = false;
};
