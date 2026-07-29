// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ZorbaCombatAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UZorbaCombatAttributeSet::UZorbaCombatAttributeSet()
{
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitMaxCombatStamina(100.0f);
	InitCombatStamina(100.0f);
}

void UZorbaCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UZorbaCombatAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZorbaCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZorbaCombatAttributeSet, CombatStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UZorbaCombatAttributeSet, MaxCombatStamina, COND_None, REPNOTIFY_Always);
}

void UZorbaCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute() ||
		Attribute == GetMaxCombatStaminaAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetCombatStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxCombatStamina());
	}
}

void UZorbaCombatAttributeSet::PostAttributeChange(
	const FGameplayAttribute& Attribute,
	float OldValue,
	float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		SetHealth(FMath::Max(0.0f, NewValue));
	}
	else if (Attribute == GetMaxCombatStaminaAttribute()
		&& GetCombatStamina() > NewValue)
	{
		SetCombatStamina(FMath::Max(0.0f, NewValue));
	}
}

void UZorbaCombatAttributeSet::PostGameplayEffectExecute(
	const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetCombatStaminaAttribute())
	{
		SetCombatStamina(FMath::Clamp(
			GetCombatStamina(),
			0.0f,
			GetMaxCombatStamina()));
	}
}

void UZorbaCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZorbaCombatAttributeSet, Health, OldHealth);
}

void UZorbaCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZorbaCombatAttributeSet, MaxHealth, OldMaxHealth);
}

void UZorbaCombatAttributeSet::OnRep_CombatStamina(const FGameplayAttributeData& OldCombatStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZorbaCombatAttributeSet, CombatStamina, OldCombatStamina);
}

void UZorbaCombatAttributeSet::OnRep_MaxCombatStamina(const FGameplayAttributeData& OldMaxCombatStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UZorbaCombatAttributeSet, MaxCombatStamina, OldMaxCombatStamina);
}
