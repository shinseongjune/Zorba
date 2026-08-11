// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ZorbaMissionFlowWidget.h"

#include "Player/ZorbaPlayerController.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UZorbaMissionFlowWidget::RebuildWidget()
{
	TSharedRef<SWidget> RootWidget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.Padding(FMargin(32.0f, 28.0f))
			.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.94f))
			[
				SNew(SBox)
				.WidthOverride(460.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.0f, 0.0f, 0.0f, 10.0f)
					[
						SAssignNew(TitleText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 30))
						.ColorAndOpacity(FLinearColor(0.93f, 0.78f, 0.36f))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.0f, 0.0f, 0.0f, 24.0f)
					[
						SAssignNew(DetailText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 16))
						.ColorAndOpacity(FLinearColor(0.82f, 0.84f, 0.88f))
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SAssignNew(StartButton, SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(FMargin(18.0f, 10.0f))
						.OnClicked(FOnClicked::CreateUObject(
							this,
							&UZorbaMissionFlowWidget::HandleStartClicked))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("START MISSION")))
							.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SAssignNew(RestartButton, SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(FMargin(18.0f, 10.0f))
						.OnClicked(FOnClicked::CreateUObject(
							this,
							&UZorbaMissionFlowWidget::HandleRestartClicked))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("RESTART")))
							.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SAssignNew(ReturnToBootButton, SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(FMargin(18.0f, 10.0f))
						.OnClicked(FOnClicked::CreateUObject(
							this,
							&UZorbaMissionFlowWidget::HandleReturnToBootClicked))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("RETURN TO BOOT")))
							.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18))
						]
					]
				]
			]
		];

	RefreshForPhase(CurrentPhase);
	return RootWidget;
}

void UZorbaMissionFlowWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	TitleText.Reset();
	DetailText.Reset();
	StartButton.Reset();
	RestartButton.Reset();
	ReturnToBootButton.Reset();
}

void UZorbaMissionFlowWidget::RefreshForPhase(EZorbaSessionPhase NewPhase)
{
	CurrentPhase = NewPhase;
	const bool bIsBoot = NewPhase == EZorbaSessionPhase::Boot;
	const bool bIsLoading = NewPhase == EZorbaSessionPhase::MissionLoading;
	const bool bIsComplete = NewPhase == EZorbaSessionPhase::MissionComplete;
	const bool bIsFailed = NewPhase == EZorbaSessionPhase::MissionFailed;
	const bool bShowWidget = bIsBoot || bIsLoading || bIsComplete || bIsFailed;
	SetVisibility(bShowWidget ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (TitleText.IsValid() && DetailText.IsValid())
	{
		if (bIsBoot)
		{
			TitleText->SetText(FText::FromString(TEXT("MISSION M01")));
			DetailText->SetText(FText::FromString(
				TEXT("Clear the castle courtyard and survive the encounter.")));
		}
		else if (bIsLoading)
		{
			TitleText->SetText(FText::FromString(TEXT("LOADING M01")));
			DetailText->SetText(FText::FromString(TEXT("Preparing the courtyard...")));
		}
		else if (bIsComplete)
		{
			TitleText->SetText(FText::FromString(TEXT("MISSION COMPLETE")));
			DetailText->SetText(FText::FromString(TEXT("The courtyard is clear.")));
		}
		else if (bIsFailed)
		{
			TitleText->SetText(FText::FromString(TEXT("MISSION FAILED")));
			DetailText->SetText(FText::FromString(TEXT("You fell before the encounter was cleared.")));
		}
	}

	if (StartButton.IsValid())
	{
		StartButton->SetVisibility(bIsBoot ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (RestartButton.IsValid())
	{
		RestartButton->SetVisibility(
			bIsComplete || bIsFailed ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (ReturnToBootButton.IsValid())
	{
		ReturnToBootButton->SetVisibility(
			bIsComplete || bIsFailed ? EVisibility::Visible : EVisibility::Collapsed);
	}
}

FReply UZorbaMissionFlowWidget::HandleStartClicked()
{
	if (AZorbaPlayerController* PlayerController =
		GetOwningPlayer<AZorbaPlayerController>())
	{
		PlayerController->StartDefaultMission();
	}
	return FReply::Handled();
}

FReply UZorbaMissionFlowWidget::HandleRestartClicked()
{
	if (AZorbaPlayerController* PlayerController =
		GetOwningPlayer<AZorbaPlayerController>())
	{
		PlayerController->RestartActiveMission();
	}
	return FReply::Handled();
}

FReply UZorbaMissionFlowWidget::HandleReturnToBootClicked()
{
	if (AZorbaPlayerController* PlayerController =
		GetOwningPlayer<AZorbaPlayerController>())
	{
		PlayerController->ReturnToBoot();
	}
	return FReply::Handled();
}
