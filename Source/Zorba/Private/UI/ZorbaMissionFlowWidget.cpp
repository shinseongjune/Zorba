// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ZorbaMissionFlowWidget.h"

void UZorbaMissionFlowWidget::RefreshProgress(
	int32 NewRemainingEnemyCount)
{
	RemainingEnemyCount = NewRemainingEnemyCount;
	OnMissionPresentationChanged();
}

void UZorbaMissionFlowWidget::RefreshForPhase(
	EZorbaSessionPhase NewPhase)
{
	CurrentPhase = NewPhase;
	OnMissionPresentationChanged();
}