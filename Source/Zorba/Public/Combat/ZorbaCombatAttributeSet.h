// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ZorbaCombatAttributeSet.generated.h"

UCLASS()
class ZORBA_API UZorbaCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UZorbaCombatAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Zorba|Combat|Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UZorbaCombatAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Zorba|Combat|Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UZorbaCombatAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CombatStamina, Category = "Zorba|Combat|Attributes")
	FGameplayAttributeData CombatStamina;
	ATTRIBUTE_ACCESSORS_BASIC(UZorbaCombatAttributeSet, CombatStamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxCombatStamina, Category = "Zorba|Combat|Attributes")
	FGameplayAttributeData MaxCombatStamina;
	ATTRIBUTE_ACCESSORS_BASIC(UZorbaCombatAttributeSet, MaxCombatStamina)

private:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_CombatStamina(const FGameplayAttributeData& OldCombatStamina);

	UFUNCTION()
	void OnRep_MaxCombatStamina(const FGameplayAttributeData& OldMaxCombatStamina);
};
