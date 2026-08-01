// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/ZorbaAttackDefinition.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZorbaEnemyCombatProfile.generated.h"

UENUM(BlueprintType)
enum class EZorbaEnemyBrainState : uint8
{
	Idle,
	Observe,
	Approach,
	Attack,
	Recover,
	Defend,
	Disabled
};

UENUM(BlueprintType)
enum class EZorbaEnemyObserveAction : uint8
{
	Watch,
	Strafe,
	SpecialMotion
};

USTRUCT(BlueprintType)
struct ZORBA_API FZorbaEnemyAttackOption
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UZorbaAttackDefinition> AttackDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	/** Distance from which the brain may commit to this attack. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "cm"))
	float MinimumSelectionDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumSelectionDistance = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "s"))
	float Cooldown = 2.5f;

	/** Useful for a long-range ambush that commits from far away. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.1"))
	float ApproachSpeedMultiplier = 1.0f;
};

/**
 * Small data contract for enemy combat decisions. It deliberately contains no
 * Behavior Tree nodes or navigation policy, so those can replace the brain's
 * movement layer later without changing authored attacks.
 */
UCLASS(BlueprintType)
class ZORBA_API UZorbaEnemyCombatProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target", meta = (ClampMin = "0.0", Units = "cm"))
	float EngagementRange = 1400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target", meta = (ClampMin = "0.0", Units = "cm"))
	float LeashRange = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Observe", meta = (ClampMin = "0.0", Units = "cm"))
	float PreferredObserveDistance = 420.0f;

	/** The enemy corrects its distance only after leaving this band. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Observe", meta = (ClampMin = "0.0", Units = "cm"))
	float ObserveDistanceTolerance = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Observe", meta = (ClampMin = "0.05", Units = "s"))
	float MinimumObserveDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Observe", meta = (ClampMin = "0.05", Units = "s"))
	float MaximumObserveDuration = 1.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Observe", meta = (ClampMin = "0.0"))
	float WatchWeight = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Observe", meta = (ClampMin = "0.0"))
	float StrafeWeight = 0.45f;

	/** A hook only; authored idle/taunt Montages can be added later in Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Observe", meta = (ClampMin = "0.0"))
	float SpecialMotionWeight = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1.0", Units = "cm"))
	float MovementAcceptanceRadius = 35.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float ObserveSpeedMultiplier = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float RetreatSpeedMultiplier = 0.6f;

	/** A moving look-ahead point that makes Strafe orbit continuously. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1.0", Units = "cm"))
	float StrafeStepDistance = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.05", Units = "s"))
	float DecisionInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "s"))
	float InitialAttackDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "s"))
	float PostAttackRecovery = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "cm"))
	float AttackRangePadding = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "1"))
	int32 MaxConcurrentAttackers = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TArray<FZorbaEnemyAttackOption> Attacks;

	/** Chance checked between attacks, never while another action is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defense", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DefenseChance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defense", meta = (ClampMin = "0.0", Units = "s"))
	float DefenseDuration = 2.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Defense", meta = (ClampMin = "0.0", Units = "s"))
	float DefenseCooldown = 4.0f;

	/** Enrage is an offensive pattern: movement speeds up and defensive choices stop. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enrage", meta = (ClampMin = "1.0"))
	float EnragedMovementSpeedMultiplier = 1.35f;

	/** Multiplies decision, observe, attack-cooldown and recovery delays while enraged. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enrage", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float EnragedActionDelayMultiplier = 0.6f;
};
