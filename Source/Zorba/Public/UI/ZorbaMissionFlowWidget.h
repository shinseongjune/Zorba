// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Core/ZorbaSessionTypes.h"
#include "ZorbaMissionFlowWidget.generated.h"

class SButton;
class STextBlock;

/** Minimal vertical-slice UI for entering and resolving the current mission. */
UCLASS()
class ZORBA_API UZorbaMissionFlowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void RefreshForPhase(EZorbaSessionPhase NewPhase);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	FReply HandleStartClicked();
	FReply HandleRestartClicked();
	FReply HandleReturnToBootClicked();

	TSharedPtr<STextBlock> TitleText;
	TSharedPtr<STextBlock> DetailText;
	TSharedPtr<SButton> StartButton;
	TSharedPtr<SButton> RestartButton;
	TSharedPtr<SButton> ReturnToBootButton;
	EZorbaSessionPhase CurrentPhase = EZorbaSessionPhase::Boot;
};
