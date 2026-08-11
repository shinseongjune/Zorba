// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZorbaGameMode.generated.h"

class AZorbaCharacter;
class AZorbaCombatEncounter;
class UZorbaMissionDefinition;
class UWorld;

UCLASS()
class ZORBA_API AZorbaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZorbaGameMode();

	UFUNCTION(BlueprintCallable, Category = "Zorba|Mission")
	void StartDefaultMission();

	UFUNCTION(BlueprintCallable, Category = "Zorba|Mission")
	void StartMission(UZorbaMissionDefinition* MissionDefinition);

	UFUNCTION(BlueprintCallable, Category = "Zorba|Mission")
	void RestartActiveMission();

	UFUNCTION(BlueprintCallable, Category = "Zorba|Mission")
	void ReturnToBoot();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleStartingNewPlayer_Implementation(
		APlayerController* NewPlayer) override;

	UPROPERTY(EditDefaultsOnly, Category = "Zorba|Mission")
	TSoftObjectPtr<UZorbaMissionDefinition> DefaultMissionDefinition;

	UPROPERTY(EditDefaultsOnly, Category = "Zorba|Mission")
	TSoftObjectPtr<UWorld> BootMap;

private:
	void InitializeBoot();
	void InitializeSandbox();
	void InitializeMission(FName MissionId);
	void BindPlayerCharacter(AController* Controller);
	void CompleteMissionIfStillActive();
	bool IsMissionActive() const;

	UFUNCTION()
	void HandleEncounterCompleted();

	UFUNCTION()
	void HandlePlayerDied(
		AZorbaCharacter* PlayerCharacter,
		AActor* Killer);

#if !UE_BUILD_SHIPPING
	void ConfigureMissionAutomation();
	void RunFailureAutomation();
	void MaybeExitAfterAutomation();
#endif

	TArray<TWeakObjectPtr<AZorbaCombatEncounter>> BoundEncounters;
	TArray<TWeakObjectPtr<AZorbaCharacter>> BoundPlayerCharacters;
	int32 RemainingEncounterCount = 0;
	bool bMissionSuccessScheduled = false;
	bool bTravelRequested = false;
	FTimerHandle MissionSuccessTimerHandle;
};
