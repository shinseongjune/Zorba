// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZorbaEnemyBasicAttackComponent.generated.h"

class AActor;
class UZorbaAttackDefinition;
class UZorbaMeleeCombatComponent;

/**
 * Enemy attack executor. Legacy auto-attack remains available for old
 * Blueprints, while the combat brain can drive explicit attack definitions.
 */
UCLASS(ClassGroup = (Zorba), meta = (BlueprintSpawnableComponent))
class ZORBA_API UZorbaEnemyBasicAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZorbaEnemyBasicAttackComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Zorba|Enemy|Combat")
	bool TryBasicAttack(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Zorba|Enemy|Combat")
	bool TryAttackDefinition(
		AActor* TargetActor,
		UZorbaAttackDefinition* RequestedAttack);

	UFUNCTION(BlueprintPure, Category = "Zorba|Enemy|Combat")
	bool IsTargetInAttackRange(
		AActor* TargetActor,
		const UZorbaAttackDefinition* RequestedAttack,
		float AdditionalPadding = 0.0f) const;

	float ResolveAttackRange(
		const UZorbaAttackDefinition* RequestedAttack) const;
	void SetExternallyDriven(bool bNewExternallyDriven);

	UFUNCTION(BlueprintPure, Category = "Zorba|Enemy|Combat")
	UZorbaAttackDefinition* GetAttackDefinition() const
	{
		return AttackDefinition;
	}

	void StopCrowdCombat();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Combat")
	TObjectPtr<UZorbaAttackDefinition> AttackDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Combat")
	bool bAutoAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Combat", meta = (ClampMin = "0.0", Units = "s"))
	float InitialAttackDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Combat", meta = (ClampMin = "0.0", Units = "s"))
	float AttackCooldown = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Combat", meta = (ClampMin = "0.0", Units = "cm"))
	float AttackRangePadding = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Crowd")
	bool bUseCrowdCoordination = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Crowd", meta = (ClampMin = "1"))
	int32 MaxConcurrentAttackers = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm"))
	float StandbyRadius = 340.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm"))
	float MovementAcceptanceRadius = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm"))
	float EngagementRange = 1200.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Enemy|Combat")
	void OnBasicAttackStarted(AActor* TargetActor);

private:
	bool CanEngageTarget(AActor* TargetActor) const;
	bool CanAttackTarget(AActor* TargetActor) const;
	bool AcquireAttackToken(AActor* TargetActor);
	void ReleaseAttackToken();
	void UpdateCrowdMovement(AActor* TargetActor, bool bApproachToAttack);
#if !UE_BUILD_SHIPPING
	void ApplyDefenseAutomation(AActor* TargetActor);
#endif

	TWeakObjectPtr<UZorbaMeleeCombatComponent> MeleeCombatComponent;
	TWeakObjectPtr<AActor> AttackTokenTarget;
	float NextAttackTime = 0.0f;
	bool bHasAttackToken = false;
	bool bExternallyDriven = false;
#if !UE_BUILD_SHIPPING
	bool bDefenseAutomationApplied = false;
#endif
};
