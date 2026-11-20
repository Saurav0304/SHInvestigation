// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/InvestigationGameState.h"
#include "InvestigationHUDWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture2D;
class UWidget;

/**
 * Investigation HUD:
 *  - Counter      "EVIDENCE 1/2" pill (always visible)
 *  - Toast        short notification when the partner finds something
 *  - Inspect      details panel with the evidence image when YOU collect something (local only)
 *  - Journal      evidence board toggled with Tab: one card per clue (image, name, finder,
 *                 description) plus "undiscovered" cards for clues still missing
 *  - Case solved  banner once everything is found
 *
 * Driven entirely by AInvestigationGameState delegates. Builds a default layout in C++;
 * a Widget Blueprint child can provide its own layout by naming widgets the same as the
 * BindWidgetOptional properties below.
 */
UCLASS()
class SHINVESTIGATION_API UInvestigationHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Investigation")
	void ToggleJournal();

	UFUNCTION(BlueprintCallable, Category = "Investigation")
	void SetJournalVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Investigation")
	bool IsJournalVisible() const { return bJournalVisible; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CounterText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ToastPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ToastText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CaseSolvedPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> InspectPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InspectTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> InspectImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InspectBody;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> JournalPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> JournalCountText;

	/** Container the evidence cards are added to. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> JournalCards;

	/** Seconds the inspect panel stays open after collecting. */
	UPROPERTY(EditAnywhere, Category = "Investigation", meta = (ClampMin = "0.5"))
	float InspectDuration = 6.f;

	/** Seconds a toast notification stays on screen. */
	UPROPERTY(EditAnywhere, Category = "Investigation", meta = (ClampMin = "0.5"))
	float ToastDuration = 4.f;

	/** Image area on a journal card. The image fills it completely (aspect kept, edges cropped if needed). */
	UPROPERTY(EditAnywhere, Category = "Investigation|Layout")
	FVector2D CardImageSize = FVector2D(360.f, 210.f);

	/** Image area in the inspect panel. The image fills it completely (aspect kept, edges cropped if needed). */
	UPROPERTY(EditAnywhere, Category = "Investigation|Layout")
	FVector2D InspectImageSize = FVector2D(460.f, 240.f);

private:
	/** The GameState can replicate after the HUD is created on clients, so retry until it exists. */
	void TryBindGameState();

	UFUNCTION()
	void HandleEvidenceListChanged();

	UFUNCTION()
	void HandleEvidenceAdded(const FCollectedEvidence& Entry);

	UFUNCTION()
	void HandleCaseSolved();

	void RefreshCounter();
	void RefreshJournal();
	UWidget* MakeEvidenceCard(const FCollectedEvidence* Entry);
	void ShowInspect(const FCollectedEvidence& Entry);
	void HideInspect();
	void ShowToast(const FText& Message);
	void HideToast();

	bool IsLocalCollector(const FCollectedEvidence& Entry) const;
	FText GetCollectorText(const FCollectedEvidence& Entry) const;

	TWeakObjectPtr<AInvestigationGameState> BoundGameState;

	FTimerHandle BindTimerHandle;
	FTimerHandle InspectTimerHandle;
	FTimerHandle ToastTimerHandle;

	bool bJournalVisible = false;
};
