// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/ZorbaMissionDefinition.h"

FPrimaryAssetId UZorbaMissionDefinition::GetPrimaryAssetId() const
{
	const FName AssetName = MissionId.IsNone() ? GetFName() : MissionId;
	return FPrimaryAssetId(TEXT("ZorbaMission"), AssetName);
}
