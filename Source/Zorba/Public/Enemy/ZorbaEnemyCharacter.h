// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "Combat/ZorbaAttackDefinition.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "TimerManager.h"
#include "ZorbaEnemyCharacter.generated.h"

class UAbilitySystemComponent;
class AZorbaEnemyCharacter;
class UStaticMeshComponent;
class UTextRenderComponent;
class UZorbaCombatAttributeSet;
class UZorbaEnemyBasicAttackComponent;
class UZorbaEnemyCombatBrainComponent;
class UZorbaEnemyCombatProfile;
class UZorbaMeleeCombatComponent;

UENUM(BlueprintType)
enum class EZorbaEnemyCombatRank : uint8
{
	Fodder UMETA(DisplayName = "Fodder"),
	Elite UMETA(DisplayName = "Elite"),
	Boss UMETA(DisplayName = "Boss")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FZorbaEnemyDeathSignature,
	AZorbaEnemyCharacter*, Enemy,
	AActor*, Killer,
	FHitResult, HitResult);

/**
 * Reusable combat root for regular enemies. AI, presentation and authored
 * reactions can derive from this class without replacing team, GAS or damage
 * ownership.
 */
UCLASS(Blueprintable)
class ZORBA_API AZorbaEnemyCharacter :
	public ACharacter,
	public IAbilitySystemInterface,
	public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AZorbaEnemyCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	UZorbaCombatAttributeSet* GetCombatAttributes() const
	{
		return CombatAttributes;
	}

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	bool IsExhausted() const;

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	EZorbaEnemyCombatRank GetCombatRank() const { return CombatRank; }

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	bool IsFodder() const
	{
		return CombatRank == EZorbaEnemyCombatRank::Fodder;
	}

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	UZorbaEnemyBasicAttackComponent* GetBasicAttackComponent() const
	{
		return BasicAttackComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Zorba|Enemy|AI")
	UZorbaEnemyCombatProfile* GetCombatProfile() const
	{
		return CombatProfile;
	}

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat|Defense")
	bool IsDefending() const;

	EZorbaMeleeDefenseResult ResolveIncomingMeleeHit(
		AActor* SourceActor,
		const UZorbaAttackDefinition* IncomingAttack,
		float& InOutHealthDamage,
		float& InOutStaminaDamage);

	UFUNCTION(BlueprintCallable, Category = "Zorba|Combat|Defense")
	void StartDefend(float Duration);

	UFUNCTION(BlueprintCallable, Category = "Zorba|Combat|Defense")
	void StopDefend();

	UPROPERTY(BlueprintAssignable, Category = "Zorba|Combat")
	FZorbaEnemyDeathSignature OnEnemyDied;

	void HandleMeleeHit(
		AActor* SourceActor,
		const FHitResult& HitResult);
	void HandleParried(
		AActor* ParryingActor,
		const FHitResult& HitResult,
		float StaminaDamage);
	void ConsumeExhausted();
	void HandleExecuted(
		AActor* ExecutingActor,
		const FHitResult& HitResult);
	void BeginSpecialAttackReaction(
		AActor* AttackingActor,
		EZorbaAttackKind AttackKind);
	void EndSpecialAttackReaction(
		EZorbaAttackKind AttackKind,
		bool bAttackLanded);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnMeleeHitReceived(
		AActor* SourceActor,
		const FHitResult& HitResult,
		float RemainingHealth,
		float RemainingStamina);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnCombatDeath(AActor* SourceActor, const FHitResult& HitResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnCombatStaminaDepleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnParried(
		AActor* ParryingActor,
		const FHitResult& HitResult,
		float RemainingStamina);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnParryInstantKill(
		AActor* ParryingActor,
		const FHitResult& HitResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnExhaustedStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnExhaustedEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnSpecialAttackReactionStarted(
		AActor* AttackingActor,
		EZorbaAttackKind AttackKind);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnSpecialAttackReactionEnded(
		EZorbaAttackKind AttackKind,
		bool bAttackLanded);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnExecuted(
		AActor* ExecutingActor,
		const FHitResult& HitResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat|Defense")
	void OnDefendStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat|Defense")
	void OnDefendStopped();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat|Defense")
	void OnGuardBroken(AActor* SourceActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaCombatAttributeSet> CombatAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaMeleeCombatComponent> MeleeCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaEnemyBasicAttackComponent> BasicAttackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|AI")
	TObjectPtr<UZorbaEnemyCombatBrainComponent> CombatBrainComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|AI")
	TObjectPtr<UZorbaEnemyCombatProfile> CombatProfile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Debug")
	TObjectPtr<UStaticMeshComponent> DebugBodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Debug")
	TObjectPtr<UTextRenderComponent> CombatRoleLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat")
	EZorbaEnemyCombatRank CombatRank = EZorbaEnemyCombatRank::Elite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Attributes", meta = (ClampMin = "1.0"))
	float InitialHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Attributes", meta = (ClampMin = "1.0"))
	float InitialCombatStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Presentation", meta = (ClampMin = "0.1"))
	float VisualScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Presentation")
	bool bShowCombatRoleLabel = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat", meta = (ClampMin = "0", ClampMax = "254"))
	uint8 TeamId = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Reaction", meta = (ClampMin = "0.0", Units = "cm/s"))
	float HitReactionStrength = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Reaction", meta = (ClampMin = "0.0", Units = "s"))
	float ParryStaggerDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Defense", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees"))
	float DefenseHalfAngleDegrees = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Defense", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockedHealthDamageMultiplier = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Defense", meta = (ClampMin = "0.0"))
	float BlockedStaminaDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Exhausted", meta = (ClampMin = "0.1", Units = "s"))
	float ExhaustedDuration = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Exhausted", meta = (ClampMin = "0.0"))
	float ExhaustedRecoveryStamina = 50.0f;

private:
	bool ApplyCombatDeltaFrom(
		AActor* SourceActor,
		const FHitResult& HitResult,
		float HealthDamage,
		float StaminaDamage);
	void Die(AActor* SourceActor, const FHitResult& HitResult);
	void ClearHitReact();
	void EnterExhausted();
	void RecoverFromExhausted();

	FTimerHandle HitReactTimerHandle;
	FTimerHandle ExhaustedTimerHandle;
	FTimerHandle DefenseTimerHandle;
	bool bSpecialAttackReactionActive = false;
};
