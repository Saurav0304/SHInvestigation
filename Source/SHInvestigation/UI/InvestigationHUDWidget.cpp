// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InvestigationHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Investigation/EvidenceData.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "InvestigationHUD"

namespace InvestigationHUD
{
	// Palette
	const FLinearColor Gold(1.f, 0.78f, 0.35f);
	const FLinearColor Paper(0.93f, 0.9f, 0.82f);
	const FLinearColor Muted(0.62f, 0.6f, 0.56f);
	const FLinearColor PanelBg(0.04f, 0.035f, 0.03f, 0.92f);
	const FLinearColor CardBg(0.11f, 0.095f, 0.08f, 1.f);
	const FLinearColor ImageBg(0.02f, 0.02f, 0.02f, 1.f);
	const FLinearColor Outline(0.55f, 0.42f, 0.2f, 0.9f);

	UTextBlock* MakeText(UWidgetTree* Tree, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justify = ETextJustify::Left, const FName Typeface = NAME_None)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		if (!Typeface.IsNone())
		{
			Font.TypefaceFontName = Typeface;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Justify);
		Text->SetShadowOffset(FVector2D(1.f, 1.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		return Text;
	}

	UBorder* MakeRounded(UWidgetTree* Tree, const FLinearColor& Fill, float Radius, const FMargin& Padding, bool bOutline = false)
	{
		UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Border->SetBrush(FSlateRoundedBoxBrush(Fill, Radius, bOutline ? Outline : FLinearColor::Transparent, bOutline ? 1.5f : 0.f));
		Border->SetPadding(Padding);
		return Border;
	}

	void Place(UCanvasPanel* Canvas, UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget))
		{
			CanvasSlot->SetAnchors(Anchors);
			CanvasSlot->SetAlignment(Alignment);
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetAutoSize(true);
		}
	}

	void AddV(UVerticalBox* Box, UWidget* Widget, float BottomPadding = 8.f, EHorizontalAlignment HAlign = HAlign_Fill)
	{
		if (UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(Widget))
		{
			BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, BottomPadding));
			BoxSlot->SetHorizontalAlignment(HAlign);
		}
	}

	/** Thin gold line used as a separator. */
	UWidget* MakeDivider(UWidgetTree* Tree)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetHeightOverride(1.5f);
		UImage* Line = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Line->SetColorAndOpacity(Outline);
		Size->SetContent(Line);
		return Size;
	}

	/**
	 * Fixed-size, clipped image area. The image is scaled by a ScaleBox, so it always uses the
	 * whole area regardless of the texture's resolution.
	 * ScaleToFill covers the area completely (crops the overflowing edge),
	 * ScaleToFit shows the whole image (may leave bars).
	 */
	UWidget* MakeImageArea(UWidgetTree* Tree, const FVector2D& Size, EStretch::Type Stretch, UImage*& OutImage)
	{
		USizeBox* Area = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Area->SetWidthOverride(Size.X);
		Area->SetHeightOverride(Size.Y);

		UBorder* Frame = MakeRounded(Tree, ImageBg, 8.f, FMargin(0.f));
		Frame->SetHorizontalAlignment(HAlign_Fill);
		Frame->SetVerticalAlignment(VAlign_Fill);
		Frame->SetClipping(EWidgetClipping::ClipToBounds);
		Area->SetContent(Frame);

		UScaleBox* Scale = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Scale->SetStretch(Stretch);
		Frame->SetContent(Scale);

		OutImage = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Scale->SetContent(OutImage);
		return Area;
	}

	/** Uses the texture at its native size; the parent ScaleBox does the scaling. */
	void SetImageTexture(UImage* Image, UTexture2D* Texture)
	{
		if (Image && Texture)
		{
			Image->SetBrushFromTexture(Texture, true);
			Image->SetColorAndOpacity(FLinearColor::White);
		}
	}
}

TSharedRef<SWidget> UInvestigationHUDWidget::RebuildWidget()
{
	using namespace InvestigationHUD;

	// Default layout only when no Blueprint layout exists.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = Canvas;

		// ---- Counter pill (top right)
		UBorder* CounterPill = MakeRounded(WidgetTree, PanelBg, 14.f, FMargin(16.f, 6.f), true);
		CounterText = MakeText(WidgetTree, 16, Gold, ETextJustify::Center, TEXT("Bold"));
		CounterPill->SetContent(CounterText);
		Place(Canvas, CounterPill, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-30.f, 30.f));

		// ---- Toast (top centre)
		ToastPanel = MakeRounded(WidgetTree, PanelBg, 12.f, FMargin(20.f, 10.f), true);
		ToastText = MakeText(WidgetTree, 18, Paper, ETextJustify::Center);
		ToastPanel->SetContent(ToastText);
		Place(Canvas, ToastPanel, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.f), FVector2D(0.f, 80.f));

		// ---- Case solved banner (top centre, below toast)
		CaseSolvedPanel = MakeRounded(WidgetTree, FLinearColor(0.25f, 0.17f, 0.04f, 0.92f), 14.f, FMargin(28.f, 12.f), true);
		UTextBlock* CaseSolvedText = MakeText(WidgetTree, 26, Gold, ETextJustify::Center, TEXT("Bold"));
		CaseSolvedText->SetText(LOCTEXT("CaseSolved", "All evidence found - case solved!"));
		CaseSolvedPanel->SetContent(CaseSolvedText);
		Place(Canvas, CaseSolvedPanel, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.f), FVector2D(0.f, 140.f));

		// ---- Inspect panel (centre)
		InspectPanel = MakeRounded(WidgetTree, PanelBg, 16.f, FMargin(24.f), true);
		UVerticalBox* InspectBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		InspectPanel->SetContent(InspectBox);

		UTextBlock* InspectLabel = MakeText(WidgetTree, 12, Muted, ETextJustify::Left, TEXT("Bold"));
		InspectLabel->SetText(LOCTEXT("InspectLabel", "EVIDENCE COLLECTED"));
		AddV(InspectBox, InspectLabel, 4.f);

		InspectTitle = MakeText(WidgetTree, 28, Gold, ETextJustify::Left, TEXT("Bold"));
		AddV(InspectBox, InspectTitle, 12.f);

		UImage* NewInspectImage = nullptr;
		AddV(InspectBox, MakeImageArea(WidgetTree, InspectImageSize, EStretch::ScaleToFill, NewInspectImage), 14.f);
		InspectImage = NewInspectImage;

		InspectBody = MakeText(WidgetTree, 17, Paper);
		InspectBody->SetAutoWrapText(true);
		InspectBody->SetWrapTextAt(InspectImageSize.X);
		AddV(InspectBox, InspectBody, 14.f);

		AddV(InspectBox, MakeDivider(WidgetTree), 10.f);
		UTextBlock* InspectHint = MakeText(WidgetTree, 13, Muted);
		InspectHint->SetText(LOCTEXT("InspectHint", "Added to the evidence journal  -  press Tab to open it"));
		AddV(InspectBox, InspectHint, 0.f);

		Place(Canvas, InspectPanel, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

		// ---- Journal / evidence board (centre)
		JournalPanel = MakeRounded(WidgetTree, PanelBg, 18.f, FMargin(28.f, 22.f), true);
		UVerticalBox* JournalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		JournalPanel->SetContent(JournalBox);

		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* JournalTitle = MakeText(WidgetTree, 26, Gold, ETextJustify::Left, TEXT("Bold"));
		JournalTitle->SetText(LOCTEXT("JournalTitle", "EVIDENCE JOURNAL"));
		if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(JournalTitle))
		{
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}
		JournalCountText = MakeText(WidgetTree, 16, Paper, ETextJustify::Right);
		if (UHorizontalBoxSlot* CountSlot = Header->AddChildToHorizontalBox(JournalCountText))
		{
			CountSlot->SetVerticalAlignment(VAlign_Center);
			CountSlot->SetPadding(FMargin(40.f, 0.f, 0.f, 0.f));
		}
		AddV(JournalBox, Header, 10.f);
		AddV(JournalBox, MakeDivider(WidgetTree), 18.f);

		JournalCards = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		AddV(JournalBox, JournalCards, 18.f);

		AddV(JournalBox, MakeDivider(WidgetTree), 10.f);
		UTextBlock* JournalHint = MakeText(WidgetTree, 13, Muted, ETextJustify::Center);
		JournalHint->SetText(LOCTEXT("JournalHint", "Shared with your partner  -  Tab to close"));
		AddV(JournalBox, JournalHint, 0.f);

		Place(Canvas, JournalPanel, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
	}

	return Super::RebuildWidget();
}

void UInvestigationHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// HUD never takes mouse input.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (ToastPanel) { ToastPanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (CaseSolvedPanel) { CaseSolvedPanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (InspectPanel) { InspectPanel->SetVisibility(ESlateVisibility::Collapsed); }
	SetJournalVisible(false);
	RefreshCounter();
	RefreshJournal();

	TryBindGameState();
	if (!BoundGameState.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimer(BindTimerHandle, this, &UInvestigationHUDWidget::TryBindGameState, 0.2f, true);
	}
}

void UInvestigationHUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindTimerHandle);
		World->GetTimerManager().ClearTimer(InspectTimerHandle);
		World->GetTimerManager().ClearTimer(ToastTimerHandle);
	}

	if (AInvestigationGameState* GameState = BoundGameState.Get())
	{
		GameState->OnEvidenceListChanged.RemoveDynamic(this, &UInvestigationHUDWidget::HandleEvidenceListChanged);
		GameState->OnEvidenceAdded.RemoveDynamic(this, &UInvestigationHUDWidget::HandleEvidenceAdded);
		GameState->OnCaseSolved.RemoveDynamic(this, &UInvestigationHUDWidget::HandleCaseSolved);
	}
	BoundGameState.Reset();

	Super::NativeDestruct();
}

void UInvestigationHUDWidget::TryBindGameState()
{
	AInvestigationGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AInvestigationGameState>() : nullptr;
	if (!GameState)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(BindTimerHandle);

	BoundGameState = GameState;
	GameState->OnEvidenceListChanged.AddDynamic(this, &UInvestigationHUDWidget::HandleEvidenceListChanged);
	GameState->OnEvidenceAdded.AddDynamic(this, &UInvestigationHUDWidget::HandleEvidenceAdded);
	GameState->OnCaseSolved.AddDynamic(this, &UInvestigationHUDWidget::HandleCaseSolved);

	// Catch up with anything that replicated before we were bound (late join).
	HandleEvidenceListChanged();
	if (GameState->IsCaseSolved())
	{
		HandleCaseSolved();
	}
}

void UInvestigationHUDWidget::HandleEvidenceListChanged()
{
	RefreshCounter();
	RefreshJournal();
}

void UInvestigationHUDWidget::HandleEvidenceAdded(const FCollectedEvidence& Entry)
{
	if (!Entry.Evidence)
	{
		return;
	}

	if (IsLocalCollector(Entry))
	{
		ShowInspect(Entry);
	}
	else
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Evidence"), Entry.Evidence->DisplayName);
		ShowToast(FText::Format(LOCTEXT("PartnerFound", "Your partner found: {Evidence}"), Args));
	}
}

void UInvestigationHUDWidget::HandleCaseSolved()
{
	if (CaseSolvedPanel)
	{
		CaseSolvedPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UInvestigationHUDWidget::RefreshCounter()
{
	const AInvestigationGameState* GameState = BoundGameState.Get();
	FFormatNamedArguments Args;
	Args.Add(TEXT("Found"), GameState ? GameState->GetCollectedEvidence().Num() : 0);
	Args.Add(TEXT("Total"), GameState ? GameState->GetTotalEvidenceCount() : 0);

	if (CounterText)
	{
		CounterText->SetText(FText::Format(LOCTEXT("Counter", "EVIDENCE  {Found} / {Total}"), Args));
	}
	if (JournalCountText)
	{
		JournalCountText->SetText(FText::Format(LOCTEXT("JournalCount", "{Found} of {Total} found"), Args));
	}
}

void UInvestigationHUDWidget::RefreshJournal()
{
	if (!JournalCards || !WidgetTree)
	{
		return;
	}

	JournalCards->ClearChildren();

	const AInvestigationGameState* GameState = BoundGameState.Get();
	const TArray<FCollectedEvidence> Collected = GameState ? GameState->GetCollectedEvidence() : TArray<FCollectedEvidence>();
	const int32 Total = FMath::Max(GameState ? GameState->GetTotalEvidenceCount() : 0, Collected.Num());

	// Found clues first, then an "undiscovered" card for every clue still missing.
	int32 CardIndex = 0;
	auto AddCard = [this, &CardIndex](UWidget* Card)
	{
		if (UHorizontalBoxSlot* CardSlot = JournalCards->AddChildToHorizontalBox(Card))
		{
			CardSlot->SetPadding(FMargin(CardIndex == 0 ? 0.f : 16.f, 0.f, 0.f, 0.f));
			CardSlot->SetVerticalAlignment(VAlign_Fill);
		}
		++CardIndex;
	};

	for (const FCollectedEvidence& Entry : Collected)
	{
		if (Entry.Evidence)
		{
			AddCard(MakeEvidenceCard(&Entry));
		}
	}
	for (int32 Missing = Collected.Num(); Missing < Total; ++Missing)
	{
		AddCard(MakeEvidenceCard(nullptr));
	}

	if (CardIndex == 0)
	{
		UTextBlock* Empty = InvestigationHUD::MakeText(WidgetTree, 16, InvestigationHUD::Muted);
		Empty->SetText(LOCTEXT("JournalEmpty", "No evidence in this area."));
		JournalCards->AddChildToHorizontalBox(Empty);
	}
}

UWidget* UInvestigationHUDWidget::MakeEvidenceCard(const FCollectedEvidence* Entry)
{
	using namespace InvestigationHUD;

	const bool bFound = Entry && Entry->Evidence;

	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CardSize->SetWidthOverride(CardImageSize.X + 32.f);

	UBorder* Card = MakeRounded(WidgetTree, CardBg, 12.f, FMargin(16.f), bFound);
	CardSize->SetContent(Card);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Card->SetContent(Column);

	// Image fills the whole image area of the card (same size on every card so they line up).
	UImage* Image = nullptr;
	UWidget* ImageArea = MakeImageArea(WidgetTree, CardImageSize, EStretch::ScaleToFill, Image);

	if (bFound && Entry->Evidence->Icon)
	{
		SetImageTexture(Image, Entry->Evidence->Icon);
	}
	else if (UScaleBox* Scale = Cast<UScaleBox>(Image->GetParent()))
	{
		// No image (undiscovered or missing icon): centred "?" / "-" instead.
		Scale->SetStretch(EStretch::None);
		UTextBlock* Unknown = MakeText(WidgetTree, 48, bFound ? Muted : FLinearColor(0.3f, 0.28f, 0.25f), ETextJustify::Center, TEXT("Bold"));
		Unknown->SetText(bFound ? LOCTEXT("NoImage", "-") : LOCTEXT("Unknown", "?"));
		Scale->SetContent(Unknown);
	}
	AddV(Column, ImageArea, 12.f);

	UTextBlock* Name = MakeText(WidgetTree, 20, bFound ? Gold : Muted, ETextJustify::Left, TEXT("Bold"));
	Name->SetText(bFound ? Entry->Evidence->DisplayName : LOCTEXT("Undiscovered", "Undiscovered"));
	AddV(Column, Name, 2.f);

	UTextBlock* Finder = MakeText(WidgetTree, 13, Muted);
	if (bFound)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Who"), GetCollectorText(*Entry));
		Finder->SetText(FText::Format(LOCTEXT("FoundBy", "Found by {Who}"), Args));
	}
	else
	{
		Finder->SetText(LOCTEXT("KeepSearching", "Keep searching..."));
	}
	AddV(Column, Finder, 10.f);

	if (bFound)
	{
		UTextBlock* Body = MakeText(WidgetTree, 15, Paper);
		Body->SetText(Entry->Evidence->Description);
		Body->SetAutoWrapText(true);
		Body->SetWrapTextAt(CardImageSize.X);
		AddV(Column, Body, 0.f);
	}

	return CardSize;
}

void UInvestigationHUDWidget::ToggleJournal()
{
	SetJournalVisible(!bJournalVisible);
}

void UInvestigationHUDWidget::SetJournalVisible(bool bVisible)
{
	bJournalVisible = bVisible;

	if (JournalPanel)
	{
		JournalPanel->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// Opening the journal replaces the inspect popup.
	if (bVisible)
	{
		HideInspect();
	}
}

void UInvestigationHUDWidget::ShowInspect(const FCollectedEvidence& Entry)
{
	if (!InspectPanel || !Entry.Evidence)
	{
		return;
	}

	// Do not cover the journal if it is open.
	if (bJournalVisible)
	{
		return;
	}

	if (InspectTitle) { InspectTitle->SetText(Entry.Evidence->DisplayName); }
	if (InspectBody) { InspectBody->SetText(Entry.Evidence->Description); }
	if (InspectImage)
	{
		InvestigationHUD::SetImageTexture(InspectImage, Entry.Evidence->Icon);
		InspectImage->SetVisibility(Entry.Evidence->Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	InspectPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	GetWorld()->GetTimerManager().SetTimer(InspectTimerHandle, this, &UInvestigationHUDWidget::HideInspect, InspectDuration, false);
}

void UInvestigationHUDWidget::HideInspect()
{
	if (InspectPanel)
	{
		InspectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInvestigationHUDWidget::ShowToast(const FText& Message)
{
	if (!ToastText)
	{
		return;
	}

	ToastText->SetText(Message);
	if (ToastPanel) { ToastPanel->SetVisibility(ESlateVisibility::HitTestInvisible); }
	GetWorld()->GetTimerManager().SetTimer(ToastTimerHandle, this, &UInvestigationHUDWidget::HideToast, ToastDuration, false);
}

void UInvestigationHUDWidget::HideToast()
{
	if (ToastPanel)
	{
		ToastPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UInvestigationHUDWidget::IsLocalCollector(const FCollectedEvidence& Entry) const
{
	const APlayerController* PC = GetOwningPlayer();
	return PC && Entry.CollectedBy && Entry.CollectedBy == PC->PlayerState;
}

FText UInvestigationHUDWidget::GetCollectorText(const FCollectedEvidence& Entry) const
{
	if (IsLocalCollector(Entry))
	{
		return LOCTEXT("CollectorYou", "you");
	}
	return Entry.CollectedBy ? LOCTEXT("CollectorPartner", "your partner") : FText::FromString(Entry.CollectedByName);
}

#undef LOCTEXT_NAMESPACE
