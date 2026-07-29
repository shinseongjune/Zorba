// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ZorbaAttackDefinition.h"

#include "Combat/ZorbaMeleeDamageEffect.h"

UZorbaAttackDefinition::UZorbaAttackDefinition()
{
	DamageEffect = UZorbaMeleeDamageEffect::StaticClass();
}

FPrimaryAssetId UZorbaAttackDefinition::GetPrimaryAssetId() const
{
	const FName AssetName = AttackId.IsNone() ? GetFName() : AttackId;
	return FPrimaryAssetId(TEXT("ZorbaAttack"), AssetName);
}
