// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/InvestigationGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "MainMenu"

namespace MainMenu
{
	UTextBlock* MakeText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize, const FLinearColor& Color)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetText(Text);
		Block->SetColorAndOpacity(FSlateColor(Color));
		Block->SetJustification(ETextJustify::Center);
		return Block;
	}

	UButton* MakeButton(UWidgetTree* Tree, FName Name, const FText& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetContent(MakeText(Tree, NAME_None, Label, 20, FLinearColor(0.05f, 0.05f, 0.05f)));
		return Button;
	}

	void Add(UVerticalBox* Box, UWidget* Widget, float BottomPadding)
	{
		if (UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(Widget))
		{
			BoxSlot->SetHorizontalAlignment(HAlign_Fill);
			BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, BottomPadding));
		}
	}
}

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	using namespace MainMenu;

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = Canvas;

		// Full-screen dark background
		UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
		Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 1.f));
		if (UCanvasPanelSlot* BgSlot = Canvas->AddChildToCanvas(Background))
		{
			BgSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			BgSlot->SetOffsets(FMargin(0.f));
		}

		// Centered column
		USizeBox* Column = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Column"));
		Column->SetWidthOverride(420.f);
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
		Column->SetContent(Box);

		Add(Box, MakeText(WidgetTree, TEXT("Title"), LOCTEXT("Title", "SH INVESTIGATION"), 40, FLinearColor(1.f, 0.8f, 0.3f)), 6.f);
		Add(Box, MakeText(WidgetTree, TEXT("Subtitle"), LOCTEXT("Subtitle", "2-player co-op investigation"), 16, FLinearColor(0.7f, 0.7f, 0.7f)), 40.f);

		HostButton = MakeButton(WidgetTree, TEXT("HostButton"), LOCTEXT("Host", "Host Game"));
		Add(Box, HostButton, 12.f);

		JoinButton = MakeButton(WidgetTree, TEXT("JoinButton"), LOCTEXT("Join", "Join Game (LAN)"));
		Add(Box, JoinButton, 12.f);

		QuitButton = MakeButton(WidgetTree, TEXT("QuitButton"), LOCTEXT("Quit", "Quit"));
		Add(Box, QuitButton, 30.f);

		StatusText = MakeText(WidgetTree, TEXT("StatusText"), FText::GetEmpty(), 16, FLinearColor::White);
		StatusText->SetAutoWrapText(true);
		Add(Box, StatusText, 0.f);

		if (UCanvasPanelSlot* ColumnSlot = Canvas->AddChildToCanvas(Column))
		{
			ColumnSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			ColumnSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ColumnSlot->SetAutoSize(true);
		}
	}

	return Super::RebuildWidget();
}

void UMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// The menu receives keyboard focus (UI-only input mode)
	SetIsFocusable(true);
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HostButton) { HostButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleHostClicked); }
	if (JoinButton) { JoinButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleJoinClicked); }
	if (QuitButton) { QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleQuitClicked); }

	if (UInvestigationGameInstance* GameInstance = GetInvestigationGameInstance())
	{
		GameInstance->OnSessionStatusChanged.AddDynamic(this, &UMainMenuWidget::HandleSessionStatusChanged);
		// Show e.g. "Connection lost" after being sent back to the menu.
		HandleSessionStatusChanged(GameInstance->GetLastStatus(), GameInstance->IsBusy());
	}
	else
	{
		HandleSessionStatusChanged(LOCTEXT("WrongGI", "Game Instance Class is not InvestigationGameInstance (Project Settings > Maps & Modes)."), true);
	}
}

void UMainMenuWidget::NativeDestruct()
{
	if (UInvestigationGameInstance* GameInstance = GetInvestigationGameInstance())
	{
		GameInstance->OnSessionStatusChanged.RemoveDynamic(this, &UMainMenuWidget::HandleSessionStatusChanged);
	}

	Super::NativeDestruct();
}

UInvestigationGameInstance* UMainMenuWidget::GetInvestigationGameInstance() const
{
	return GetGameInstance<UInvestigationGameInstance>();
}

void UMainMenuWidget::HandleHostClicked()
{
	if (UInvestigationGameInstance* GameInstance = GetInvestigationGameInstance())
	{
		GameInstance->HostSession();
	}
}

void UMainMenuWidget::HandleJoinClicked()
{
	if (UInvestigationGameInstance* GameInstance = GetInvestigationGameInstance())
	{
		GameInstance->FindAndJoinSession();
	}
}

void UMainMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMainMenuWidget::HandleSessionStatusChanged(const FText& Status, bool bBusy)
{
	if (StatusText)
	{
		StatusText->SetText(Status);
	}

	// No double clicks while a session operation is running.
	if (HostButton) { HostButton->SetIsEnabled(!bBusy); }
	if (JoinButton) { JoinButton->SetIsEnabled(!bBusy); }
}

#undef LOCTEXT_NAMESPACE
