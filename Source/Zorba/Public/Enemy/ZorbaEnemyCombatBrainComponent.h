// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemy/ZorbaEnemyCombatProfile.h"
#include "ZorbaEnemyCombatBrainComponent.generated.h"

class AActor;
class AZorbaEnemyCharacter;
class UZorbaEnemyBasicAttackComponent;
class UZorbaEnemyCombatProfile;
class UZorbaMeleeCombatComponent;

/**
 * Minimal combat decision layer. Target choice, movement intent and attack
 * selection live here; authored attacks and hit timing remain in combat data.
 */
UCLASS(ClassGroup = (Zorba), meta = (BlueprintSpawnableComponent))
class ZORBA_API UZorbaEnemyCombatBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZorbaEnemyCombatBrainComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Zorba|Enemy|AI")
	EZorbaEnemyBrainState GetBrainState() const { return BrainState; }

	UFUNCTION(BlueprintPure, Category = "Zorba|Enemy|AI")
	EZorbaEnemyObserveAction GetObserveAction() const { return ObserveAction; }

	UFUNCTION(BlueprintCallable, Category = "Zorba|Enemy|AI")
	void RefreshCombatProfile();

	void StopBrain();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Enemy|AI")
	void OnBrainStateChanged(
		EZorbaEnemyBrainState PreviousState,
		EZorbaEnemyBrainState NewState);

	/** Presentation hook for watch/strafe/taunt-like motion. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Enemy|AI")
	void OnObserveActionStarted(EZorbaEnemyObserveAction NewAction);

	/** VFX/animation may bind here without changing attack selection. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Enemy|AI")
	void OnAttackSignalStarted(
		EZorbaAttackSignal AttackSignal,
		UZorbaAttackDefinition* AttackDefinition);

private:
	bool CanEngageTarget(AActor* Candidate) const;
	void UpdateDecision(float CurrentTime);
	void BeginObserve(float CurrentTime);
	void UpdateObserveMovement(float CurrentTime);
	bool TryBeginDefense(float CurrentTime);
	bool TrySelectAttack(float CurrentTime);
	bool IsOptionAvailable(
		const FZorbaEnemyAttackOption& Option,
		float Distance,
		float CurrentTime) const;
	void UpdateApproach(float CurrentTime);
	void UpdateAttack(float CurrentTime);
	void MoveToward(const FVector& DesiredLocation, float SpeedMultiplier);
	void FaceTarget() const;
	void SetBrainOwnsFacing(bool bOwnsFacing);
	void SetState(EZorbaEnemyBrainState NewState);
	void ReleaseCombatTokens();
	float ResolveOptionCooldown(const FZorbaEnemyAttackOption& Option) const;
#if !UE_BUILD_SHIPPING
	void ApplyAutomationSetup(AActor* CurrentTarget);
#endif

	TWeakObjectPtr<AZorbaEnemyCharacter> EnemyOwner;
	TWeakObjectPtr<AActor> TargetActor;
	TWeakObjectPtr<UZorbaEnemyBasicAttackComponent> AttackExecutor;
	TWeakObjectPtr<UZorbaMeleeCombatComponent> MeleeCombatComponent;
	TObjectPtr<UZorbaEnemyCombatProfile> ActiveProfile;
	TMap<TWeakObjectPtr<UZorbaAttackDefinition>, float> CooldownEndTimes;
	FVector HomeLocation = FVector::ZeroVector;
	EZorbaEnemyBrainState BrainState = EZorbaEnemyBrainState::Disabled;
	EZorbaEnemyObserveAction ObserveAction = EZorbaEnemyObserveAction::Watch;
	int32 SelectedAttackIndex = INDEX_NONE;
	float BaseWalkSpeed = 350.0f;
	float NextDecisionTime = 0.0f;
	float StateEndTime = 0.0f;
	float NextDefenseTime = 0.0f;
	float ObserveStrafeDirection = 1.0f;
	bool bHasAttackToken = false;
	bool bHasSpecialToken = false;
	bool bOriginalOrientRotationToMovement = true;
	bool bMovementFacingModeCaptured = false;
#if !UE_BUILD_SHIPPING
	bool bAutomationConfigured = false;
	bool bMovementAutomation = false;
	bool bMovementAutomationReported = false;
	float MovementAutomationStartTime = 0.0f;
	FVector MovementAutomationStartLocation = FVector::ZeroVector;
	FVector MovementAutomationTargetLocation = FVector::ZeroVector;
#endif
};
