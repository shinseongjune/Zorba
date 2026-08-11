// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ZorbaPlayerController.h"

#include "Core/ZorbaGameMode.h"
#include "Core/ZorbaGameState.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ZorbaMissionFlowWidget.h"

AZorbaPlayerController::AZorbaPlayerController()
{
	bReplicates = true;
	bShowMouseCursor = false;
}

void AZorbaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		MissionFlowWidget = CreateWidget<UZorbaMissionFlowWidget>(
			this,
			UZorbaMissionFlowWidget::StaticClass());
		if (MissionFlowWidget)
		{
			MissionFlowWidget->AddToViewport(1000);
		}
		BindMissionUI();
	}
}

void AZorbaPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundGameState)
	{
		BoundGameState->OnSessionPhaseChanged.RemoveDynamic(
			this,
			&AZorbaPlayerController::HandleSessionPhaseChanged);
		BoundGameState = nullptr;
	}
	if (MissionFlowWidget)
	{
		MissionFlowWidget->RemoveFromParent();
		MissionFlowWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AZorbaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

void AZorbaPlayerController::ClientReceiveSystemMessage_Implementation(const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
}

void AZorbaPlayerController::TogglePause()
{
	const bool bShouldPause = !UGameplayStatics::IsGamePaused(this);
	UGameplayStatics::SetGamePaused(this, bShouldPause);

	bShowMouseCursor = bShouldPause;

	if (bShouldPause)
	{
		SetInputMode(FInputModeGameAndUI());
		ClientReceiveSystemMessage(TEXT("Game paused."));
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		ClientReceiveSystemMessage(TEXT("Game resumed."));
	}
}

void AZorbaPlayerController::StartDefaultMission()
{
	if (HasAuthority())
	{
		if (AZorbaGameMode* ZorbaGameMode =
			GetWorld()->GetAuthGameMode<AZorbaGameMode>())
		{
			ZorbaGameMode->StartDefaultMission();
		}
		return;
	}
	ServerStartDefaultMission();
}

void AZorbaPlayerController::RestartActiveMission()
{
	if (HasAuthority())
	{
		if (AZorbaGameMode* ZorbaGameMode =
			GetWorld()->GetAuthGameMode<AZorbaGameMode>())
		{
			ZorbaGameMode->RestartActiveMission();
		}
		return;
	}
	ServerRestartActiveMission();
}

void AZorbaPlayerController::ReturnToBoot()
{
	if (HasAuthority())
	{
		if (AZorbaGameMode* ZorbaGameMode =
			GetWorld()->GetAuthGameMode<AZorbaGameMode>())
		{
			ZorbaGameMode->ReturnToBoot();
		}
		return;
	}
	ServerReturnToBoot();
}

void AZorbaPlayerController::BindMissionUI()
{
	BoundGameState = GetWorld() ? GetWorld()->GetGameState<AZorbaGameState>() : nullptr;
	if (!BoundGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("Mission UI could not bind: ZorbaGameState is missing."));
		return;
	}

	BoundGameState->OnSessionPhaseChanged.AddUniqueDynamic(
		this,
		&AZorbaPlayerController::HandleSessionPhaseChanged);
	HandleSessionPhaseChanged(BoundGameState->GetSessionPhase());
}

void AZorbaPlayerController::HandleSessionPhaseChanged(
	EZorbaSessionPhase NewPhase)
{
	if (MissionFlowWidget)
	{
		MissionFlowWidget->RefreshForPhase(NewPhase);
	}
	ApplyMissionInputMode(NewPhase);
}

void AZorbaPlayerController::ApplyMissionInputMode(
	EZorbaSessionPhase NewPhase)
{
	const bool bUseMissionUI =
		NewPhase == EZorbaSessionPhase::Boot
		|| NewPhase == EZorbaSessionPhase::MissionLoading
		|| NewPhase == EZorbaSessionPhase::MissionComplete
		|| NewPhase == EZorbaSessionPhase::MissionFailed;
	if (bUseMissionUI)
	{
		bShowMouseCursor = true;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		FInputModeUIOnly InputMode;
		SetInputMode(InputMode);
		return;
	}

	bShowMouseCursor = false;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	SetInputMode(FInputModeGameOnly());
}

void AZorbaPlayerController::ServerStartDefaultMission_Implementation()
{
	StartDefaultMission();
}

void AZorbaPlayerController::ServerRestartActiveMission_Implementation()
{
	RestartActiveMission();
}

void AZorbaPlayerController::ServerReturnToBoot_Implementation()
{
	ReturnToBoot();
}
