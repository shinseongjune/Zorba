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
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	FName AttackId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Attack")
	TSoftObjectPtr<UAnimMontage> Montage;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|TargetAssist")
	bool bAllowTargetAssist = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|TargetAssist", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "Degrees"))
	float MaxAssistAngleDegrees = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|TargetAssist", meta = (ClampMin = "0.0", Units = "cm"))
	float AssistRange = 250.0f;
};