// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ZorbaGameMode.h"

#include "Core/ZorbaGameState.h"
#include "Core/ZorbaSessionTypes.h"
#include "Player/ZorbaCharacter.h"
#include "Player/ZorbaPlayerController.h"
#include "Player/ZorbaPlayerState.h"

AZorbaGameMode::AZorbaGameMode()
{
	GameStateClass = AZorbaGameState::StaticClass();
	PlayerControllerClass = AZorbaPlayerController::StaticClass();
	PlayerStateClass = AZorbaPlayerState::StaticClass();
	DefaultPawnClass = AZorbaCharacter::StaticClass();
}

void AZorbaGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>())
	{
		ZorbaGameState->SetSessionPhase(EZorbaSessionPhase::Boot);
	}
}
