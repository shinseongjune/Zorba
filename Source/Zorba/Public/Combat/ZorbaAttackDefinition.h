// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScalableFloat.h"
#include "ZorbaAttackDefinition.generated.h"

class UAnimMontage;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EZorbaAttackShape : uint8
{
	ForwardArc UMETA(DisplayName = "Forward Arc"),
	SphereSweep UMETA(DisplayName = "Sphere Sweep"),
	CapsuleSweep UMETA(DisplayName = "Capsule Sweep"),
	BoxSweep UMETA(DisplayName = "Box Sweep"),
	Radial UMETA(DisplayName = "Radial")
};

UENUM(BlueprintType)
enum class EZorbaAttackDirectionPolicy : uint8
{
	MoveInputThenFacing UMETA(DisplayName = "Move Input Then Facing"),
	FacingOnly UMETA(DisplayName = "Facing Only")
};

UENUM(BlueprintType)
enum class EZorbaAttackKind : uint8
{
	Standard UMETA(DisplayName = "Standard"),
	Heavy UMETA(DisplayName = "Heavy"),
	DerivedHeavy UMETA(DisplayName = "Derived Heavy"),
	Opportunity UMETA(DisplayName = "Opportunity"),
	Execution UMETA(DisplayName = "Execution")
};

/** How an incoming attack interacts with a defender that is facing it. */
UENUM(BlueprintType)
enum class EZorbaDefenseInteraction : uint8
{
	Standard UMETA(DisplayName = "Block Or Parry"),
	GuardBreak UMETA(DisplayName = "Parry Or Guard Break"),
	DodgeOnly UMETA(DisplayName = "Dodge Only")
};

/** Player-facing read for attacks that need a distinct presentation cue. */
UENUM(BlueprintType)
enum class EZorbaAttackSignal : uint8
{
	None UMETA(DisplayName = "None"),
	Parry UMETA(DisplayName = "Parry"),
	Dodge UMETA(DisplayName = "Dodge"),
	Ambush UMETA(DisplayName = "Ambush")
};

UENUM(BlueprintType)
enum class EZorbaMeleeDefenseResult : uint8
{
	None UMETA(DisplayName = "None"),
	Blocked UMETA(DisplayName = "Blocked"),
	Parried UMETA(DisplayName = "Parried"),
	GuardBroken UMETA(DisplayName = "Guard Broken")
};

USTRUCT(BlueprintType)
struct ZORBA_API FZorbaAttackHitPhase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	FName PhaseId = TEXT("Primary");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	EZorbaAttackShape Shape = EZorbaAttackShape::ForwardArc;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack", meta = (Units = "cm"))
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack", meta = (ClampMin = "-180.0", ClampMax = "180.0", Units = "Degrees"))
	float LocalYawOffsetDegrees = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack", meta = (ClampMin = "0.0", Units = "cm"))
	float Range = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack", meta = (ClampMin = "0.0", Units = "cm"))
	float Radius = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees"))
	float HalfAngleDegrees = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack", meta = (ClampMin = "0.0", Units = "cm"))
	float HalfHeight = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack", meta = (Units = "cm"))
	FVector BoxHalfExtent = FVector(100.0f, 75.0f, 100.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack", meta = (ClampMin = "1"))
	int32 MaxTargets = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	FScalableFloat DamageMultiplier = 1.0f;
};

UCLASS(BlueprintType)
class ZORBA_API UZorbaAttackDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UZorbaAttackDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	FName AttackId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	EZorbaAttackKind AttackKind = EZorbaAttackKind::Standard;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Defense")
	EZorbaDefenseInteraction DefenseInteraction =
		EZorbaDefenseInteraction::Standard;

	/** Non-None signals participate in the crowd-wide exclusive special cue token. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Presentation")
	EZorbaAttackSignal AttackSignal = EZorbaAttackSignal::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	TSoftObjectPtr<UAnimMontage> Montage;

	/** Optional paired reaction montage for the locked target of a special attack. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	TSoftObjectPtr<UAnimMontage> TargetMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	EZorbaAttackDirectionPolicy DirectionPolicy =
		EZorbaAttackDirectionPolicy::MoveInputThenFacing;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	TArray<FZorbaAttackHitPhase> HitPhases;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Damage")
	FScalableFloat HealthDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Damage")
	FScalableFloat StaminaDamage = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Cost", meta = (ClampMin = "0.0"))
	FScalableFloat CombatStaminaCost = 0.0f;

	/** Light attacks open their heavy branch when AttackActive ends. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Combo")
	bool bOpenHeavyBranchAfterHitWindow = false;

	/** Health ratio at or below which an Execution attack may select a target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Execution", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExecutionHealthThresholdRatio = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Execution", meta = (ClampMin = "0.0"))
	FScalableFloat SourceHealthRecovery = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Execution", meta = (ClampMin = "0.0"))
	FScalableFloat SourceStaminaRecovery = 0.0f;

	/** Invulnerability granted when an execution resolves instantly without playing the paired Montage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Execution", meta = (ClampMin = "0.0", Units = "s"))
	float InstantExecutionInvulnerabilityDuration = 1.25f;

	/** Desired attacker-to-target distance before paired opportunity/execution motion begins. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Presentation", meta = (ClampMin = "0.0", Units = "cm"))
	float SpecialTargetAlignmentDistance = 110.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|TargetAssist")
	bool bAllowTargetAssist = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|TargetAssist", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "Degrees"))
	float MaxAssistAngleDegrees = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|TargetAssist", meta = (ClampMin = "0.0", Units = "cm"))
	float AssistRange = 250.0f;
};
