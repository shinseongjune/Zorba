// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ZorbaMeleeDamageEffect.generated.h"

/**
 * Default instant damage effect for data-driven melee attacks.
 * Attack definitions can replace this class, while these SetByCaller names keep
 * the first playable hit path useful without requiring a throwaway GE asset.
 */
UCLASS()
class ZORBA_API UZorbaMeleeDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UZorbaMeleeDamageEffect();

	static const FName HealthDamageDataName;
	static const FName StaminaDamageDataName;
};
