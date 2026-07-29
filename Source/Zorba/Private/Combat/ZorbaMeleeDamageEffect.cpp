// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ZorbaMeleeDamageEffect.h"

#include "Combat/ZorbaCombatAttributeSet.h"

const FName UZorbaMeleeDamageEffect::HealthDamageDataName(
	TEXT("Data.Damage.Health"));

const FName UZorbaMeleeDamageEffect::StaminaDamageDataName(
	TEXT("Data.Damage.Stamina"));

UZorbaMeleeDamageEffect::UZorbaMeleeDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat HealthMagnitude;
	HealthMagnitude.DataName = HealthDamageDataName;

	FGameplayModifierInfo& HealthModifier =
		Modifiers.AddDefaulted_GetRef();
	HealthModifier.Attribute =
		UZorbaCombatAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Additive;
	HealthModifier.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(HealthMagnitude);

	FSetByCallerFloat StaminaMagnitude;
	StaminaMagnitude.DataName = StaminaDamageDataName;

	FGameplayModifierInfo& StaminaModifier =
		Modifiers.AddDefaulted_GetRef();
	StaminaModifier.Attribute =
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute();
	StaminaModifier.ModifierOp = EGameplayModOp::Additive;
	StaminaModifier.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(StaminaMagnitude);
}
