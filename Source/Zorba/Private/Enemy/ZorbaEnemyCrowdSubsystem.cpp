// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/ZorbaEnemyCrowdSubsystem.h"

#include "Enemy/ZorbaEnemyCharacter.h"
#include "GameFramework/Actor.h"

int32 UZorbaEnemyCrowdSubsystem::RegisterEnemy(
	AZorbaEnemyCharacter* Enemy)
{
	if (!IsValid(Enemy))
	{
		return INDEX_NONE;
	}

	const TWeakObjectPtr<AZorbaEnemyCharacter> EnemyKey(Enemy);
	if (const int32* ExistingSlot = FormationSlots.Find(EnemyKey))
	{
		return *ExistingSlot;
	}

	const int32 AssignedSlot = NextFormationSlot++;
	FormationSlots.Add(EnemyKey, AssignedSlot);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Crowd enemy registered: Enemy=%s Slot=%d"),
		*GetNameSafe(Enemy),
		AssignedSlot);
	return AssignedSlot;
}

void UZorbaEnemyCrowdSubsystem::UnregisterEnemy(
	AZorbaEnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	ReleaseAttackToken(Enemy);
	ReleaseSpecialAttackToken(Enemy);
	FormationSlots.Remove(TWeakObjectPtr<AZorbaEnemyCharacter>(Enemy));
}

bool UZorbaEnemyCrowdSubsystem::TryAcquireAttackToken(
	AZorbaEnemyCharacter* Enemy,
	AActor* TargetActor,
	int32 MaxConcurrentAttackers)
{
	if (!IsValid(Enemy)
		|| Enemy->IsDead()
		|| !IsValid(TargetActor)
		|| MaxConcurrentAttackers <= 0)
	{
		return false;
	}

	RegisterEnemy(Enemy);
	CompactAttackers(TargetActor);
	TArray<TWeakObjectPtr<AZorbaEnemyCharacter>>& Attackers =
		AttackersByTarget.FindOrAdd(TWeakObjectPtr<AActor>(TargetActor));
	const TWeakObjectPtr<AZorbaEnemyCharacter> EnemyKey(Enemy);
	if (Attackers.Contains(EnemyKey))
	{
		return true;
	}

	if (Attackers.Num() >= MaxConcurrentAttackers)
	{
		return false;
	}

	Attackers.Add(EnemyKey);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Crowd attack token acquired: Enemy=%s Target=%s Active=%d Limit=%d"),
		*GetNameSafe(Enemy),
		*GetNameSafe(TargetActor),
		Attackers.Num(),
		MaxConcurrentAttackers);
	return true;
}

void UZorbaEnemyCrowdSubsystem::ReleaseAttackToken(
	AZorbaEnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	const TWeakObjectPtr<AZorbaEnemyCharacter> EnemyKey(Enemy);
	for (auto Iterator = AttackersByTarget.CreateIterator(); Iterator; ++Iterator)
	{
		TArray<TWeakObjectPtr<AZorbaEnemyCharacter>>& Attackers =
			Iterator.Value();
		const int32 RemovedCount = Attackers.Remove(EnemyKey);
		if (RemovedCount > 0)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("Crowd attack token released: Enemy=%s Active=%d"),
				*GetNameSafe(Enemy),
				Attackers.Num());
		}

		Attackers.RemoveAll(
			[](const TWeakObjectPtr<AZorbaEnemyCharacter>& Candidate)
			{
				return !Candidate.IsValid() || Candidate->IsDead();
			});
		if (Attackers.IsEmpty())
		{
			Iterator.RemoveCurrent();
		}
	}
}

bool UZorbaEnemyCrowdSubsystem::TryAcquireSpecialAttackToken(
	AZorbaEnemyCharacter* Enemy,
	AActor* TargetActor)
{
	if (!IsValid(Enemy) || Enemy->IsDead() || !IsValid(TargetActor))
	{
		return false;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	TWeakObjectPtr<AZorbaEnemyCharacter>& Holder =
		SpecialAttackerByTarget.FindOrAdd(TargetKey);
	if (Holder.IsValid() && !Holder->IsDead() && Holder.Get() != Enemy)
	{
		return false;
	}

	Holder = Enemy;
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Crowd special cue token acquired: Enemy=%s Target=%s"),
		*GetNameSafe(Enemy),
		*GetNameSafe(TargetActor));
	return true;
}

void UZorbaEnemyCrowdSubsystem::ReleaseSpecialAttackToken(
	AZorbaEnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	for (auto Iterator = SpecialAttackerByTarget.CreateIterator(); Iterator; ++Iterator)
	{
		const TWeakObjectPtr<AZorbaEnemyCharacter>& Holder = Iterator.Value();
		if (!Holder.IsValid() || Holder->IsDead() || Holder.Get() == Enemy)
		{
			if (Holder.Get() == Enemy)
			{
				UE_LOG(
					LogTemp,
					Log,
					TEXT("Crowd special cue token released: Enemy=%s"),
					*GetNameSafe(Enemy));
			}
			Iterator.RemoveCurrent();
		}
	}
}

FVector UZorbaEnemyCrowdSubsystem::GetStandbyLocation(
	const AZorbaEnemyCharacter* Enemy,
	const AActor* TargetActor,
	float StandbyRadius) const
{
	if (!IsValid(Enemy) || !IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	const int32* Slot = FormationSlots.Find(
		TWeakObjectPtr<AZorbaEnemyCharacter>(
			const_cast<AZorbaEnemyCharacter*>(Enemy)));
	const int32 SlotIndex = Slot ? *Slot : 0;
	const int32 SlotCount = FMath::Max(4, FormationSlots.Num());
	const float Angle =
		(2.0f * PI * static_cast<float>(SlotIndex % SlotCount))
		/ static_cast<float>(SlotCount);
	const FVector Offset(
		FMath::Cos(Angle) * FMath::Max(0.0f, StandbyRadius),
		FMath::Sin(Angle) * FMath::Max(0.0f, StandbyRadius),
		0.0f);
	FVector Result = TargetActor->GetActorLocation() + Offset;
	Result.Z = Enemy->GetActorLocation().Z;
	return Result;
}

int32 UZorbaEnemyCrowdSubsystem::GetActiveAttackerCount(
	const AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return 0;
	}

	const TArray<TWeakObjectPtr<AZorbaEnemyCharacter>>* Attackers =
		AttackersByTarget.Find(
			TWeakObjectPtr<AActor>(const_cast<AActor*>(TargetActor)));
	if (!Attackers)
	{
		return 0;
	}

	int32 ValidCount = 0;
	for (const TWeakObjectPtr<AZorbaEnemyCharacter>& Attacker : *Attackers)
	{
		if (Attacker.IsValid() && !Attacker->IsDead())
		{
			++ValidCount;
		}
	}
	return ValidCount;
}

void UZorbaEnemyCrowdSubsystem::CompactAttackers(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	if (TArray<TWeakObjectPtr<AZorbaEnemyCharacter>>* Attackers =
		AttackersByTarget.Find(TargetKey))
	{
		Attackers->RemoveAll(
			[](const TWeakObjectPtr<AZorbaEnemyCharacter>& Candidate)
			{
				return !Candidate.IsValid() || Candidate->IsDead();
			});
		if (Attackers->IsEmpty())
		{
			AttackersByTarget.Remove(TargetKey);
		}
	}
}
