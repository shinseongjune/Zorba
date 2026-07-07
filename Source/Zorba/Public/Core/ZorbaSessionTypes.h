// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZorbaSessionTypes.generated.h"

UENUM(BlueprintType)
enum class EZorbaSessionPhase : uint8
{
	Boot UMETA(DisplayName = "Boot"),
	MainMenu UMETA(DisplayName = "Main Menu"),
	Campaign UMETA(DisplayName = "Campaign"),
	MissionLoading UMETA(DisplayName = "Mission Loading"),
	MissionActive UMETA(DisplayName = "Mission Active"),
	MissionComplete UMETA(DisplayName = "Mission Complete")
};
