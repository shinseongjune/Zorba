// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/ZorbaEnemyCombatProfile.h"

FPrimaryAssetId UZorbaEnemyCombatProfile::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZorbaEnemyCombatProfile"), GetFName());
}
