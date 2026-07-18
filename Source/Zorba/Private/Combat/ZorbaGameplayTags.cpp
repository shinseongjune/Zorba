// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ZorbaGameplayTags.h"

namespace ZorbaGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Input_Attack_Primary,
		"Input.Attack.Primary",
		"Primary attack input.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Input_Attack_Heavy,
		"Input.Attack.Heavy",
		"Heavy attack input.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Input_Defend,
		"Input.Defend",
		"Defend input.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Input_Dodge,
		"Input.Dodge",
		"Dodge input.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Attacking,
		"State.Attacking",
		"Actor is performing an attack.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Defending,
		"State.Defending",
		"Actor is defending.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_ParryWindow,
		"State.ParryWindow",
		"Actor can currently parry.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_HitReact,
		"State.HitReact",
		"Actor is reacting to a hit.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_StaminaDepleted,
		"State.StaminaDepleted",
		"Actor has no combat stamina.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Exhausted,
		"State.Exhausted",
		"Enemy is vulnerable to an opportunity attack.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Dead,
		"State.Dead",
		"Actor is dead.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		State_Invulnerable,
		"State.Invulnerable",
		"Actor temporarily ignores damage.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Event_Melee_TraceBegin,
		"Event.Melee.TraceBegin",
		"Begin melee hit tracing.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Event_Melee_TraceEnd,
		"Event.Melee.TraceEnd",
		"End melee hit tracing.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Event_Combo_WindowBegin,
		"Event.Combo.WindowBegin",
		"Begin accepting the next combo input.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Event_Combo_WindowEnd,
		"Event.Combo.WindowEnd",
		"Stop accepting the next combo input.");
}