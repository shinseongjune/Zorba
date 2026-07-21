// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ZorbaAttackDefinition.h"

FPrimaryAssetId UZorbaAttackDefinition::GetPrimaryAssetId() const
{
	const FName AssetName = AttackId.IsNone() ? GetFName() : AttackId;
	return FPrimaryAssetId(TEXT("ZorbaAttack"), AssetName);
}