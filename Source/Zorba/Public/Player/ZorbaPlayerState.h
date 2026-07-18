// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ZorbaPlayerState.generated.h"

class UAbilitySystemComponent;
class UZorbaCombatAttributeSet;

UCLASS()
class ZORBA_API AZorbaPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AZorbaPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat")
	UZorbaCombatAttributeSet* GetCombatAttributes() const { return CombatAttributes; }

	UFUNCTION(BlueprintCallable, Category = "Zorba|Player")
	void AddMissionScore(int32 Delta);

	UFUNCTION(BlueprintPure, Category = "Zorba|Player")
	int32 GetMissionScore() const { return MissionScore; }

private:
	UFUNCTION()
	void OnRep_MissionScore();

	UPROPERTY(ReplicatedUsing = OnRep_MissionScore, VisibleInstanceOnly, Category = "Zorba|Player")
	int32 MissionScore = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZorbaCombatAttributeSet> CombatAttributes;
};
