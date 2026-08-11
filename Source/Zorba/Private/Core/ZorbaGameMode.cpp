// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ZorbaGameMode.h"

#include "AbilitySystemComponent.h"
#include "Combat/ZorbaCombatAttributeSet.h"
#include "Core/ZorbaCampaignSubsystem.h"
#include "Core/ZorbaGameState.h"
#include "Core/ZorbaSessionTypes.h"
#include "Enemy/ZorbaCombatEncounter.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ZorbaCharacter.h"
#include "Player/ZorbaPlayerController.h"
#include "Player/ZorbaPlayerState.h"
#include "World/ZorbaMissionDefinition.h"
#if !UE_BUILD_SHIPPING
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#endif

#if !UE_BUILD_SHIPPING
namespace
{
	bool GRestartTravelVerified = false;
	bool GReturnToBootTravelVerified = false;
}
#endif

AZorbaGameMode::AZorbaGameMode()
{
	GameStateClass = AZorbaGameState::StaticClass();
	PlayerControllerClass = AZorbaPlayerController::StaticClass();
	PlayerStateClass = AZorbaPlayerState::StaticClass();
	DefaultPawnClass = AZorbaCharacter::StaticClass();

	DefaultMissionDefinition = TSoftObjectPtr<UZorbaMissionDefinition>(
		FSoftObjectPath(TEXT("/Game/00_Core/Data/DA_Mission_M01.DA_Mission_M01")));
	BootMap = TSoftObjectPtr<UWorld>(
		FSoftObjectPath(TEXT("/Game/00_Core/Maps/L_Boot.L_Boot")));
}

void AZorbaGameMode::BeginPlay()
{
	Super::BeginPlay();

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(
		this,
		true);
	UZorbaCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UZorbaCampaignSubsystem>()
		: nullptr;
	const FName PendingMissionId = Campaign
		? Campaign->GetPendingMissionId()
		: NAME_None;
	const bool bIsDirectM01 = CurrentLevelName.Equals(
		TEXT("L_M01_Graybox"),
		ESearchCase::IgnoreCase);
	const bool bIsBoot = CurrentLevelName.Equals(
		TEXT("L_Boot"),
		ESearchCase::IgnoreCase);

	if (!PendingMissionId.IsNone() || bIsDirectM01)
	{
		const FName MissionId = PendingMissionId.IsNone()
			? FName(TEXT("M01"))
			: PendingMissionId;
		if (Campaign)
		{
			Campaign->SetPendingMission(MissionId);
		}
		InitializeMission(MissionId);
	}
	else if (bIsBoot)
	{
		InitializeBoot();
	}
	else
	{
		InitializeSandbox();
	}
}

void AZorbaGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MissionSuccessTimerHandle);
	for (const TWeakObjectPtr<AZorbaCombatEncounter>& Encounter : BoundEncounters)
	{
		if (Encounter.IsValid())
		{
			Encounter->OnEncounterCompleted.RemoveDynamic(
				this,
				&AZorbaGameMode::HandleEncounterCompleted);
		}
	}
	for (const TWeakObjectPtr<AZorbaCharacter>& PlayerCharacter : BoundPlayerCharacters)
	{
		if (PlayerCharacter.IsValid())
		{
			PlayerCharacter->OnPlayerDied.RemoveDynamic(
				this,
				&AZorbaGameMode::HandlePlayerDied);
		}
	}
	BoundEncounters.Reset();
	BoundPlayerCharacters.Reset();
	Super::EndPlay(EndPlayReason);
}

void AZorbaGameMode::HandleStartingNewPlayer_Implementation(
	APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	BindPlayerCharacter(NewPlayer);
}

void AZorbaGameMode::StartDefaultMission()
{
	UZorbaMissionDefinition* MissionDefinition =
		DefaultMissionDefinition.LoadSynchronous();
	if (!MissionDefinition)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Mission start rejected: default mission definition is missing."));
		return;
	}

	StartMission(MissionDefinition);
}

void AZorbaGameMode::StartMission(UZorbaMissionDefinition* MissionDefinition)
{
	AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>();
	if (!HasAuthority()
		|| bTravelRequested
		|| !ZorbaGameState
		|| ZorbaGameState->GetSessionPhase() != EZorbaSessionPhase::Boot
		|| !MissionDefinition
		|| MissionDefinition->MissionId.IsNone()
		|| MissionDefinition->MissionMap.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("Mission start rejected: invalid authority or definition."));
		return;
	}

	if (UZorbaCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UZorbaCampaignSubsystem>()
		: nullptr)
	{
		Campaign->SetPendingMission(MissionDefinition->MissionId);
	}
	bTravelRequested = true;
	ZorbaGameState->SetActiveMissionId(MissionDefinition->MissionId);
	ZorbaGameState->SetSessionPhase(EZorbaSessionPhase::MissionLoading);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Mission travel requested: Mission=%s Map=%s"),
		*MissionDefinition->MissionId.ToString(),
		*MissionDefinition->MissionMap.ToSoftObjectPath().ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(
		this,
		MissionDefinition->MissionMap);
}

void AZorbaGameMode::RestartActiveMission()
{
	if (!HasAuthority())
	{
		return;
	}

	AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>();
	const EZorbaSessionPhase CurrentPhase = ZorbaGameState
		? ZorbaGameState->GetSessionPhase()
		: EZorbaSessionPhase::Boot;
	if (bTravelRequested
		|| !ZorbaGameState
		|| (CurrentPhase != EZorbaSessionPhase::MissionComplete
			&& CurrentPhase != EZorbaSessionPhase::MissionFailed))
	{
		UE_LOG(LogTemp, Warning, TEXT("Mission restart rejected: mission is not terminal."));
		return;
	}
	const FName MissionId = ZorbaGameState
		? ZorbaGameState->GetActiveMissionId()
		: NAME_None;
	if (MissionId.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("Mission restart rejected: no active mission id."));
		return;
	}

	if (UZorbaCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UZorbaCampaignSubsystem>()
		: nullptr)
	{
		Campaign->SetPendingMission(MissionId);
	}
	bTravelRequested = true;
	ZorbaGameState->SetSessionPhase(EZorbaSessionPhase::MissionLoading);

	if (UZorbaMissionDefinition* MissionDefinition =
		DefaultMissionDefinition.LoadSynchronous();
		MissionDefinition
			&& MissionDefinition->MissionId == MissionId
			&& !MissionDefinition->MissionMap.IsNull())
	{
		UE_LOG(LogTemp, Display, TEXT("Mission restart requested: Mission=%s"), *MissionId.ToString());
		UGameplayStatics::OpenLevelBySoftObjectPtr(
			this,
			MissionDefinition->MissionMap);
		return;
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(
		this,
		true);
	UE_LOG(LogTemp, Warning, TEXT("Mission restart using current map fallback: %s"), *CurrentLevelName);
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
}

void AZorbaGameMode::ReturnToBoot()
{
	AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>();
	const EZorbaSessionPhase CurrentPhase = ZorbaGameState
		? ZorbaGameState->GetSessionPhase()
		: EZorbaSessionPhase::Boot;
	if (!HasAuthority()
		|| bTravelRequested
		|| !ZorbaGameState
		|| (CurrentPhase != EZorbaSessionPhase::MissionComplete
			&& CurrentPhase != EZorbaSessionPhase::MissionFailed)
		|| BootMap.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("Return to boot rejected: authority or boot map is invalid."));
		return;
	}

	if (UZorbaCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UZorbaCampaignSubsystem>()
		: nullptr)
	{
		Campaign->ClearPendingMission();
	}
	bTravelRequested = true;
	ZorbaGameState->SetActiveMissionId(NAME_None);
	ZorbaGameState->SetSessionPhase(EZorbaSessionPhase::MissionLoading);

	UE_LOG(LogTemp, Display, TEXT("Return to boot requested."));
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, BootMap);
}

void AZorbaGameMode::InitializeBoot()
{
	if (UZorbaCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UZorbaCampaignSubsystem>()
		: nullptr)
	{
		Campaign->ClearPendingMission();
	}
	if (AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>())
	{
		ZorbaGameState->SetActiveMissionId(NAME_None);
		ZorbaGameState->SetSessionPhase(EZorbaSessionPhase::Boot);
	}
	UE_LOG(LogTemp, Display, TEXT("Session phase initialized: Boot"));

#if !UE_BUILD_SHIPPING
	ConfigureMissionAutomation();
#endif
}

void AZorbaGameMode::InitializeSandbox()
{
	if (UZorbaCampaignSubsystem* Campaign = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UZorbaCampaignSubsystem>()
		: nullptr)
	{
		Campaign->ClearPendingMission();
	}
	if (AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>())
	{
		ZorbaGameState->SetActiveMissionId(NAME_None);
		ZorbaGameState->SetSessionPhase(EZorbaSessionPhase::Campaign);
	}
	UE_LOG(LogTemp, Display, TEXT("Session phase initialized: Campaign sandbox"));
}

void AZorbaGameMode::InitializeMission(FName MissionId)
{
	AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>();
	if (!ZorbaGameState)
	{
		UE_LOG(LogTemp, Error, TEXT("Mission initialization failed: ZorbaGameState is missing."));
		return;
	}

	TArray<AActor*> EncounterActors;
	UGameplayStatics::GetAllActorsOfClass(
		this,
		AZorbaCombatEncounter::StaticClass(),
		EncounterActors);
	for (AActor* EncounterActor : EncounterActors)
	{
		AZorbaCombatEncounter* Encounter =
			Cast<AZorbaCombatEncounter>(EncounterActor);
		if (!Encounter)
		{
			continue;
		}
		Encounter->OnEncounterCompleted.AddUniqueDynamic(
			this,
			&AZorbaGameMode::HandleEncounterCompleted);
		BoundEncounters.Add(Encounter);
	}
	RemainingEncounterCount = BoundEncounters.Num();

	for (FConstPlayerControllerIterator Iterator =
		GetWorld()->GetPlayerControllerIterator();
		Iterator;
		++Iterator)
	{
		BindPlayerCharacter(Iterator->Get());
	}

	ZorbaGameState->SetActiveMissionId(MissionId);
	ZorbaGameState->SetSessionPhase(EZorbaSessionPhase::MissionActive);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Session phase initialized: MissionActive Mission=%s Encounters=%d"),
		*MissionId.ToString(),
		RemainingEncounterCount);
	if (RemainingEncounterCount <= 0)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Mission cannot complete: no combat encounters are registered."));
	}

#if !UE_BUILD_SHIPPING
	ConfigureMissionAutomation();
#endif
}

void AZorbaGameMode::BindPlayerCharacter(AController* Controller)
{
	AZorbaCharacter* PlayerCharacter = Controller
		? Cast<AZorbaCharacter>(Controller->GetPawn())
		: nullptr;
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->OnPlayerDied.RemoveDynamic(
		this,
		&AZorbaGameMode::HandlePlayerDied);
	PlayerCharacter->OnPlayerDied.AddUniqueDynamic(
		this,
		&AZorbaGameMode::HandlePlayerDied);
	BoundPlayerCharacters.AddUnique(PlayerCharacter);
}

void AZorbaGameMode::HandleEncounterCompleted()
{
	if (!IsMissionActive() || RemainingEncounterCount <= 0)
	{
		return;
	}

	RemainingEncounterCount = FMath::Max(0, RemainingEncounterCount - 1);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Mission encounter cleared: Remaining=%d"),
		RemainingEncounterCount);
	if (RemainingEncounterCount == 0 && !bMissionSuccessScheduled)
	{
		bMissionSuccessScheduled = true;
		MissionSuccessTimerHandle = GetWorldTimerManager().SetTimerForNextTick(
			this,
			&AZorbaGameMode::CompleteMissionIfStillActive);
	}
}

void AZorbaGameMode::CompleteMissionIfStillActive()
{
	bMissionSuccessScheduled = false;
	if (!IsMissionActive())
	{
		return;
	}

	if (AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>())
	{
		ZorbaGameState->SetSessionPhase(EZorbaSessionPhase::MissionComplete);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Session phase changed: MissionComplete Mission=%s"),
			*ZorbaGameState->GetActiveMissionId().ToString());
#if !UE_BUILD_SHIPPING
		MaybeExitAfterAutomation();
#endif
	}
}

void AZorbaGameMode::HandlePlayerDied(
	AZorbaCharacter* PlayerCharacter,
	AActor* Killer)
{
	if (!IsMissionActive())
	{
		return;
	}

	bMissionSuccessScheduled = false;
	GetWorldTimerManager().ClearTimer(MissionSuccessTimerHandle);
	if (AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>())
	{
		ZorbaGameState->SetSessionPhase(EZorbaSessionPhase::MissionFailed);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Session phase changed: MissionFailed Mission=%s Player=%s Killer=%s"),
			*ZorbaGameState->GetActiveMissionId().ToString(),
			*GetNameSafe(PlayerCharacter),
			*GetNameSafe(Killer));
#if !UE_BUILD_SHIPPING
		MaybeExitAfterAutomation();
#endif
	}
}

bool AZorbaGameMode::IsMissionActive() const
{
	const AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>();
	return ZorbaGameState
		&& ZorbaGameState->GetSessionPhase() == EZorbaSessionPhase::MissionActive;
}

#if !UE_BUILD_SHIPPING
void AZorbaGameMode::ConfigureMissionAutomation()
{
	FString TestMode;
	if (!FParse::Value(
		FCommandLine::Get(),
		TEXT("ZorbaMissionTest="),
		TestMode))
	{
		return;
	}

	const AZorbaGameState* ZorbaGameState = GetGameState<AZorbaGameState>();
	const bool bExitWhenVerified = FParse::Param(
		FCommandLine::Get(),
		TEXT("ZorbaMissionTestExit"));
	if (TestMode.Equals(TEXT("Start"), ESearchCase::IgnoreCase)
		&& ZorbaGameState
		&& ZorbaGameState->GetSessionPhase() == EZorbaSessionPhase::Boot)
	{
		FTimerHandle AutomationTimer;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&AZorbaGameMode::StartDefaultMission,
			0.25f,
			false);
		UE_LOG(LogTemp, Display, TEXT("Mission automation armed: Mode=Start"));
	}
	else if (TestMode.Equals(TEXT("Restart"), ESearchCase::IgnoreCase)
		&& GRestartTravelVerified
		&& IsMissionActive())
	{
		UE_LOG(LogTemp, Display, TEXT("Mission restart automation verified: MissionActive"));
		if (bExitWhenVerified)
		{
			FPlatformMisc::RequestExit(false);
		}
	}
	else if (TestMode.Equals(TEXT("Return"), ESearchCase::IgnoreCase)
		&& GReturnToBootTravelVerified
		&& ZorbaGameState
		&& ZorbaGameState->GetSessionPhase() == EZorbaSessionPhase::Boot)
	{
		UE_LOG(LogTemp, Display, TEXT("Mission return automation verified: Boot"));
		if (bExitWhenVerified)
		{
			FPlatformMisc::RequestExit(false);
		}
	}
	else if ((TestMode.Equals(TEXT("Fail"), ESearchCase::IgnoreCase)
			|| (TestMode.Equals(TEXT("Restart"), ESearchCase::IgnoreCase)
				&& !GRestartTravelVerified)
			|| (TestMode.Equals(TEXT("Return"), ESearchCase::IgnoreCase)
				&& !GReturnToBootTravelVerified))
		&& IsMissionActive())
	{
		FTimerHandle AutomationTimer;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&AZorbaGameMode::RunFailureAutomation,
			0.75f,
			false);
		UE_LOG(LogTemp, Display, TEXT("Mission automation armed: Mode=%s"), *TestMode);
	}
}

void AZorbaGameMode::RunFailureAutomation()
{
	AZorbaCharacter* PlayerCharacter = Cast<AZorbaCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
	UAbilitySystemComponent* AbilitySystem = PlayerCharacter
		? PlayerCharacter->GetAbilitySystemComponent()
		: nullptr;
	if (!PlayerCharacter || !AbilitySystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Mission failure automation could not find the player ASC."));
		return;
	}

	const float CurrentHealth = AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetHealthAttribute());
	AbilitySystem->ApplyModToAttribute(
		UZorbaCombatAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive,
		-CurrentHealth);
	FHitResult HitResult;
	HitResult.ImpactPoint = PlayerCharacter->GetActorLocation();
	PlayerCharacter->HandleMeleeHit(
		this,
		HitResult,
		EZorbaMeleeDefenseResult::None);
}

void AZorbaGameMode::MaybeExitAfterAutomation()
{
	if (!FParse::Param(
		FCommandLine::Get(),
		TEXT("ZorbaMissionTestExit")))
	{
		return;
	}

	FString TestMode;
	FParse::Value(
		FCommandLine::Get(),
		TEXT("ZorbaMissionTest="),
		TestMode);
	if (TestMode.Equals(TEXT("Restart"), ESearchCase::IgnoreCase)
		&& !GRestartTravelVerified)
	{
		GRestartTravelVerified = true;
		FTimerHandle AutomationTimer;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&AZorbaGameMode::RestartActiveMission,
			0.1f,
			false);
		UE_LOG(LogTemp, Display, TEXT("Mission restart automation requested."));
		return;
	}
	if (TestMode.Equals(TEXT("Return"), ESearchCase::IgnoreCase)
		&& !GReturnToBootTravelVerified)
	{
		GReturnToBootTravelVerified = true;
		FTimerHandle AutomationTimer;
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&AZorbaGameMode::ReturnToBoot,
			0.1f,
			false);
		UE_LOG(LogTemp, Display, TEXT("Mission return automation requested."));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("Mission automation complete; requesting clean exit."));
	FPlatformMisc::RequestExit(false);
}
#endif
