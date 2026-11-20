// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UInvestigationGameInstance;

/**
 * Main menu: Host Game / Join Game / Quit plus a status line.
 * All session logic is in UInvestigationGameInstance; this widget only forwards clicks and
 * displays OnSessionStatusChanged. Builds a default layout in C++ when no Blueprint layout exists.
 */
UCLASS()
class SHINVESTIGATION_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> JoinButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

private:
	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleJoinClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleSessionStatusChanged(const FText& Status, bool bBusy);

	UInvestigationGameInstance* GetInvestigationGameInstance() const;
};
