// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZorbaEnemyCrowdSubsystem.generated.h"

class AActor;
class AZorbaEnemyCharacter;

/**
 * Lightweight crowd authority for the first slice. Enemies keep their own
 * combat state; this subsystem only limits simultaneous attackers and assigns
 * stable standby positions around a shared target.
 */
UCLASS()
class ZORBA_API UZorbaEnemyCrowdSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	int32 RegisterEnemy(AZorbaEnemyCharacter* Enemy);
	void UnregisterEnemy(AZorbaEnemyCharacter* Enemy);

	bool TryAcquireAttackToken(
		AZorbaEnemyCharacter* Enemy,
		AActor* TargetActor,
		int32 MaxConcurrentAttackers);
	void ReleaseAttackToken(AZorbaEnemyCharacter* Enemy);

	/** Only one parry/dodge/ambush cue may be active for a target at a time. */
	bool TryAcquireSpecialAttackToken(
		AZorbaEnemyCharacter* Enemy,
		AActor* TargetActor);
	void ReleaseSpecialAttackToken(AZorbaEnemyCharacter* Enemy);

	FVector GetStandbyLocation(
		const AZorbaEnemyCharacter* Enemy,
		const AActor* TargetActor,
		float StandbyRadius) const;

	int32 GetActiveAttackerCount(const AActor* TargetActor) const;

private:
	void CompactAttackers(AActor* TargetActor);

	TMap<TWeakObjectPtr<AZorbaEnemyCharacter>, int32> FormationSlots;
	TMap<
		TWeakObjectPtr<AActor>,
		TArray<TWeakObjectPtr<AZorbaEnemyCharacter>>> AttackersByTarget;
	TMap<
		TWeakObjectPtr<AActor>,
		TWeakObjectPtr<AZorbaEnemyCharacter>> SpecialAttackerByTarget;
	int32 NextFormationSlot = 0;
};
