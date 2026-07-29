// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "ZorbaCombatEncounter.generated.h"

class AZorbaEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FZorbaEncounterStartedSignature,
	int32, EnemyCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FZorbaEncounterCompletedSignature);

/** Tracks one local combat group and emits the mission-facing clear signal. */
UCLASS(Blueprintable)
class ZORBA_API AZorbaCombatEncounter : public AActor
{
	GENERATED_BODY()

public:
	AZorbaCombatEncounter();

	UFUNCTION(BlueprintCallable, Category = "Zorba|Encounter")
	void StartEncounter();

	UFUNCTION(BlueprintPure, Category = "Zorba|Encounter")
	int32 GetRemainingEnemyCount() const { return RemainingEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "Zorba|Encounter")
	bool IsEncounterActive() const { return bEncounterActive; }

	UPROPERTY(BlueprintAssignable, Category = "Zorba|Encounter")
	FZorbaEncounterStartedSignature OnEncounterStarted;

	UPROPERTY(BlueprintAssignable, Category = "Zorba|Encounter")
	FZorbaEncounterCompletedSignature OnEncounterCompleted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Encounter")
	FName EncounterId = TEXT("CrowdEncounter01");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Encounter")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Encounter", meta = (ClampMin = "0.0", Units = "cm"))
	float DiscoveryRadius = 1500.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Encounter")
	void OnEncounterClearPresentation();

private:
	UFUNCTION()
	void HandleEnemyDied(
		AZorbaEnemyCharacter* Enemy,
		AActor* Killer,
		FHitResult HitResult);

	void CompleteEncounter();
#if !UE_BUILD_SHIPPING
	void ConfigureCrowdAutomation();
	void RunFodderParryAutomation();
	void RunCompletionAutomation();
#endif

	TArray<TWeakObjectPtr<AZorbaEnemyCharacter>> TrackedEnemies;
	int32 RemainingEnemyCount = 0;
	bool bEncounterActive = false;
};
