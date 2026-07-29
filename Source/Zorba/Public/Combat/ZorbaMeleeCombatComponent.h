// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZorbaMeleeCombatComponent.generated.h"

class AActor;
class UAbilitySystemComponent;
class UAnimInstance;
class UAnimMontage;
class UPrimitiveComponent;
class USceneComponent;
class UZorbaAttackDefinition;
struct FZorbaAttackHitPhase;

UCLASS(ClassGroup = (Zorba), meta = (BlueprintSpawnableComponent))
class ZORBA_API UZorbaMeleeCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZorbaMeleeCombatComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	bool BeginAttack(
		UZorbaAttackDefinition* AttackDefinition,
		const FVector& AttackDirection,
		AActor* LockedTarget = nullptr);

	bool TransitionAttack(
		UZorbaAttackDefinition* AttackDefinition,
		const FVector& AttackDirection);

	AActor* FindBestEligibleTarget(
		UZorbaAttackDefinition* AttackDefinition,
		const FVector& AttackDirection) const;

	UFUNCTION(BlueprintCallable, Category = "Zorba|Combat")
	bool InterruptAttack(float BlendOutTime = 0.1f);

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	FVector GetCurrentAttackDirection() const;

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	bool IsAttackInProgress() const { return bAttackInProgress; }

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	bool IsHitWindowOpen() const { return bHitWindowOpen; }

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	bool IsHeavyBranchWindowOpen() const
	{
		return bHeavyBranchWindowOpen;
	}

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	UZorbaAttackDefinition* GetActiveAttackDefinition() const
	{
		return ActiveAttackDefinition;
	}

	/** Applies the execution Data Asset's recovery and a short, independently-owned invulnerability window. */
	void ApplyInstantExecutionBenefits(
		const UZorbaAttackDefinition* ExecutionDefinition);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaAttackDefinition> ActiveAttackDefinition;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zorba|Combat")
	FVector CurrentAttackDirection = FVector::ForwardVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zorba|Combat")
	bool bAttackInProgress = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zorba|Combat")
	bool bHitWindowOpen = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zorba|Combat")
	bool bHitCommitted = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zorba|Combat")
	bool bHitLanded = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zorba|Combat")
	bool bHeavyBranchWindowOpen = false;

	/** First sacred doctrine: player melee attacks from this rear cone bypass directional defense. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Sacred Doctrine")
	bool bRearAttackDoctrineEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Sacred Doctrine", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees"))
	float RearAttackHalfAngleDegrees = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Sacred Doctrine", meta = (ClampMin = "1.0"))
	float RearAttackHealthDamageMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Debug")
	bool bDrawAttackDirection = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Trace", meta = (ClampMin = "0.0", Units = "cm"))
	float WeaponTraceRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Trace")
	FName TraceBaseComponentName = TEXT("Trace_Base");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Trace")
	FName TraceTipComponentName = TEXT("Trace_Tip");

private:
	UFUNCTION()
	void HandleMontageNotifyBegin(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload);

	UFUNCTION()
	void HandleMontageNotifyEnd(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload);

	void HandleMontageBlendingOut(
		UAnimMontage* Montage,
		bool bInterrupted);

	void HandleMontageEnded(
		UAnimMontage* Montage,
		bool bInterrupted);

	bool IsNotifyFromActiveMontage(
		const FBranchingPointNotifyPayload& BranchingPointPayload) const;

	void OpenHitWindow();
	void CloseHitWindow();
	void CommitHit();
	void CleanupAttack();
	bool CanPayAttackCost(
		const UZorbaAttackDefinition* AttackDefinition) const;
	void PayAttackCost(
		const UZorbaAttackDefinition* AttackDefinition);
	void BeginLockedTargetPresentation();
	void EndLockedTargetPresentation(bool bAttackCompleted);
	void ApplySourceRecovery();
	void ApplySourceRecoveryFromDefinition(
		const UZorbaAttackDefinition* AttackDefinition,
		const TCHAR* RecoveryReason);
	void ClearInstantExecutionInvulnerability();
	void BindAnimInstance(UAnimInstance* AnimInstance);
	void UnbindAnimInstance();

	void ResolveTraceComponents();
	USceneComponent* FindSceneComponentByName(FName ComponentName) const;
	void SampleWeaponTrace();
	void SweepTraceSegment(const FVector& Start, const FVector& End);
	void RememberWeaponContact(const FHitResult& HitResult);

	void GatherEligibleTargets(
		const UZorbaAttackDefinition* AttackDefinition,
		const FZorbaAttackHitPhase& HitPhase,
		const FVector& AttackDirection,
		bool bRespectAlreadyHit,
		TArray<AActor*>& OutTargets) const;

	bool IsEligibleTarget(
		AActor* TargetActor,
		const UZorbaAttackDefinition* AttackDefinition,
		const FZorbaAttackHitPhase& HitPhase,
		const FVector& PhaseOrigin,
		const FVector& PhaseDirection,
		bool bRespectAlreadyHit = true) const;

	FHitResult ResolveHitResult(
		AActor* TargetActor,
		const FVector& PhaseOrigin) const;

	bool ApplyDamageToTarget(
		AActor* TargetActor,
		const FZorbaAttackHitPhase& HitPhase,
		const FHitResult& HitResult);
	bool IsSacredDoctrineRearAttack(
		const AActor* SourceActor,
		const AActor* TargetActor) const;

	TWeakObjectPtr<UAnimInstance> BoundAnimInstance;
	TWeakObjectPtr<UAnimMontage> ActiveMontage;
	TWeakObjectPtr<AActor> ActiveLockedTarget;
	TWeakObjectPtr<UAnimInstance> LockedTargetAnimInstance;
	TWeakObjectPtr<UAnimMontage> ActiveTargetMontage;
	TWeakObjectPtr<UAbilitySystemComponent> AttackTagAbilitySystem;
	TWeakObjectPtr<UAbilitySystemComponent> InstantExecutionAbilitySystem;
	int32 ActiveMontageInstanceId = INDEX_NONE;
	TWeakObjectPtr<USceneComponent> TraceBaseComponent;
	TWeakObjectPtr<USceneComponent> TraceTipComponent;

	FVector PreviousTraceBase = FVector::ZeroVector;
	FVector PreviousTraceTip = FVector::ZeroVector;
	bool bHasPreviousTraceSample = false;

	TMap<TWeakObjectPtr<AActor>, FHitResult> WeaponContacts;
	TSet<TWeakObjectPtr<AActor>> ActorsHitThisAttack;
	FTimerHandle InstantExecutionInvulnerabilityTimerHandle;
	bool bActiveAttackGrantedInvulnerability = false;
	bool bInstantExecutionInvulnerabilityActive = false;
};
