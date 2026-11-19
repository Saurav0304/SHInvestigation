// Copyright Epic Games, Inc. All Rights Reserved.


#include "SHInvestigationPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "SHInvestigation.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "GameFramework/Pawn.h"
#include "Interaction/InteractionComponent.h"
#include "UI/InteractionPromptWidget.h"

ASHInvestigationPlayerController::ASHInvestigationPlayerController()
{
	// default prompt works without any Blueprint layout; a WBP child can be assigned in the BP to restyle it
	InteractionPromptWidgetClass = UInteractionPromptWidget::StaticClass();
}

void ASHInvestigationPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogSHInvestigation, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	// create the interaction prompt and bind it to whatever pawn we already have
	RefreshInteractionPrompt();
}

void ASHInvestigationPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	// On clients the pawn can arrive (replicate) before or after BeginPlay.
	// BeginPlay handles the "before" case, this handles "after" and any later re-possession.
	if (HasActorBegunPlay())
	{
		RefreshInteractionPrompt();
	}
}

void ASHInvestigationPlayerController::RefreshInteractionPrompt()
{
	// UI only exists for the player sitting at this machine
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!InteractionPromptWidget)
	{
		if (!InteractionPromptWidgetClass)
		{
			UE_LOG(LogSHInvestigation, Warning, TEXT("'%s' has no InteractionPromptWidgetClass set. Set it in the PlayerController Blueprint's Class Defaults."), *GetNameSafe(this));
			return;
		}

		InteractionPromptWidget = CreateWidget<UInteractionPromptWidget>(this, InteractionPromptWidgetClass);
		if (!InteractionPromptWidget)
		{
			UE_LOG(LogSHInvestigation, Error, TEXT("Could not spawn interaction prompt widget."));
			return;
		}

		InteractionPromptWidget->AddToPlayerScreen(1);
	}

	// works for any pawn type that has an interaction component; nullptr simply hides the prompt
	UInteractionComponent* InteractionComponent = GetPawn() ? GetPawn()->FindComponentByClass<UInteractionComponent>() : nullptr;
	InteractionPromptWidget->SetInteractionComponent(InteractionComponent);
}

void ASHInvestigationPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool ASHInvestigationPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
