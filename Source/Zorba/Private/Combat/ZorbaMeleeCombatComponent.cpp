// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ZorbaMeleeCombatComponent.h"

#include "Combat/ZorbaAttackDefinition.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

UZorbaMeleeCombatComponent::UZorbaMeleeCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UZorbaMeleeCombatComponent::BeginAttack(
	UZorbaAttackDefinition* AttackDefinition,
	const FVector& AttackDirection)
{
	if (!IsValid(AttackDefinition))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Melee attack rejected: AttackDefinition is missing."));
		return false;
	}

	FVector FlatDirection = AttackDirection;
	FlatDirection.Z = 0.0f;
	FlatDirection = FlatDirection.GetSafeNormal();

	if (FlatDirection.IsNearlyZero())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Melee attack rejected: AttackDirection is zero."));
		return false;
	}

	ActiveAttackDefinition = AttackDefinition;
	CurrentAttackDirection = FlatDirection;

	if (bDrawAttackDirection && GetWorld())
	{
		const AActor* OwnerActor = GetOwner();

		if (OwnerActor)
		{
			float ArrowLength = 200.0f;

			if (AttackDefinition->HitPhases.IsValidIndex(0))
			{
				ArrowLength = FMath::Max(
					AttackDefinition->HitPhases[0].Range,
					100.0f);
			}

			const FVector ArrowStart =
				OwnerActor->GetActorLocation()
				+ FVector(0.0f, 0.0f, 100.0f);

			const FVector ArrowEnd =
				ArrowStart
				+ CurrentAttackDirection * ArrowLength;

			DrawDebugDirectionalArrow(
				GetWorld(),
				ArrowStart,
				ArrowEnd,
				25.0f,
				FColor::Cyan,
				false,
				1.25f,
				0,
				4.0f);
		}
	}

	return true;
}

FVector UZorbaMeleeCombatComponent::GetCurrentAttackDirection() const
{
	return CurrentAttackDirection;
}