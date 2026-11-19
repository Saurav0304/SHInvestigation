// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InteractionPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Interaction/InteractionComponent.h"

TSharedRef<SWidget> UInteractionPromptWidget::RebuildWidget()
{
	// Build a default layout only when no Blueprint layout exists (pure C++ class or empty WBP).
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = Canvas;

		PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
		PromptText->SetJustification(ETextJustify::Center);
		PromptText->SetShadowOffset(FVector2D(1.f, 1.f));
		PromptText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));

		FSlateFontInfo Font = PromptText->GetFont();
		Font.Size = DefaultFontSize;
		PromptText->SetFont(Font);

		// Bottom-centre of the screen, sized to its text.
		if (UCanvasPanelSlot* TextSlot = Canvas->AddChildToCanvas(PromptText))
		{
			TextSlot->SetAnchors(FAnchors(0.5f, 1.f));
			TextSlot->SetAlignment(FVector2D(0.5f, 1.f));
			TextSlot->SetPosition(FVector2D(0.f, -DefaultBottomOffset));
			TextSlot->SetAutoSize(true);
		}
	}

	return Super::RebuildWidget();
}

void UInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Hidden until something is focused.
	SetVisibility(ESlateVisibility::Collapsed);
	bPromptVisible = false;
}

void UInteractionPromptWidget::NativeDestruct()
{
	SetInteractionComponent(nullptr);

	Super::NativeDestruct();
}

void UInteractionPromptWidget::SetInteractionComponent(UInteractionComponent* NewComponent)
{
	if (BoundComponent.Get() == NewComponent)
	{
		return;
	}

	if (UInteractionComponent* OldComponent = BoundComponent.Get())
	{
		OldComponent->OnFocusChanged.RemoveDynamic(this, &UInteractionPromptWidget::HandleFocusChanged);
	}

	BoundComponent = NewComponent;

	if (NewComponent)
	{
		NewComponent->OnFocusChanged.AddDynamic(this, &UInteractionPromptWidget::HandleFocusChanged);

		// Show the current state immediately in case something is already focused.
		HandleFocusChanged(NewComponent->GetFocusedActor(), NewComponent->GetFocusedPrompt());
	}
	else
	{
		HandleFocusChanged(nullptr, FText::GetEmpty());
	}
}

void UInteractionPromptWidget::HandleFocusChanged(AActor* NewFocus, const FText& Prompt)
{
	const bool bShow = NewFocus != nullptr;

	if (bShow && PromptText)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Prompt"), Prompt);
		PromptText->SetText(FText::Format(PromptFormat, Args));
	}

	// HitTestInvisible: visible but never steals mouse clicks.
	SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (bShow != bPromptVisible)
	{
		bPromptVisible = bShow;
		OnPromptVisibilityChanged(bShow);
	}
}
