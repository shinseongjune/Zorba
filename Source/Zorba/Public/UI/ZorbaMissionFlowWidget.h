// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Core/ZorbaSessionTypes.h"
#include "ZorbaMissionFlowWidget.generated.h"

/** Mission-state contract consumed by the project-owned UMG presentation. */
UCLASS(Abstract, Blueprintable)
class ZORBA_API UZorbaMissionFlowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void RefreshProgress(int32 NewRemainingEnemyCount);
	void RefreshForPhase(EZorbaSessionPhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Zorba|Mission")
	FText GetMissionObjectiveText() const
	{
		return FText::FromString(TEXT("Defeat all enemies and survive."));
	}

	UFUNCTION(BlueprintPure, Category = "Zorba|Mission")
	int32 GetRemainingEnemyCount() const
	{
		return RemainingEnemyCount;
	}

	UFUNCTION(BlueprintPure, Category = "Zorba|Mission")
	EZorbaSessionPhase GetCurrentPhase() const
	{
		return CurrentPhase;
	}

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Mission")
	void OnMissionPresentationChanged();

private:
	EZorbaSessionPhase CurrentPhase = EZorbaSessionPhase::Boot;
	int32 RemainingEnemyCount = 0;
};