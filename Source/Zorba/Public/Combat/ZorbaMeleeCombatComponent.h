// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZorbaMeleeCombatComponent.generated.h"

class UZorbaAttackDefinition;

UCLASS(ClassGroup = (Zorba), meta = (BlueprintSpawnableComponent))
class ZORBA_API UZorbaMeleeCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZorbaMeleeCombatComponent();

	bool BeginAttack(
		UZorbaAttackDefinition* AttackDefinition,
		const FVector& AttackDirection);

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	FVector GetCurrentAttackDirection() const;

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaAttackDefinition> ActiveAttackDefinition;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zorba|Combat")
	FVector CurrentAttackDirection = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zorba|Combat|Debug")
	bool bDrawAttackDirection = true;
};