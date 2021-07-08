/*
 * ArcEmu MMORPG Server
 * Copyright (C) 2005-2007 Ascent Team <http://www.ascentemu.com/>
 * Copyright (C) 2008-2012 <http://www.ArcEmu.org/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "StdAfx.h"

void CreateDummySpell(uint32 id)
{
	const char* name = "Dummy Trigger";
	SpellEntry* sp = new SpellEntry;
	memset(sp, 0, sizeof(SpellEntry));
	sp->Id = id;
	sp->Attributes = 384;
	sp->AttributesEx = 268435456;
	sp->ATTRIBUTESEX2 = 4;
	sp->CastingTimeIndex = 1;
	sp->procChance = 75;
	sp->rangeIndex = 13;
	sp->EquippedItemClass = uint32(-1);
	sp->Effect[0] = SPELL_EFFECT_DUMMY;
	sp->EffectImplicitTargetA[0] = 25;
	sp->NameHash = crc32((const unsigned char*)name, (unsigned int)strlen(name));
	sp->dmg_multiplier[0] = 1.0f;
	sp->StanceBarOrder = -1;
	dbcSpell.SetRow(id, sp);
	sWorld.dummyspells.push_back(sp);
}

// Copies effect number 'fromEffect' in 'fromSpell' to effect number 'toEffect' in 'toSpell'
void CopyEffect(SpellEntry *fromSpell, uint8 fromEffect, SpellEntry *toSpell, uint8 toEffect)
{
    if (!fromSpell || !toSpell || fromEffect > 2 || toEffect > 2)
        return;

    uint32 *from = fromSpell->Effect;
    uint32 *to = toSpell->Effect;
    // Copy 20 values starting at Effect
    for (uint8 index = 0; index < 20; index++)
    {
        to[index * 3 + toEffect] = from[index * 3 + fromEffect];
    }
}

uint32 GetSpellClass(SpellEntry *sp)
{
    skilllinespell* sls;
    sls = dbcSkillLineSpell.LookupRow(sp->Id);
    switch (sls->Id)
    {
    case SKILL_ARMS:
    case SKILL_FURY:
    case SKILL_PROTECTION:
        return WARRIOR;
    case SKILL_HOLY2:
    case SKILL_PROTECTION2:
    case SKILL_RETRIBUTION:
        return PALADIN;
    case SKILL_BEAST_MASTERY:
    case SKILL_SURVIVAL:
    case SKILL_MARKSMANSHIP:
        return HUNTER;
    case SKILL_ASSASSINATION:
    case SKILL_COMBAT:
    case SKILL_SUBTLETY:
        return ROGUE;
    case SKILL_DISCIPLINE:
    case SKILL_HOLY:
    case SKILL_SHADOW_MAGIC:
        return PRIEST;
    case SKILL_ENHANCEMENT:
    case SKILL_RESTORATION:
    case SKILL_ELEMENTAL_COMBAT:
        return SHAMAN;
    case SKILL_FROST:
    case SKILL_FIRE:
    case SKILL_ARCANE:
        return MAGE;
    case SKILL_AFFLICTION:
    case SKILL_DEMONOLOGY:
    case SKILL_DESTRUCTION:
        return WARLOCK;
    case SKILL_RESTORATION2:
    case SKILL_BALANCE:
    case SKILL_FERAL_COMBAT:
        return DRUID;
    case SKILL_FROST2:
    case SKILL_UNHOLY:
    case SKILL_BLOOD:
        return DEATHKNIGHT;
    }

    return 0;
}

void ApplyCoefFixes()
{

}

void ApplyNormalFixes()
{
	//Updating spell.dbc

	Log.Success("World", "Processing %u spells...", dbcSpell.GetNumRows());

	//checking if the DBCs have been extracted from an english client, based on namehash of spell 4, the first with a different name in non-english DBCs
	SpellEntry* sp = dbcSpell.LookupEntry(4);
	if(crc32((const unsigned char*)sp->Name, (unsigned int)strlen(sp->Name)) != SPELL_HASH_WORD_OF_RECALL_OTHER)
	{
		Log.LargeErrorMessage("You are using DBCs extracted from an unsupported client.", "ArcEmu supports only enUS and enGB!!!", NULL);
		abort();
	}

	uint32 cnt = dbcSpell.GetNumRows();
	uint32 effect;
	uint32 result;

	map<uint32, uint32> talentSpells;
	map<uint32, uint32>::iterator talentSpellIterator;
	uint32 i, j;
	for(i = 0; i < dbcTalent.GetNumRows(); ++i)
	{
		TalentEntry* tal = dbcTalent.LookupRow(i);
		for(j = 0; j < 5; ++j)
			if(tal->RankID[j] != 0)
				talentSpells.insert(make_pair(tal->RankID[j], tal->TalentTree));

	}

	for(uint32 x = 0; x < cnt; x++)
	{
		// Read every SpellEntry row
		sp = dbcSpell.LookupRow(x);

		uint32 rank = 0;
		uint32 namehash = 0;

		sp->self_cast_only = false;
		sp->apply_on_shapeshift_change = false;
		sp->always_apply = false;

		// hash the name
		//!!!!!!! representing all strings on 32 bits is dangerous. There is a chance to get same hash for a lot of strings ;)
		namehash = crc32((const unsigned char*)sp->Name, (unsigned int)strlen(sp->Name));
		sp->NameHash   = namehash; //need these set before we start processing spells

		float radius = std::max(::GetRadius(dbcSpellRadius.LookupEntry(sp->EffectRadiusIndex[0])), ::GetRadius(dbcSpellRadius.LookupEntry(sp->EffectRadiusIndex[1])));
		radius = std::max(::GetRadius(dbcSpellRadius.LookupEntry(sp->EffectRadiusIndex[2])), radius);
		radius = std::max(GetMaxRange(dbcSpellRange.LookupEntry(sp->rangeIndex)), radius);
		sp->base_range_or_radius_sqr = radius * radius;

		sp->ai_target_type = GetAiTargetType(sp);
		// NEW SCHOOLS AS OF 2.4.0:
		/* (bitwise)
		SCHOOL_NORMAL = 1,
		SCHOOL_HOLY   = 2,
		SCHOOL_FIRE   = 4,
		SCHOOL_NATURE = 8,
		SCHOOL_FROST  = 16,
		SCHOOL_SHADOW = 32,
		SCHOOL_ARCANE = 64
		*/

		// Save School as SchoolMask, and set School as an index
		sp->SchoolMask = sp->School;
		for(i = 0; i < SCHOOL_COUNT; i++)
		{
			if(sp->School & (1 << i))
			{
				sp->School = i;
				break;
			}
		}

		ARCEMU_ASSERT(sp->School < SCHOOL_COUNT);

		// correct caster/target aura states
		if(sp->CasterAuraState > 1)
			sp->CasterAuraState = 1 << (sp->CasterAuraState - 1);

		if(sp->TargetAuraState > 1)
			sp->TargetAuraState = 1 << (sp->TargetAuraState - 1);

		// apply on shapeshift change
		if(sp->NameHash == SPELL_HASH_TRACK_HUMANOIDS)
			sp->apply_on_shapeshift_change = true;

		if(sp->NameHash == SPELL_HASH_BLOOD_FURY
			|| sp->NameHash == SPELL_HASH_SHADOWSTEP
			|| sp->NameHash == SPELL_HASH_PSYCHIC_HORROR)
			sp->always_apply = true;

		//there are some spells that change the "damage" value of 1 effect to another : devastate = bonus first then damage
		//this is a total bullshit so remove it when spell system supports effect overwriting
		for(uint32 col1_swap = 0; col1_swap < 2 ; col1_swap++)
			for(uint32 col2_swap = col1_swap ; col2_swap < 3 ; col2_swap++)
				if(sp->Effect[col1_swap] == SPELL_EFFECT_WEAPON_PERCENT_DAMAGE && sp->Effect[col2_swap] == SPELL_EFFECT_DUMMYMELEE)
				{
					uint32 temp;
					float ftemp;
					temp = sp->Effect[col1_swap];
					sp->Effect[col1_swap] = sp->Effect[col2_swap] ;
					sp->Effect[col2_swap] = temp;
					temp = sp->EffectDieSides[col1_swap];
					sp->EffectDieSides[col1_swap] = sp->EffectDieSides[col2_swap] ;
					sp->EffectDieSides[col2_swap] = temp;
					//temp = sp->EffectBaseDice[col1_swap];	sp->EffectBaseDice[col1_swap] = sp->EffectBaseDice[col2_swap] ;		sp->EffectBaseDice[col2_swap] = temp;
					//ftemp = sp->EffectDicePerLevel[col1_swap];			sp->EffectDicePerLevel[col1_swap] = sp->EffectDicePerLevel[col2_swap] ;				sp->EffectDicePerLevel[col2_swap] = ftemp;
					ftemp = sp->EffectRealPointsPerLevel[col1_swap];
					sp->EffectRealPointsPerLevel[col1_swap] = sp->EffectRealPointsPerLevel[col2_swap] ;
					sp->EffectRealPointsPerLevel[col2_swap] = ftemp;
					temp = sp->EffectBasePoints[col1_swap];
					sp->EffectBasePoints[col1_swap] = sp->EffectBasePoints[col2_swap] ;
					sp->EffectBasePoints[col2_swap] = temp;
					temp = sp->EffectMechanic[col1_swap];
					sp->EffectMechanic[col1_swap] = sp->EffectMechanic[col2_swap] ;
					sp->EffectMechanic[col2_swap] = temp;
					temp = sp->EffectImplicitTargetA[col1_swap];
					sp->EffectImplicitTargetA[col1_swap] = sp->EffectImplicitTargetA[col2_swap] ;
					sp->EffectImplicitTargetA[col2_swap] = temp;
					temp = sp->EffectImplicitTargetB[col1_swap];
					sp->EffectImplicitTargetB[col1_swap] = sp->EffectImplicitTargetB[col2_swap] ;
					sp->EffectImplicitTargetB[col2_swap] = temp;
					temp = sp->EffectRadiusIndex[col1_swap];
					sp->EffectRadiusIndex[col1_swap] = sp->EffectRadiusIndex[col2_swap] ;
					sp->EffectRadiusIndex[col2_swap] = temp;
					temp = sp->EffectApplyAuraName[col1_swap];
					sp->EffectApplyAuraName[col1_swap] = sp->EffectApplyAuraName[col2_swap] ;
					sp->EffectApplyAuraName[col2_swap] = temp;
					temp = sp->EffectAmplitude[col1_swap];
					sp->EffectAmplitude[col1_swap] = sp->EffectAmplitude[col2_swap] ;
					sp->EffectAmplitude[col2_swap] = temp;
					ftemp = sp->EffectMultipleValue[col1_swap];
					sp->EffectMultipleValue[col1_swap] = sp->EffectMultipleValue[col2_swap] ;
					sp->EffectMultipleValue[col2_swap] = ftemp;
					temp = sp->EffectChainTarget[col1_swap];
					sp->EffectChainTarget[col1_swap] = sp->EffectChainTarget[col2_swap] ;
					sp->EffectChainTarget[col2_swap] = temp;
					temp = sp->EffectMiscValue[col1_swap];
					sp->EffectMiscValue[col1_swap] = sp->EffectMiscValue[col2_swap] ;
					sp->EffectMiscValue[col2_swap] = temp;
					temp = sp->EffectTriggerSpell[col1_swap];
					sp->EffectTriggerSpell[col1_swap] = sp->EffectTriggerSpell[col2_swap] ;
					sp->EffectTriggerSpell[col2_swap] = temp;
					ftemp = sp->EffectPointsPerComboPoint[col1_swap];
					sp->EffectPointsPerComboPoint[col1_swap] = sp->EffectPointsPerComboPoint[col2_swap] ;
					sp->EffectPointsPerComboPoint[col2_swap] = ftemp;
				}

		for(uint32 b = 0; b < 3; ++b)
		{
			if(sp->EffectTriggerSpell[b] != 0 && dbcSpell.LookupEntryForced(sp->EffectTriggerSpell[b]) == NULL)
			{
				/* proc spell referencing non-existent spell. create a dummy spell for use w/ it. */
				CreateDummySpell(sp->EffectTriggerSpell[b]);
			}

			if(sp->Attributes & ATTRIBUTES_ONLY_OUTDOORS && sp->EffectApplyAuraName[b] == SPELL_AURA_MOUNTED)
			{
				sp->Attributes &= ~ATTRIBUTES_ONLY_OUTDOORS;
			}
		}

		if(!strcmp(sp->Name, "Hearthstone") || !strcmp(sp->Name, "Stuck") || !strcmp(sp->Name, "Astral Recall"))
			sp->self_cast_only = true;

		sp->proc_interval = 0;//trigger at each event
		sp->c_is_flags = 0;
		sp->spell_coef_flags = 0;
		sp->Dspell_coef_override = -1;
		sp->OTspell_coef_override = -1;
		sp->casttime_coef = 0;
		sp->fixed_dddhcoef = -1;
		sp->fixed_hotdotcoef = -1;
        sp->SP_coef_override = 0;
        sp->AP_coef_override = 0;
        sp->RAP_coef_override = 0;
        sp->aura_remove_flags = 0;
        sp->spell_cannot_be_resist = false;
        sp->spell_special_always_critical = false;

		talentSpellIterator = talentSpells.find(sp->Id);
		if(talentSpellIterator == talentSpells.end())
			sp->talent_tree = 0;
		else
			sp->talent_tree = talentSpellIterator->second;

		// parse rank text
		if(sscanf(sp->Rank, "Rank %d", (unsigned int*)&rank) != 1)
			rank = 0;

		//seal of command
		else if(namehash == SPELL_HASH_SEAL_OF_COMMAND)
			sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC;

		//judgement of command
		else if(namehash == SPELL_HASH_JUDGEMENT_OF_COMMAND)
			sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC;

		else if(namehash == SPELL_HASH_ARCANE_SHOT)
			sp->c_is_flags |= SPELL_FLAG_IS_NOT_USING_DMG_BONUS;

		else if(namehash == SPELL_HASH_SERPENT_STING)
			sp->c_is_flags |= SPELL_FLAG_IS_NOT_USING_DMG_BONUS;

		//Rogue: Poison time fix for 2.3
		if(strstr(sp->Name, "Crippling Poison") && sp->Effect[0] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)    //I, II
			sp->EffectBasePoints[0] = 3599;
		if(strstr(sp->Name, "Mind-numbing Poison") && sp->Effect[0] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)    //I,II,III
			sp->EffectBasePoints[0] = 3599;
		if(strstr(sp->Name, "Instant Poison") && sp->Effect[0] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)    //I,II,III,IV,V,VI,VII
			sp->EffectBasePoints[0] = 3599;
		if(strstr(sp->Name, "Deadly Poison") && sp->Effect[0] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)    //I,II,III,IV,V,VI,VII
			sp->EffectBasePoints[0] = 3599;
		if(strstr(sp->Name, "Wound Poison") && sp->Effect[0] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)    //I,II,III,IV,V
			sp->EffectBasePoints[0] = 3599;
		if(strstr(sp->Name, "Anesthetic Poison") && sp->Effect[0] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)    //I
			sp->EffectBasePoints[0] = 3599;

		if(strstr(sp->Name, "Sharpen Blade") && sp->Effect[0] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)    //All BS stones
			sp->EffectBasePoints[0] = 3599;

		//these mostly do not mix so we can use else
		// look for seal, etc in name
		if(strstr(sp->Name, "Seal of"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_SEAL;
		else if(strstr(sp->Name, "Hand of"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_HAND;
		else if(strstr(sp->Name, "Blessing"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_BLESSING;
		else if(strstr(sp->Name, "Curse"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_CURSE;
		else if(strstr(sp->Name, "Corruption"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_CORRUPTION;
		else if(strstr(sp->Name, "Aspect"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_ASPECT;
		else if(strstr(sp->Name, "Sting") || strstr(sp->Name, "sting"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_STING;
		// don't break armor items!
		else if(strstr(sp->Name, "Fel Armor") || strstr(sp->Name, "Frost Armor") || strstr(sp->Name, "Ice Armor") || strstr(sp->Name, "Mage Armor") || strstr(sp->Name, "Molten Armor") || strstr(sp->Name, "Demon Skin") || strstr(sp->Name, "Demon Armor"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_ARMOR;
		else if(strstr(sp->Name, "Aura")
		        && !strstr(sp->Name, "Trueshot") && !strstr(sp->Name, "Moonkin")
		        && !strstr(sp->Name, "Crusader") && !strstr(sp->Name, "Sanctity") && !strstr(sp->Name, "Devotion") && !strstr(sp->Name, "Retribution") && !strstr(sp->Name, "Concentration") && !strstr(sp->Name, "Shadow Resistance") && !strstr(sp->Name, "Frost Resistance") && !strstr(sp->Name, "Fire Resistance")
		       )
			sp->BGR_one_buff_on_target |= SPELL_TYPE_AURA;
		else if(strstr(sp->Name, "Track") == sp->Name)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_TRACK;
		else if(namehash == SPELL_HASH_GIFT_OF_THE_WILD || namehash == SPELL_HASH_MARK_OF_THE_WILD)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_MARK_GIFT;
		else if(namehash == SPELL_HASH_IMMOLATION_TRAP || namehash == SPELL_HASH_FREEZING_TRAP || namehash == SPELL_HASH_FROST_TRAP || namehash == SPELL_HASH_EXPLOSIVE_TRAP || namehash == SPELL_HASH_SNAKE_TRAP)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_HUNTER_TRAP;
		else if(namehash == SPELL_HASH_ARCANE_INTELLECT || namehash == SPELL_HASH_ARCANE_BRILLIANCE)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_MAGE_INTEL;
		else if(namehash == SPELL_HASH_AMPLIFY_MAGIC || namehash == SPELL_HASH_DAMPEN_MAGIC)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_MAGE_MAGI;
		else if(namehash == SPELL_HASH_FIRE_WARD || namehash == SPELL_HASH_FROST_WARD)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_MAGE_WARDS;
		else if(namehash == SPELL_HASH_SHADOW_PROTECTION || namehash == SPELL_HASH_PRAYER_OF_SHADOW_PROTECTION)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_PRIEST_SH_PPROT;
		else if(namehash == SPELL_HASH_WATER_SHIELD || namehash == SPELL_HASH_EARTH_SHIELD || namehash == SPELL_HASH_LIGHTNING_SHIELD)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_SHIELD;
		else if(namehash == SPELL_HASH_POWER_WORD__FORTITUDE || namehash == SPELL_HASH_PRAYER_OF_FORTITUDE)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_FORTITUDE;
		else if(namehash == SPELL_HASH_DIVINE_SPIRIT || namehash == SPELL_HASH_PRAYER_OF_SPIRIT)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_SPIRIT;
//		else if( strstr( sp->Name, "Curse of Weakness") || strstr( sp->Name, "Curse of Agony") || strstr( sp->Name, "Curse of Recklessness") || strstr( sp->Name, "Curse of Tongues") || strstr( sp->Name, "Curse of the Elements") || strstr( sp->Name, "Curse of Idiocy") || strstr( sp->Name, "Curse of Shadow") || strstr( sp->Name, "Curse of Doom"))
//		else if(namehash==4129426293 || namehash==885131426 || namehash==626036062 || namehash==3551228837 || namehash==2784647472 || namehash==776142553 || namehash==3407058720 || namehash==202747424)
//		else if( strstr( sp->Name, "Curse of "))
//            type |= SPELL_TYPE_WARLOCK_CURSES;
		else if(strstr(sp->Name, "Immolate") || strstr(sp->Name, "Conflagrate"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_WARLOCK_IMMOLATE;
		else if(strstr(sp->Name, "Amplify Magic") || strstr(sp->Name, "Dampen Magic"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_MAGE_AMPL_DUMP;
		else if(strstr(sp->Description, "Battle Elixir"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_ELIXIR_BATTLE;
		else if(strstr(sp->Description, "Guardian Elixir"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_ELIXIR_GUARDIAN;
		else if(strstr(sp->Description, "Battle and Guardian elixir"))
			sp->BGR_one_buff_on_target |= SPELL_TYPE_ELIXIR_FLASK;
		else if(namehash == SPELL_HASH_HUNTER_S_MARK)		// hunter's mark
			sp->BGR_one_buff_on_target |= SPELL_TYPE_HUNTER_MARK;
		else if(namehash == SPELL_HASH_COMMANDING_SHOUT || namehash == SPELL_HASH_BATTLE_SHOUT)
			sp->BGR_one_buff_on_target |= SPELL_TYPE_WARRIOR_SHOUT;
		else if(strstr(sp->Description, "Finishing move") == sp->Description)
			sp->c_is_flags |= SPELL_FLAG_IS_FINISHING_MOVE;
		if(IsDamagingSpell(sp))
			sp->c_is_flags |= SPELL_FLAG_IS_DAMAGING;
		if(IsHealingSpell(sp))
			sp->c_is_flags |= SPELL_FLAG_IS_HEALING;
		if(IsTargetingStealthed(sp))
			sp->c_is_flags |= SPELL_FLAG_IS_TARGETINGSTEALTHED;

		if(sp->NameHash == SPELL_HASH_HEMORRHAGE)
			sp->c_is_flags |= SPELL_FLAG_IS_MAXSTACK_FOR_DEBUFF;

        for (uint32 b = 0; b < 3; ++b)
        {
            if (sp->Effect[b] == SPELL_EFFECT_PERSISTENT_AREA_AURA)
            {
                sp->EffectImplicitTargetA[b] = EFF_TARGET_SELF;
                sp->EffectImplicitTargetB[b] = 0;
            }

            // 3.0.3 totemzzz
            if (sp->Effect[b] == SPELL_EFFECT_APPLY_RAID_AREA_AURA)
            {
                sp->Effect[b] = SPELL_EFFECT_APPLY_GROUP_AREA_AURA;
            }
        }

		//stupid spell ranking problem
		if(sp->spellLevel == 0)
		{
			uint32 new_level = 0;

			if(strstr(sp->Name, "Apprentice "))
				new_level = 1;
			else if(strstr(sp->Name, "Journeyman "))
				new_level = 2;
			else if(strstr(sp->Name, "Expert "))
				new_level = 3;
			else if(strstr(sp->Name, "Artisan "))
				new_level = 4;
			else if(strstr(sp->Name, "Master "))
				new_level = 5;
			else if(strstr(sp->Name, "Grand Master "))
				new_level = 6;

			if(new_level != 0)
			{
				uint32 teachspell = 0;
				if(sp->Effect[0] == SPELL_EFFECT_LEARN_SPELL)
					teachspell = sp->EffectTriggerSpell[0];
				else if(sp->Effect[1] == SPELL_EFFECT_LEARN_SPELL)
					teachspell = sp->EffectTriggerSpell[1];
				else if(sp->Effect[2] == SPELL_EFFECT_LEARN_SPELL)
					teachspell = sp->EffectTriggerSpell[2];

				if(teachspell)
				{
					SpellEntry* spellInfo;
					spellInfo = CheckAndReturnSpellEntry(teachspell);
					spellInfo->spellLevel = new_level;
					sp->spellLevel = new_level;
				}
			}
		}

		/*FILE * f = fopen("C:\\spells.txt", "a");
		fprintf(f, "case 0x%08X:		// %s\n", namehash, sp->Name);
		fclose(f);*/

		// find diminishing status
		sp->DiminishStatus = GetDiminishingGroup(namehash);

		//another grouping rule

		//Quivers, Ammo Pouches and Thori'dal the Star's Fury
		if((namehash == SPELL_HASH_HASTE && sp->Attributes & 0x10000) || sp->Id == 44972)
		{
			sp->Attributes &= ~ATTRIBUTES_PASSIVE;//Otherwise we couldn't remove them
			sp->BGR_one_buff_on_target |= SPELL_TYPE_QUIVER_HASTE;
		}

		switch(namehash)
		{
				//case SPELL_HASH_SANCTITY_AURA:
			case SPELL_HASH_DEVOTION_AURA:
			case SPELL_HASH_RETRIBUTION_AURA:
			case SPELL_HASH_CONCENTRATION_AURA:
			case SPELL_HASH_SHADOW_RESISTANCE_AURA:
			case SPELL_HASH_FIRE_RESISTANCE_AURA:
			case SPELL_HASH_FROST_RESISTANCE_AURA:
			case SPELL_HASH_CRUSADER_AURA:
				sp->BGR_one_buff_from_caster_on_self = SPELL_TYPE2_PALADIN_AURA;
				break;
		}

		switch(namehash)
		{
			case SPELL_HASH_BLOOD_PRESENCE:
			case SPELL_HASH_FROST_PRESENCE:
			case SPELL_HASH_UNHOLY_PRESENCE:
				sp->BGR_one_buff_from_caster_on_self = SPELL_TYPE3_DEATH_KNIGHT_AURA;
				break;
		}

		// HACK FIX: Break roots/fear on damage.. this needs to be fixed properly!
		if(!(sp->AuraInterruptFlags & AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN))
		{
			for(uint32 z = 0; z < 3; ++z)
			{
				if(sp->EffectApplyAuraName[z] == SPELL_AURA_MOD_FEAR ||
				        sp->EffectApplyAuraName[z] == SPELL_AURA_MOD_ROOT)
				{
					sp->AuraInterruptFlags |= AURA_INTERRUPT_ON_UNUSED2;
					break;
				}

				if((sp->Effect[z] == SPELL_EFFECT_SCHOOL_DAMAGE && sp->Spell_Dmg_Type == SPELL_DMG_TYPE_MELEE) || sp->Effect[z] == SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL || sp->Effect[z] == SPELL_EFFECT_WEAPON_DAMAGE || sp->Effect[z] == SPELL_EFFECT_WEAPON_PERCENT_DAMAGE || sp->Effect[z] == SPELL_EFFECT_DUMMYMELEE)
					sp->is_melee_spell = true;
				if((sp->Effect[z] == SPELL_EFFECT_SCHOOL_DAMAGE && sp->Spell_Dmg_Type == SPELL_DMG_TYPE_RANGED))
				{
					//Log.Notice( "SpellFixes" , "Ranged Spell: %u [%s]" , sp->Id , sp->Name );
					sp->is_ranged_spell = true;
				}
			}
		}

		// set extra properties
		sp->RankNumber = rank;


		// various flight spells
		// these make vehicles and other charmed stuff fliable
		if( sp->activeIconID == 2158 )
			sp->Attributes |= ATTRIBUTES_PASSIVE;

		uint32 pr = sp->procFlags;
		for(uint32 y = 0; y < 3; y++)
		{
			// get the effect number from the spell
			effect = sp->Effect[y];

			//spell group

			if(effect == SPELL_EFFECT_APPLY_AURA)
			{
				uint32 aura = sp->EffectApplyAuraName[y];
				if(aura == SPELL_AURA_PROC_TRIGGER_SPELL ||
				        aura == SPELL_AURA_PROC_TRIGGER_DAMAGE
				  )//search for spellid in description
				{
					const char* p = sp->Description;
					while((p = strstr(p, "$")) != 0)
					{
						p++;
						//got $  -> check if spell
						if(*p >= '0' && *p <= '9')
						{
							//woot this is spell id

							result = atoi(p);
						}
					}
					pr = 0;

					uint32 len = (uint32)strlen(sp->Description);
					for(i = 0; i < len; ++i)
						sp->Description[i] = static_cast<char>(tolower(sp->Description[i]));
					//dirty code for procs, if any1 got any better idea-> u are welcome
					//139944 --- some magic number, it will trigger on all hits etc
					//for seems to be smth like custom check
					if(strstr(sp->Description, "your ranged criticals"))
						pr |= PROC_ON_RANGED_CRIT_ATTACK;
					if(strstr(sp->Description, "chance on hit"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "takes damage"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED;
					if(strstr(sp->Description, "attackers when hit"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "character strikes an enemy"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "strike you with a melee attack"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "target casts a spell"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "your harmful spells land"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "on spell critical hit"))
						pr |= PROC_ON_SPELL_CRIT_HIT;
					if(strstr(sp->Description, "spell critical strikes"))
						pr |= PROC_ON_SPELL_CRIT_HIT;
					if(strstr(sp->Description, "being able to resurrect"))
						pr |= PROC_ON_DIE;
					if(strstr(sp->Description, "any damage caused"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED;
					if(strstr(sp->Description, "the next melee attack against the caster"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "when successfully hit"))
						pr |= PROC_ON_MELEE_ATTACK ;
					if(strstr(sp->Description, "an enemy on hit"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "when it hits"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "when successfully hit"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "on a successful hit"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "damage to attacker on hit"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "on a hit"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "strikes you with a melee attack"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "when caster takes damage"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED;
					if(strstr(sp->Description, "when the caster is using melee attacks"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "when struck in combat") || strstr(sp->Description, "When struck in combat"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "successful melee attack"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "chance per attack"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "chance per hit"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "that strikes a party member"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "when hit by a melee attack"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "landing a melee critical strike"))
						pr |= PROC_ON_CRIT_ATTACK;
					if(strstr(sp->Description, "your critical strikes"))
						pr |= PROC_ON_CRIT_ATTACK;
					if(strstr(sp->Description, "whenever you deal ranged damage"))
						pr |= PROC_ON_RANGED_ATTACK;
					if(strstr(sp->Description, "you deal melee damage"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "your melee attacks"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "damage with your Sword"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "when struck in melee combat"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "any successful spell cast against the priest"))
						pr |= PROC_ON_SPELL_HIT_RECEIVED;
					if(strstr(sp->Description, "the next melee attack on the caster"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "striking melee or ranged attackers"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_RANGED_ATTACK_RECEIVED;
					if(strstr(sp->Description, "when damaging an enemy in melee"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "victim of a critical strike"))
						pr |= PROC_ON_CRIT_HIT_RECEIVED;
					if(strstr(sp->Description, "on successful melee or ranged attack"))
						pr |= PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
					if(strstr(sp->Description, "enemy that strikes you in melee"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "after getting a critical strike"))
						pr |= PROC_ON_CRIT_ATTACK;
					if(strstr(sp->Description, "whenever damage is dealt to you"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED;
					if(strstr(sp->Description, "when ranged or melee damage is dealt"))
						pr |= PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
					if(strstr(sp->Description, "damaging melee attacks"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "on melee or ranged attack"))
						pr |= PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
					if(strstr(sp->Description, "on a melee swing"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "Chance on melee"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "spell criticals against you"))
						pr |= PROC_ON_SPELL_CRIT_HIT_RECEIVED;
					if(strstr(sp->Description, "after being struck by a melee or ranged critical hit"))
						pr |= PROC_ON_CRIT_HIT_RECEIVED;
					if(strstr(sp->Description, "on a critical hit"))
						if(strstr(sp->Description, "critical hit"))
							pr |= PROC_ON_CRIT_ATTACK;
					if(strstr(sp->Description, "strikes the caster"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "a spell, melee or ranged attack hits the caster"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED;
					if(strstr(sp->Description, "after dealing a critical strike"))
						pr |= PROC_ON_CRIT_ATTACK;
					if(strstr(sp->Description, "each melee or ranged damage hit against the priest"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_RANGED_ATTACK_RECEIVED;
					if(strstr(sp->Description, "a chance to deal additional"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "chance to get an extra attack"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "melee attacks have"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "any damage spell hits a target"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "giving each melee attack a chance"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "damage when hit"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED; //maybe melee damage ?
					if(strstr(sp->Description, "gives your"))
					{
						if(strstr(sp->Description, "finishing moves"))
							pr |= PROC_ON_CAST_SPELL;
						else if(strstr(sp->Description, "melee"))
							pr |= PROC_ON_MELEE_ATTACK;
						else if(strstr(sp->Description, "sinister strike, backstab, gouge and shiv"))
							pr |= PROC_ON_CAST_SPELL;
						else if(strstr(sp->Description, "chance to daze the target"))
							pr |= PROC_ON_CAST_SPELL;
						else pr |= PROC_ON_CAST_SPECIFIC_SPELL;
					}
					if(strstr(sp->Description, "chance to add an additional combo") && strstr(sp->Description, "critical"))
						pr |= PROC_ON_CRIT_ATTACK;
					else if(strstr(sp->Description, "chance to add an additional combo"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "victim of a melee or ranged critical strike"))
						pr |= PROC_ON_CRIT_HIT_RECEIVED;
					if(strstr(sp->Description, "getting a critical effect from"))
						pr |= PROC_ON_SPELL_CRIT_HIT_RECEIVED;
					if(strstr(sp->Description, "damaging attack is taken"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED;
					if(strstr(sp->Description, "struck by a Stun or Immobilize"))
						pr |= PROC_ON_SPELL_HIT_RECEIVED;
					if(strstr(sp->Description, "melee critical strike"))
						pr |= PROC_ON_CRIT_ATTACK;
					if(strstr(sp->Name, "Bloodthirst"))
						pr |= PROC_ON_MELEE_ATTACK | static_cast<uint32>(PROC_TARGET_SELF);
					if(strstr(sp->Description, "experience or honor"))
						pr |= PROC_ON_GAIN_EXPIERIENCE;
					if(strstr(sp->Description, "hit by a melee or ranged attack"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_RANGED_ATTACK_RECEIVED;
					if(strstr(sp->Description, "enemy strikes the caster"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "melee and ranged attacks against you"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_RANGED_ATTACK_RECEIVED;
					if(strstr(sp->Description, "when a block occurs"))
						pr |= PROC_ON_BLOCK_RECEIVED;
					if(strstr(sp->Description, "dealing a critical strike from a weapon swing, spell, or ability"))
						pr |= PROC_ON_CRIT_ATTACK | PROC_ON_SPELL_CRIT_HIT;
					if(strstr(sp->Description, "dealing a critical strike from a weapon swing, spell, or ability"))
						pr |= PROC_ON_CRIT_ATTACK | PROC_ON_SPELL_CRIT_HIT;
					if(strstr(sp->Description, "shadow bolt critical strikes increase shadow damage"))
						pr |= PROC_ON_SPELL_CRIT_HIT;
					if(strstr(sp->Description, "after being hit with a shadow or fire spell"))
						pr |= PROC_ON_SPELL_LAND_RECEIVED;
					if(strstr(sp->Description, "giving each melee attack"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "each strike has"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "your Fire damage spell hits"))
						pr |= PROC_ON_CAST_SPELL;		//this happens only on hit ;)
					if(strstr(sp->Description, "corruption, curse of agony, siphon life and seed of corruption spells also cause"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "pain, mind flay and vampiric touch spells also cause"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "shadow damage spells have"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "on successful spellcast"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "your spell criticals have"))
						pr |= PROC_ON_SPELL_CRIT_HIT | PROC_ON_SPELL_CRIT_HIT_RECEIVED;
					if(strstr(sp->Description, "after dodging their attack"))
					{
						pr |= PROC_ON_DODGE_RECEIVED;
						if(strstr(sp->Description, "add a combo point"))
							pr |= PROC_TARGET_SELF;
					}
					if(strstr(sp->Description, "fully resisting"))
						pr |= PROC_ON_RESIST_RECEIVED;
					if(strstr(sp->Description, "Your Shadow Word: Pain, Mind Flay and Vampiric Touch spells also cause the target"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "chance on spell hit"))
						pr |= PROC_ON_CAST_SPELL;
					if(strstr(sp->Description, "your melee and ranged attacks"))
						pr |= PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
					//////////////////////////////////////////////////
					//proc dmg flags
					//////////////////////////////////////////////////
					if(strstr(sp->Description, "each attack blocked"))
						pr |= PROC_ON_BLOCK_RECEIVED;
					if(strstr(sp->Description, "into flame, causing an additional"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "victim of a critical melee strike"))
						pr |= PROC_ON_CRIT_HIT_RECEIVED;
					if(strstr(sp->Description, "damage to melee attackers"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "target blocks a melee attack"))
						pr |= PROC_ON_BLOCK_RECEIVED;
					if(strstr(sp->Description, "ranged and melee attacks to deal"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_RANGED_ATTACK_RECEIVED;
					if(strstr(sp->Description, "damage on hit"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED;
					if(strstr(sp->Description, "chance on hit"))
						pr |= PROC_ON_MELEE_ATTACK;
					if(strstr(sp->Description, "after being hit by any damaging attack"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED;
					if(strstr(sp->Description, "striking melee or ranged attackers"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_RANGED_ATTACK_RECEIVED;
					if(strstr(sp->Description, "damage to attackers when hit"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "striking melee attackers"))
						pr |= PROC_ON_MELEE_ATTACK_RECEIVED;
					if(strstr(sp->Description, "whenever the caster takes damage"))
						pr |= PROC_ON_ANY_DAMAGE_RECEIVED;
					if(strstr(sp->Description, "damage on every attack"))
						pr |= PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
					if(strstr(sp->Description, "chance to reflect Fire spells"))
						pr |= PROC_ON_SPELL_HIT_RECEIVED;
					if(strstr(sp->Description, "hunter takes on the aspects of a hawk"))
						pr |= PROC_TARGET_SELF | PROC_ON_RANGED_ATTACK;
					if(strstr(sp->Description, "successful auto shot attacks"))
						pr |= PROC_ON_AUTO_SHOT_HIT;
					if(strstr(sp->Description, "after getting a critical effect from your"))
						pr = PROC_ON_SPELL_CRIT_HIT;
				}//end "if procspellaura"

				// Fix if it's a periodic trigger with amplitude = 0, to avoid division by zero
				else if((aura == SPELL_AURA_PERIODIC_TRIGGER_SPELL || aura == SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE) && sp->EffectAmplitude[y] == 0)
				{
					sp->EffectAmplitude[y] = 1000;
				}
				else if(aura == SPELL_AURA_SCHOOL_ABSORB && sp->AuraFactoryFunc == NULL)
					sp->AuraFactoryFunc = (void * (*)) &AbsorbAura::Create;
			}//end "if aura"
		}//end "for each effect"
		sp->procFlags = pr;

		if(strstr(sp->Description, "Must remain seated"))
		{
			sp->RecoveryTime = 1000;
			sp->CategoryRecoveryTime = 1000;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// procintervals
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		//omg lightning shield trigger spell id's are all wrong ?
		//if you are bored you could make these by hand but i guess we might find other spells with this problem..and this way it's safe
		if(strstr(sp->Name, "Lightning Shield") && sp->EffectTriggerSpell[0])
		{
			//check if we can find in the description
			const char* startofid = strstr(sp->Description, "for $");
			if(startofid)
			{
				startofid += strlen("for $");
				sp->EffectTriggerSpell[0] = atoi(startofid);   //get new lightning shield trigger id
			}
			sp->proc_interval = urand(2500, 5000); //few seconds
		}
		//mage ignite talent should proc only on some chances
		else if(strstr(sp->Name, "Ignite") && sp->Id >= 11119 && sp->Id <= 12848 && sp->EffectApplyAuraName[0] == SPELL_AURA_DUMMY)
		{
			//check if we can find in the description
			const char* startofid = strstr(sp->Description, "an additional ");
			if(startofid)
			{
				startofid += strlen("an additional ");
				sp->EffectBasePoints[0] = atoi(startofid); //get new value. This is actually level*8 ;)
			}
			sp->Effect[0] = SPELL_EFFECT_APPLY_AURA; //aura
			sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL; //force him to use procspell effect
			sp->EffectTriggerSpell[0] = 12654; //evil , but this is good for us :D
			sp->procFlags = PROC_ON_SPELL_CRIT_HIT; //add procflag here since this was not processed with the others !
		}
		// Winter's Chill handled by frost school
		else if(strstr(sp->Name, "Winter's Chill"))
		{
			sp->School = SCHOOL_FROST;
		}
		//more triggered spell ids are wrong. I think blizz is trying to outsmart us :S
		//Chain Heal all ranks %50 heal value (49 + 1)
		else if(strstr(sp->Name, "Chain Heal"))
		{
			sp->EffectDieSides[0] = 49;
		}
		else if(strstr(sp->Name, "Touch of Weakness"))
		{
			//check if we can find in the description
			const char* startofid = strstr(sp->Description, "cause $");
			if(startofid)
			{
				startofid += strlen("cause $");
				sp->EffectTriggerSpell[0] = atoi(startofid);
				sp->EffectTriggerSpell[1] = sp->EffectTriggerSpell[0]; //later versions of this spell changed to eff[1] the aura
				sp->procFlags = uint32(PROC_ON_MELEE_ATTACK_RECEIVED);
			}
		}
		else if(strstr(sp->Name, "Firestone Passive"))
		{
			//Enchants the main hand weapon with fire, granting each attack a chance to deal $17809s1 additional fire damage.
			//check if we can find in the description
			char* startofid = strstr(sp->Description, "to deal $");
			if(startofid)
			{
				startofid += strlen("to deal $");
				sp->EffectTriggerSpell[0] = atoi(startofid);
				sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
				sp->procFlags = PROC_ON_MELEE_ATTACK;
				sp->procChance = 50;
			}
		}
		//some procs trigger at intervals
		else if(strstr(sp->Name, "Water Shield"))
		{
			sp->proc_interval = 3000; //few seconds
			sp->procFlags |= PROC_TARGET_SELF;
		}
		else if(strstr(sp->Name, "Earth Shield"))
			sp->proc_interval = 3000; //few seconds
		else if(strstr(sp->Name, "Poison Shield"))
			sp->proc_interval = 3000; //few seconds
		else if(strstr(sp->Name, "Infused Mushroom"))
			sp->proc_interval = 10000; //10 seconds
		else if(strstr(sp->Name, "Aviana's Purpose"))
			sp->proc_interval = 10000; //10 seconds
		//don't change to namehash since we are searching only a portion of the name
		else if(strstr(sp->Name, "Crippling Poison"))
		{
			sp->c_is_flags |= SPELL_FLAG_IS_POISON;
		}
		else if(strstr(sp->Name, "Mind-numbing Poison"))
		{
			sp->c_is_flags |= SPELL_FLAG_IS_POISON;
		}
		else if(strstr(sp->Name, "Instant Poison"))
		{
			sp->c_is_flags |= SPELL_FLAG_IS_POISON;
		}
		else if(strstr(sp->Name, "Deadly Poison"))
		{
			sp->c_is_flags |= SPELL_FLAG_IS_POISON;
		}
		else if(strstr(sp->Name, "Wound Poison"))
		{
			sp->c_is_flags |= SPELL_FLAG_IS_POISON;
		}
		else if(strstr(sp->Name, "Scorpid Poison"))
		{
			// groups?
			sp->c_is_flags |= SPELL_FLAG_IS_POISON;
		}

		if(sp->NameHash == SPELL_HASH_ILLUMINATION)
			sp->procFlags |= PROC_TARGET_SELF;

		// Set default mechanics if we don't already have one
		if(!sp->MechanicsType)
		{
			//Set Silencing spells mechanic.
			if(sp->EffectApplyAuraName[0] == SPELL_AURA_MOD_SILENCE ||
			        sp->EffectApplyAuraName[1] == SPELL_AURA_MOD_SILENCE ||
			        sp->EffectApplyAuraName[2] == SPELL_AURA_MOD_SILENCE)
				sp->MechanicsType = MECHANIC_SILENCED;

			//Set Stunning spells mechanic.
			if(sp->EffectApplyAuraName[0] == SPELL_AURA_MOD_STUN ||
			        sp->EffectApplyAuraName[1] == SPELL_AURA_MOD_STUN ||
			        sp->EffectApplyAuraName[2] == SPELL_AURA_MOD_STUN)
				sp->MechanicsType = MECHANIC_STUNNED;

			//Set Fearing spells mechanic
			if(sp->EffectApplyAuraName[0] == SPELL_AURA_MOD_FEAR ||
			        sp->EffectApplyAuraName[1] == SPELL_AURA_MOD_FEAR ||
			        sp->EffectApplyAuraName[2] == SPELL_AURA_MOD_FEAR)
				sp->MechanicsType = MECHANIC_FLEEING;

			//Set Interrupted spells mech
			if(sp->Effect[0] == SPELL_EFFECT_INTERRUPT_CAST ||
			        sp->Effect[1] == SPELL_EFFECT_INTERRUPT_CAST ||
			        sp->Effect[2] == SPELL_EFFECT_INTERRUPT_CAST)
				sp->MechanicsType = MECHANIC_INTERRUPTED;
		}

		if(sp->proc_interval != 0)
			sp->procFlags |= PROC_REMOVEONUSE;

		// Seal of Command - Proc Chance
		if(sp->NameHash == SPELL_HASH_SEAL_OF_COMMAND)
		{
			sp->procChance = 25;
			sp->School = SCHOOL_HOLY; //the procspells of the original seal of command have physical school instead of holy
			sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC; //heh, crazy spell uses melee/ranged/magic dmg type for 1 spell. Now which one is correct ?
		}

		/* Decapitate */
		if(sp->NameHash == SPELL_HASH_DECAPITATE)
			sp->procChance = 30;

		//shaman - shock, has no spellgroup.very dangerous move !

		//mage - fireball. Only some of the spell has the flags

		if(sp->NameHash == SPELL_HASH_DIVINE_SHIELD || sp->NameHash == SPELL_HASH_DIVINE_PROTECTION || sp->NameHash == SPELL_HASH_BLESSING_OF_PROTECTION)
			sp->MechanicsType = MECHANIC_INVULNARABLE;

		/* hackfix for this - FIX ME LATER - Burlex */
		if(namehash == SPELL_HASH_SEAL_FATE)
			sp->procFlags = 0;

		if(
		    ((sp->Attributes & ATTRIBUTES_TRIGGER_COOLDOWN) && (sp->AttributesEx & ATTRIBUTESEX_NOT_BREAK_STEALTH)) //rogue cold blood
		    || ((sp->Attributes & ATTRIBUTES_TRIGGER_COOLDOWN) && (!sp->AttributesEx || sp->AttributesEx & ATTRIBUTESEX_REMAIN_OOC))
		)
		{
			sp->c_is_flags |= SPELL_FLAG_IS_REQUIRECOOLDOWNUPDATE;
		}

		if(namehash == SPELL_HASH_SHRED || namehash == SPELL_HASH_BACKSTAB || namehash == SPELL_HASH_AMBUSH || namehash == SPELL_HASH_GARROTE || namehash == SPELL_HASH_RAVAGE)
		{
			// FIX ME: needs different flag check
			sp->FacingCasterFlags = SPELL_INFRONT_STATUS_REQUIRE_INBACK;
		}
	}

	/////////////////////////////////////////////////////////////////
	//SPELL COEFFICIENT SETTINGS START
	//////////////////////////////////////////////////////////////////

	for(uint32 x = 0; x < cnt; x++)
	{
		// get spellentry
		sp = dbcSpell.LookupRow(x);

		//Setting Cast Time Coefficient
		SpellCastTime* sd = dbcSpellCastTime.LookupEntry(sp->CastingTimeIndex);
		float castaff = float(GetCastTime(sd));
		if(castaff < 1500)
			castaff = 1500;
		else if(castaff > 7000)
			castaff = 7000;

		sp->casttime_coef = castaff / 3500;

		SpellEntry* spz;
		bool spcheck = false;

		//Flag for DoT and HoT
		for(i = 0 ; i < 3 ; i++)
		{
			if(sp->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_DAMAGE ||
			        sp->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_HEAL ||
			        sp->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_LEECH)
			{
				sp->spell_coef_flags |= SPELL_FLAG_IS_DOT_OR_HOT_SPELL;
				break;
			}
		}

		//Flag for DD or DH
		for(i = 0 ; i < 3 ; i++)
		{
			if(sp->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL && sp->EffectTriggerSpell[i])
			{
				spz = dbcSpell.LookupEntryForced(sp->EffectTriggerSpell[i]);
				if(spz &&
				        (spz->Effect[i] == SPELL_EFFECT_SCHOOL_DAMAGE ||
				         spz->Effect[i] == SPELL_EFFECT_HEAL)
				  )
					spcheck = true;
			}
			if(sp->Effect[i] == SPELL_EFFECT_SCHOOL_DAMAGE ||
			        sp->Effect[i] == SPELL_EFFECT_HEAL ||
			        spcheck
			  )
			{
				sp->spell_coef_flags |= SPELL_FLAG_IS_DD_OR_DH_SPELL;
				break;
			}
		}

		for(i = 0 ; i < 3; i++)
		{
			switch(sp->EffectImplicitTargetA[i])
			{
					//AoE
				case EFF_TARGET_ALL_TARGETABLE_AROUND_LOCATION_IN_RADIUS:
				case EFF_TARGET_ALL_ENEMY_IN_AREA:
				case EFF_TARGET_ALL_ENEMY_IN_AREA_INSTANT:
				case EFF_TARGET_ALL_PARTY_AROUND_CASTER:
				case EFF_TARGET_ALL_ENEMIES_AROUND_CASTER:
				case EFF_TARGET_IN_FRONT_OF_CASTER:
				case EFF_TARGET_ALL_ENEMY_IN_AREA_CHANNELED:
				case EFF_TARGET_ALL_PARTY_IN_AREA_CHANNELED:
				case EFF_TARGET_ALL_FRIENDLY_IN_AREA:
				case EFF_TARGET_ALL_TARGETABLE_AROUND_LOCATION_IN_RADIUS_OVER_TIME:
				case EFF_TARGET_ALL_PARTY:
				case EFF_TARGET_LOCATION_INFRONT_CASTER:
				case EFF_TARGET_BEHIND_TARGET_LOCATION:
				case EFF_TARGET_LOCATION_INFRONT_CASTER_AT_RANGE:
					{
						sp->spell_coef_flags |= SPELL_FLAG_AOE_SPELL;
						break;
					}
			}
		}

		for(i = 0 ; i < 3 ; i++)
		{
			switch(sp->EffectImplicitTargetB[i])
			{
					//AoE
				case EFF_TARGET_ALL_TARGETABLE_AROUND_LOCATION_IN_RADIUS:
				case EFF_TARGET_ALL_ENEMY_IN_AREA:
				case EFF_TARGET_ALL_ENEMY_IN_AREA_INSTANT:
				case EFF_TARGET_ALL_PARTY_AROUND_CASTER:
				case EFF_TARGET_ALL_ENEMIES_AROUND_CASTER:
				case EFF_TARGET_IN_FRONT_OF_CASTER:
				case EFF_TARGET_ALL_ENEMY_IN_AREA_CHANNELED:
				case EFF_TARGET_ALL_PARTY_IN_AREA_CHANNELED:
				case EFF_TARGET_ALL_FRIENDLY_IN_AREA:
				case EFF_TARGET_ALL_TARGETABLE_AROUND_LOCATION_IN_RADIUS_OVER_TIME:
				case EFF_TARGET_ALL_PARTY:
				case EFF_TARGET_LOCATION_INFRONT_CASTER:
				case EFF_TARGET_BEHIND_TARGET_LOCATION:
				case EFF_TARGET_LOCATION_INFRONT_CASTER_AT_RANGE:
					{
						sp->spell_coef_flags |= SPELL_FLAG_AOE_SPELL;
						break;
					}
			}
		}

		//Special Cases
		//Holy Light & Flash of Light
		if(sp->NameHash == SPELL_HASH_HOLY_LIGHT ||
		        sp->NameHash == SPELL_HASH_FLASH_OF_LIGHT)
			sp->spell_coef_flags |= SPELL_FLAG_IS_DD_OR_DH_SPELL;


		//Additional Effect (not healing or damaging)
		for(i = 0 ; i < 3 ; i++)
		{
			if(sp->Effect[i] == SPELL_EFFECT_NULL)
				continue;

			switch(sp->Effect[i])
			{
				case SPELL_EFFECT_SCHOOL_DAMAGE:
				case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
				case SPELL_EFFECT_HEALTH_LEECH:
				case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
				case SPELL_EFFECT_ADD_EXTRA_ATTACKS:
				case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
				case SPELL_EFFECT_POWER_BURN:
				case SPELL_EFFECT_ATTACK:
				case SPELL_EFFECT_HEAL:
				case SPELL_EFFECT_HEAL_MAX_HEALTH:
				case SPELL_EFFECT_DUMMY:
					continue;
			}

			switch(sp->EffectApplyAuraName[i])
			{
				case SPELL_AURA_PERIODIC_DAMAGE:
				case SPELL_AURA_PROC_TRIGGER_DAMAGE:
				case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
				case SPELL_AURA_POWER_BURN:
				case SPELL_AURA_PERIODIC_HEAL:
				case SPELL_AURA_MOD_INCREASE_HEALTH:
				case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
				case SPELL_AURA_DUMMY:
					continue;
			}

			sp->spell_coef_flags |= SPELL_FLAG_ADITIONAL_EFFECT;
			break;

		}

		//Calculating fixed coeficients
		//Channeled spells
		if(sp->ChannelInterruptFlags != 0)
		{
			float Duration = float(GetDuration(dbcSpellDuration.LookupEntry(sp->DurationIndex)));
			if(Duration < 1500) Duration = 1500;
			else if(Duration > 7000) Duration = 7000;
			sp->fixed_hotdotcoef = (Duration / 3500.0f);

			if(sp->spell_coef_flags & SPELL_FLAG_ADITIONAL_EFFECT)
				sp->fixed_hotdotcoef *= 0.95f;
			if(sp->spell_coef_flags & SPELL_FLAG_AOE_SPELL)
				sp->fixed_hotdotcoef *= 0.5f;
		}

		//Combined standard and over-time spells
		else if(sp->spell_coef_flags & SPELL_FLAG_IS_DD_DH_DOT_SPELL)
		{
			float Duration = float(GetDuration(dbcSpellDuration.LookupEntry(sp->DurationIndex)));
			float Portion_to_Over_Time = (Duration / 15000.0f) / ((Duration / 15000.0f) + sp->casttime_coef);
			float Portion_to_Standard = 1.0f - Portion_to_Over_Time;

			sp->fixed_dddhcoef = sp->casttime_coef * Portion_to_Standard;
			sp->fixed_hotdotcoef = (Duration / 15000.0f) * Portion_to_Over_Time;
		}

		//////////////////////////////////////////////////////
		// CLASS-SPECIFIC SPELL FIXES						//
		//////////////////////////////////////////////////////

		// Paladin - Consecration
		if(sp->NameHash == SPELL_HASH_CONSECRATION)
		{
			sp->School = SCHOOL_HOLY; //Consecration is a holy redirected spell.
			sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC; //Speaks for itself.
		}

		// Disengage
		// Only works in combat
		if(sp->Id == 781)
			sp->CustomFlags = CUSTOM_FLAG_SPELL_REQUIRES_COMBAT;

	}

	//Fully loaded coefficients, we must share channeled coefficient to its triggered spells
	for(uint32 x = 0; x < cnt; x++)
	{
		// get spellentry
		sp = dbcSpell.LookupRow(x);
		SpellEntry* spz;

		//Case SPELL_AURA_PERIODIC_TRIGGER_SPELL
		for(i = 0 ; i < 3 ; i++)
		{
			if(sp->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL)
			{
				spz = CheckAndReturnSpellEntry(sp->EffectTriggerSpell[i]);
				if(spz != NULL)
				{
					if(sp->Dspell_coef_override >= 0)
						spz->Dspell_coef_override = sp->Dspell_coef_override;
					else
					{
						//we must set bonus per tick on triggered spells now (i.e. Arcane Missiles)
						if(sp->ChannelInterruptFlags != 0)
						{
							float Duration = float(GetDuration(dbcSpellDuration.LookupEntry(sp->DurationIndex)));
							float amp = float(sp->EffectAmplitude[i]);
							sp->fixed_dddhcoef = sp->fixed_hotdotcoef * amp / Duration;
						}
						spz->fixed_dddhcoef = sp->fixed_dddhcoef;
					}

					if(sp->OTspell_coef_override >= 0)
						spz->OTspell_coef_override = sp->OTspell_coef_override;
					else
					{
						//we must set bonus per tick on triggered spells now (i.e. Arcane Missiles)
						if(sp->ChannelInterruptFlags != 0)
						{
							float Duration = float(GetDuration(dbcSpellDuration.LookupEntry(sp->DurationIndex)));
							float amp = float(sp->EffectAmplitude[i]);
							sp->fixed_hotdotcoef *= amp / Duration;
						}
						spz->fixed_hotdotcoef = sp->fixed_hotdotcoef;
					}
					break;
				}
			}
		}
	}

	/////////////////////////////////////////////////////////////////
	//SPELL COEFFICIENT SETTINGS END
	/////////////////////////////////////////////////////////////////

	/**********************************************************
	 * Misc stuff (questfixes etc)
	 **********************************************************/

	// list of guardians that should inherit casters level
	//fire elemental
	sp = CheckAndReturnSpellEntry(32982);
	if(sp != NULL)
		sp->c_is_flags |= SPELL_FLAG_IS_INHERITING_LEVEL;

	//Earth elemental
	sp = CheckAndReturnSpellEntry(33663);
	if(sp != NULL)
		sp->c_is_flags |= SPELL_FLAG_IS_INHERITING_LEVEL;

	/**********************************************************
	 * Scaled Mounts
	 **********************************************************/
	//Big Blizzard Bear
	sp = CheckAndReturnSpellEntry(58983);
	if(sp != NULL)
		sp->Effect[0] = SPELL_EFFECT_NULL;
	//Winged Steed of Ebon Blade
	sp = CheckAndReturnSpellEntry(54729);
	if(sp != NULL)
		sp->Effect[0] = SPELL_EFFECT_NULL;
	//Headless Horsemen Mount
	sp = CheckAndReturnSpellEntry(48025);
	if(sp != NULL)
		sp->Effect[0] = SPELL_EFFECT_NULL;
	//Magic Broom
	sp = CheckAndReturnSpellEntry(47977);
	if(sp != NULL)
		sp->Effect[0] = SPELL_EFFECT_NULL;
	//Magic Rooster
	sp = CheckAndReturnSpellEntry(65917);
	if(sp != NULL)
		sp->Effect[0] = SPELL_EFFECT_NULL;
	//Invincible
	sp = CheckAndReturnSpellEntry(72286);
	if(sp != NULL)
		sp->Effect[0] = SPELL_EFFECT_NULL;

    uint32 cntt = dbcSpell.GetNumRows();


    for (uint32 zx = 0; zx < cntt; zx++)
    {
        // Read every SpellEntry row
        SpellEntry* sp = dbcSpell.LookupRow(zx);

        sp->ProcsPerMinute = 0;
        sp->spell_can_crit = true;
        sp->AdditionalAura = 0;
        //sp->Unique = false;

        switch (sp->Id)
        {
        case 3286: // BIND HARDCODING
        {
            sp->Effect[0] = 11; sp->Effect[1] = 24;
            sp->EffectImplicitTargetA[0] = sp->EffectImplicitTargetA[1] = 149;
        }break;

        case 1455: // Spell 1455 Proc Chance (Life Tap Rank 2:
        case 1456: // Spell 1456 Proc Chance (Life Tap Rank 3:
        case 8182: // Spell 8182 Proc Chance (Frost Resistance Rank 1:
        case 8185: // Spell 8185 Proc Chance (Fire Resistance Rank 1:
        case 10476: // Spell 10476 Proc Chance (Frost Resistance Rank 2:
        case 10477: // Spell 10477 Proc Chance (Frost Resistance Rank 3:
        case 10534: // Spell 10534 Proc Chance (Fire Resistance Rank 2:
        case 10535: // Spell 10535 Proc Chance (Fire Resistance Rank 3:
        case 11687: // Spell 11687 Proc Chance (Life Tap Rank 4:
        case 11688: // Spell 11688 Proc Chance (Life Tap Rank 5:
        case 11689: // Spell 11689 Proc Chance (Life Tap Rank 6:
        case 12292: // Spell 12292 Proc Chance (Death Wish :
        case 18803: // Spell 18803 Proc Chance (Focus :
            sp->procChance = 100;
            break;

        case 3391: // Spell 3391 Proc Chance (Thrash :
        case 15494: // Spell 15494 Proc Chance (Fury of Forgewright :
        case 15601: // Spell 15601 Proc Chance (Hand of Justice :
        case 15642: // Spell 15642 Proc Chance (Ironfoe :
        case 16843: // Spell 16843 Proc Chance (Crimson Fury :
        case 18797: // Spell 18797 Proc Chance (Flurry Axe :
        case 19105: // Spell 19105 Proc Chance (MHTest01 Effect :
        case 19109: // Spell 19109 Proc Chance (MHTest02 Effect :
        case 27035: // Spell 27035 Proc Chance (Sword Specialization (OLD: :
        case 21919: // Spell 21919 Proc Chance (Thrash :
            sp->procChance = 10;
            break;

            // Spell 12284 Proc Chance (Mace Specialization Rank 1:
        case 12284:
            sp->procChance = 1;
            break;

            // Spell 12322 Proc Chance (Unbridled Wrath Rank 1:
        case 12322:
            sp->ProcsPerMinute = 3;
            break;

            // Spell 12701 Proc Chance (Mace Specialization Rank 2:
        case 12701:
            sp->procChance = 2;
            break;

            // Spell 12702 Proc Chance (Mace Specialization Rank 3:
        case 12702:
            sp->procChance = 3;
            break;

            // Spell 12703 Proc Chance (Mace Specialization Rank 4:
        case 12703:
            sp->procChance = 4;
            break;

            // Spell 12704 Proc Chance (Mace Specialization Rank 5:
        case 12704:
            sp->procChance = 6;
            break;

            // Spell 12999 Proc Chance (Unbridled Wrath Rank 2:
        case 12999:
            sp->ProcsPerMinute = 6;
            break;

            // Spell 13000 Proc Chance (Unbridled Wrath Rank 3:
        case 13000:
            sp->ProcsPerMinute = 9;
            break;

            // Spell 13001 Proc Chance (Unbridled Wrath Rank 4:
        case 13001:
            sp->ProcsPerMinute = 12;
            break;

            // Spell 13002 Proc Chance (Unbridled Wrath Rank 5:
        case 13002:
            sp->ProcsPerMinute = 15;
            break;

            // Spell 14076 Proc Chance (Dirty Tricks Rank 1:
        case 14076:
            sp->procChance = 30;
            break;

            // Spell 14094 Proc Chance (Dirty Tricks Rank 2:
        case 14094:
            sp->procChance = 60;
            break;

            // Spell 15600 Proc Chance (Hand of Justice :
        case 15600:
            sp->procChance = 2;
            break;

            // Spell 23158 Proc Chance (Concussive Shot Cooldown Reduction :
        case 23158:
            sp->procChance = 4;
            break;

            // Spell 27521 Proc Chance (Mana Restore :
        case 27521:
            sp->procChance = 2;
            break;

            // Spell 27867 Proc Chance (Freeze :
        case 27867:
            sp->procChance = 2;
            break;

            // Spell 27498 Crusader's Wrath
        case 27498:
            sp->procChance = 7;
            break;

            //Martyrdom
        case 14531:
        case 14774:
        {
            sp->procFlags = PROC_ON_CRIT_HIT_RECEIVED | PROC_ON_RANGED_CRIT_ATTACK_RECEIVED;
        }break;

        // Impact proc
        case 64343:
        {
            sp->AuraInterruptFlags |= AURA_INTERRUPT_ON_CAST_SPELL;
        }break;

        // Elemental Focus
        case 16164:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 16246:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            //sp->procCharges++; //   first   charge gets lost when   it gets procced
        }break;

        //shaman - Stormstrike
        case 17364:
        {
            sp->procFlags = PROC_ON_SPELL_HIT_RECEIVED;
        }break;

        //shaman - Improved Stormstrike
        case 51521:
        case 51522:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //shaman - Hex
        case 51514:
        {
            // Damage   caused may interrupt the effect.
            sp->AuraInterruptFlags |= AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN;
        }break;

        case 52752:
        {
            sp->spell_can_crit = false;
        }break;

        // shaman   -   Maelstrom   Weapon
        case 51528:
        {
            sp->procChance = 0;
            sp->ProcsPerMinute = 1.3f;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;
        case 51529:
        {
            sp->procChance = 0;
            sp->ProcsPerMinute = 2.6f;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;
        case 51530:
        {
            sp->procChance = 0;
            sp->ProcsPerMinute = 3.9f;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;
        case 51531:
        {
            sp->procChance = 0;
            sp->ProcsPerMinute = 5.2f;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;
        case 51532:
        {
            sp->procChance = 0;
            sp->ProcsPerMinute = 6.5f;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;

        // Maelstorm proc   charge removal
        case 53817:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //Improved Fire Nova
        case 16086:
        {
            sp->EffectApplyAuraName[1] = SPELL_AURA_ADD_FLAT_MODIFIER;
        }break;
        //Improved Fire Nova
        case 16544:
        {
            sp->EffectApplyAuraName[1] = SPELL_AURA_ADD_FLAT_MODIFIER;
        }break;

        // Astral   Shift
        case 52179:
        {
            sp->EffectAmplitude[0] = 0;
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
            sp->EffectMiscValue[0] = 127;
        }break;

        // Shamanistic Rage
        case 30823:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        case 51479:
        case 51478:
        case 51474: // Astral Shift
        {
            sp->procChance = 100;
            sp->EffectImplicitTargetA[0] = 1;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_SPELL_LAND_RECEIVED;
            sp->EffectTriggerSpell[0] = 52179;
            sp->Effect[1] = 0;
        }break;

        case 51975:// shaman    -   Poison Cleansing Totem
        case 52025:// shaman    -   Disease Cleansing   Totem
        {

            SpellEntry*  sp2 = dbcSpell.LookupEntryForced(58780);
            if (sp2 != NULL)
            {
                sp->EffectRadiusIndex[0] = sp2->EffectRadiusIndex[0];   //30 yards
                sp->EffectImplicitTargetA[0] = EFF_TARGET_ALL_PARTY_IN_AREA;
            }
        }break;

        case 64395:
        {
            sp->School = SCHOOL_ARCANE;
        }break;

        // Flurry
        case 12319:
        case 12971:
        case 12972:
        case 12973:
        case 12974:
        case 16256:
        case 16281:
        case 16282:
        case 16283:
        case 16284:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
        }break;

        // Flurry   proc
        case 12966:
        case 12967:
        case 12968:
        case 12969:
        case 12970:
        case 16257:
        case 16277:
        case 16278:
        case 16279:
        case 16280:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            //sp->procCharges++; //   first   charge gets lost when   it gets procced
        }break;

        //Minor Glyph Research + Northrend Inscription Research
        case 61177:
        case 61288:
        case 60893:
        {
            sp->Effect[1] = 0;
            sp->EffectBasePoints[1] = 0;
            sp->EffectImplicitTargetA[1] = 0;
            sp->EffectDieSides[1] = 0;
        }break;

        // Shaman   Totem   items   fixes
        // Totem of Survival,   Totem   of the Tundra
        case 46097:
        case 43860:
        case 43861:
        case 43862:
        case 60564:
        case 60571:
        case 60572:
        case 37575:
        {
            sp->EffectSpellClassMask[0][0] = 0x00100000 | 0x10000000 | 0x80000000; //Earth + Flame + Frost Shocks
            sp->EffectSpellClassMask[0][1] = 0x08000000;    // Wind Shock
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_SELF;
        }break;
        case 60567:
        {
            sp->EffectImplicitTargetA[1] = EFF_TARGET_SELF;
        }break;

        // Totem of Third   Wind
        case 46098:
        case 34138:
        case 42370:
        case 43728:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x00000080;    // Lesser   Healing Wave
        }break;

        // Totem of the Elemental   Plane
        case 60770:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x00000001;    // Lightning Bolt
        }break;

        // Fathom-Brooch of the Tidewalker
        case 37247:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->proc_interval = 45000;
        }break;

        //Warlock: Backlash
        case 34935:
        case 34938:
        case 34939:
        {
            sp->proc_interval = 8000;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        case 34936:
        {
            sp->AuraInterruptFlags |= AURA_INTERRUPT_ON_CAST_SPELL;
        }break;

        //Priest - Holy Nova
        case 15237:
        {
            sp->Effect[0] = sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectTriggerSpell[1] = 23455;
        }break;

        case 15430:
        {
            sp->Effect[0] = sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectTriggerSpell[1] = 23458;
        }break;

        case 15431:
        {
            sp->Effect[0] = sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectTriggerSpell[1] = 23459;
        }break;

        case 25331:
        {
            sp->Effect[0] = sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectTriggerSpell[1] = 27803;
        }break;

        case 27799:
        {
            sp->Effect[0] = sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectTriggerSpell[1] = 27804;
        }break;

        case 27800:
        {
            sp->Effect[0] = sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectTriggerSpell[1] = 27805;
        }break;

        case 27801:
        {
            sp->Effect[0] = sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectTriggerSpell[1] = 25329;
        }break;

        case 48077:
        {
            sp->Effect[0] = sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectTriggerSpell[1] = 48075;
        }break;

        case 48078:
        {
            sp->Effect[0] = sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectTriggerSpell[1] = 48076;
        }break;

        // Moroes' garrote targets a single enemy   instead of us
        case 37066:
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_SINGLE_ENEMY;
        }break;

        // Penance
        case 47540:
        case 53005:
        case 53006:
        case 53007:
        {
            sp->AttributesEx = 0;
            sp->DurationIndex = 566; // Change to instant cast as script will cast the real channeled spell.
            sp->ChannelInterruptFlags = 0; // Remove channeling behaviour.
        }break;

        // Penance triggered healing spells have wrong targets.
        case 47750:
        case 52983:
        case 52984:
        case 52985:
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_SINGLE_FRIEND;
        }break;

        // Hymn of Hope
        case 60931:
        case 60933:
        {
            sp->EffectImplicitTargetA[0] = 37;
        }break;

        //paladin   -   Reckoning
        case 20177:
        case 20179:
        case 20180:
        case 20181:
        case 20182:
        {
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        //paladin   -   Reckoning   Effect = proc   on proc
        case 20178:
        {
            sp->procChance = 100;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        //paladin   -   Judgement   of Wisdom
        case 20186:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 20268;
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
            sp->procChance = 35;
            sp->Effect[1] = 0;
            sp->EffectApplyAuraName[1] = 0;
        }break;

        //paladin   -   Judgement   of Light
        case 20185:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 20267;
            sp->procChance = 35;
            sp->Effect[1] = 0;
            sp->EffectApplyAuraName[1] = 0;
        }break;

        case 20267:
        case 20268:
        case 20341:
        case 20342:
        case 20343:
        case 27163:
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_SINGLE_ENEMY;
        }break;
        //paladin   -   Eye for an Eye
        case 9799:
        case 25988:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT_RECEIVED | PROC_ON_CRIT_HIT_RECEIVED;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 25997;
        }break;

        //paladin Blessed Life
        case 31828:
        case 31829:
        case 31830:
        {
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
            sp->EffectTriggerSpell[0] = 31828;
        }break;

        //paladin   -   Light's Grace
        case 31833:
        case 31835:
        case 31836:
        {
            sp->EffectSpellClassMask[0][0] = 0x80000000;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //Paladin - Sacred Cleansing
        case 53551:
        case 53552:
        case 53553:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x00001000;
            sp->EffectSpellClassMask[0][1] = 0x00001000;
            sp->EffectSpellClassMask[0][2] = 0x00001000;
        }break;
        //Shield of the Templar
        case 53709:
        case 53710:
        case 53711:
        {
            sp->EffectSpellClassMask[0][0] = 64;
            sp->EffectSpellClassMask[0][1] = 16384;
            sp->EffectSpellClassMask[0][2] = 1048576;
            sp->EffectSpellClassMask[1][0] = 64;
            sp->EffectSpellClassMask[1][1] = 16384;
            sp->EffectSpellClassMask[1][2] = 1048576;
            sp->EffectSpellClassMask[2][0] = 64;
            sp->EffectSpellClassMask[2][1] = 16384;
            sp->EffectSpellClassMask[2][2] = 1048576;
        }break;
        //Paladin - Infusion of light
        case 53569:
        case 53576:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 53672:
        case 54149:
        {
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
        }break;
        //Paladin - Vindication
        case 9452:
        case 26016:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_ANY_HOSTILE_ACTION;
            sp->procChance = 25;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA; // hoax
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
            sp->maxstack = 1;
        }break;
        //Paladin - Art of War
        case 53486:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK | PROC_ON_SPELL_CRIT_HIT;
            sp->EffectApplyAuraName[2] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[2] = 53489;
        }break;
        case 53488:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK | PROC_ON_SPELL_CRIT_HIT;
            sp->EffectApplyAuraName[2] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[2] = 59578;
        }break;
        case 53489:
        case 59578:
        {
            sp->AuraInterruptFlags |= AURA_INTERRUPT_ON_CAST_SPELL;
        }break;

        //shaman - Lightning Overload
        case 30675: // rank 1
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 39805;//proc something (we will owerride this)
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 4;
        }break;

        case 30678:  // rank 2
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 39805;//proc something (we will owerride this)
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 8;
        }break;

        case 30679: // rank 3
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 39805;//proc something (we will owerride this)
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 12;
        }break;

        case 30680: // rank 4
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 39805;//proc something (we will owerride this)
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 16;
        }break;

        case 30681: // rank 5
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 39805;//proc something (we will owerride this)
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 20;
        }break;

        //shaman - Purge
        case 370:
        case 8012:
        case 27626:
        case 33625:
        {
            sp->DispelType = DISPEL_MAGIC;
        }break;

        case 57663: // Totem of Wrath
        {
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_RAID_AREA_AURA;
        }break;

        //Shaman - Eye of   the Storm
        case 29062:
        case 29064:
        case 29065:
        {
            sp->procFlags = PROC_ON_CRIT_HIT_RECEIVED;
        }break;

        //shaman - Elemental Devastation
        case 29179:
        case 29180:
        case 30160:
            //shaman - Ancestral healing
        case 16176:
        case 16235:
        case 16240:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        //shaman - Ancestral healing proc   spell
        case 16177:
        case 16236:
        case 16237:
        {
            sp->rangeIndex = 4;
        }break;

        case 1856: // Vanish r1
        case 1857: // Vanish r2
        case 11305: // Sprint
        case 14183: // Premeditation
        case 14185: // Preparation
        case 26889: // Vanish r3
        case 31665: // Master of Sub
        case 36554: // Shadow Step
        case 46784: // Shadowsong Panther
        case 51724: // Sap
        case 58427: // Overkill
        {
            sp->AttributesEx |= ATTRIBUTESEX_NOT_BREAK_STEALTH;
        }break;

        //rogue -   Find Weakness.
        case 31233:
        case 31239:
        case 31240:
        case 31241:
        case 31242:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //rogue -   Cheap   Shot
        case 1833:
        {
            sp->ATTRIBUTESEX2 &= ~ATTRIBUTESEX2_REQ_BEHIND_TARGET;  //Doesn't   require to be   behind target
            //sp->ATTRIBUTESEX3 &= ATTRIBUTESEX3_ONLY_TARGET_PLAYERS;  //Doesn't   require to be   behind target
        }break;

        //rogue -   Camouflage.
        case 13975:
        case 14062:
        case 14063:
        case 14064:
        case 14065:
        {
            sp->EffectMiscValue[0] = SMT_MISC_EFFECT;
        }break;

        //rogue -   Mace Specialization.
        case 13709:
        case 13800:
        case 13801:
        case 13802:
        case 13803:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;

        //rogue - Shadowstep
        case 36563:
        {
            sp->EffectMiscValue[1] = SMT_MISC_EFFECT;
            sp->procFlags = 0;
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
            sp->AttributesEx |= ATTRIBUTESEX_NOT_BREAK_STEALTH;
        }break;

        case 44373:
        {
            sp->procFlags = 0;
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
            sp->AttributesEx |= ATTRIBUTESEX_NOT_BREAK_STEALTH;
        }break;

        //Rogue  - Ruthlessness
        case 14156:
        case 14160:
        case 14161:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;//proc   spell
            sp->EffectTriggerSpell[0] = 14157;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        //priest    -   Holy Concentration
        case 34753:
        case 34859:
        case 34860:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 34754://Make it similar to Mage Clearcasting
        {
            sp->procCharges = 2;
            sp->ATTRIBUTESEX2 = 4;
            sp->ATTRIBUTESEX3 = 1073741824;
            sp->procFlags = 87376;
        }break;

        //guardian spirit
        case 47788:
        {
            sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[2] = SPELL_AURA_DUMMY;
            sp->AuraInterruptFlags = 0;
        }break;

        case 48153:
        {
            sp->EffectBasePoints[0] = 50;
        }break;

        //Priest:   Blessed Recovery
        case 27811:
        {
            sp->EffectTriggerSpell[0] = 27813;
            sp->procFlags = PROC_ON_CRIT_HIT_RECEIVED;
        }break;

        case 27815:
        {
            sp->EffectTriggerSpell[0] = 27817;
            sp->procFlags = PROC_ON_CRIT_HIT_RECEIVED;
        }break;

        case 27816:
        {
            sp->EffectTriggerSpell[0] = 27818;
            sp->procFlags = PROC_ON_CRIT_HIT_RECEIVED;
        }break;

        //priest-   Blessed Resilience
        case 33142:
        case 33145:
        case 33146:
            //priest-   Focused Will
        case 45234:
        case 45243:
        case 45244:
        {
            sp->procFlags = PROC_ON_CRIT_HIT_RECEIVED;
        }break;
        //Priest:   Improved Vampiric Embrace (Rank1)
        case 27839:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_ADD_PCT_MODIFIER;
        }break;

        case 20154: // Seal of Righteousness
        case 21084: // Seal of Righteousness
        {
            sp->Effect[0] = sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = sp->EffectApplyAuraName[2] = SPELL_AURA_DUMMY;
            sp->spell_cannot_be_resist = true;
        }break;
        case 20164: // Seal of Justice
        case 20165: // Seal of Light
        case 20166: // Seal of Wisdom
        case 31801: // Seal of Vengeance
        case 53736: // Seal of Corruption
            //LEGACY - These aren't ingame anymore... or are they?
        case 38008: // Seal of Blood(30s)
        case 31892: // Seal of Blood(30m)
        case 53720: // Seal of the Martyr
        {
            sp->Effect[0] = sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = sp->EffectApplyAuraName[2] = SPELL_AURA_DUMMY;
        }break;
        case 20375: // Seal of Command
        {
            sp->ProcsPerMinute = 7;
            sp->Effect[0] = sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = sp->EffectApplyAuraName[2] = SPELL_AURA_DUMMY;
        }break;
        case 57770:
        {
            sp->rangeIndex = 1;
        }break;

        //paladin - hammer of the righteous
        case 53595:
        {
            sp->speed = 0;
            sp->is_melee_spell = true;
        }break;

        //paladin   -   Spiritual   Attunement
        case 31785:
        case 33776:
        {
            sp->procFlags = PROC_ON_SPELL_LAND_RECEIVED;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 31786;
        }break;

        //fix   for the right   Enchant ID for Enchant Cloak - Major Resistance
        case 27962:
        case 36285:
        {
            sp->EffectMiscValue[0] = 2998;
        }break;

        //muhaha,   rewriting   Retaliation spell   as old one :D
        case 20230:
        {
            sp->Effect[0] = 6; //aura
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 22858; //evil   ,   but this is good for us :D
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED; //add procflag here since this was not processed   with the others !
        }break;

        //"bloodthirst" new version is ok   but old version is wrong from   now on :(
        case 23881:
        {
            sp->Effect[1] = 64; //cast on   us, it is   good
            sp->EffectTriggerSpell[1] = 23885; //evil   ,   but this is good for us :D
        }break;

        case 23892:
        {
            sp->Effect[1] = 64;
            sp->EffectTriggerSpell[1] = 23886; //evil   ,   but this is good for us :D
        }break;

        case 23893:
        {
            sp->Effect[1] = 64; //
            sp->EffectTriggerSpell[1] = 23887; //evil   ,   but this is good for us :D
        }break;

        case 23894:
        {
            sp->Effect[1] = 64; //
            sp->EffectTriggerSpell[1] = 23888; //evil   ,   but this is good for us :D
        }break;

        case 25251:
        {
            sp->Effect[1] = 64; //aura
            sp->EffectTriggerSpell[1] = 25252; //evil   ,   but this is good for us :D
        }break;

        case 30335:
        {
            sp->Effect[1] = 64; //aura
            sp->EffectTriggerSpell[1] = 30339; //evil   ,   but this is good for us :D
        }break;

        // Hunter   -   Master Tactician
        case 34506:
        case 34507:
        case 34508:
        case 34838:
        case 34839:
        {
            sp->procFlags = PROC_ON_RANGED_ATTACK;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        // Hunter   -   T.N.T
        case 56333:
        case 56336:
        case 56337:
        {
            sp->procFlags = PROC_ON_TRAP_TRIGGER | PROC_ON_CAST_SPECIFIC_SPELL | PROC_ON_CRIT_ATTACK | PROC_ON_PHYSICAL_ATTACK | PROC_ON_RANGED_ATTACK;
            sp->EffectSpellClassMask[0][0] = 0x00000004;
            sp->EffectSpellClassMask[0][1] = sp->EffectSpellClassMask[1][1];
            sp->EffectSpellClassMask[1][0] = 0x0;
        }break;

        // Hunter   -   Spirit Bond
        case 19578:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_PET_OWNER | SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 19579;
        }break;

        case 20895:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_PET_OWNER | SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 24529;
        }break;

        case 19579:
        case 24529:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA; //we   should do   the same for player too as we   did for pet
            sp->EffectApplyAuraName[1] = sp->EffectApplyAuraName[0];
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PET;
            sp->EffectBasePoints[1] = sp->EffectBasePoints[0];
            sp->EffectAmplitude[1] = sp->EffectAmplitude[0];
            sp->EffectDieSides[1] = sp->EffectDieSides[0];
        }break;

        // Hunter   -   Animal Handler
        case 34453:
        case 34454:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectApplyAuraName[1] = SPELL_AURA_MOD_HIT_CHANCE;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PET;
        }break;

        // Hunter   -   Catlike Reflexes
        case 34462:
        case 34464:
        case 34465:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectApplyAuraName[1] = sp->EffectApplyAuraName[0];
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PET;
        }break;

        // Hunter   -   Serpent's   Swiftness
        case 34466:
        case 34467:
        case 34468:
        case 34469:
        case 34470:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectApplyAuraName[1] = SPELL_AURA_MOD_HASTE;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PET;
        }break;

        // Hunter   -   Ferocious   Inspiration
        case 34455:
        case 34459:
        case 34460:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
            sp->EffectTriggerSpell[0] = 34456;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT | PROC_TARGET_SELF;
            sp->Effect[1] = 0; //remove this
        }break;

        // Hunter   -   Focused Fire
        case 35029:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_PET_OWNER | SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 35060;
        }break;
        case 35030:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_PET_OWNER | SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 35061;
        }break;
        case 35060:
        case 35061:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_EXPIREING_WITH_PET;
        }break;

        // Hunter   -   Thick   Hide
        case 19609:
        case 19610:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_PET_OWNER;
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_RESISTANCE; //we do not support armor   rating for pets yet !
            sp->EffectBasePoints[0] *= 10; //just   give it a   little juice :P
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;

        // Hunter   -   Ferocity
        case 19612:
        case 19599:
        case 19600:
        case 19601:
        case 19602:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_CRIT_PERCENT;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;

        // Hunter   -   Bestial Swiftness
        case 19596:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_INCREASE_SPEED;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;

        // Hunter   -   Endurance   Training
        case 19583:
        case 19584:
        case 19585:
        case 19586:
        case 19587:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_PET_OWNER;
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;
        // Hunter - Thrill of the Hunt
        case 34497:
        case 34498:
        case 34499:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
            sp->procChance = sp->EffectBasePoints[0] + 1;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 34720;
        }break;

        // Hunter   -   Expose Weakness
        case 34500:
        case 34502:
        case 34503:
        {
            sp->procFlags = PROC_ON_RANGED_CRIT_ATTACK | PROC_ON_SPELL_CRIT_HIT;
        }break;
        // hunter - Wild Quiver
        case 53215:
        case 53216:
        case 53217:
        {
            sp->procFlags = PROC_ON_RANGED_ATTACK;
        }break;

        //Hunter - Frenzy
        case 19621:
        case 19622:
        case 19623:
        case 19624:
        case 19625:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 19615;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
            sp->procChance = sp->EffectBasePoints[0];
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->c_is_flags = SPELL_FLAG_IS_EXPIREING_WITH_PET | SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
        }break;

        case 56641:
        case 34120:
        case 49051:
        case 49052: //Steady Shot cast time fix
        {
            sp->CastingTimeIndex = 5; // Set 2 sec cast time
        }break;

        case 61846:
        case 61847: // Aspect of the Dragonhawk
        {   // need to copy Mod Dodge Percent aura from a separate spell
            CopyEffect(dbcSpell.LookupEntryForced(61848), 0, sp, 2);
        }break;

        //Hunter - Unleashed Fury
        case 19616:
        case 19617:
        case 19618:
        case 19619:
        case 19620:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_DAMAGE_PERCENT_DONE;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
            sp->EffectMiscValue[0] = 1; //tweekign melee dmg
            sp->c_is_flags |= SPELL_FLAG_IS_EXPIREING_WITH_PET | SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
        }break;

        //Hunter : Pathfinding
        case 19559:
        case 19560:
        {
            sp->EffectMiscValue[0] = SMT_MISC_EFFECT;
        }break;

        //Hunter : Rapid Killing - might need   to add honor trigger too here. I'm guessing you receive Xp too so   i'm avoiding double proc
        case 34948:
        case 34949:
        {
            sp->procFlags = PROC_ON_GAIN_EXPIERIENCE;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        // Winter's chill
        case 12579:
        {
            sp->c_is_flags |= SPELL_FLAG_CANNOT_PROC_ON_SELF;
        }break;

        //Mage - Icy Veins
        case 12472:
        {
            sp->EffectMiscValue[1] = SMT_TRIGGER;
        }break;

        //Mage - Ignite
        case 11119:
        case 11120:
        case 12846:
        case 12847:
        case 12848:
        {
            sp->Effect[0] = 6; //aura
            sp->EffectApplyAuraName[0] = 42; //force him to use procspell effect
            sp->EffectTriggerSpell[0] = 12654;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        //Mage - Wand   Specialization. Not the forst   thing   we messed   up. Blizz   uses attack as magic and wandds as weapons :S
        case 6057:
        case 6085:
            //Priest - Wand Specialization
        case 14524:
        case 14525:
        case 14526:
        case 14527:
        case 14528:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_ADD_PCT_MODIFIER;
            sp->EffectMiscValue[0] = SMT_MISC_EFFECT;
        }break;

        //Mage - Elemental Precision
        case 29438:
        case 29439:
        case 29440:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_ADD_PCT_MODIFIER;
            sp->EffectMiscValue[0] = SMT_COST;
        }break;

        //Mage - Arcane Concentration
        case 11213:
        case 12574:
        case 12575:
        case 12576:
        case 12577:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        //Mage - ClearCasting   Effect
        //Druid - Clearcasting Effect
        //Druid - Nature's Grace effect
        case 16870:
        case 12536:
        case 16886:
        {
            sp->procCharges = 2;
        }break;

        //Mage - Arcane Blast
        case 36032:
        {
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
            sp->c_is_flags = SPELL_FLAG_IS_FORCEDDEBUFF;
            sp->maxstack = 4;
            sp->aura_remove_flags = CUSTOM_AURA_REMOVE_SPECIAL;
        }break;

        //rogue - Prey on the weak
        case 51685:
        case 51686:
        case 51687:
        case 51688:
        case 51689:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
        }break;

        //rogue -   Seal Fate
        case 14186:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 20;
        }break;
        case 14190:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 40;
        }break;
        case 14193:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 60;
        }break;
        case 14194:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 80;
        }break;
        case 14195:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 100;
        }break;

        // druid - Nature's Grace
        case 16880:
        case 61345:
        case 61346:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        //druid -   Blood   Frenzy
        case 16954:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 100;
        }break;
        case 16952:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 50;
        }break;

        //druid -   Primal Fury
        case 16961:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 100;
        }break;
        case 16958:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 50;
        }break;

        //druid -   Intensity
        case 17106:
        case 17107:
        case 17108:
        {
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        // Druid - Nurturing Instinct
        case 33872:
        {
            sp->AdditionalAura = 47179;
        }break;
        case 33873:
        {
            sp->AdditionalAura = 47180;
        }break;

        //Improved Sprint
        case 13743:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 50;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
        }break;
        case 13875:
        {
            sp->procChance = 100;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
        }break;

        //Warlock - healthstones
        case 6201:
        case 6202:
        case 5699:
        case 11729:
        case 11730:
        case 27230:
        case 47871:
        case 47878:
        {
            sp->Reagent[1] = 0;
            sp->ReagentCount[1] = 0;
        }break;
        //warlock   -    Seed   of Corruption
        case 27243:
        {
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[1] = 27285;
            sp->procFlags = PROC_ON_SPELL_HIT_RECEIVED | PROC_ON_DIE;
            sp->procChance = 100;
        }break;
        //warlock - soul link
        case 19028:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_SPLIT_DAMAGE;
            sp->EffectBasePoints[0] = 20;
            sp->EffectMiscValue[0] = 127;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
            sp->c_is_flags |= SPELL_FLAG_IS_EXPIREING_WITH_PET;

            sp->Effect[1] = 6;
            sp->EffectApplyAuraName[1] = 79;
            sp->EffectBasePoints[1] = 4;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_SELF;
            sp->EffectImplicitTargetB[1] = EFF_TARGET_PET;
            sp->c_is_flags |= SPELL_FLAG_IS_EXPIREING_WITH_PET;
        }break;

        //warlock: Nightfall
        case 18094:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 17941;
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION;
            sp->procChance = 2;
            sp->EffectSpellClassMask[0][0] = 0x0;
        }break;
        case 18095:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 17941;
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION;
            sp->procChance = 4;
            sp->EffectSpellClassMask[0][0] = 0x0;
        }break;

        //Warlock - Backdraft
        case 47258:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 54274;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 100;
            sp->procCharges = 4;
            sp->EffectSpellClassMask[0][0] = 0x00800000;
            sp->EffectSpellClassMask[0][1] = 0x00800000;
        }break;
        case 47269:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 54276;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 100;
            sp->procCharges = 4;
            sp->EffectSpellClassMask[0][0] = 0x00800000;
            sp->EffectSpellClassMask[0][1] = 0x00800000;
        }break;
        case 47260:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 54277;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 100;
            sp->procCharges = 4;
            sp->EffectSpellClassMask[0][0] = 0x00800000;
            sp->EffectSpellClassMask[0][1] = 0x00800000;
        }break;
        //Warlock - Eradication
        case 47195:
        case 47196:
        case 47197:
        {
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION;
            sp->EffectSpellClassMask[0][0] = 2;
            sp->proc_interval = 30000;
        }break;

        //warlock: Improved Enslave Demon
        case 18821:
        {
            sp->EffectMiscValue[0] = SMT_SPELL_VALUE_PCT;
            sp->EffectBasePoints[0] = -(sp->EffectBasePoints[0] + 2);
        }break;
        //Mage - Improved   Blizzard
        case 11185:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 12484;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 12487:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 12485;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 12488:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 12486;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //mage - Combustion
        case 11129:
        {
            sp->procFlags = PROC_ON_CAST_SPELL | PROC_ON_SPELL_CRIT_HIT | PROC_TARGET_SELF;
            sp->procCharges = 0;
            sp->c_is_flags |= SPELL_FLAG_IS_REQUIRECOOLDOWNUPDATE;
        }break;

        //mage - Master of Elements
        case 29074:
        case 29075:
        case 29076:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[1] = 29077;
            sp->EffectImplicitTargetA[1] = 1;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
            sp->procChance = 100;
        }break;

        //mage: Blazing Speed
        case 31641:
        case 31642:
        {
            sp->EffectTriggerSpell[0] = 31643;
        }break;

        //mage talent   "frostbyte". we make it to be   dummy
        case 11071:
        case 12496:
        case 12497:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 12494;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = sp->EffectBasePoints[0];
        }break;

        //rogue-shiv -> add 1   combo   point
        case 5938:
        {
            sp->Effect[1] = 80;
            sp->ATTRIBUTESEX2 &= ~ATTRIBUTESEX2_REQ_BEHIND_TARGET;  //Doesn't   require to be   behind target
            sp->is_melee_spell = true;
        }break;

        //warlock   -   Demonic Sacrifice
        case 18789:
        case 18790:
        case 18791:
        case 18792:
        case 35701:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_EXPIREING_ON_PET;
        }break;


        //warlock   -   Demonic Tactics
        case 30242:
        {
            sp->Effect[0] = 0; //disble this.   This is just blizz crap. Pure   proove that they suck   :P
            sp->EffectImplicitTargetB[1] = EFF_TARGET_PET;
            sp->EffectApplyAuraName[2] = SPELL_AURA_MOD_SPELL_CRIT_CHANCE; //lvl 1 has it   fucked up   :O
            sp->EffectImplicitTargetB[2] = EFF_TARGET_PET;
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_PET_OWNER;
        }break;
        case 30245:
        case 30246:
        case 30247:
        case 30248:
        {
            sp->Effect[0] = 0; //disble this.   This is just blizz crap. Pure   proove that they suck   :P
            sp->EffectImplicitTargetB[1] = EFF_TARGET_PET;
            sp->EffectImplicitTargetB[2] = EFF_TARGET_PET;
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_PET_OWNER;
        }break;

        //warlock   -   Demonic Resilience
        case 30319:
        case 30320:
        case 30321:
        {
            sp->EffectApplyAuraName[1] = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PET;
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_PET_OWNER;
        }break;

        //warlock   -   Improved Imp
        case 18694:
        case 18695:
        case 18696:
        case 18705:
        case 18706:
        case 18707:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;

        //warlock   -   Improved Succubus
        case 18754:
        case 18755:
        case 18756:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PET;
        }break;

        //warlock   -   Fel Intellect
        case 18731:
        case 18743:
        case 18744:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_PERCENT_STAT;
            sp->EffectMiscValue[0] = 3;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;

        //warlock   -   Fel Stamina
        case 18748:
        case 18749:
        case 18750:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_PERCENT_STAT;
            sp->EffectMiscValue[0] = 2;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;

        //warlock   -   Unholy Power
        case 18769:
        case 18770:
        case 18771:
        case 18772:
        case 18773:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET;
            sp->EffectApplyAuraName[0] = SPELL_AURA_ADD_PCT_MODIFIER;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
            //this is   required since blizz uses   spells for melee attacks while we   use fixed   functions
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_MOD_DAMAGE_PERCENT_DONE;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PET;
            sp->EffectMiscValue[1] = SCHOOL_NORMAL;
            sp->EffectBasePoints[1] = sp->EffectBasePoints[0];
        }break;

        //warlock   -   Master Demonologist -   25 spells   here
        case 23785:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET | SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 23784;
        }break;
        case 23822:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET | SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 23830;
        }break;
        case 23823:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET | SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 23831;
        }break;
        case 23824:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET | SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 23832;
        }break;
        case 23825:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_CASTED_ON_PET_SUMMON_ON_PET | SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 35708;
        }break;
        //and   the rest
        case 23784:
        case 23830:
        case 23831:
        case 23832:
        case 35708:
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;

        case 23759:
        case 23760:
        case 23761:
        case 23762:
        case 23826:
        case 23827:
        case 23828:
        case 23829:
        case 23833:
        case 23834:
        case 23835:
        case 23836:
        case 23837:
        case 23838:
        case 23839:
        case 23840:
        case 23841:
        case 23842:
        case 23843:
        case 23844:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
        }break;

        case 35702:
        case 35703:
        case 35704:
        case 35705:
        case 35706:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_EXPIREING_WITH_PET;
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
        }break;
        //warlock   -   Improved Healthstone
        case 18692:
        case 18693:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_ADD_PCT_MODIFIER;
            sp->EffectMiscValue[0] = SMT_MISC_EFFECT;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_SELF;
        }break;
        //warlock   -   Improved Drain Soul
        case 18213:
        case 18372:
        {
            sp->procFlags = PROC_ON_TARGET_DIE;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
            sp->procChance = 100;
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 18371;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_SELF;
            sp->Effect[2] = 0;   //remove this   effect
        }break;

        //warlock   -   Shadow Embrace
        case 32385:
        case 32387:
        case 32392:
        case 32393:
        case 32394:
        {
            sp->procChance = 100;
            sp->procFlags = PROC_ON_SPELL_HIT;
            sp->Effect[1] = 0;  //remove this   effect ? Maybe remove   the other   one :P xD
        }break;
        //warlock - soul leech
        case 30293:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 30294;
            sp->procChance = 10;
            sp->procFlags = PROC_ON_SPELL_HIT;
        }break;
        case 30295:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 30294;
            sp->procChance = 20;
            sp->procFlags = PROC_ON_SPELL_HIT;
        }break;
        case 30296:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 30294;
            sp->procChance = 30;
            sp->procFlags = PROC_ON_SPELL_HIT;
        }break;
        //warlock   -   Pyroclasm
        case 18073:
        case 18096:
        case 63245:
        {
            sp->Effect[0] = 0; //delete this owerride   effect :P
            sp->EffectTriggerSpell[1] = 18093; //trigger spell was wrong :P
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION;
            sp->procChance = 26; //god, save us from fixed values   !
        }break;

        //Mage - Improved Scorch
        case 11095:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 33;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 12872:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 66;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 12873:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //Nature's Grasp
        case 16689:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 339;
            sp->Effect[1] = 0;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED | PROC_REMOVEONUSE;
            sp->AuraInterruptFlags = 0; //we remove it on   proc or timeout
            sp->procChance = 100;
        }break;
        case 16810:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 1062;
            sp->Effect[1] = 0;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED | PROC_REMOVEONUSE;
            sp->AuraInterruptFlags = 0; //we remove it on   proc or timeout
            sp->procChance = 100;
        }break;
        case 16811:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 5195;
            sp->Effect[1] = 0;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED | PROC_REMOVEONUSE;
            sp->AuraInterruptFlags = 0; //we remove it on   proc or timeout
            sp->procChance = 100;
        }break;
        case 16812:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 5196;
            sp->Effect[1] = 0;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED | PROC_REMOVEONUSE;
            sp->AuraInterruptFlags = 0; //we remove it on   proc or timeout
            sp->procChance = 100;
        }break;
        case 16813:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 9852;
            sp->Effect[1] = 0;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED | PROC_REMOVEONUSE;
            sp->AuraInterruptFlags = 0; //we remove it on   proc or timeout
            sp->procChance = 100;
        }break;
        case 17329:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 9853;
            sp->Effect[1] = 0;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED | PROC_REMOVEONUSE;
            sp->AuraInterruptFlags = 0; //we remove it on   proc or timeout
            sp->procChance = 100;
        }break;
        case 27009:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 26989;
            sp->Effect[1] = 0;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED | PROC_REMOVEONUSE;
            sp->AuraInterruptFlags = 0; //we remove it on   proc or timeout
            sp->procChance = 100;
        }break;

        //wrath of air totem targets sorounding creatures   instead of us
        case 2895:
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_SELF;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_SELF;
            sp->EffectImplicitTargetA[2] = 0;
            sp->EffectImplicitTargetB[0] = 0;
            sp->EffectImplicitTargetB[1] = 0;
            sp->EffectImplicitTargetB[2] = 0;
        }break;
        //Druid: Natural Shapeshifter
        case 16833:
        case 16834:
        case 16835:
        {
            sp->DurationIndex = 0;
        }break;


        //Priest - Inspiration proc spell
        case 14893:
        case 15357:
        case 15359:
        {
            sp->rangeIndex = 4;
        }break;

        //Relentless Strikes
        case 14179:
        case 58422:
        case 58423:
        case 58424:
        case 58425:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;//proc    spell
            sp->EffectTriggerSpell[0] = 14181;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
            sp->procChance = 100;
        }break;

        //priest - surge of light
        case 33150:
        case 33154:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        case 33151:
        {
            sp->procCharges = 2;
        }break;

        //Seal of   Justice -lowered proc   chance (experimental values !)
        case 31895:
        {
            sp->procChance = 20;
            sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[2] = SPELL_AURA_DUMMY;
        }break;
        // judgement of command shit
        case 20425:
        {
            sp->Effect[1] = 2;
            sp->EffectImplicitTargetA[1] = 6;
        }break;
        //Berserking
        case 26635:
        {
            sp->Attributes = 327680;
            sp->CasterAuraState = 0;
        }break;
        //Escape Artist
        case 20589:
        {
            sp->Attributes = 65552;
            sp->ATTRIBUTESEX2 = 0;
            sp->Effect[0] = 108;
            sp->Effect[1] = 108;
            sp->EffectDieSides[0] = 1;
            sp->EffectDieSides[1] = 1;
            sp->EffectBasePoints[0] = 9;
            sp->EffectBasePoints[1] = 9;
            sp->EffectMiscValue[0] = 7;
            sp->EffectMiscValue[1] = 11;
        }break;
        //Zapthrottle Mote Extractor
        case 30427:
        {
            sp->Effect[0] = SPELL_EFFECT_DUMMY;
        }break;
        //Goblin Jumper Cable
        case 22999:
        case 8342:
        case 54732:
        {
            sp->Effect[0] = 113;
            sp->EffectMiscValue[0] = 120;
        }break;
        //rogue -   intiative
        case 13976:
        case 13979:
        case 13980:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        //this an   on equip item   spell(2824) :    ice arrow(29501)
        case 29501:
        {
            sp->procChance = 30;//some say it   is triggered every now and then
            sp->procFlags = PROC_ON_RANGED_ATTACK;
        }break;

        //deep wounds
        case 12834:
        case 12849:
        case 12867:
        {
            sp->EffectTriggerSpell[0] = 12868;
            sp->procFlags = PROC_ON_CRIT_ATTACK | PROC_ON_RANGED_CRIT_ATTACK | PROC_ON_SPELL_CRIT_HIT;
        }break;

        //warrior   -   second wind should trigger on   self
        case 29841:
        case 29842:
        {
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        // Improved Revenge
        case 12797:
        case 12799:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //warrior   -   berserker   rage is missing 1   effect = regenerate rage
        case 18499:
        {
            sp->Effect[2] = 6;
            sp->EffectApplyAuraName[2] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[2] = 37521; //not sure   if this is the one. In my   time this   adds 30 rage
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        //warrior   -   improved berserker rage
        case 20500:
        case 20501:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;

        // warrior - Spell Reflection
        case 23920:
        {
            sp->procFlags = PROC_NULL; //   No need to proc
        }break;


        // warrior - Titan's Grip
        case 46917: // main spell
        {
            SpellEntry * sp2 = dbcSpell.LookupEntryForced(49152);  // move this aura   into main   spell
            if (sp != NULL)
            {
                sp->Effect[1] = sp2->Effect[0];
                sp->EffectApplyAuraName[1] = sp2->EffectApplyAuraName[0];
                sp->EffectMiscValue[1] = sp2->EffectMiscValue[0];
                sp->EffectSpellClassMask[1][0] = sp2->EffectSpellClassMask[0][0];
                sp->EffectSpellClassMask[1][1] = sp2->EffectSpellClassMask[0][1];
                sp->EffectSpellClassMask[1][2] = sp2->EffectSpellClassMask[0][2];
                sp->EffectBasePoints[1] = sp2->EffectBasePoints[0];
                sp->EffectDieSides[1] = sp2->EffectDieSides[0];
            }
        }break;

        // Charge   -   Changing from   dummy   for the power   regen
        case 100:
        case 6178:
        case 11578:
        {
            sp->Effect[1] = SPELL_EFFECT_ENERGIZE;
            sp->EffectMiscValue[1] = 1;
        }break;

        //warrior   -   Rampage
        case 30030:
        case 30033:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
            sp->EffectTriggerSpell[0] = sp->EffectTriggerSpell[1];
            //          sp->Unique = true; // Crow: It should be, but what if we have 2 warriors?
        }break;

        // warrior - overpower
        case 7384:
        case 7887:
        case 11584:
        case 11585:
        {
            sp->Attributes |= ATTRIBUTES_CANT_BE_DPB;
        }break;

        // warrior - heroic fury
        case 60970:
        {
            sp->Effect[1] = SPELL_EFFECT_DUMMY;
        }break;

        case 20608: //Reincarnation
        {
            for (uint32 i = 0; i < 8; ++i)
            {
                if (sp->Reagent[i])
                {
                    sp->Reagent[i] = 0;
                    sp->ReagentCount[i] = 0;
                }
            }
        }break;

        // druid - travel   form
        case 5419:
        {
            sp->Attributes |= ATTRIBUTES_MOUNT_CASTABLE;
        }break;

        // druid - Naturalist
        case 17069:
        case 17070:
        case 17071:
        case 17072:
        case 17073:
        {
            sp->EffectApplyAuraName[1] = SPELL_AURA_MOD_DAMAGE_PERCENT_DONE;
            sp->EffectMiscValue[1] = 1;
        }break;

        // Druid: Omen of Clarity
        case 16864:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_CAST_SPELL;
            sp->procChance = 6;
            sp->proc_interval = 10000;
        }break;

        //Serpent-Coil Braid
        case 37447:
        case 61062://Heroes' Frostfire Garb AND Valorous Frostfire Garb
        {
            sp->Effect[0] = 6;
            sp->Effect[1] = 6;
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectTriggerSpell[1] = 37445;
            sp->maxstack = 1;
        }break;

        // Mark of Conquest
        case 33510:
        {
            sp->procChance = 7;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;

        //Paladin   -   Improved Lay on Hands
        case 20234:
        case 20235:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        // Sudden   Death
        case 29724:
        case 29725:
        case 29723:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;

        // Taste for Blood
        case 56638:
        case 56637:
        case 56636:
        {
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION;
            sp->EffectSpellClassMask[0][0] = 0x0;
            sp->EffectSpellClassMask[1][0] = 0x0;
        }break;

        // Flametongue weapon
        /*case 58792:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;
        case 58791:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;
        case 58784:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;
        case 16313:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;
        case 16312:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;
        case 16311:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;
        case 15569:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;
        case 15568:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;
        case 15567:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;
        case 10400:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 10444;
            sp->procChance = 100;
        }break;*/

        //windfury weapon
        case 33757:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectTriggerSpell[0] = 25504;
            sp->procChance = 20;
            sp->proc_interval = 3000;
            sp->maxstack = 1;
        }break;

        // dot heals
        case 38394:
        {
            sp->procFlags = 1024;
            //sp->SpellGroupType = 6;
        }break;

        case 16972:
        case 16974:
        case 16975:
        {   //fix   for Predatory   Strikes
            sp->RequiredShapeShift = (1 << (FORM_BEAR - 1)) | (1 << (FORM_DIREBEAR - 1)) | (1 << (FORM_CAT - 1));
        }break;

        case 20134:
        {
            sp->procChance = 50;
        }break;

        /* aspect   of the pack -   change to   AA */
        case 13159:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_GROUP_AREA_AURA;
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
        }break;

        /* aspect   of the cheetah - add proc   flags   */
        case 5118:
        {
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;;
        }break;


        case SPELL_RANGED_GENERAL:
        case SPELL_RANGED_THROW:
        case 26679:
        case 29436:
        case 37074:
        case 41182:
        case 41346:
        {
            if (sp->RecoveryTime == 0 && sp->StartRecoveryTime == 0)
                sp->RecoveryTime = 1600;
        }break;

        case SPELL_RANGED_WAND:
        {
            sp->Spell_Dmg_Type = SPELL_DMG_TYPE_RANGED;
            if (sp->RecoveryTime == 0 && sp->StartRecoveryTime == 0)
                sp->RecoveryTime = 1600;
        }break;

        /**********************************************************
        *   Misc stuff (NPC SPELLS)
        **********************************************************/

        case 5106:
        {
            sp->AuraInterruptFlags |= AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN;
        }break;

        /**********************************************************
        *   Misc stuff (QUEST)
        **********************************************************/
        // Bat Net
        case 52151:
        {
            sp->EffectImplicitTargetA[0] = 6;
        }break;

        // queststuff
        case 30877:
        {
            sp->EffectImplicitTargetB[0] = 0;
        }break;

        // quest stuff
        case 23179:
        {
            sp->EffectMiscValue[0] = 1434;
        }break;

        // arcane charges
        case 45072:
        {
            sp->Attributes |= ATTRIBUTES_MOUNT_CASTABLE;
        }break;

        case 48252: //Prototype Neural Needle
        case 45634: //Neural Needle
        {
            sp->Effect[1] = 0;
        }break;
        //Tuber whistle
        case 36652:
        {
            sp->Effect[1] = 3;
        }break;
        //Cure Caribou Hide
        case 46387:
        {
            sp->ReagentCount[0] = 1;
            sp->ReagentCount[1] = 0;
            sp->Reagent[1] = 0;
        }break;
        //Create Totem of Issliruk
        case 46816:
        {
            sp->Reagent[0] = 0;
            sp->ReagentCount[0] = 0;
        }break;
        /**********************************************************
        *   Misc stuff (ITEMS)
        **********************************************************/

        // Nitro Boosts
        case 55004:
        {
            CopyEffect(dbcSpell.LookupEntryForced(54861), 0, sp, 2);
            sp->DurationIndex = 39;
        }break;

        // Gnomish Lightning Generator
        case 55039:
        {
            sp->InterruptFlags = 0;
            sp->AuraInterruptFlags = 0;
            sp->ChannelInterruptFlags = 0;
        }break;

        // Libram of Divinity
        case 28853:
            // Libram of Light
        case 28851:
            // Blessed Book of Nagrand
        case 32403:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_ADD_FLAT_MODIFIER;
            sp->EffectMiscValue[0] = 0;
            sp->EffectSpellClassMask[0][0] = 1073741824;
        }break;
        // Libram of Mending
        case 43741:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 100;
            sp->EffectSpellClassMask[0][0] = 0x80000000;
            sp->EffectSpellClassMask[0][1] = 0x0;
            sp->EffectSpellClassMask[0][2] = 0x0;
            sp->EffectSpellClassMask[1][0] = 0x0;
            sp->EffectSpellClassMask[1][1] = 0x0;
            sp->EffectSpellClassMask[1][2] = 0x0;
            sp->EffectSpellClassMask[2][0] = 0x0;
            sp->EffectSpellClassMask[2][1] = 0x0;
            sp->EffectSpellClassMask[2][2] = 0x0;
        }break;

        // Druid Trinket Living Root of the Wildheart
        case 37336:
        {
            sp->Effect[0] = SPELL_EFFECT_DUMMY;// oh noez, we   make it DUMMY, no   other   way to fix it   atm
        }break;

        // Palla trinket
        case 43745:
        case 34258:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 8388608 | 520;
            sp->procChance = 100;
        }break;

        // Lesser Rune of Warding / Grater Rune of Warding
        case 42135:
        case 42136:
        {
            sp->EffectImplicitTargetA[1] = 6;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED;
        }break;

        //Idol of terror
        case 43737:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        // Lesser   Heroism (Tenacious Earthstorm   Diamond)
        case 32844:
        {
            sp->procChance = 5;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;

        // Darkmoon Card:   Heroism
        case 23689:
        {
            sp->ProcsPerMinute = 2.5f;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;

        // Darkmoon Card:   Maelstrom
        case 23686:
        {
            sp->ProcsPerMinute = 2.5f;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;

        // dragonspine trophy
        case 34774:
        {
            sp->ProcsPerMinute = 1.5f;
            sp->proc_interval = 20000;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;

        // Romulo's Poison Vial
        case 34586:
        {
            sp->ProcsPerMinute = 1.6f;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;

        // madness of   the betrayer
        case 40475:
        {
            sp->procChance = 50;
            sp->ProcsPerMinute = 1.0f;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;

        // Band of the Eternal Defender
        case 35077:
        {
            sp->proc_interval = 55000;
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
        }break;
        // Band of the Eternal Champion
        case 35080:
        {
            sp->proc_interval = 55000;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;
        // Band of the Eternal Restorer
        case 35086:
            // Band of the Eternal Sage
        case 35083:
        {
            sp->proc_interval = 55000;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        // Sonic Booster
        case 54707:
        {
            sp->ProcsPerMinute = 1.0f;
            sp->proc_interval = 60000;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;
        case 60301: // Meteorite Whetstone
        case 60317: // Signet of Edward the Odd
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;
        // Bandit's Insignia
        case 60442:
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;
        // Fury of the Five Flights
        case 60313:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;
        // Grim Toll
        case 60436:
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;
        // Illustration of the Dragon Soul
        case 60485:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        // Soul of the Dead
        case 60537:
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        // Anvil of Titans
        case 62115:
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;
        // Embrace of the Spider
        case 60490:
            // Flow of Knowledge
        case 62114:
            // Forge Ember
        case 60473:
            // Mystical Skyfire Diamond
        case 32837:
            // Sundial of the Exiled
        case 60063:
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        // Majestic Dragon Figurine
        case 60524:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        // Mirror of Truth
        case 33648:
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_CRIT_ATTACK;
        }break;

        // Vestige of Haldor
        case 60306:
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;

        //Energized
        case 43750:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;


        //Spell Haste   Trinket
        case 33297:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
        }break;
        case 57345: // Darkmoon Card: Greatness
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->proc_interval = 45000;
        }break;

        case 67702: // Death's Choice/Verdict
        {
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
            sp->proc_interval = 45000;
        }break;

        case 67771: // Death's Choice/Verdict (heroic)
        {
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
            sp->proc_interval = 45000;
        }break;

        case 72413: //Ashen Bands
        {
            sp->procChance = 10;
        }break;

        // Swordguard Embroidery
        case 55776:
        {
            sp->proc_interval = 60000;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;
        // Chuchu's Tiny Box of Horrors
        case 61618:
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;

        case 57351: // Darkmoon Card: Berserker!
        {
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION | PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK | PROC_ON_ANY_DAMAGE_RECEIVED;; // when you strike, or are struck in combat
        }break;
        case 60196:
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_SELF;
        }break;
        // Darkmoon Card: Death
        case 57352:
        {
            sp->proc_interval = 45000;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK | PROC_ON_CAST_SPELL;
        }break;
        case 60493: //Dying Curse
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->proc_interval = 45000;
        }break;

        // Ashtongue Talisman   of Shadows
        case 40478:
        {
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION;
        }break;

        // Ashtongue Talisman   of Swiftness
        case 40485:
            // Ashtongue Talisman   of Valor
        case 40458:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        //Ashtongue Talisman of Zeal
        case 40470:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 50;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectTriggerSpell[0] = 40472;
            sp->maxstack = 1;
        }break;

        // Memento of   Tyrande
        case 37655:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 10;
            sp->proc_interval = 45000;
        }break;

        // Ashtongue Talisman   of Insight
        case 40482:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;


        //Ashtongue Talisman of Equilibrium
        case 40442:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 40;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectTriggerSpell[0] = 40452;
            sp->maxstack = 1;
            sp->Effect[1] = 6;
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 25;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectTriggerSpell[1] = 40445;
            sp->maxstack = 1;
            sp->Effect[2] = 6;
            sp->EffectApplyAuraName[2] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 25;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectTriggerSpell[2] = 40446;
            sp->maxstack = 1;
        }break;

        //Ashtongue Talisman of Acumen
        case 40438:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 10;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectTriggerSpell[0] = 40441;
            sp->maxstack = 1;
            sp->Effect[1] = 6;
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 10;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectTriggerSpell[1] = 40440;
            sp->maxstack = 1;
        }break;

        //Ashtongue Talisman of Lethality
        case 40460:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 20;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectTriggerSpell[0] = 40461;
            sp->maxstack = 1;
        }break;
        //Leatherworking Drums
        case 35475://Drums of War
        case 35476://Drums of Battle
        case 35478://Drums of Restoration
        case 35477://Drums of Speed
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_ALL_PARTY;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_ALL_PARTY;
            sp->EffectImplicitTargetA[2] = EFF_TARGET_NONE;
            sp->EffectImplicitTargetB[0] = EFF_TARGET_NONE;
            sp->EffectImplicitTargetB[1] = EFF_TARGET_NONE;
            sp->EffectImplicitTargetB[2] = EFF_TARGET_NONE;
        }break;

        case 46699:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_CONSUMES_NO_AMMO;
        }break;
        /**********************************************************
        *   Misc stuff (ITEM SETS)
        **********************************************************/

        //Item Set: Malorne Harness
        case 37306:
        case 37311:
            //Item Set: Deathmantle
        case 37170:
        {
            sp->procChance = 4;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;

        //Item Set: Netherblade
        case 37168:
        {
            sp->procChance = 15;
        }break;

        //Item Set: Tirisfal Regalia
        case 37443:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        //Item Set: Avatar Regalia
        case 37600:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 6;
        }break;

        //Item Set: Incarnate   Raiment
        case 37568:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //Item Set: Voidheart   Raiment
        case 37377:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 5;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->proc_interval = 20;
            sp->EffectTriggerSpell[0] = 37379;
        }break;
        case 39437:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 5;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->proc_interval = 20;
            sp->EffectTriggerSpell[0] = 37378;
        }break;

        //Item Set: Cataclysm   Raiment
        case 37227:
        {
            sp->proc_interval = 60000;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        //Item Set: Cataclysm   Regalia
        case 37228:
        {
            sp->procChance = 7;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 37237:
        {
            sp->procChance = 25;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        //Item Set: Cataclysm   Harness
        case 37239:
        {
            sp->procChance = 2;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;

        //Item Set: Cyclone Regalia
        case 37213:
        {
            sp->procChance = 11;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        //Item Set: Lightbringer Battlegear
        case 38427:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procChance = 20;
        }break;

        //Item Set: Crystalforge Battlegear
        case 37195:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 6;
        }break;

        //Item Set: Crystalforge Raiment
        case 37189:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
            sp->proc_interval = 60000;
        }break;

        case 37188:
            //Item Set: Crystalforge Armor
        case 37191:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //Item Set: Destroyer   Armor
        case 37525:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED;
            sp->procChance = 7;
        }break;

        //Item Set: Destroyer   Battlegear
        case 37528:
            //Item Set: Warbringer Armor
        case 37516:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 100;
        }break;

        /**********************************************************
        *   Misc stuff (GLYPH)
        **********************************************************/

        case 55680:// Glyph of Prayer of Healing
        case 56176:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 56161;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 58631: // Glyph of Icy Touch
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        // Glyph of reneved life
        case 58059:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;

        // Glyph of Vigor
        case 56805:
        {
            CopyEffect(dbcSpell.LookupEntryForced(21975), 0, sp, 2);
        }break;

        // Glyph of Lesser Healing Wave
        case 55438:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
        }break;

        // Glyph of Frostbolt
        case 56370:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;

        // Glyph of Revenge
        case 58364:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x0;
            sp->EffectSpellClassMask[0][1] = 0x0;
        }break;
        // Glyph of Revenge Proc
        case 58363:
        {
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
        }break;
        case 56218://Glyph of Corruption
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 17941;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 4;
            sp->EffectSpellClassMask[0][0] = 0x0;
        }break;

        //////////////////////////////////////////////////////
        // CLASS-SPECIFIC   SPELL   FIXES                           //
        //////////////////////////////////////////////////////

    /* Note: when   applying spell hackfixes,   please follow   a   template */

        //////////////////////////////////////////
        // WARRIOR                                  //
        //////////////////////////////////////////
        /*case 7405: //   Sunder Armor
        case 7386: //   Sunder Armor
        case 8380: //   Sunder Armor
        case 11596: // Sunder   Armor
        case 11597: // Sunder   Armor
        case 25225: // Sunder   Armor
        case 30330: // Mortal   Strike Ranks
        case 25248: // Mortal   Strike Ranks
        case 21553: // Mortal   Strike Ranks
        case 21552: // Mortal   Strike Ranks
        case 21551: // Mortal   Strike Ranks
        case 12294: // Mortal   Strike Ranks
        case 16511: // Hemo Rank 1
        case 17347: // Hemo Rank 2
        case 17348: // Hemo Rank 3
        case 26864: // Hemo Rank 4
        {
            sp->Unique = true;
        }break;*/
        // Wrecking Crew
        case 46867:
        case 56611:
        case 56612:
        case 56613:
        case 56614:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
        }break;
        // Enrage
        case 12317:
        case 13045:
        case 13046:
        case 13047:
        case 13048:
        {
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
        }break;
        // Improved Defensive Stance (Missing Parry Flag)
        case 29593:
        case 29594:
        {
            sp->procFlags = PROC_ON_BLOCK_RECEIVED | PROC_ON_DODGE_RECEIVED;
        }break;
        // Sword and Board
        case 46951:
        case 46952:
        case 46953:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        // Sword and Board Refresh
        case 50227:
        {
            sp->Effect[1] = 3;
        }break;
        // Sword specialization
        case 12281:
        case 13960:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procChance = 1;
        }break;
        case 12812:
        case 13961:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procChance = 2;
        }break;
        case 12813:
        case 13962:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procChance = 3;
        }break;
        case 12814:
        case 13963:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procChance = 4;
        }break;
        case 12815:
        case 13964:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procChance = 5;
        }break;
        // vigilance
        case 50720:
        {
            sp->MaxTargets = 1;
            //sp->Unique = true;
            sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[2] = SPELL_AURA_MOD_MAX_AFFECTED_TARGETS;
            sp->EffectImplicitTargetA[2] = 57;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED;
        }break;

        // Damage Shield
        case 58872:
        case 58874:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_BLOCK_RECEIVED;
            sp->procChance = 100;
            sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[2] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectImplicitTargetA[2] = 1;
            sp->EffectTriggerSpell[2] = 59653;
        }break;

        // Improved Hamstring
        case 12289:
        case 12668:
        case 23695:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 2;
        }break;

        // Whirlwind
        case 1680:
        {
            //sp->AllowBackAttack = true;
        }break;

        // Shockwave
        case 46968:
        {
            sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MELEE;
        }break;

        // Blood Craze
        case 16487:
        case 16489:
        case 16492:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT_RECEIVED | PROC_ON_CRIT_HIT_RECEIVED;
        }break;

        // Gag Order
        case 12311:
        case 12958:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        //////////////////////////////////////////
        // PALADIN                                  //
        //////////////////////////////////////////

        // Insert   paladin spell   fixes   here
        // Light's Grace PROC
        case 31834:
        {
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
        }break;

        // Seal of Command - Holy   damage, but melee   mechanics   (crit   damage, chance, etc)
        case 20424:
        {
            sp->rangeIndex = 4;
            sp->is_melee_spell = true;
            sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC;
            sp->School = SCHOOL_HOLY;
        }break;

        // Illumination
        case 20210:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 18350;
            sp->procChance = 20;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 20212:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 18350;
            sp->procChance = 40;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 20213:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 18350;
            sp->procChance = 60;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 20214:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 18350;
            sp->procChance = 80;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 20215:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 18350;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        // Heart of the Crusader rank 1
        case 20335:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 21183;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 8388608;
        }break;

        // Heart of the Crusader rank 2
        case 20336:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 54498;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 8388608;
        }break;

        // Heart of the Crusader rank 3
        case 20337:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 54499;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 8388608;
        }break;

        // Pursuit of Justice rank 1
        case 26022:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED;
            sp->EffectImplicitTargetA[1] = 1;
            sp->EffectBasePoints[1] = 8;
        }break;

        // Pursuit of Justice rank 2
        case 26023:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED;
            sp->EffectImplicitTargetA[1] = 1;
            sp->EffectBasePoints[1] = 15;
        }break;

        // Righteous Vengeance
        case 53380:
        case 53381:
        case 53382:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 61840;
            sp->procFlags = PROC_ON_CRIT_ATTACK | PROC_ON_SPELL_CRIT_HIT;
            sp->EffectSpellClassMask[0][0] = 0xE14BFF42;
        }break;

        // Sheat of Light (Hot Effect)
        case 53501:
        case 53502:
        case 53503:
        {
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[1] = 54203;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        // Sacred Shield
        case 53601:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
            sp->procFlags |= static_cast<uint32>(PROC_TARGET_SELF);
            sp->EffectTriggerSpell[0] = 58597;
            sp->proc_interval = 6000;
        }break;

        // SoC/SoV Dot's
        case 31803:
        case 53742:
        {
            sp->School = SCHOOL_HOLY;
            sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC;
            //sp->Unique = true;
        }break;

        // Judgement of righteousness
        case 20187:
            // Judgement of command
        case 20467:
            // Judgement of Justice/Wisdom/Light
        case 53733:
            // Judgement of Blood
        case 31898:
            // Judgement of Martyr
        case 53726:
            // Judgement of Corruption/Vengeance
        case 31804:
        {
            sp->School = SCHOOL_HOLY;
            sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC;
        }break;

        // Shield of Righteousness
        case 53600:
        case 61411:
        {
            sp->School = SCHOOL_HOLY;
            sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC;
        }break;

        // Beacon of the Light (Dummy Aura)
        case 53563:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
            sp->c_is_flags = SPELL_FLAG_IS_FORCEDBUFF;
        }break;

        //////////////////////////////////////////
        // HUNTER                               //
        //////////////////////////////////////////

        //Hunter - Go for the Throat
        case 34950:
        case 34954:
        {
            sp->procFlags = PROC_ON_RANGED_CRIT_ATTACK;
        }break;
        case 34952:
        case 34953:
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;

        //Ranged Weapon Specialization
        case 19507:
        case 19508:
        case 19509:
        case 19510:
        case 19511:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_DAMAGE_PERCENT_DONE;
            sp->EffectMiscValue[0] = 1;
        }break;
        //Two Handed Weapon Specialization and One Handed Weapon Specializacion
        //Warrior and Paladin and Death Knight - this will change on 3.1.0
        case 20111:
        case 20112:
        case 20113:
        case 12163:
        case 12711:
        case 12712:
        case 16538:
        case 16539:
        case 16540:
        case 16541:
        case 16542:
        case 20196:
        case 20197:
        case 20198:
        case 20199:
        case 20200:
        case 55107:
        case 55108:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_DAMAGE_PERCENT_DONE;
            sp->EffectMiscValue[0] = 2;
        }break;
        //Frost Trap
        case 13809:
        {
            sp->procFlags = PROC_ON_TRAP_TRIGGER;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;

        //Snakes from Snake Trap cast this, shouldn't stack
        case 25810:
        case 25809:
        {
            sp->maxstack = 1;
        }break;

        case 27065:
        case 20904:
        case 20903:
        case 20902:
        case 20901:
        case 20900:
        case 19434:
        {
            // sp->Unique = true;
        }break;

        //Hunter : Entrapment
        case 19184:
        case 19387:
        case 19388:
        {
            sp->procFlags = PROC_ON_TRAP_TRIGGER;
        }break;
        // Hunter - Arcane Shot  - Rank 6 to 11
        case 14285:
        case 14286:
        case 14287:
        case 27019:
        case 49044:
        case 49045:
        {
            sp->Effect[0] = 2;
            sp->Effect[1] = 0;
            sp->EffectBasePoints[0] = sp->EffectBasePoints[1];
            sp->EffectBasePoints[1] = 0;
            sp->EffectImplicitTargetA[0] = 6;
            sp->EffectImplicitTargetA[1] = 0;
        }break;

        // Misdirection
        case 34477:
        {
            sp->MaxTargets = 1;
            //sp->Unique = true;
            sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[2] = SPELL_AURA_MOD_MAX_AFFECTED_TARGETS;
        }break;

        // Misdirection proc
        case 35079:
        {
            sp->MaxTargets = 1;
            //sp->Unique = true;
        }break;
        // Aspect of the Viper
        case 34074:
        {
            sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[2] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[2] = 34075;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK;
        }break;
        // Viper String
        case 3034:
        {
            sp->EffectMultipleValue[0] = 3;
        }break;
        // Improved Steady Shot
        case 53221:
        case 53222:
        case 53224:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x0;
            sp->EffectSpellClassMask[0][1] = 0x0;
        }break;


        // Lock and Load
        case 56342:
        case 56343:
        case 56344:
        {
            sp->procFlags = PROC_ON_TRAP_TRIGGER;
            sp->procChance = sp->EffectBasePoints[0] + 1;
        }break;

        // Lock and load proc
        case 56453:
        {
            sp->DurationIndex = 9;
        }break;

        // Master's Call
        case 53271:
        {
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 54216;
            sp->Effect[1] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[1] = 54216;
        }break;

        // Haunting party
        case 53290:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 57669;
            sp->EffectImplicitTargetA[0] = 1;
            sp->procChance = 33;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 53291:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 57669;
            sp->EffectImplicitTargetA[0] = 1;
            sp->procChance = 66;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 53292:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 57669;
            sp->EffectImplicitTargetA[0] = 1;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        // Flare
        case 1543:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_FORCEDDEBUFF | SPELL_FLAG_IS_TARGETINGSTEALTHED;
        }break;


        //////////////////////////////////////////
        // ROGUE                                    //
        //////////////////////////////////////////

        // Cheat Death
        case 31228:
        case 31229:
        case 31230:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
        }break;

        // Cloack of shadows
        // Cloack of shadows PROC
        case 31224:
        case 35729:
        {
            sp->DispelType = DISPEL_MAGIC;
            sp->AttributesEx |= 1024;
        }break;

        // Honor Among Thieves PROC
        case 52916:
        {
            sp->proc_interval = 4000; //workaround
            sp->procFlags = PROC_ON_CRIT_ATTACK | PROC_ON_RANGED_CRIT_ATTACK | PROC_ON_SPELL_CRIT_HIT;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectImplicitTargetA[1] = 38;
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[1] = 51699;
        }break;

        // Shadow Dance
        case 51713:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
        }break;

        // Let's hack   the entire cheating death   effect to   damage perc resist ;)
        case 45182: // Cheating Death   buff
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
            sp->EffectMiscValue[0] = uint32(-91);
        }break;

        // Wound Poison Stuff
        case 27189:
        case 13224:
        case 13223:
        case 13222:
        case 13218:
        {
            //sp->Unique = true;
        }break;

        // Killing Spree
        case 51690:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->Effect[1] = 0;
            sp->Effect[2] = 0;
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
            sp->AttributesEx |= ATTRIBUTESEX_NOT_BREAK_STEALTH;
        }break;

        // Focused Attacks
        case 51634:
        case 51635:
        case 51636:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
        }break;

        // Setup
        case 13983:
        {
            sp->proc_interval = 1000;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->procFlags = PROC_ON_RESIST_RECEIVED;
            sp->procChance = 33;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;
        case 14070:
        {
            sp->proc_interval = 1000;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->procFlags = PROC_ON_RESIST_RECEIVED;
            sp->procChance = 66;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;
        case 14071:
        {
            sp->proc_interval = 1000;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->procFlags = PROC_ON_RESIST_RECEIVED;
            sp->procChance = 100;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;

        // Setup PROC
        case 15250:
        {
            sp->proc_interval = 1000;
        }break;

        // Mutilate
        case 1329:
        case 34411:
        case 34412:
        case 34413:
        case 48663:
        case 48666:
        {
            sp->Effect[1] = 0;
            sp->procChance = 0;
            sp->ATTRIBUTESEX2 &= ~ATTRIBUTESEX2_REQ_BEHIND_TARGET;
        }break;

        case 35541:
        case 35550:
        case 35551:
        case 35552:
        case 35553:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->procChance = 20;
        }break;

        //////////////////////////////////////////
        // PRIEST                                   //
        //////////////////////////////////////////

        // Dispersion (org spell)
        case 47585:
        {
            sp->AdditionalAura = 47218;
        }break;

        // Dispersion (remove im effects, in 3.1 there is a spell for this)
        case 47218:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_MECHANIC_IMMUNITY;
            sp->EffectApplyAuraName[1] = SPELL_AURA_MECHANIC_IMMUNITY;
            sp->EffectMiscValue[0] = 7;
            sp->EffectMiscValue[1] = 11;
            sp->EffectImplicitTargetA[0] = 1;
            sp->EffectImplicitTargetA[1] = 1;
        }break;

        // Mass dispel
        case 32375:
        case 32592:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 100;
            sp->EffectImplicitTargetA[1] = 1;
        }break;

        // Power Infusion
        case 10060:
        {
            sp->c_is_flags = SPELL_FLAG_IS_FORCEDBUFF;
        }break;

        // Prayer of mending (tricky one :() rank 1
        case 33076:
        {
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 41635;
            sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PARTY_MEMBER;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PARTY_MEMBER;
            sp->EffectImplicitTargetB[0] = 0;
            sp->EffectImplicitTargetB[1] = 0;
        }break;

        // Prayer of mending (tricky one :() rank 2
        case 48112:
        {
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 48110;
            sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PARTY_MEMBER;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PARTY_MEMBER;
            sp->EffectImplicitTargetB[0] = 0;
            sp->EffectImplicitTargetB[1] = 0;
        }break;

        // Prayer of mending (tricky one :() rank 3
        case 48113:
        {
            sp->Effect[0] = SPELL_EFFECT_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 48111;
            sp->Effect[1] = SPELL_EFFECT_DUMMY;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PARTY_MEMBER;
            sp->EffectImplicitTargetA[1] = EFF_TARGET_PARTY_MEMBER;
            sp->EffectImplicitTargetB[0] = 0;
            sp->EffectImplicitTargetB[1] = 0;
        }break;

        // triggered spell, rank 1
        case 41635:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 41637;
            sp->EffectBasePoints[0] = 800;
            sp->procCharges = 0;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
        }break;

        // triggered spell, rank 2
        case 48110:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 41637;
            sp->EffectBasePoints[0] = 905;
            sp->procCharges = 0;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
        }break;

        // triggered spell, rank 3
        case 48111:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 41637;
            sp->EffectBasePoints[0] = 1043;
            sp->procCharges = 0;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
        }break;

        // triggered on hit, this   is the spell that   does the healing/jump
        case 41637:
        {
            sp->Effect[0] = SPELL_EFFECT_DUMMY;
            sp->EffectBasePoints[0] = 5;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_SELF;
            sp->EffectImplicitTargetB[0] = 0;
        }break;

        // Inner Focus
        case 14751:
        {
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
        }break;

        // Divine Aegis
        case 47509:
        case 47511:
        case 47515:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectImplicitTargetA[0] = 21;
            sp->EffectTriggerSpell[0] = 47753;
        }break;

        // Insert   priest spell fixes here

        //////////////////////////////////////////
        // SHAMAN                                   //
        //////////////////////////////////////////
        case 51466:
        case 51470: //Elemental Oath
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_ADD_FLAT_MODIFIER;
            sp->EffectMiscValue[1] = SMT_EFFECT_3;
            sp->EffectSpellClassMask[1][0] = 0;
            sp->EffectSpellClassMask[1][1] = 0x00004000; // Clearcasting
        }break;
        case 51562:
        case 51563:
        case 51564:
        case 51565:
        case 51566: // Tidal Waves
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x00000100;    // Chain heal
            sp->EffectSpellClassMask[0][1] = 0x00000000;
            sp->EffectSpellClassMask[0][2] = 0x00000010;    // Riptide
        }break;
        case 53390:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 51940:
        case 51989:
        case 52004:
        case 52005:
        case 52007:
        case 52008: // Earthliving Weapon
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 20;
        }break;
        case 55198: // Tidal Force
        {
            sp->Effect[0] = SPELL_EFFECT_DUMMY;
            sp->EffectApplyAuraName[0] = 0;
            sp->EffectTriggerSpell[0] = 55166;
        }break;
        case 55166:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case 51525:
        case 51526:
        case 51527: // Static Shock
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = sp->Id;
            sp->EffectSpellClassMask[0][0] = 0;
        }break;
        case 16180:
        case 16196:
        case 16198: // Improved Water Shield
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x000000C0; // Healing Wave and Lesser Healing Wave
            sp->EffectSpellClassMask[0][2] = 0x00000010; //Riptide
            sp->EffectTriggerSpell[0] = sp->Id;
        }break;
        case 16187:
        case 16205:
        case 16206:
        case 16207:
        case 16208: // Restorative Totems
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_ADD_PCT_MODIFIER;
            sp->EffectMiscValue[0] = SMT_DAMAGE_DONE;
            sp->EffectApplyAuraName[1] = SPELL_AURA_ADD_PCT_MODIFIER;
        }break;
        case 31616: // Nature's Guardian
        {
            sp->spell_can_crit = false;
        }break;

        //////////////////////////////////////////
        // MAGE                                 //
        //////////////////////////////////////////

        // Insert   mage spell fixes here

        // Invisibility
        case 66:
        {
            sp->EffectTriggerSpell[1] = 32612;
            sp->EffectAmplitude[1] = 3000;
        }break;

        // Invisibility part    2
        case 32612:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
            sp->Effect[2] = 0;
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL | AURA_INTERRUPT_ON_START_ATTACK | AURA_INTERRUPT_ON_HOSTILE_SPELL_INFLICTED;
        }break;

        //improved blink
        case 31569:
        case 31570:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        // Magic Absorption
        case 29441:
        case 29444:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 29442;
        }break;

        //Missile Barrage
        case 44404:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 4;
        }break;
        case 54486:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 8;
        }break;
        case 54488:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 12;
        }break;
        case 54489:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 16;
        }break;
        case 54490:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 20;
        }break;
        //Fiery Payback
        case 44440:
        case 44441:
        {
            sp->procChance = 100;
            sp->procFlags = PROC_ON_SPELL_HIT_RECEIVED | PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_RANGED_ATTACK_RECEIVED | PROC_ON_ANY_DAMAGE_RECEIVED;
        }break;

        //Fingers of Frost
        case 44543:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 7;
        }break;
        case 44545:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 15;
        }break;

        //Conjure Refreshment table
        case 43985:
        case 58661:
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_DYNAMIC_OBJECT;
        }break;

        // Ritual of Refreshment
        case 43987:
        case 58659:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_REQUIRECOOLDOWNUPDATE;
        }break;

        // Living bomb
        case 44457:
        case 55359:
        case 55360:
        {
            // sp->c_is_flags |= SPELL_FLAG_ON_ONLY_ONE_TARGET;
        }break;

        // Arcane Potency proc
        case 57529:
        case 57531:
        {
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
        }break;

        // Burnout
        case 44449:
        case 44469:
        case 44470:
        case 44471:
        case 44472:
        {
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[1] = 44450;
            sp->EffectImplicitTargetA[1] = 1;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        //////////////////////////////////////////
        // WARLOCK                                  //
        //////////////////////////////////////////

        // Insert   warlock spell   fixes   here
    // Demonic Knowledge
        case 35691:
        case 35692:
        case 35693:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = 100;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectImplicitTargetA[1] = 5;
            sp->EffectTriggerSpell[1] = 35696;
        }break;

        // Demonic Knowledge PROC
        case 35696:
        {
            sp->EffectImplicitTargetA[0] = 1;
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
        }break;

        // Demonic Pact AA
        case 48090:
        {
            //sp->Effect[0] = SPELL_EFFECT_APPLY_AREA_AURA;
            //sp->Effect[1] = SPELL_EFFECT_APPLY_AREA_AURA;
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_RAID_AREA_AURA;
        }break;

        // Demonic Pact
        case 53646:
        {
            sp->procChance = 100;
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;
        // Unstable Affliction
        case 30108:
        case 30404:
        case 30405:
        case 47841:
        case 47843:
        {
            sp->procFlags = PROC_ON_PRE_DISPELL_AURA_RECEIVED;
            sp->procChance = 100;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectImplicitTargetA[1] = 6;
            sp->EffectTriggerSpell[1] = 31117;
        }break;

        // Death's Embrace
        case 47198:
        case 47199:
        case 47200:
        {
            sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[2] = SPELL_AURA_DUMMY;
            sp->EffectImplicitTargetA[2] = 1;
        }break;

        // Everlasting affliction
        case 47201:
        case 47202:
        case 47203:
        case 47204:
        case 47205:
        {
            sp->procFlags = PROC_ON_SPELL_HIT;
        }break;

        //warlock - Molten Core
        case 47245:
        case 47246:
        case 47247:
        {
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION;
        }break;

        // pandemic
        case 58435:
        case 58436:
        case 58437:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 58691;
            sp->EffectImplicitTargetA[0] = 6;
        }break;

        // Mana Feed
        case 30326:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 32554;
            sp->EffectImplicitTargetA[0] = EFF_TARGET_PET;
        }break;

        // Fel Synergy
        case 47230:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 54181;
        }break;
        case 47231:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 54181;
        }break;

        //////////////////////////////////////////
        // DRUID                                    //
        //////////////////////////////////////////

        // Insert   druid   spell   fixes   here
        case 22570:
        case 49802: // Maim
        {
            sp->AuraInterruptFlags |= AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN;
            sp->Attributes |= ATTRIBUTES_STOP_ATTACK;
        }break;

        // RAVAGE
        case 6785:
        {
            sp->EffectBasePoints[0] = 162;              //r1
        }break;
        case 6787:
        {
            sp->EffectBasePoints[0] = 239;              //r2
        }break;
        case 9866:
        {
            sp->EffectBasePoints[0] = 300;              //r3
        }break;
        case 9867:
        {
            sp->EffectBasePoints[0] = 377;              //r4
        }break;
        case 27005:
        {
            sp->EffectBasePoints[0] = 566;              //r5
        }break;
        case 48578:
        {
            sp->EffectBasePoints[0] = 1405;             //r6
        }break;
        case 48579:
        {
            sp->EffectBasePoints[0] = 1770;             //r7
        }break;

        //SHRED
        case 5221:
        {
            sp->EffectBasePoints[0] = 54;               //r1
        }break;
        case 6800:
        {
            sp->EffectBasePoints[0] = 72;               //r2
        }break;
        case 8992:
        {
            sp->EffectBasePoints[0] = 99;               //r3
        }break;
        case 9829:
        {
            sp->EffectBasePoints[0] = 144;              //r4
        }break;
        case 9830:
        {
            sp->EffectBasePoints[0] = 180;              //r5
        }break;
        case 27001:
        {
            sp->EffectBasePoints[0] = 236;              //r6
        }break;
        case 27002:
        {
            sp->EffectBasePoints[0] = 405;              //r7
        }break;
        case 48571:
        {
            sp->EffectBasePoints[0] = 630;              //r8
        }break;
        case 48572:
        {
            sp->EffectBasePoints[0] = 742;              //r9
        }break;

        // Natural reaction
        case 57878:
        case 57880:
        case 57881:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->procFlags = 0;
            sp->procChance = 0;
            sp->EffectTriggerSpell[1] = 0;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;

        // Dash
        case 1850:
        case 9821:
        case 33357:
        {
            sp->RequiredShapeShift = 1;
        }break;

        // Totem of Wrath
        case 30708:
        {
            sp->Effect[0] = 6;
            sp->EffectImplicitTargetA[0] = 22;
            sp->EffectImplicitTargetB[0] = 15;
            sp->EffectRadiusIndex[0] = 10;
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_ENEMY_AREA_AURA;
        }break;

        // Mangle - Cat
        case 33876:
        {
            sp->EffectBasePoints[0] = 198; //rank 1
        }break;
        case 33982:
        {
            sp->EffectBasePoints[0] = 256; //rank 2
        }break;
        case 33983:
        {
            sp->EffectBasePoints[0] = 330; //rank 3
        }break;
        case 48565:
        {
            sp->EffectBasePoints[0] = 536; //rank 4
        }break;
        case 48566:
        {
            sp->EffectBasePoints[0] = 634; //rank 5
        }break;
        // Mangle - Bear
        case 33878:
        {
            sp->EffectBasePoints[0] = 86; //rank 1
        }break;
        case 33986:
        {
            sp->EffectBasePoints[0] = 120; //rank 2
        }break;
        case 33987:
        {
            sp->EffectBasePoints[0] = 155; //rank 3
        }break;
        case 48563:
        {
            sp->EffectBasePoints[0] = 251; //rank 4
        }break;
        case 48564:
        {
            sp->EffectBasePoints[0] = 299; //rank 5
        }break;

        //Druid - Master Shapeshifter
        case 48411:
        case 48412:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
            sp->Effect[1] = 0;
            sp->Effect[2] = 0;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 100;
        }break;
        case 48418:
        {
            sp->RequiredShapeShift = (uint32)(1 << (FORM_BEAR - 1)) | (1 << (FORM_DIREBEAR - 1));
        }break;
        case 48420:
        {
            sp->RequiredShapeShift = (uint32)(1 << (FORM_CAT - 1));
        }break;
        case 48421:
        {
            sp->RequiredShapeShift = (uint32)1 << (FORM_MOONKIN - 1);
        }break;
        case 48422:
        {
            sp->RequiredShapeShift = (uint32)1 << (FORM_TREE - 1);
        }break;
        //Owlkin Frenzy
        case 48389:
        case 48392:
        case 48393:
        {
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION | PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_RANGED_ATTACK_RECEIVED;
            sp->EffectSpellClassMask[0][0] = 0x0;
        }break;
        // Infected Wounds
        case 48483:
        case 48484:
        case 48485:
        {
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION | PROC_ON_MELEE_ATTACK;
        }break;
        // Swipe (cat) max targets, fixed in 3.1
        case 62078:
        {
            sp->MaxTargets = 10;
        }break;
        // faerie fire (feral dmg)
        case 16857:
        {
            sp->Effect[1] = SPELL_EFFECT_SCHOOL_DAMAGE;
            sp->EffectBasePoints[1] = 1;
            sp->EffectImplicitTargetA[1] = 6;
        }break;

        // King of the jungle dmg buff
        case 51185:
        {
            sp->DurationIndex = 1;
            sp->RequiredShapeShift = (uint32)(1 << (FORM_BEAR - 1)) | (1 << (FORM_DIREBEAR - 1));
        }break;
        case 60200:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
        }break;

        // Eclipse
        case 48516:
        case 48521:
        case 48525:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 48517;
            sp->EffectTriggerSpell[1] = 48518;
        }break;

        // Living Seed
        case 48496:
        case 48499:
        case 48500:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectImplicitTargetA[0] = 21;
            sp->EffectTriggerSpell[0] = 48503;
        }break;

        // Healing way
        case 29203:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = 108; //ADD PCT MOD
            sp->EffectMiscValue[0] = 0;
            sp->EffectSpellClassMask[0][0] = 64;
            sp->EffectSpellClassMask[0][1] = 0;
            sp->EffectSpellClassMask[0][2] = 0;
        }break;

        case 50334:
        {
            sp->AdditionalAura = 58923;
        }break;

        case 17002:
        {
            sp->AdditionalAura = 24867;
        }break;
        case 24866:
        {
            sp->AdditionalAura = 24864;
        }break;

        case 24867:
        case 24864:
        {
            sp->apply_on_shapeshift_change = true;
        }break;

        case 24905:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;

        case 33881:
        case 33882:
        case 33883:
        {
            sp->procFlags = PROC_ON_CRIT_HIT_RECEIVED;
        }break;

        case 22842:
        case 22895:
        case 22896:
        case 26999:
        {
            sp->Effect[0] = 6;
            sp->EffectApplyAuraName[0] = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 22845;
        }break;

        case 16850:
        {
            sp->procChance = 3;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 16923:
        {
            sp->procChance = 6;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 16924:
        {
            sp->procChance = 9;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        case 24932:
        {
            sp->Effect[1] = 0;
            sp->Effect[2] = 0; //removing strange effects.
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_GROUP_AREA_AURA;
        }break;

        case 34299:
        {
            sp->proc_interval = 6000;//6 secs
        }break;
        case 49376:
        {
            sp->Effect[1] = 41;
            sp->EffectImplicitTargetA[1] = 6;
        }break;

        //////////////////////////////////////////
        // DEATH KNIGHT                         //
        //////////////////////////////////////////

        // Merciless Combat
        case 49024:
        case 49538:
        {
            sp->EffectMiscValue[0] = 7278;
        }break;

        case 46619:
        {
            sp->Effect[0] = SPELL_EFFECT_NULL;
        }break;

        //Desecration
        case 55666:
        case 55667:
        {
            sp->proc_interval = 15000;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        case 50397:
        {
            sp->NameHash = 0;
        }break;

        //////////////////////////////////////////
        // BOSSES                               //
        //////////////////////////////////////////

        // Insert   boss spell fixes here

        // War Stomp
        case 20549:
        {
            sp->RequiredShapeShift = 0;
        }break;

        // Dark Glare
        case 26029:
        {
            sp->cone_width = 15.0f; // 15   degree cone
        }break;

        // Commendation of Kael'thas
        case 45057:
        {
            sp->proc_interval = 30000;
        }break;

        // Recently Dropped Flag
        case 42792:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_FORCEDDEBUFF;
        }break;

        case 43958:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
            sp->DurationIndex = 6; //   10 minutes.
            sp->c_is_flags |= SPELL_FLAG_IS_FORCEDDEBUFF;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_ADD_PCT_MODIFIER;
            sp->EffectMiscValue[1] = SMT_RESIST_DISPEL;
            sp->EffectBasePoints[1] = 90;
        }break;

        // Recently Dropped Flag
        case 43869:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
            sp->c_is_flags |= SPELL_FLAG_IS_FORCEDDEBUFF;
        }break;

        case 48978:
        case 61216:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
            sp->EffectMiscValue[1] = 1;
        }break;

        case 49390:
        case 61221:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
            sp->EffectMiscValue[1] = 2;
        }break;

        case 49391:
        case 61222:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
            sp->EffectMiscValue[1] = 3;
        }break;

        case 49392:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
            sp->EffectMiscValue[1] = 4;
        }break;

        case 49393:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
            sp->EffectMiscValue[1] = 5;
        }break;

        // Furious Attacks
        case 46910:
        {
            sp->EffectTriggerSpell[0] = 56112;
            sp->procChance = 50;
            sp->procFlags |= PROC_ON_MELEE_ATTACK;
        }break;

        case 46911:
        {
            sp->EffectTriggerSpell[0] = 56112;
            sp->procChance = 100;
            sp->procFlags |= PROC_ON_MELEE_ATTACK;
        }break;

        // Rogue:   Hunger for Blood!
        case 51662:
        {
            sp->Effect[1] = SPELL_EFFECT_DUMMY;
        }break;

        // Mage: Focus Magic
        case 54646:
        {
            //sp->c_is_flags = SPELL_FLAG_ON_ONLY_ONE_TARGET;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
            sp->procChance = 100;
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[1] = 54648;
        }break;

        // Mage: Hot Streak
        case 44445:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 48108;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
            sp->procChance = 33;
            sp->EffectSpellClassMask[0][0] = 0x0;
        }break;
        case 44446:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 48108;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
            sp->procChance = 66;
            sp->EffectSpellClassMask[0][0] = 0x0;
        }break;
        case 44448:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 48108;
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
            sp->procChance = 100;
            sp->EffectSpellClassMask[0][0] = 0x0;
        }break;

        case 1122:
        {
            sp->EffectBasePoints[0] = 0;
        }break;

        case 1860:
        case 20719:
        {
            sp->Effect[0] = SPELL_EFFECT_DUMMY;
        }break;
        // Bloodsurge
        case 46913:
        case 46914:
        case 46915:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK | PROC_ON_ANY_HOSTILE_ACTION | PROC_ON_CAST_SPELL;
        }break;
        case 46916:
        {
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
        }break;
        //Waylay
        case 51692:
        case 51696:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
        }break;
        // Cold Blood
        case 14177:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
        }break;
        // priest   -   mind flay
        case 15407:
        case 17311:
        case 17312:
        case 17313:
        case 17314:
        case 18807:
        case 25387:
        case 48155:
        case 48156:
        {
            sp->EffectImplicitTargetA[0] = EFF_TARGET_SINGLE_ENEMY;
        }break;

        // Death and Decay
        case 43265:
        case 49936:
        case 49937:
        case 49938:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PERIODIC_DAMAGE;
        }break;
        //Warlock   Chaos   bolt
        case 50796:
        case 59170:
        case 59171:
        case 59172:
        {
            sp->Attributes = ATTRIBUTES_IGNORE_INVULNERABILITY;
        }break;
        //Force debuff's
        // Hypothermia
        case 41425:
            // Forbearance
        case 25771:
            // Weakened Soul
        case 6788:
        {
            sp->c_is_flags = SPELL_FLAG_IS_FORCEDDEBUFF;
        }break;
        // Death Knight spell   fixes
        //Blade Barrier
        case 49182:
        case 49500:
        case 49501:
        case 55225:
        case 55226:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 100;
        }break;
        // Killing Machine
        case 51123:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->ProcsPerMinute = 1;
        }break;
        case 51127:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->ProcsPerMinute = 2;
        }break;
        case 51128:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->ProcsPerMinute = 3;
        }break;
        case 51129:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->ProcsPerMinute = 4;
        }break;
        case 51130:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
            sp->ProcsPerMinute = 5;
        }break;
        case 49175:
        case 50031:
        case 51456: // Improved Icy Touch
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_ADD_PCT_MODIFIER;
        }break;
        case 49143:
        case 51416:
        case 51417:
        case 51418:
        case 51419:
        case 55268: // Frost Strike
        {
            sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC;
        }break;
        case 55090:
        case 55265:
        case 55270:
        case 55271: // Scourge Strike
        {
            sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC;
        }break;
        case 1843:  // Disarm
        {
            sp->Effect[0] = 0;  // to prevent abuse at Arathi
        }break;
        // Bone Shield
        case 49222:
        {
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
            sp->proc_interval = 3000;
        }break;
        // Shadow   of Death
        case 49157:
        {
            sp->Effect[0] = 0;  // don't want   DKs to be   always invincible
        }break;
        // Death Grip
        case 49576:
        {
            sp->FacingCasterFlags = 0;
        }break;
        // shadow   of death
        case 54223:
        {
            sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[2] = SPELL_AURA_DUMMY;
            sp->ATTRIBUTESEX3 |= CAN_PERSIST_AND_CASTED_WHILE_DEAD;
        }break;
        case 54749://Burning Determination
        case 54747:
        {
            sp->procFlags = PROC_ON_SPELL_LAND_RECEIVED;
        }break;
        case 48266://blood presence
        {
            sp->EffectTriggerSpell[1] = 50475;
            sp->procFlags = PROC_ON_ANY_HOSTILE_ACTION | PROC_ON_MELEE_ATTACK;
        }break;
        case 50475:
        {
            sp->Effect[0] = SPELL_EFFECT_NULL;
        }break;
        case 48263://Frost Presence
        {
            sp->AdditionalAura = 61261;
        }break;
        case 48265://Unholy Presence
        {
            sp->AdditionalAura = 49772;
        }break;
        case 56364:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case 44443://Firestarter
        case 44442:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x0;
            sp->EffectSpellClassMask[0][1] = 0x0;
        }break;
        //Mage - Brain Freeze
        case 44546:
        case 44584:
        case 44549:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x0;
            sp->EffectSpellClassMask[0][1] = 0x0;
        }break;
        case 54741:
        {
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
        }break;

        case 56368:
        {
            sp->Effect[1] = sp->Effect[0];
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;
        case 48020:
        {
            sp->Effect[1] = SPELL_EFFECT_DUMMY;
        }break;
        case 48018: // Demonic Circle dummy shit.
        {
            sp->AdditionalAura = 62388;
            sp->EffectImplicitTargetA[0] = 1;
        }break;

        //Noise Machine - Sonic Shield
        case 54808:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 55019;
            sp->procFlags = PROC_ON_MELEE_ATTACK_RECEIVED;
            sp->proc_interval = 60000;
            sp->procChance = 50;
        }break;
        //Pendulum of Telluric Currents
        case 60482:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 60483;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = 15;
        }break;
        //Tears of Bitter Anguish
        case 58901:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->EffectTriggerSpell[0] = 58904;
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            sp->procChance = 10;
        }break;
        case 20578:
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_MOVEMENT | AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN;
            break;
        case 51729:
        {
            //sp->buffIndexType = 0;
            //sp->buffType = 0;
            //sp->AreaGroupId = 0;
        }break;
        case 58691://Pandemic
        {
            sp->spell_can_crit = false;
        }break;
        case 54197:
        {
            sp->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
        }break;
        //Warlock - Nether Protection
        case 30299:
        case 30301:
        case 30302:
        {
            sp->procFlags = PROC_ON_SPELL_HIT_RECEIVED;
        }break;

        case 71905:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;

        //////////////////////////////////////////
        // ITEMSETS                             //
        //////////////////////////////////////////

        case 70765: // Divine Storm Cooldown Clear
        {
            sp->procChance = 20; // Crow: I got a feeling...
            //sp->procChance = 40; // Current blizzlike settings.
        }break;

        //////////////////////////////////////////////////////////////////
        //AREA AURA TARGETS - START
        //////////////////////////////////////////////////////////////////

        case 57658: // Shaman - totem of the wrath
        case 57660:
        case 57662:
        case 34123: // Druid - tree of life
        {
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_RAID_AREA_AURA;
        }break;

        //Spells using Aura 109
        case 50040:
        case 50041:
        case 50043:
        case 64745:
        case 60675:
        case 60685:
        case 60686:
        case 60687:
        case 60688:
        case 60690:
        case 64936:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procChance = sp->EffectBasePoints[0] + 1;
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;

        // Glyph of hex
        case 63291:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
        }break;
        }

        switch (sp->NameHash)
        {
            //Pal
        case SPELL_HASH_CONCENTRATION_AURA:
        case SPELL_HASH_RETRIBUTION_AURA:
        case SPELL_HASH_DIVINE_GUARDIAN:
            //Hunter
        case SPELL_HASH_ASPECT_OF_THE_WILD:
        case SPELL_HASH_TRUESHOT_AURA:
            //Death knight
        case SPELL_HASH_PATH_OF_FROST:
            //Druid
        case SPELL_HASH_MOONKIN_AURA:
        case SPELL_HASH_IMPROVED_MOONKIN_FORM:
            //Warlock
        case SPELL_HASH_BLOOD_PACT:
        case SPELL_HASH_FEL_INTELLIGENCE:
        {
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_RAID_AREA_AURA;
        }break;

        //Pal
        case SPELL_HASH_DEVOTION_AURA:
        case SPELL_HASH_CRUSADER_AURA:
        case SPELL_HASH_SHADOW_RESISTANCE_AURA:
        case SPELL_HASH_FROST_RESISTANCE_AURA:
        case SPELL_HASH_FIRE_RESISTANCE_AURA:
            //Hunter
        case SPELL_HASH_ASPECT_OF_THE_PACK:
            //Shaman
        case SPELL_HASH_FIRE_RESISTANCE:
        case SPELL_HASH_FROST_RESISTANCE:
        case SPELL_HASH_NATURE_RESISTANCE:
        case SPELL_HASH_STONESKIN:
        case SPELL_HASH_STRENGTH_OF_EARTH:
        case SPELL_HASH_WINDFURY_TOTEM:
        case SPELL_HASH_WRATH_OF_AIR_TOTEM:
            //Priest
        case SPELL_HASH_PRAYER_OF_FORTITUDE:
        case SPELL_HASH_PRAYER_OF_SHADOW_PROTECTION:
        case SPELL_HASH_PRAYER_OF_SPIRIT:
            //Warrior
        case SPELL_HASH_BATTLE_SHOUT:
        case SPELL_HASH_COMMANDING_SHOUT:
        {
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_GROUP_AREA_AURA;
        }break;

        //Hunter
        case SPELL_HASH_ASPECT_OF_THE_BEAST:
        {
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_PET_AREA_AURA;
        }break;
        //Rogue
        case SPELL_HASH_HONOR_AMONG_THIEVES:
        {
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_GROUP_AREA_AURA;
        }break;

        //////////////////////////////////////////////////////
        // CLASS-SPECIFIC NAMEHASH FIXES                    //
        //////////////////////////////////////////////////////
        ////////////// WARRIOR ///////////////////

        case SPELL_HASH_BLADESTORM:
            //sp->AllowBackAttack = true;
            break;

        case SPELL_HASH_MORTAL_STRIKE:
            sp->maxstack = 1; // Healing reduction shouldn't stack
            break;

        case SPELL_HASH_TRAUMA:
            sp->procFlags = PROC_ON_CRIT_ATTACK;
            break;

        case SPELL_HASH_SLAM:
            sp->Effect[0] = SPELL_EFFECT_SCHOOL_DAMAGE;
            break;

        case SPELL_HASH_HOLY_SHIELD:
            sp->procFlags = PROC_ON_BLOCK_RECEIVED;
            break;

            ////////////// PALADIN ///////////////////

        case SPELL_HASH_JUDGEMENT_OF_WISDOM:
        case SPELL_HASH_JUDGEMENT_OF_LIGHT:
        case SPELL_HASH_JUDGEMENT_OF_JUSTICE:
        case SPELL_HASH_HEART_OF_THE_CRUSADER:
            sp->maxstack = 1;
            break;

        case SPELL_HASH_SEAL_OF_LIGHT:
        {
            sp->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[2] = SPELL_AURA_DUMMY;
            sp->ProcsPerMinute = 10;    /* this will do */
        }break;

        case SPELL_HASH_SEALS_OF_THE_PURE:
        {
            sp->EffectSpellClassMask[0][0] = 8389632 | 4194312 | 512;
            sp->EffectSpellClassMask[0][1] = 134217728 | 536870912;
            sp->EffectSpellClassMask[0][2] = 0;
            sp->EffectSpellClassMask[1][0] = 2048;
            sp->EffectSpellClassMask[1][1] = 0;
            sp->EffectSpellClassMask[1][2] = 0;
        }break;

        case SPELL_HASH_JUDGEMENTS_OF_THE_PURE:
        {   // Ignore our triggered spells.
            if (sp->Id != 54153 && sp->Id != 53655 && sp->Id != 53656 && sp->Id != 53657 && sp->Id != 54152)
            {
                sp->EffectSpellClassMask[0][0] = 8388608;
                sp->EffectSpellClassMask[0][1] = 0;
                sp->EffectSpellClassMask[0][2] = 0;
                sp->EffectSpellClassMask[1][0] = 8389632 | 4194312 | 512;
                sp->EffectSpellClassMask[1][1] = 134217728 | 536870912 | 33554432;
                sp->EffectSpellClassMask[1][2] = 0;
                sp->EffectSpellClassMask[2][0] = 2048;
                sp->EffectSpellClassMask[2][1] = 0;
                sp->EffectSpellClassMask[2][2] = 0;
                sp->procFlags = PROC_ON_CAST_SPELL;
            }
        }break;
        case SPELL_HASH_DEVOURING_PLAGUE:
        {
            sp->MaxTargets = 1;
        }break;
        case SPELL_HASH_IMPROVED_DEVOTION_AURA:
        {
            sp->EffectApplyAuraName[1] = SPELL_AURA_MOD_HEALING_PCT;
            sp->EffectBasePoints[1] = 6;
            sp->EffectMiscValue[1] = 127;
        }break;
        case SPELL_HASH_AVENGER_S_SHIELD:
        {
            sp->Spell_Dmg_Type = SPELL_DMG_TYPE_MAGIC;
        }break;
        case SPELL_HASH_SHIELD_OF_RIGHTEOUSNESS:
        {
            sp->EffectChainTarget[0] = 0;
        }break;
        case SPELL_HASH_AIMED_SHOT:
        {
            sp->maxstack = 1; // Healing reduction shouldn't stack
        }break;
        case SPELL_HASH_EXPLOSIVE_SHOT:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PERIODIC_DAMAGE;
        }break;
        case SPELL_HASH_MORTAL_SHOTS:
        {
            sp->EffectSpellClassMask[0][0] += 1;
        }break;
        case SPELL_HASH_MEND_PET:
        {
            sp->ChannelInterruptFlags = 0;
        }break;
        case SPELL_HASH_EAGLE_EYE:
        {
            sp->Effect[1] = 0;
        }break;
        case SPELL_HASH_ENTRAPMENT:
        {
            if (sp->EffectApplyAuraName[0] == SPELL_AURA_MOD_ROOT)
            {
                sp->EffectImplicitTargetA[0] = 15;
                sp->EffectRadiusIndex[0] = 13;
            }
        }break;
        case SPELL_HASH_WILD_QUIVER:
        {
            sp->EffectApplyAuraName[1] = 0;
        }break;
        case SPELL_HASH_REMORSELESS_ATTACKS:
        {
            sp->procFlags = PROC_ON_GAIN_EXPIERIENCE;
        }break;
        case SPELL_HASH_UNFAIR_ADVANTAGE:
        {
            sp->procFlags = PROC_ON_DODGE_RECEIVED;
        }break;
        case SPELL_HASH_COMBAT_POTENCY:
        {
            sp->procFlags = PROC_ON_MELEE_ATTACK;
        }break;
        case SPELL_HASH_PAIN_AND_SUFFERING:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case SPELL_HASH_FOCUSED_ATTACKS:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
        }break;
        case SPELL_HASH_SEAL_FATE:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;
        case SPELL_HASH_VILE_POISONS:
        {
            sp->EffectSpellClassMask[0][0] = 8388608; // envenom
            sp->EffectSpellClassMask[0][1] = 8;
            sp->EffectSpellClassMask[1][0] = 8192 | 268435456 | 65536; //poisons
            sp->EffectSpellClassMask[1][1] = 524288;
        }break;
        case SPELL_HASH_STEALTH:
        {
            if (!(sp->AuraInterruptFlags & AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN))
                sp->AuraInterruptFlags |= AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN;

            // fuck this
            sp->EffectBasePoints[1] = 0;
        }break;
        case SPELL_HASH_NERVES_OF_STEEL:
        {
            sp->CasterAuraState = 6;
            sp->EffectBasePoints[0] = -31;
            sp->EffectApplyAuraName[0] = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
        }break;
        case SPELL_HASH_DISARM_TRAP:
        {
            sp->Effect[0] = SPELL_EFFECT_DUMMY;
        }break;
        case SPELL_HASH_BORROWED_TIME:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->EffectApplyAuraName[1] = SPELL_AURA_ADD_PCT_MODIFIER;
        }break;
        case SPELL_HASH_IMPROVED_SPIRIT_TAP:
        {
            sp->procFlags = PROC_ON_SPELL_CRIT_HIT;
        }break;
        case SPELL_HASH_MISERY:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case SPELL_HASH_POWER_INFUSION:
        case SPELL_HASH_HEROISM:
        case SPELL_HASH_BLOODLUST:
        {
            //sp->buffType = SPELL_TYPE_HASTE;
        }break;
        case SPELL_HASH_HEX:
        {
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN;
        }break;
        case SPELL_HASH_LIGHTNING_SHIELD:
        {
            sp->spell_can_crit = false;
        }break;
        case SPELL_HASH_FROSTBRAND_WEAPON:
        {
            sp->ProcsPerMinute = 9.0f;
            //sp->Flags3 |= FLAGS3_ENCHANT_OWN_ONLY;
        }break;
        case SPELL_HASH_NATURE_S_GUARDIAN:
        {
            sp->procFlags = PROC_ON_SPELL_HIT_RECEIVED | PROC_ON_MELEE_ATTACK_RECEIVED | PROC_ON_RANGED_ATTACK_RECEIVED | PROC_ON_ANY_DAMAGE_RECEIVED;
            sp->proc_interval = 8000;
            sp->EffectTriggerSpell[0] = 31616;
        }break;
        case SPELL_HASH_WINDFURY_WEAPON:
        case SPELL_HASH_FLAMETONGUE_WEAPON:
        case SPELL_HASH_ROCKBITER_WEAPON:
        case SPELL_HASH_EARTHLIVING_WEAPON:
        {
            //sp->Flags3 |= FLAGS3_ENCHANT_OWN_ONLY;
        }break;
        case SPELL_HASH_STONECLAW_TOTEM_PASSIVE:
        {
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
        }break;
        case SPELL_HASH_FLAMETONGUE_TOTEM:
        {
            sp->AreaAuraTarget = SPELL_EFFECT_APPLY_GROUP_AREA_AURA;
            if (sp->Attributes & ATTRIBUTES_PASSIVE)
            {
                sp->EffectImplicitTargetA[0] = EFF_TARGET_SELF;
                sp->EffectImplicitTargetB[0] = 0;
                sp->EffectImplicitTargetA[1] = EFF_TARGET_SELF;
                sp->EffectImplicitTargetB[1] = 0;
            }
        }break;
        case SPELL_HASH_UNLEASHED_RAGE:
        {
            sp->procFlags = PROC_ON_CRIT_ATTACK;
        }break;
        case SPELL_HASH_HEALING_STREAM_TOTEM:
        {
            if (sp->Effect[0] == SPELL_EFFECT_DUMMY)
            {
                sp->EffectRadiusIndex[0] = 10; // 30 yards
                sp->Effect[0] = SPELL_EFFECT_HEAL;
            }
        }break;
        case SPELL_HASH_MANA_SPRING_TOTEM:
        {
            if (sp->Effect[0] == SPELL_EFFECT_DUMMY)
            {
                sp->Effect[0] = SPELL_EFFECT_ENERGIZE;
                sp->EffectMiscValue[0] = POWER_TYPE_MANA;
            }
        }break;
        case SPELL_HASH_FAR_SIGHT:
        {
            sp->Effect[1] = 0;
        }break;
        case SPELL_HASH_HYPOTHERMIA:
        {
            sp->c_is_flags |= SPELL_FLAG_IS_FORCEDDEBUFF;
        }break;
        case SPELL_HASH_IMPROVED_COUNTERSPELL:
        {
            sp->procFlags = PROC_ON_CAST_SPECIFIC_SPELL;
            sp->EffectSpellClassMask[0][0] = 0x00004000;    // Proc on counterspell only
        }break;
        case SPELL_HASH_SHADOW_WEAVING:
        {
            sp->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            sp->procFlags = PROC_ON_CAST_SPELL;
            sp->procChance = sp->EffectBasePoints[0] + 1;
        }break;
        case SPELL_HASH_SHADOW_TRANCE:
        {
            sp->AuraInterruptFlags = AURA_INTERRUPT_ON_CAST_SPELL;
        }break;
        case SPELL_HASH_ERADICATION:
        {
            sp->EffectTriggerSpell[0] = 54370;
            sp->procFlags = PROC_ON_SPELL_HIT;
        }break;
        case SPELL_HASH_INFECTED_WOUNDS:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case SPELL_HASH_POUNCE:
        {
            sp->always_apply = true;
        }break;
        case SPELL_HASH_OWLKIN_FRENZY:
        {
            sp->procFlags = PROC_ON_ANY_DAMAGE_RECEIVED;
        }break;
        case SPELL_HASH_EARTH_AND_MOON:
        {
            sp->procFlags = PROC_ON_CAST_SPELL;
        }break;
        case SPELL_HASH_STARFALL:
        {
            if (sp->Effect[1] == SPELL_EFFECT_TRIGGER_SPELL)
            {//we can only attack one target with main star
                sp->MaxTargets = 1;
            }
        }break;
        case SPELL_HASH_SHRED:
        {
            sp->MechanicsType = MECHANIC_BLEEDING;
        }break;
        case SPELL_HASH_NURTURING_INSTINCT:
        {
            sp->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;
        case SPELL_HASH_PRIMAL_TENACITY:
        {
            sp->DurationIndex = 21;
            sp->EffectBasePoints[1] = 0;
            sp->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
        }break;
        case SPELL_HASH_PROWL:
        {
            sp->EffectBasePoints[0] = 0;
        }break;
        }

        if (sp->EquippedItemClass == 2 && sp->EquippedItemSubClass & 262156) // 4 + 8 + 262144 ( becomes item classes 2, 3 and 18 which correspond to bow, gun and crossbow respectively)
            sp->is_ranged_spell = true;

        //////////////////////////////////////////////////////////////////
        //AREA AURA TARGETS - END
        //////////////////////////////////////////////////////////////////

        if (sp->ATTRIBUTESEX2 & ATTRIBUTESEX2_CANT_CRIT) //I can haz hacky? :O
            sp->spell_can_crit = false;
    }

    //Spell Coef Overrides
    uint32 acntt = dbcSpell.GetNumRows();
    for (uint32 zzx = 0; zzx < acntt; zzx++)
    {
        // Read every SpellEntry row
        SpellEntry* sp = dbcSpell.LookupRow(zzx);

    switch (sp->Id)
    {

    case 17: // Power Word: Shield - Rank 1
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 116: // Frostbolt - Rank 1
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 120: // Cone of Cold - Rank 1
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 122: // Frost Nova - Rank 1
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 133: // Fireball - Rank 1
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 324: // Lightning Shield - Rank 1
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 325: // Lightning Shield - Rank 2
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 331: // Healing Wave - Rank 1
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 332: // Healing Wave - Rank 2
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 339: // Entangling Roots - Rank 1
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 348: // Immolate - Rank 1
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 403: // Lightning Bolt - Rank 1
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 421: // Chain Lightning - Rank 1
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 529: // Lightning Bolt - Rank 2
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 547: // Healing Wave - Rank 3
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 548: // Lightning Bolt - Rank 3
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 585: // Smite - Rank 1
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 589: // Shadow Word: Pain - Rank 1
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 591: // Smite - Rank 2
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 592: // Power Word: Shield - Rank 2
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 594: // Shadow Word: Pain - Rank 2
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 596: // Prayer of Healing - Rank 1
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 598: // Smite - Rank 3
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 600: // Power Word: Shield - Rank 3
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 603: // Curse of Doom - Rank 1
    {
        sp->SP_coef_override = float(2.000000f);
    }break;
    case 635: // Holy Light - Rank 1
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 639: // Holy Light - Rank 2
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 647: // Holy Light - Rank 3
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 686: // Shadow Bolt - Rank 1
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 689: // Drain Life - Rank 1
    {
        sp->SP_coef_override = float(0.143000f);
    }break;
    case 695: // Shadow Bolt - Rank 2
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 699: // Drain Life - Rank 2
    {
        sp->SP_coef_override = float(0.143000f);
    }break;
    case 703: // Garrote - Rank 1
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 705: // Shadow Bolt - Rank 3
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 707: // Immolate - Rank 2
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 709: // Drain Life - Rank 3
    {
        sp->SP_coef_override = float(0.143000f);
    }break;
    case 740: // Tranquility - Rank 1
    {
        sp->SP_coef_override = float(0.538000f);
    }break;
    case 755: // Health Funnel - Rank 1
    {
        sp->SP_coef_override = float(0.448500f);
    }break;
    case 774: // Rejuvenation - Rank 1
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 837: // Frostbolt - Rank 3
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 879: // Exorcism - Rank 1
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 905: // Lightning Shield - Rank 3
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 913: // Healing Wave - Rank 4
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 915: // Lightning Bolt - Rank 4
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 930: // Chain Lightning - Rank 2
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 943: // Lightning Bolt - Rank 5
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 945: // Lightning Shield - Rank 4
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 970: // Shadow Word: Pain - Rank 3
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 974: // Earth Shield - Rank 1
    {
        sp->SP_coef_override = float(0.476100f);
    }break;
    case 980: // Curse of Agony - Rank 1
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 984: // Smite - Rank 4
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 992: // Shadow Word: Pain - Rank 4
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 996: // Prayer of Healing - Rank 2
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 1004: // Smite - Rank 5
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 1014: // Curse of Agony - Rank 2
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 1026: // Holy Light - Rank 4
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 1042: // Holy Light - Rank 5
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 1058: // Rejuvenation - Rank 2
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 1062: // Entangling Roots - Rank 2
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 1064: // Chain Heal - Rank 1
    {
        sp->SP_coef_override = float(0.800000f);
    }break;
    case 1079: // Rip - Rank 1
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 1088: // Shadow Bolt - Rank 4
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 1094: // Immolate - Rank 3
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 1106: // Shadow Bolt - Rank 5
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 1120: // Drain Soul - Rank 1
    {
        sp->SP_coef_override = float(0.429000f);
    }break;
    case 1430: // Rejuvenation - Rank 3
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 1449: // Arcane Explosion - Rank 1
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 1463: // Mana Shield - Rank 1
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 1495: // Mongoose Bite - Rank 1
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 1776: // Gouge
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 1777: // Gouge - Rank 2
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 1949: // Hellfire - Rank 1
    {
        sp->SP_coef_override = float(0.120000f);
    }break;
    case 1978: // Serpent Sting - Rank 1
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 2060: // Greater Heal - Rank 1
    {
        sp->SP_coef_override = float(1.613500f);
    }break;
    case 2061: // Flash Heal - Rank 1
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 2090: // Rejuvenation - Rank 4
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 2091: // Rejuvenation - Rank 5
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 2120: // Flamestrike - Rank 1
    {
        sp->SP_coef_override = float(0.122000f);
    }break;
    case 2121: // Flamestrike - Rank 2
    {
        sp->SP_coef_override = float(0.122000f);
    }break;
    case 2136: // Fire Blast - Rank 1
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 2137: // Fire Blast - Rank 2
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 2138: // Fire Blast - Rank 3
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 2643: // Multi-Shot - Rank 1
    {
        sp->RAP_coef_override = float(0.200000f);
    }break;
    case 2767: // Shadow Word: Pain - Rank 5
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 2812: // Holy Wrath - Rank 1
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 2818: // Deadly Poison - Rank 1
    {
        sp->AP_coef_override = float(0.024000f);
    }break;
    case 2819: // Deadly Poison II - Rank 2
    {
        sp->AP_coef_override = float(0.024000f);
    }break;
    case 2860: // Chain Lightning - Rank 3
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 2912: // Starfire - Rank 1
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 2941: // Immolate - Rank 4
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 2944: // Devouring Plague - Rank 1
    {
        sp->SP_coef_override = float(0.184900f);
    }break;
    case 2948: // Scorch - Rank 1
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 3009: // Claw - Rank 8
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 3010: // Claw - Rank 7
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 3044: // Arcane Shot - Rank 1
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 3140: // Fireball - Rank 4
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 3472: // Holy Light - Rank 6
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 3606: // Attack - Rank 1
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 3627: // Rejuvenation - Rank 6
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 3698: // Health Funnel - Rank 2
    {
        sp->SP_coef_override = float(0.448500f);
    }break;
    case 3699: // Health Funnel - Rank 3
    {
        sp->SP_coef_override = float(0.448500f);
    }break;
    case 3700: // Health Funnel - Rank 4
    {
        sp->SP_coef_override = float(0.448500f);
    }break;
    case 3747: // Power Word: Shield - Rank 4
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 5143: // Arcane Missiles - Rank 1
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 5144: // Arcane Missiles - Rank 2
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 5145: // Arcane Missiles - Rank 3
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 5176: // Wrath - Rank 1
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 5177: // Wrath - Rank 2
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 5178: // Wrath - Rank 3
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 5179: // Wrath - Rank 4
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 5180: // Wrath - Rank 5
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 5185: // Healing Touch - Rank 1
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 5186: // Healing Touch - Rank 2
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 5187: // Healing Touch - Rank 3
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 5188: // Healing Touch - Rank 4
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 5189: // Healing Touch - Rank 5
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 5195: // Entangling Roots - Rank 3
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 5196: // Entangling Roots - Rank 4
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 5308: // Execute - Rank 1
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 5570: // Insect Swarm - Rank 1
    {
        sp->SP_coef_override = float(0.127000f);
    }break;
    case 5614: // Exorcism - Rank 2
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 5615: // Exorcism - Rank 3
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 5676: // Searing Pain - Rank 1
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 5740: // Rain of Fire - Rank 1
    {
        sp->SP_coef_override = float(0.693200f);
    }break;
    case 6041: // Lightning Bolt - Rank 6
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 6060: // Smite - Rank 6
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 6065: // Power Word: Shield - Rank 5
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 6066: // Power Word: Shield - Rank 6
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 6074: // Renew - Rank 2
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 6075: // Renew - Rank 3
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 6076: // Renew - Rank 4
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 6077: // Renew - Rank 5
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 6078: // Renew - Rank 6
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 6131: // Frost Nova - Rank 3
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 6217: // Curse of Agony - Rank 3
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 6219: // Rain of Fire - Rank 2
    {
        sp->SP_coef_override = float(0.693200f);
    }break;
    case 6222: // Corruption - Rank 2
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 6223: // Corruption - Rank 3
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 6229: // Shadow Ward - Rank 1
    {
        sp->SP_coef_override = float(0.300000f);
    }break;
    case 6343: // Thunder Clap - Rank 1
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 6350: // Attack - Rank 2
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 6351: // Attack - Rank 3
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 6352: // Attack - Rank 4
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 6353: // Soul Fire - Rank 1
    {
        sp->SP_coef_override = float(1.150000f);
    }break;
    case 6572: // Revenge - Rank 1
    {
        sp->AP_coef_override = float(0.207000f);
    }break;
    case 6574: // Revenge - Rank 2
    {
        sp->AP_coef_override = float(0.207000f);
    }break;
    case 6778: // Healing Touch - Rank 6
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 6780: // Wrath - Rank 6
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 47632: // Death Coil DK - Rank 1
    {
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 49892: // Death Coil DK - Rank 2
    {
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 49893: // Death Coil DK - Rank 3
    {
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 49894: // Death Coil DK - Rank 4
    {
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 49895: // Death Coil DK - Rank 5
    {
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 6789: // Death Coil - Rank 1
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 7322: // Frostbolt - Rank 4
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 7379: // Revenge - Rank 3
    {
        sp->AP_coef_override = float(0.207000f);
    }break;
    case 7641: // Shadow Bolt - Rank 6
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 7648: // Corruption - Rank 4
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 7651: // Drain Life - Rank 4
    {
        sp->SP_coef_override = float(0.143000f);
    }break;
    case 8004: // Lesser Healing Wave - Rank 1
    {
        sp->SP_coef_override = float(0.808200f);
    }break;
    case 8005: // Healing Wave - Rank 7
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 8008: // Lesser Healing Wave - Rank 2
    {
        sp->SP_coef_override = float(0.808200f);
    }break;
    case 8010: // Lesser Healing Wave - Rank 3
    {
        sp->SP_coef_override = float(0.808200f);
    }break;
    case 8026: // Flametongue Weapon Proc - Rank 1
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 8028: // Flametongue Weapon Proc - Rank 2
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 8029: // Flametongue Weapon Proc - Rank 3
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 8034: // Frostbrand Attack - Rank 1
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 8037: // Frostbrand Attack - Rank 2
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 8042: // Earth Shock - Rank 1
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 8044: // Earth Shock - Rank 2
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 8045: // Earth Shock - Rank 3
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 8046: // Earth Shock - Rank 4
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 8050: // Flame Shock - Rank 1
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 8052: // Flame Shock - Rank 2
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 8053: // Flame Shock - Rank 3
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 8056: // Frost Shock - Rank 1
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 8058: // Frost Shock - Rank 2
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 8092: // Mind Blast - Rank 1
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 8102: // Mind Blast - Rank 2
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 8103: // Mind Blast - Rank 3
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 8104: // Mind Blast - Rank 4
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 8105: // Mind Blast - Rank 5
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 8106: // Mind Blast - Rank 6
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 8134: // Lightning Shield - Rank 5
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 8187: // Magma Totem - Rank 1
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 8198: // Thunder Clap - Rank 2
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 8204: // Thunder Clap - Rank 3
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 8205: // Thunder Clap - Rank 4
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 8288: // Drain Soul - Rank 2
    {
        sp->SP_coef_override = float(0.429000f);
    }break;
    case 8289: // Drain Soul - Rank 3
    {
        sp->SP_coef_override = float(0.429000f);
    }break;
    case 8400: // Fireball - Rank 5
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 8401: // Fireball - Rank 6
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 8402: // Fireball - Rank 7
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 8406: // Frostbolt - Rank 5
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 8407: // Frostbolt - Rank 6
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 8408: // Frostbolt - Rank 7
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 8412: // Fire Blast - Rank 4
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 8413: // Fire Blast - Rank 5
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 8416: // Arcane Missiles - Rank 4
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 8417: // Arcane Missiles - Rank 5
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 8422: // Flamestrike - Rank 3
    {
        sp->SP_coef_override = float(0.122000f);
    }break;
    case 8423: // Flamestrike - Rank 4
    {
        sp->SP_coef_override = float(0.122000f);
    }break;
    case 8437: // Arcane Explosion - Rank 2
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 8438: // Arcane Explosion - Rank 3
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 8439: // Arcane Explosion - Rank 4
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 8444: // Scorch - Rank 2
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 8445: // Scorch - Rank 3
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 8446: // Scorch - Rank 4
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 8492: // Cone of Cold - Rank 2
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 8494: // Mana Shield - Rank 2
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 8495: // Mana Shield - Rank 3
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 8629: // Gouge - Rank 3
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 8631: // Garrote - Rank 2
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 8632: // Garrote - Rank 3
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 8633: // Garrote - Rank 4
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 8680: // Instant Poison - Rank 1
    {
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 8685: // Instant Poison II - Rank 2
    {
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 8689: // Instant Poison III - Rank 3
    {
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 8903: // Healing Touch - Rank 7
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 8905: // Wrath - Rank 7
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 8910: // Rejuvenation - Rank 7
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 8918: // Tranquility - Rank 2
    {
        sp->SP_coef_override = float(0.538000f);
    }break;
    case 8921: // Moonfire - Rank 1
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 8924: // Moonfire - Rank 2
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 8925: // Moonfire - Rank 3
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 8926: // Moonfire - Rank 4
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 8927: // Moonfire - Rank 5
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 8928: // Moonfire - Rank 6
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 8929: // Moonfire - Rank 7
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 8936: // Regrowth - Rank 1
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 8938: // Regrowth - Rank 2
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 8939: // Regrowth - Rank 3
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 8940: // Regrowth - Rank 4
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 8941: // Regrowth - Rank 5
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 8949: // Starfire - Rank 2
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 8950: // Starfire - Rank 3
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 8951: // Starfire - Rank 4
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 9472: // Flash Heal - Rank 2
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 9473: // Flash Heal - Rank 3
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 9474: // Flash Heal - Rank 4
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 9492: // Rip - Rank 2
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 9493: // Rip - Rank 3
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 9750: // Regrowth - Rank 6
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 9752: // Rip - Rank 4
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 9758: // Healing Touch - Rank 8
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 9839: // Rejuvenation - Rank 8
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 9840: // Rejuvenation - Rank 9
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 9841: // Rejuvenation - Rank 10
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 9852: // Entangling Roots - Rank 5
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 9853: // Entangling Roots - Rank 6
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 9856: // Regrowth - Rank 7
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 9857: // Regrowth - Rank 8
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 9858: // Regrowth - Rank 9
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 9862: // Tranquility - Rank 3
    {
        sp->SP_coef_override = float(0.538000f);
    }break;
    case 9863: // Tranquility - Rank 4
    {
        sp->SP_coef_override = float(0.538000f);
    }break;
    case 9875: // Starfire - Rank 5
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 9876: // Starfire - Rank 6
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 9888: // Healing Touch - Rank 9
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 9889: // Healing Touch - Rank 10
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 9894: // Rip - Rank 5
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 9896: // Rip - Rank 6
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 9912: // Wrath - Rank 8
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 10148: // Fireball - Rank 8
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 10149: // Fireball - Rank 9
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 10150: // Fireball - Rank 10
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 10151: // Fireball - Rank 11
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 10159: // Cone of Cold - Rank 3
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 10160: // Cone of Cold - Rank 4
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 10161: // Cone of Cold - Rank 5
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 10179: // Frostbolt - Rank 8
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 10180: // Frostbolt - Rank 9
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 10181: // Frostbolt - Rank 10
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 10191: // Mana Shield - Rank 4
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 10192: // Mana Shield - Rank 5
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 10193: // Mana Shield - Rank 6
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 10197: // Fire Blast - Rank 6
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 10199: // Fire Blast - Rank 7
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 10201: // Arcane Explosion - Rank 5
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 10202: // Arcane Explosion - Rank 6
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 10205: // Scorch - Rank 5
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 10206: // Scorch - Rank 6
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 10207: // Scorch - Rank 7
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 10211: // Arcane Missiles - Rank 6
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 10212: // Arcane Missiles - Rank 7
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 10215: // Flamestrike - Rank 5
    {
        sp->SP_coef_override = float(0.122000f);
    }break;
    case 10216: // Flamestrike - Rank 6
    {
        sp->SP_coef_override = float(0.122000f);
    }break;
    case 10230: // Frost Nova - Rank 4
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 10312: // Exorcism - Rank 4
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 10313: // Exorcism - Rank 5
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 10314: // Exorcism - Rank 6
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 10318: // Holy Wrath - Rank 2
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 52212:
    {
        sp->AP_coef_override = float(0.048050f);
    }break;
    case 43265:    // Death and Decay Rank 1
    {
        sp->AP_coef_override = float(0.048050f);
    }break;
    case 49936:    // Death and Decay Rank 2
    {
        sp->AP_coef_override = float(0.048050f);
    }break;
    case 49937:    // Death and Decay Rank 3
    {
        sp->AP_coef_override = float(0.048050f);
    }break;
    case 49938:    // Death and Decay Rank 4
    {
        sp->AP_coef_override = float(0.048050f);
    }break;
    case 10328: // Holy Light - Rank 7
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 10329: // Holy Light - Rank 8
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 10391: // Lightning Bolt - Rank 7
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 10392: // Lightning Bolt - Rank 8
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 10395: // Healing Wave - Rank 8
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 10396: // Healing Wave - Rank 9
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 10412: // Earth Shock - Rank 5
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 10413: // Earth Shock - Rank 6
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 10414: // Earth Shock - Rank 7
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 10431: // Lightning Shield - Rank 6
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 10432: // Lightning Shield - Rank 7
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 10435: // Attack - Rank 5
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 10436: // Attack - Rank 6
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 10445: // Flametongue Weapon Proc - Rank 4
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 10447: // Flame Shock - Rank 4
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 10448: // Flame Shock - Rank 5
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 10458: // Frostbrand Attack - Rank 3
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 10466: // Lesser Healing Wave - Rank 4
    {
        sp->SP_coef_override = float(0.808200f);
    }break;
    case 10467: // Lesser Healing Wave - Rank 5
    {
        sp->SP_coef_override = float(0.808200f);
    }break;
    case 10468: // Lesser Healing Wave - Rank 6
    {
        sp->SP_coef_override = float(0.808200f);
    }break;
    case 10472: // Frost Shock - Rank 3
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 10473: // Frost Shock - Rank 4
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 10579: // Magma Totem - Rank 2
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 10580: // Magma Totem - Rank 3
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 10581: // Magma Totem - Rank 4
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 10605: // Chain Lightning - Rank 4
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 10622: // Chain Heal - Rank 2
    {
        sp->SP_coef_override = float(0.800000f);
    }break;
    case 10623: // Chain Heal - Rank 3
    {
        sp->SP_coef_override = float(0.800000f);
    }break;
    case 10892: // Shadow Word: Pain - Rank 6
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 10893: // Shadow Word: Pain - Rank 7
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 10894: // Shadow Word: Pain - Rank 8
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 10898: // Power Word: Shield - Rank 7
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 10899: // Power Word: Shield - Rank 8
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 10900: // Power Word: Shield - Rank 9
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 10901: // Power Word: Shield - Rank 10
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 10915: // Flash Heal - Rank 5
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 10916: // Flash Heal - Rank 6
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 10917: // Flash Heal - Rank 7
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 10927: // Renew - Rank 7
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 10928: // Renew - Rank 8
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 10929: // Renew - Rank 9
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 10933: // Smite - Rank 7
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 10934: // Smite - Rank 8
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 10945: // Mind Blast - Rank 7
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 10946: // Mind Blast - Rank 8
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 10947: // Mind Blast - Rank 9
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 10960: // Prayer of Healing - Rank 3
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 10961: // Prayer of Healing - Rank 4
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 10963: // Greater Heal - Rank 2
    {
        sp->SP_coef_override = float(1.613500f);
    }break;
    case 10964: // Greater Heal - Rank 3
    {
        sp->SP_coef_override = float(1.613500f);
    }break;
    case 10965: // Greater Heal - Rank 4
    {
        sp->SP_coef_override = float(1.613500f);
    }break;
    case 11113: // Blast Wave - Rank 1
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 11285: // Gouge - Rank 4
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 11286: // Gouge - Rank 5
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 11289: // Garrote - Rank 5
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 11290: // Garrote - Rank 6
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 11335: // Instant Poison IV - Rank 4
    {
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 11336: // Instant Poison V - Rank 5
    {
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 11337: // Instant Poison VI - Rank 6
    {
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 11353: // Deadly Poison III - Rank 3
    {
        sp->AP_coef_override = float(0.024000f);
    }break;
    case 11354: // Deadly Poison IV - Rank 4
    {
        sp->AP_coef_override = float(0.024000f);
    }break;
    case 11366: // Pyroblast - Rank 1
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 11426: // Ice Barrier - Rank 1
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 11580: // Thunder Clap - Rank 5
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 11581: // Thunder Clap - Rank 6
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 11600: // Revenge - Rank 4
    {
        sp->AP_coef_override = float(0.207000f);
    }break;
    case 11601: // Revenge - Rank 5
    {
        sp->AP_coef_override = float(0.207000f);
    }break;
    case 11659: // Shadow Bolt - Rank 7
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 11660: // Shadow Bolt - Rank 8
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 11661: // Shadow Bolt - Rank 9
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 11665: // Immolate - Rank 5
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 11667: // Immolate - Rank 6
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 11668: // Immolate - Rank 7
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 11671: // Corruption - Rank 5
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 11672: // Corruption - Rank 6
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 11675: // Drain Soul - Rank 4
    {
        sp->SP_coef_override = float(0.429000f);
    }break;
    case 11677: // Rain of Fire - Rank 3
    {
        sp->SP_coef_override = float(0.693200f);
    }break;
    case 11678: // Rain of Fire - Rank 4
    {
        sp->SP_coef_override = float(0.693200f);
    }break;
    case 11683: // Hellfire - Rank 2
    {
        sp->SP_coef_override = float(0.120000f);
    }break;
    case 11684: // Hellfire - Rank 3
    {
        sp->SP_coef_override = float(0.120000f);
    }break;
    case 11693: // Health Funnel - Rank 5
    {
        sp->SP_coef_override = float(0.448500f);
    }break;
    case 11694: // Health Funnel - Rank 6
    {
        sp->SP_coef_override = float(0.448500f);
    }break;
    case 11695: // Health Funnel - Rank 7
    {
        sp->SP_coef_override = float(0.448500f);
    }break;
    case 11699: // Drain Life - Rank 5
    {
        sp->SP_coef_override = float(0.143000f);
    }break;
    case 11700: // Drain Life - Rank 6
    {
        sp->SP_coef_override = float(0.143000f);
    }break;
    case 11711: // Curse of Agony - Rank 4
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 11712: // Curse of Agony - Rank 5
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 11713: // Curse of Agony - Rank 6
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 11739: // Shadow Ward - Rank 2
    {
        sp->SP_coef_override = float(0.300000f);
    }break;
    case 11740: // Shadow Ward - Rank 3
    {
        sp->SP_coef_override = float(0.300000f);
    }break;
    case 12505: // Pyroblast - Rank 2
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 12522: // Pyroblast - Rank 3
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 12523: // Pyroblast - Rank 4
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 12524: // Pyroblast - Rank 5
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 12525: // Pyroblast - Rank 6
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 12526: // Pyroblast - Rank 7
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 12809: // Concussion Blow
    {
        sp->AP_coef_override = float(0.750000f);
    }break;
    case 13018: // Blast Wave - Rank 2
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 13019: // Blast Wave - Rank 3
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 13020: // Blast Wave - Rank 4
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 13021: // Blast Wave - Rank 5
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 13031: // Ice Barrier - Rank 2
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 13032: // Ice Barrier - Rank 3
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 13033: // Ice Barrier - Rank 4
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 13218: // Wound Poison - Rank 1
    {
        sp->AP_coef_override = float(0.008000f);
    }break;
    case 13222: // Wound Poison II - Rank 2
    {
        sp->AP_coef_override = float(0.008000f);
    }break;
    case 13223: // Wound Poison III - Rank 3
    {
        sp->AP_coef_override = float(0.008000f);
    }break;
    case 13224: // Wound Poison IV - Rank 4
    {
        sp->AP_coef_override = float(0.008000f);
    }break;
    case 13549: // Serpent Sting - Rank 2
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 13550: // Serpent Sting - Rank 3
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 13551: // Serpent Sting - Rank 4
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 13552: // Serpent Sting - Rank 5
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 13553: // Serpent Sting - Rank 6
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 13554: // Serpent Sting - Rank 7
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 13555: // Serpent Sting - Rank 8
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 13797: // Immolation Trap - Rank 1
    {
        sp->RAP_coef_override = float(0.020000f);
    }break;
    case 14269: // Mongoose Bite - Rank 2
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 14270: // Mongoose Bite - Rank 3
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 14271: // Mongoose Bite - Rank 4
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 14281: // Arcane Shot - Rank 2
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 14282: // Arcane Shot - Rank 3
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 14283: // Arcane Shot - Rank 4
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 14284: // Arcane Shot - Rank 5
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 14285: // Arcane Shot - Rank 6
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 14286: // Arcane Shot - Rank 7
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 14287: // Arcane Shot - Rank 8
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 14288: // Multi-Shot - Rank 2
    {
        sp->RAP_coef_override = float(0.200000f);
    }break;
    case 14289: // Multi-Shot - Rank 3
    {
        sp->RAP_coef_override = float(0.200000f);
    }break;
    case 14290: // Multi-Shot - Rank 4
    {
        sp->RAP_coef_override = float(0.200000f);
    }break;
    case 14298: // Immolation Trap - Rank 2
    {
        sp->RAP_coef_override = float(0.020000f);
    }break;
    case 14299: // Immolation Trap - Rank 3
    {
        sp->RAP_coef_override = float(0.020000f);
    }break;
    case 14300: // Immolation Trap - Rank 4
    {
        sp->RAP_coef_override = float(0.020000f);
    }break;
    case 14301: // Immolation Trap - Rank 5
    {
        sp->RAP_coef_override = float(0.020000f);
    }break;
    case 14914: // Holy Fire - Rank 1
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 15207: // Lightning Bolt - Rank 9
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 15208: // Lightning Bolt - Rank 10
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 15237: // Holy Nova - Rank 1
    {
        sp->SP_coef_override = float(0.160600f);
    }break;
    case 15261: // Holy Fire - Rank 8
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 15262: // Holy Fire - Rank 2
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 15263: // Holy Fire - Rank 3
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 15264: // Holy Fire - Rank 4
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 15265: // Holy Fire - Rank 5
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 15266: // Holy Fire - Rank 6
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 15267: // Holy Fire - Rank 7
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 15407: // Mind Flay - Rank 1
    {
        sp->SP_coef_override = float(0.257000f);
    }break;
    case 15430: // Holy Nova - Rank 2
    {
        sp->SP_coef_override = float(0.160600f);
    }break;
    case 15431: // Holy Nova - Rank 3
    {
        sp->SP_coef_override = float(0.160600f);
    }break;
    case 16343: // Flametongue Weapon Proc - Rank 5
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 16344: // Flametongue Weapon Proc - Rank 6
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 16352: // Frostbrand Attack - Rank 4
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 16353: // Frostbrand Attack - Rank 5
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 16827: // Claw - Rank 1
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 16828: // Claw - Rank 2
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 16829: // Claw - Rank 3
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 16830: // Claw - Rank 4
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 16831: // Claw - Rank 5
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 16832: // Claw - Rank 6
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 16857: // Faerie Fire (Feral)
    {
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 17253: // Bite - Rank 1
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 17255: // Bite - Rank 2
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 17256: // Bite - Rank 3
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 17257: // Bite - Rank 4
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 17258: // Bite - Rank 5
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 17259: // Bite - Rank 6
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 17260: // Bite - Rank 7
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 17261: // Bite - Rank 8
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 17311: // Mind Flay - Rank 2
    {
        sp->SP_coef_override = float(0.257000f);
    }break;
    case 17312: // Mind Flay - Rank 3
    {
        sp->SP_coef_override = float(0.257000f);
    }break;
    case 17313: // Mind Flay - Rank 4
    {
        sp->SP_coef_override = float(0.257000f);
    }break;
    case 17314: // Mind Flay - Rank 5
    {
        sp->SP_coef_override = float(0.257000f);
    }break;
    case 17877: // Shadowburn - Rank 1
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 17919: // Searing Pain - Rank 2
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 17920: // Searing Pain - Rank 3
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 17921: // Searing Pain - Rank 4
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 17922: // Searing Pain - Rank 5
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 17923: // Searing Pain - Rank 6
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 17924: // Soul Fire - Rank 2
    {
        sp->SP_coef_override = float(1.150000f);
    }break;
    case 17925: // Death Coil - Rank 2
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 17926: // Death Coil - Rank 3
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 17962: // Conflagrate
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 18220: // Dark Pact - Rank 1
    {
        sp->SP_coef_override = float(0.960000f);
    }break;
    case 18807: // Mind Flay - Rank 6
    {
        sp->SP_coef_override = float(0.257000f);
    }break;
    case 18809: // Pyroblast - Rank 8
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 18867: // Shadowburn - Rank 2
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 18868: // Shadowburn - Rank 3
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 18869: // Shadowburn - Rank 4
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 18870: // Shadowburn - Rank 5
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 18871: // Shadowburn - Rank 6
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 18937: // Dark Pact - Rank 2
    {
        sp->SP_coef_override = float(0.960000f);
    }break;
    case 18938: // Dark Pact - Rank 3
    {
        sp->SP_coef_override = float(0.960000f);
    }break;
    case 19236: // Desperate Prayer - Rank 1
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 19238: // Desperate Prayer - Rank 2
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 19240: // Desperate Prayer - Rank 3
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 19241: // Desperate Prayer - Rank 4
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 19242: // Desperate Prayer - Rank 5
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 19243: // Desperate Prayer - Rank 6
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 19276: // Devouring Plague - Rank 2
    {
        sp->SP_coef_override = float(0.184900f);
    }break;
    case 19277: // Devouring Plague - Rank 3
    {
        sp->SP_coef_override = float(0.184900f);
    }break;
    case 19278: // Devouring Plague - Rank 4
    {
        sp->SP_coef_override = float(0.184900f);
    }break;
    case 19279: // Devouring Plague - Rank 5
    {
        sp->SP_coef_override = float(0.184900f);
    }break;
    case 19280: // Devouring Plague - Rank 6
    {
        sp->SP_coef_override = float(0.184900f);
    }break;
    case 19750: // Flash of Light - Rank 1
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 19939: // Flash of Light - Rank 2
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 19940: // Flash of Light - Rank 3
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 19941: // Flash of Light - Rank 4
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 19942: // Flash of Light - Rank 5
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 19943: // Flash of Light - Rank 6
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 20116: // Consecration - Rank 2
    {
        sp->SP_coef_override = float(0.040000f);
    }break;
    case 20167: // Seal of Light
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 20168: // Seal of Wisdom
    {
        sp->SP_coef_override = float(0.250000f);
    }break;
    case 20252: // Intercept
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 20267: // Judgement of Light - Rank 1
    {
        sp->SP_coef_override = float(0.100000f);
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 20424: // Seal of Command
    {
        sp->SP_coef_override = float(0.230000f);
    }break;
    case 20467: // Judgement of Command - Rank 1
    {
        sp->SP_coef_override = float(0.250000f);
        sp->AP_coef_override = float(0.160000f);
    }break;
    case 20658: // Execute - Rank 2
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 20660: // Execute - Rank 3
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 20661: // Execute - Rank 4
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 20662: // Execute - Rank 5
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 20922: // Consecration - Rank 3
    {
        sp->SP_coef_override = float(0.040000f);
    }break;
    case 20923: // Consecration - Rank 4
    {
        sp->SP_coef_override = float(0.040000f);
    }break;
    case 20924: // Consecration - Rank 5
    {
        sp->SP_coef_override = float(0.040000f);
    }break;
    case 20925: // Holy Shield - Rank 1
    {
        sp->SP_coef_override = float(0.090000f);
        sp->AP_coef_override = float(0.056000f);
    }break;
    case 20927: // Holy Shield - Rank 2
    {
        sp->SP_coef_override = float(0.090000f);
        sp->AP_coef_override = float(0.056000f);
    }break;
    case 20928: // Holy Shield - Rank 3
    {
        sp->SP_coef_override = float(0.090000f);
        sp->AP_coef_override = float(0.056000f);
    }break;
    case 25742: // Seal of Righteousness
    {
        sp->SP_coef_override = float(0.044000f);
        sp->AP_coef_override = float(0.022000f);
    }break;
    case 23455: // Holy Nova - Rank 1
    {
        sp->SP_coef_override = float(0.303500f);
    }break;
    case 23458: // Holy Nova - Rank 2
    {
        sp->SP_coef_override = float(0.303500f);
    }break;
    case 23459: // Holy Nova - Rank 3
    {
        sp->SP_coef_override = float(0.303500f);
    }break;
    case 24239: // Hammer of Wrath - Rank 3
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 24274: // Hammer of Wrath - Rank 2
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 24275: // Hammer of Wrath - Rank 1
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 24583: // Scorpid Poison - Rank 2
    {
        sp->RAP_coef_override = float(0.015000f);
    }break;
    case 24586: // Scorpid Poison - Rank 3
    {
        sp->RAP_coef_override = float(0.015000f);
    }break;
    case 24587: // Scorpid Poison - Rank 4
    {
        sp->RAP_coef_override = float(0.015000f);
    }break;
    case 24640: // Scorpid Poison - Rank 1
    {
        sp->RAP_coef_override = float(0.015000f);
    }break;
    case 24844: // Lightning Breath - Rank 1
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 24974: // Insect Swarm - Rank 2
    {
        sp->SP_coef_override = float(0.127000f);
    }break;
    case 24975: // Insect Swarm - Rank 3
    {
        sp->SP_coef_override = float(0.127000f);
    }break;
    case 24976: // Insect Swarm - Rank 4
    {
        sp->SP_coef_override = float(0.127000f);
    }break;
    case 24977: // Insect Swarm - Rank 5
    {
        sp->SP_coef_override = float(0.127000f);
    }break;
    case 25008: // Lightning Breath - Rank 2
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 25009: // Lightning Breath - Rank 3
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 25010: // Lightning Breath - Rank 4
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 25011: // Lightning Breath - Rank 5
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 25012: // Lightning Breath - Rank 6
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 25210: // Greater Heal - Rank 6
    {
        sp->SP_coef_override = float(1.613500f);
    }break;
    case 25213: // Greater Heal - Rank 7
    {
        sp->SP_coef_override = float(1.613500f);
    }break;
    case 25217: // Power Word: Shield - Rank 11
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 25218: // Power Word: Shield - Rank 12
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 25221: // Renew - Rank 11
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 25222: // Renew - Rank 12
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 25233: // Flash Heal - Rank 8
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 25234: // Execute - Rank 6
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 25235: // Flash Heal - Rank 9
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 25236: // Execute - Rank 7
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 25264: // Thunder Clap - Rank 7
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 25269: // Revenge - Rank 7
    {
        sp->AP_coef_override = float(0.207000f);
    }break;
    case 25288: // Revenge - Rank 6
    {
        sp->AP_coef_override = float(0.207000f);
    }break;
    case 25292: // Holy Light - Rank 9
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 25294: // Multi-Shot - Rank 5
    {
        sp->RAP_coef_override = float(0.200000f);
    }break;
    case 25295: // Serpent Sting - Rank 9
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 25297: // Healing Touch - Rank 11
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 25298: // Starfire - Rank 7
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 25299: // Rejuvenation - Rank 11
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 25304: // Frostbolt - Rank 11
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 25306: // Fireball - Rank 12
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 25307: // Shadow Bolt - Rank 10
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 25308: // Prayer of Healing - Rank 6
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 25309: // Immolate - Rank 8
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 25311: // Corruption - Rank 7
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 25314: // Greater Heal - Rank 5
    {
        sp->SP_coef_override = float(1.613500f);
    }break;
    case 25315: // Renew - Rank 10
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 25316: // Prayer of Healing - Rank 5
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 25329: // Holy Nova - Rank 7
    {
        sp->SP_coef_override = float(0.303500f);
    }break;
    case 25331: // Holy Nova - Rank 7
    {
        sp->SP_coef_override = float(0.160600f);
    }break;
    case 25345: // Arcane Missiles - Rank 8
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 25349: // Deadly Poison V - Rank 5
    {
        sp->AP_coef_override = float(0.024000f);
    }break;
    case 25357: // Healing Wave - Rank 10
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 25363: // Smite - Rank 9
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 25364: // Smite - Rank 10
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 25367: // Shadow Word: Pain - Rank 9
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 25368: // Shadow Word: Pain - Rank 10
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 25372: // Mind Blast - Rank 10
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 25375: // Mind Blast - Rank 11
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 25384: // Holy Fire - Rank 9
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 25387: // Mind Flay - Rank 7
    {
        sp->SP_coef_override = float(0.257000f);
    }break;
    case 25391: // Healing Wave - Rank 11
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 25396: // Healing Wave - Rank 12
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 25420: // Lesser Healing Wave - Rank 7
    {
        sp->SP_coef_override = float(0.808200f);
    }break;
    case 25422: // Chain Heal - Rank 4
    {
        sp->SP_coef_override = float(0.800000f);
    }break;
    case 25423: // Chain Heal - Rank 5
    {
        sp->SP_coef_override = float(0.800000f);
    }break;
    case 25437: // Desperate Prayer - Rank 7
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 25439: // Chain Lightning - Rank 5
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 25442: // Chain Lightning - Rank 6
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 25448: // Lightning Bolt - Rank 11
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 25449: // Lightning Bolt - Rank 12
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 25454: // Earth Shock - Rank 8
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 25457: // Flame Shock - Rank 7
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 25464: // Frost Shock - Rank 5
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 25467: // Devouring Plague - Rank 7
    {
        sp->SP_coef_override = float(0.184900f);
    }break;
    case 25469: // Lightning Shield - Rank 8
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 25472: // Lightning Shield - Rank 9
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 25488: // Flametongue Weapon Proc - Rank 7
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 25501: // Frostbrand Attack - Rank 6
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 25530: // Attack - Rank 7
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 25550: // Magma Totem - Rank 5
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 25902: // Holy Shock - Rank 3
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 25903: // Holy Shock - Rank 3
    {
        sp->SP_coef_override = float(0.810000f);
    }break;
    case 25911: // Holy Shock - Rank 2
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 25912: // Holy Shock - Rank 1
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 25913: // Holy Shock - Rank 2
    {
        sp->SP_coef_override = float(0.810000f);
    }break;
    case 25914: // Holy Shock - Rank 1
    {
        sp->SP_coef_override = float(0.810000f);
    }break;
    case 26573: // Consecration - Rank 1
    {
        sp->SP_coef_override = float(0.040000f);
    }break;
    case 26839: // Garrote - Rank 7
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 26884: // Garrote - Rank 8
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 26890: // Instant Poison VII - Rank 7
    {
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 26968: // Deadly Poison VI - Rank 6
    {
        sp->AP_coef_override = float(0.024000f);
    }break;
    case 26978: // Healing Touch - Rank 12
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 26979: // Healing Touch - Rank 13
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 26980: // Regrowth - Rank 10
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 26981: // Rejuvenation - Rank 12
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 26982: // Rejuvenation - Rank 13
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 26983: // Tranquility - Rank 5
    {
        sp->SP_coef_override = float(0.538000f);
    }break;
    case 26984: // Wrath - Rank 9
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 26985: // Wrath - Rank 10
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 26986: // Starfire - Rank 8
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 26987: // Moonfire - Rank 11
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 26988: // Moonfire - Rank 12
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 26989: // Entangling Roots - Rank 7
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 27008: // Rip - Rank 7
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 27013: // Insect Swarm - Rank 6
    {
        sp->SP_coef_override = float(0.127000f);
    }break;
    case 27016: // Serpent Sting - Rank 10
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 27019: // Arcane Shot - Rank 9
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 27021: // Multi-Shot - Rank 6
    {
        sp->RAP_coef_override = float(0.200000f);
    }break;
    case 27024: // Immolation Trap - Rank 6
    {
        sp->RAP_coef_override = float(0.020000f);
    }break;
    case 27049: // Claw - Rank 9
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 27050: // Bite - Rank 9
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 27060: // Scorpid Poison - Rank 5
    {
        sp->RAP_coef_override = float(0.015000f);
    }break;
    case 27070: // Fireball - Rank 13
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 27071: // Frostbolt - Rank 12
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 27072: // Frostbolt - Rank 13
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 27073: // Scorch - Rank 8
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 27074: // Scorch - Rank 9
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 27075: // Arcane Missiles - Rank 9
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 27078: // Fire Blast - Rank 8
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 27079: // Fire Blast - Rank 9
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 27080: // Arcane Explosion - Rank 7
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 27082: // Arcane Explosion - Rank 8
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 27086: // Flamestrike - Rank 7
    {
        sp->SP_coef_override = float(0.122000f);
    }break;
    case 27087: // Cone of Cold - Rank 6
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 27088: // Frost Nova - Rank 5
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 27131: // Mana Shield - Rank 7
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 27132: // Pyroblast - Rank 9
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 27133: // Blast Wave - Rank 6
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 27134: // Ice Barrier - Rank 5
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 27135: // Holy Light - Rank 10
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 27136: // Holy Light - Rank 11
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 27137: // Flash of Light - Rank 7
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 27138: // Exorcism - Rank 7
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 27139: // Holy Wrath - Rank 3
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 27173: // Consecration - Rank 6
    {
        sp->SP_coef_override = float(0.040000f);
    }break;
    case 27175: // Holy Shock - Rank 4
    {
        sp->SP_coef_override = float(0.810000f);
    }break;
    case 27176: // Holy Shock - Rank 4
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 27179: // Holy Shield - Rank 4
    {
        sp->SP_coef_override = float(0.090000f);
        sp->AP_coef_override = float(0.056000f);
    }break;
    case 27180: // Hammer of Wrath - Rank 4
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 27187: // Deadly Poison VII - Rank 7
    {
        sp->AP_coef_override = float(0.024000f);
    }break;
    case 27189: // Wound Poison V - Rank 5
    {
        sp->AP_coef_override = float(0.008000f);
    }break;
    case 27209: // Shadow Bolt - Rank 11
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 27210: // Searing Pain - Rank 7
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 27211: // Soul Fire - Rank 3
    {
        sp->SP_coef_override = float(1.150000f);
    }break;
    case 27212: // Rain of Fire - Rank 5
    {
        sp->SP_coef_override = float(0.693200f);
    }break;
    case 27213: // Hellfire - Rank 4
    {
        sp->SP_coef_override = float(0.120000f);
    }break;
    case 27215: // Immolate - Rank 9
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 27216: // Corruption - Rank 8
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 27217: // Drain Soul - Rank 5
    {
        sp->SP_coef_override = float(0.429000f);
    }break;
    case 27218: // Curse of Agony - Rank 7
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 27219: // Drain Life - Rank 7
    {
        sp->SP_coef_override = float(0.143000f);
    }break;
    case 27220: // Drain Life - Rank 8
    {
        sp->SP_coef_override = float(0.143000f);
    }break;
    case 27223: // Death Coil - Rank 4
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 27243: // Seed of Corruption - Rank 1
    {
        sp->SP_coef_override = float(0.250000f);
    }break;
    case 27259: // Health Funnel - Rank 8
    {
        sp->SP_coef_override = float(0.448500f);
    }break;
    case 27263: // Shadowburn - Rank 7
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 27265: // Dark Pact - Rank 4
    {
        sp->SP_coef_override = float(0.960000f);
    }break;
    case 27799: // Holy Nova - Rank 4
    {
        sp->SP_coef_override = float(0.160600f);
    }break;
    case 27800: // Holy Nova - Rank 5
    {
        sp->SP_coef_override = float(0.160600f);
    }break;
    case 27801: // Holy Nova - Rank 6
    {
        sp->SP_coef_override = float(0.160600f);
    }break;
    case 27803: // Holy Nova - Rank 4
    {
        sp->SP_coef_override = float(0.303500f);
    }break;
    case 27804: // Holy Nova - Rank 5
    {
        sp->SP_coef_override = float(0.303500f);
    }break;
    case 27805: // Holy Nova - Rank 6
    {
        sp->SP_coef_override = float(0.303500f);
    }break;
    case 28610: // Shadow Ward - Rank 4
    {
        sp->SP_coef_override = float(0.300000f);
    }break;
    case 29228: // Flame Shock - Rank 6
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 29722: // Incinerate - Rank 1
    {
        sp->SP_coef_override = float(0.713900f);
    }break;
    case 30108: // Unstable Affliction - Rank 1
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 30283: // Shadowfury - Rank 1
    {
        sp->SP_coef_override = float(0.193200f);
    }break;
    case 30357: // Revenge - Rank 8
    {
        sp->AP_coef_override = float(0.207000f);
    }break;
    case 30404: // Unstable Affliction - Rank 2
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 30405: // Unstable Affliction - Rank 3
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 30413: // Shadowfury - Rank 2
    {
        sp->SP_coef_override = float(0.193200f);
    }break;
    case 30414: // Shadowfury - Rank 3
    {
        sp->SP_coef_override = float(0.193200f);
    }break;
    case 30451: // Arcane Blast - Rank 1
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 30455: // Ice Lance - Rank 1
    {
        sp->SP_coef_override = float(0.142900f);
    }break;
    case 30459: // Searing Pain - Rank 8
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 30546: // Shadowburn - Rank 8
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 30824: // Shamanistic Rage
    {
        sp->AP_coef_override = float(0.300000f);
    }break;
    case 30910: // Curse of Doom - Rank 2
    {
        sp->SP_coef_override = float(2.000000f);
    }break;
    case 31117: // Unstable Affliction
    {
        sp->SP_coef_override = float(1.800000f);
    }break;
    case 31661: // Dragon's Breath - Rank 1
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 53742: // Blood Corruption
    case 31803: // Holy Vengeance
    {
        sp->SP_coef_override = float(0.013000f);
        sp->AP_coef_override = float(0.025000f);
    }break;
    case 31893: // Seal of Blood
    {
        sp->SP_coef_override = float(0.020000f);
        sp->AP_coef_override = float(0.030000f);
    }break;
    case 31898: // Judgement of Blood - Rank 1
    {
        sp->SP_coef_override = float(0.250000f);
        sp->AP_coef_override = float(0.160000f);
    }break;
    case 31935: // Avenger's Shield - Rank 1
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 32231: // Incinerate - Rank 2
    {
        sp->SP_coef_override = float(0.713900f);
    }break;
    case 32379: // Shadow Word: Death - Rank 1
    {
        sp->SP_coef_override = float(0.429600f);
    }break;
    case 32546: // Binding Heal - Rank 1
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 32593: // Earth Shield - Rank 2
    {
        sp->SP_coef_override = float(0.476100f);
    }break;
    case 32594: // Earth Shield - Rank 3
    {
        sp->SP_coef_override = float(0.476100f);
    }break;
    case 32699: // Avenger's Shield - Rank 2
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 32700: // Avenger's Shield - Rank 3
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 32996: // Shadow Word: Death - Rank 2
    {
        sp->SP_coef_override = float(0.429600f);
    }break;
    case 33041: // Dragon's Breath - Rank 2
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 33042: // Dragon's Breath - Rank 3
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 33043: // Dragon's Breath - Rank 4
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 33073: // Holy Shock - Rank 5
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 33074: // Holy Shock - Rank 5
    {
        sp->SP_coef_override = float(0.810000f);
    }break;
    case 33405: // Ice Barrier - Rank 6
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 33745: // Lacerate - Rank 1
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 33763: // Lifebloom - Rank 1
    {
        sp->SP_coef_override = float(0.095180f);
    }break;
    case 33933: // Blast Wave - Rank 7
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 33938: // Pyroblast - Rank 10
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 34428: // Victory Rush
    {
        sp->AP_coef_override = float(0.450000f);
    }break;
    case 34861: // Circle of Healing - Rank 1
    {
        sp->SP_coef_override = float(0.402000f);
    }break;
    case 34863: // Circle of Healing - Rank 2
    {
        sp->SP_coef_override = float(0.402000f);
    }break;
    case 34864: // Circle of Healing - Rank 3
    {
        sp->SP_coef_override = float(0.402000f);
    }break;
    case 34865: // Circle of Healing - Rank 4
    {
        sp->SP_coef_override = float(0.402000f);
    }break;
    case 34866: // Circle of Healing - Rank 5
    {
        sp->SP_coef_override = float(0.402000f);
    }break;
    case 34889: // Fire Breath - Rank 1
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 34914: // Vampiric Touch - Rank 1
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 34916: // Vampiric Touch - Rank 2
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 34917: // Vampiric Touch - Rank 3
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 35290: // Gore - Rank 1
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 35291: // Gore - Rank 2
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 35292: // Gore - Rank 3
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 35293: // Gore - Rank 4
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 35294: // Gore - Rank 5
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 35295: // Gore - Rank 6
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 35323: // Fire Breath - Rank 2
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 35387: // Poison Spit - Rank 1
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 35389: // Poison Spit - Rank 2
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 35392: // Poison Spit - Rank 3
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 36916: // Mongoose Bite - Rank 5
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 38692: // Fireball - Rank 14
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 38697: // Frostbolt - Rank 14
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 38699: // Arcane Missiles - Rank 10
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 38704: // Arcane Missiles - Rank 11
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 38764: // Gouge - Rank 6
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 41637: // Prayer of Mending
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 42198: // Blizzard - Rank 7
    {
        sp->SP_coef_override = float(0.143700f);
    }break;
    case 42208: // Blizzard - Rank 1
    {
        sp->SP_coef_override = float(0.143700f);
    }break;
    case 42209: // Blizzard - Rank 2
    {
        sp->SP_coef_override = float(0.143700f);
    }break;
    case 42210: // Blizzard - Rank 3
    {
        sp->SP_coef_override = float(0.143700f);
    }break;
    case 42211: // Blizzard - Rank 4
    {
        sp->SP_coef_override = float(0.143700f);
    }break;
    case 42212: // Blizzard - Rank 5
    {
        sp->SP_coef_override = float(0.143700f);
    }break;
    case 42213: // Blizzard - Rank 6
    {
        sp->SP_coef_override = float(0.143700f);
    }break;
    case 42230: // Hurricane - Rank 4
    {
        sp->SP_coef_override = float(0.128980f);
    }break;
    case 42231: // Hurricane - Rank 1
    {
        sp->SP_coef_override = float(0.128980f);
    }break;
    case 42232: // Hurricane - Rank 2
    {
        sp->SP_coef_override = float(0.128980f);
    }break;
    case 42233: // Hurricane - Rank 3
    {
        sp->SP_coef_override = float(0.128980f);
    }break;
    case 42832: // Fireball - Rank 15
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 42833: // Fireball - Rank 16
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 42841: // Frostbolt - Rank 15
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 42842: // Frostbolt - Rank 16
    {
        sp->SP_coef_override = float(0.814300f);
    }break;
    case 42843: // Arcane Missiles - Rank 12
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 42846: // Arcane Missiles - Rank 13
    {
        sp->SP_coef_override = float(0.285700f);
    }break;
    case 42858: // Scorch - Rank 10
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 42859: // Scorch - Rank 11
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 42872: // Fire Blast - Rank 10
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 42873: // Fire Blast - Rank 11
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 42890: // Pyroblast - Rank 11
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 42891: // Pyroblast - Rank 12
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 42894: // Arcane Blast - Rank 2
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 42896: // Arcane Blast - Rank 3
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 42897: // Arcane Blast - Rank 4
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 42913: // Ice Lance - Rank 2
    {
        sp->SP_coef_override = float(0.142900f);
    }break;
    case 42914: // Ice Lance - Rank 3
    {
        sp->SP_coef_override = float(0.142900f);
    }break;
    case 42917: // Frost Nova - Rank 6
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 42920: // Arcane Explosion - Rank 9
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 42921: // Arcane Explosion - Rank 10
    {
        sp->SP_coef_override = float(0.212800f);
    }break;
    case 42925: // Flamestrike - Rank 8
    {
        sp->SP_coef_override = float(0.122000f);
    }break;
    case 42926: // Flamestrike - Rank 9
    {
        sp->SP_coef_override = float(0.122000f);
    }break;
    case 42930: // Cone of Cold - Rank 7
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 42931: // Cone of Cold - Rank 8
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 42937: // Blizzard - Rank 8
    {
        sp->SP_coef_override = float(0.143700f);
    }break;
    case 42938: // Blizzard - Rank 9
    {
        sp->SP_coef_override = float(0.143700f);
    }break;
    case 42944: // Blast Wave - Rank 8
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 42945: // Blast Wave - Rank 9
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 42949: // Dragon's Breath - Rank 5
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 42950: // Dragon's Breath - Rank 6
    {
        sp->SP_coef_override = float(0.193600f);
    }break;
    case 43019: // Mana Shield - Rank 8
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 43020: // Mana Shield - Rank 9
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 43038: // Ice Barrier - Rank 7
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 43039: // Ice Barrier - Rank 8
    {
        sp->SP_coef_override = float(0.805300f);
    }break;
    case 44425: // Arcane Barrage - Rank 1
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 44457: // Living Bomb - Rank 1
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 44614: // Frostfire Bolt - Rank 1
    {
        sp->SP_coef_override = float(0.857100f);
    }break;
    case 44780: // Arcane Barrage - Rank 2
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 44781: // Arcane Barrage - Rank 3
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 46968: // Shockwave
    {
        sp->AP_coef_override = float(0.750000f);
    }break;
    case 47470: // Execute - Rank 8
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 47471: // Execute - Rank 9
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 47501: // Thunder Clap - Rank 8
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 47502: // Thunder Clap - Rank 9
    {
        sp->AP_coef_override = float(0.120000f);
    }break;
    case 47610: // Frostfire Bolt - Rank 2
    {
        sp->SP_coef_override = float(0.857100f);
    }break;
    case 47666: // Penance - Rank 1
    {
        sp->SP_coef_override = float(0.229000f);
    }break;
    case 47750: // Penance - Rank 1
    {
        sp->SP_coef_override = float(0.535000f);
    }break;
    case 47808: // Shadow Bolt - Rank 12
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 47809: // Shadow Bolt - Rank 13
    {
        sp->SP_coef_override = float(0.856900f);
    }break;
    case 47810: // Immolate - Rank 10
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 47811: // Immolate - Rank 11
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 47812: // Corruption - Rank 9
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 47813: // Corruption - Rank 10
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 47814: // Searing Pain - Rank 9
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 47815: // Searing Pain - Rank 10
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 47819: // Rain of Fire - Rank 6
    {
        sp->SP_coef_override = float(0.693200f);
    }break;
    case 47820: // Rain of Fire - Rank 7
    {
        sp->SP_coef_override = float(0.693200f);
    }break;
    case 47823: // Hellfire - Rank 5
    {
        sp->SP_coef_override = float(0.120000f);
    }break;
    case 47824: // Soul Fire - Rank 5
    {
        sp->SP_coef_override = float(1.150000f);
    }break;
    case 47825: // Soul Fire - Rank 6
    {
        sp->SP_coef_override = float(1.150000f);
    }break;
    case 47826: // Shadowburn - Rank 9
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 47827: // Shadowburn - Rank 10
    {
        sp->SP_coef_override = float(0.429300f);
    }break;
    case 47835: // Seed of Corruption - Rank 2
    {
        sp->SP_coef_override = float(0.250000f);
    }break;
    case 47836: // Seed of Corruption - Rank 3
    {
        sp->SP_coef_override = float(0.250000f);
    }break;
    case 47837: // Incinerate - Rank 3
    {
        sp->SP_coef_override = float(0.713900f);
    }break;
    case 47838: // Incinerate - Rank 4
    {
        sp->SP_coef_override = float(0.713900f);
    }break;
    case 47841: // Unstable Affliction - Rank 4
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 47843: // Unstable Affliction - Rank 5
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 47846: // Shadowfury - Rank 4
    {
        sp->SP_coef_override = float(0.193200f);
    }break;
    case 47847: // Shadowfury - Rank 5
    {
        sp->SP_coef_override = float(0.193200f);
    }break;
    case 47855: // Drain Soul - Rank 6
    {
        sp->SP_coef_override = float(0.429000f);
    }break;
    case 47856: // Health Funnel - Rank 9
    {
        sp->SP_coef_override = float(0.448500f);
    }break;
    case 47857: // Drain Life - Rank 9
    {
        sp->SP_coef_override = float(0.143000f);
    }break;
    case 47859: // Death Coil - Rank 5
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 47860: // Death Coil - Rank 6
    {
        sp->SP_coef_override = float(0.214000f);
    }break;
    case 47863: // Curse of Agony - Rank 8
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 47864: // Curse of Agony - Rank 9
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 47867: // Curse of Doom - Rank 3
    {
        sp->SP_coef_override = float(2.000000f);
    }break;
    case 47890: // Shadow Ward - Rank 5
    {
        sp->SP_coef_override = float(0.300000f);
    }break;
    case 47891: // Shadow Ward - Rank 6
    {
        sp->SP_coef_override = float(0.300000f);
    }break;
    case 48062: // Greater Heal - Rank 8
    {
        sp->SP_coef_override = float(1.613500f);
    }break;
    case 48063: // Greater Heal - Rank 9
    {
        sp->SP_coef_override = float(1.613500f);
    }break;
    case 48065: // Power Word: Shield - Rank 13
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 48066: // Power Word: Shield - Rank 14
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 48067: // Renew - Rank 13
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 48068: // Renew - Rank 14
    {
        sp->SP_coef_override = float(0.360000f);
    }break;
    case 48070: // Flash Heal - Rank 10
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 48071: // Flash Heal - Rank 11
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 48072: // Prayer of Healing - Rank 7
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 48075: // Holy Nova - Rank 8
    {
        sp->SP_coef_override = float(0.303500f);
    }break;
    case 48076: // Holy Nova - Rank 9
    {
        sp->SP_coef_override = float(0.303500f);
    }break;
    case 48077: // Holy Nova - Rank 8
    {
        sp->SP_coef_override = float(0.160600f);
    }break;
    case 48078: // Holy Nova - Rank 9
    {
        sp->SP_coef_override = float(0.160600f);
    }break;
    case 48088: // Circle of Healing - Rank 6
    {
        sp->SP_coef_override = float(0.402000f);
    }break;
    case 48089: // Circle of Healing - Rank 7
    {
        sp->SP_coef_override = float(0.402000f);
    }break;
    case 48119: // Binding Heal - Rank 2
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 48120: // Binding Heal - Rank 3
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 48122: // Smite - Rank 11
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 48123: // Smite - Rank 12
    {
        sp->SP_coef_override = float(0.714000f);
    }break;
    case 48124: // Shadow Word: Pain - Rank 11
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 48125: // Shadow Word: Pain - Rank 12
    {
        sp->SP_coef_override = float(0.182900f);
    }break;
    case 48126: // Mind Blast - Rank 12
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 48127: // Mind Blast - Rank 13
    {
        sp->SP_coef_override = float(0.428000f);
    }break;
    case 48134: // Holy Fire - Rank 10
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 48135: // Holy Fire - Rank 11
    {
        sp->SP_coef_override = float(0.024000f);
    }break;
    case 48155: // Mind Flay - Rank 8
    {
        sp->SP_coef_override = float(0.257000f);
    }break;
    case 48156: // Mind Flay - Rank 9
    {
        sp->SP_coef_override = float(0.257000f);
    }break;
    case 48157: // Shadow Word: Death - Rank 3
    {
        sp->SP_coef_override = float(0.429600f);
    }break;
    case 48158: // Shadow Word: Death - Rank 4
    {
        sp->SP_coef_override = float(0.429600f);
    }break;
    case 48159: // Vampiric Touch - Rank 4
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 48160: // Vampiric Touch - Rank 5
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 48172: // Desperate Prayer - Rank 8
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 48173: // Desperate Prayer - Rank 9
    {
        sp->SP_coef_override = float(0.806800f);
    }break;
    case 48181: // Haunt - Rank 1
    {
        sp->SP_coef_override = float(0.479300f);
    }break;
    case 48299: // Devouring Plague - Rank 8
    {
        sp->SP_coef_override = float(0.184900f);
    }break;
    case 48300: // Devouring Plague - Rank 9
    {
        sp->SP_coef_override = float(0.184900f);
    }break;
    case 48377: // Healing Touch - Rank 14
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 48378: // Healing Touch - Rank 15
    {
        sp->SP_coef_override = float(1.610400f);
    }break;
    case 48438: // Wild Growth - Rank 1
    {
        sp->SP_coef_override = float(0.115050f);
    }break;
    case 48440: // Rejuvenation - Rank 14
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 48441: // Rejuvenation - Rank 15
    {
        sp->SP_coef_override = float(0.376040f);
    }break;
    case 48442: // Regrowth - Rank 11
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 48443: // Regrowth - Rank 12
    {
        sp->SP_coef_override = float(0.188000f);
    }break;
    case 48446: // Tranquility - Rank 6
    {
        sp->SP_coef_override = float(0.538000f);
    }break;
    case 48447: // Tranquility - Rank 7
    {
        sp->SP_coef_override = float(0.538000f);
    }break;
    case 48450: // Lifebloom - Rank 2
    {
        sp->SP_coef_override = float(0.095180f);
    }break;
    case 48451: // Lifebloom - Rank 3
    {
        sp->SP_coef_override = float(0.095180f);
    }break;
    case 48459: // Wrath - Rank 11
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 48461: // Wrath - Rank 12
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 48462: // Moonfire - Rank 13
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 48463: // Moonfire - Rank 14
    {
        sp->SP_coef_override = float(0.130000f);
    }break;
    case 48464: // Starfire - Rank 9
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 48465: // Starfire - Rank 10
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 48466: // Hurricane - Rank 5
    {
        sp->SP_coef_override = float(0.128980f);
    }break;
    case 48468: // Insect Swarm - Rank 7
    {
        sp->SP_coef_override = float(0.127000f);
    }break;
    case 48567: // Lacerate - Rank 2
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 48675: // Garrote - Rank 9
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 48676: // Garrote - Rank 10
    {
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 48781: // Holy Light - Rank 12
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 48782: // Holy Light - Rank 13
    {
        sp->SP_coef_override = float(1.660000f);
    }break;
    case 48784: // Flash of Light - Rank 8
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 48785: // Flash of Light - Rank 9
    {
        sp->SP_coef_override = float(1.000000f);
    }break;
    case 48800: // Exorcism - Rank 8
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 48801: // Exorcism - Rank 9
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 48805: // Hammer of Wrath - Rank 5
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 48806: // Hammer of Wrath - Rank 6
    {
        sp->SP_coef_override = float(0.150000f);
        sp->AP_coef_override = float(0.150000f);
    }break;
    case 48816: // Holy Wrath - Rank 4
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 48817: // Holy Wrath - Rank 5
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 48818: // Consecration - Rank 7
    {
        sp->SP_coef_override = float(0.040000f);
    }break;
    case 48819: // Consecration - Rank 8
    {
        sp->SP_coef_override = float(0.040000f);
    }break;
    case 48820: // Holy Shock - Rank 6
    {
        sp->SP_coef_override = float(0.810000f);
    }break;
    case 48821: // Holy Shock - Rank 7
    {
        sp->SP_coef_override = float(0.810000f);
    }break;
    case 48822: // Holy Shock - Rank 6
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 48823: // Holy Shock - Rank 7
    {
        sp->SP_coef_override = float(0.428600f);
    }break;
    case 48826: // Avenger's Shield - Rank 4
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 48827: // Avenger's Shield - Rank 5
    {
        sp->SP_coef_override = float(0.070000f);
        sp->AP_coef_override = float(0.070000f);
    }break;
    case 48951: // Holy Shield - Rank 5
    {
        sp->SP_coef_override = float(0.090000f);
        sp->AP_coef_override = float(0.056000f);
    }break;
    case 48952: // Holy Shield - Rank 6
    {
        sp->SP_coef_override = float(0.090000f);
        sp->AP_coef_override = float(0.056000f);
    }break;
    case 49000: // Serpent Sting - Rank 11
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 49001: // Serpent Sting - Rank 12
    {
        sp->RAP_coef_override = float(0.040000f);
    }break;
    case 49044: // Arcane Shot - Rank 10
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 49045: // Arcane Shot - Rank 11
    {
        sp->RAP_coef_override = float(0.150000f);
    }break;
    case 49047: // Multi-Shot - Rank 7
    {
        sp->RAP_coef_override = float(0.200000f);
    }break;
    case 49048: // Multi-Shot - Rank 8
    {
        sp->RAP_coef_override = float(0.200000f);
    }break;
    case 49053: // Immolation Trap - Rank 7
    {
        sp->RAP_coef_override = float(0.020000f);
    }break;
    case 49054: // Immolation Trap - Rank 8
    {
        sp->RAP_coef_override = float(0.020000f);
    }break;
    case 49230: // Earth Shock - Rank 9
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 49231: // Earth Shock - Rank 10
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 49232: // Flame Shock - Rank 8
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 49233: // Flame Shock - Rank 9
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 49235: // Frost Shock - Rank 6
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 49236: // Frost Shock - Rank 7
    {
        sp->SP_coef_override = float(0.385800f);
    }break;
    case 49237: // Lightning Bolt - Rank 13
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 49238: // Lightning Bolt - Rank 14
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 49270: // Chain Lightning - Rank 7
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 49271: // Chain Lightning - Rank 8
    {
        sp->SP_coef_override = float(0.400000f);
    }break;
    case 49272: // Healing Wave - Rank 13
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 49273: // Healing Wave - Rank 14
    {
        sp->SP_coef_override = float(1.610600f);
    }break;
    case 49275: // Lesser Healing Wave - Rank 8
    {
        sp->SP_coef_override = float(0.808200f);
    }break;
    case 49276: // Lesser Healing Wave - Rank 9
    {
        sp->SP_coef_override = float(0.808200f);
    }break;
    case 49280: // Lightning Shield - Rank 10
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 49281: // Lightning Shield - Rank 11
    {
        sp->SP_coef_override = float(0.330000f);
    }break;
    case 49283: // Earth Shield - Rank 4
    {
        sp->SP_coef_override = float(0.476100f);
    }break;
    case 49284: // Earth Shield - Rank 5
    {
        sp->SP_coef_override = float(0.476100f);
    }break;
    case 49799: // Rip - Rank 8
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 49800: // Rip - Rank 9
    {
        sp->AP_coef_override = float(0.010000f);
    }break;
    case 49821: // Mind Sear - Rank 1
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 50288: // Starfall - Rank 1
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 50294: // Starfall - Rank 1
    {
        sp->SP_coef_override = float(0.012000f);
    }break;
    case 50464: // Nourish - Rank 1
    {
        sp->SP_coef_override = float(0.661100f);
    }break;
    case 50516: // Typhoon - Rank 1
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 50590: // Immolation - Rank 1
    {
        sp->SP_coef_override = float(0.162200f);
    }break;
    case 50796: // Chaos Bolt - Rank 1
    {
        sp->SP_coef_override = float(0.713900f);
    }break;
    case 51505: // Lava Burst - Rank 1
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 52041: // Healing Stream Totem
    {
        sp->SP_coef_override = float(0.045000f);
    }break;
    case 52046: // Healing Stream Totem
    {
        sp->SP_coef_override = float(0.045000f);
    }break;
    case 52047: // Healing Stream Totem
    {
        sp->SP_coef_override = float(0.045000f);
    }break;
    case 52048: // Healing Stream Totem
    {
        sp->SP_coef_override = float(0.045000f);
    }break;
    case 52049: // Healing Stream Totem
    {
        sp->SP_coef_override = float(0.045000f);
    }break;
    case 52050: // Healing Stream Totem
    {
        sp->SP_coef_override = float(0.045000f);
    }break;
    case 52471: // Claw - Rank 10
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 52472: // Claw - Rank 11
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 52473: // Bite - Rank 10
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 52474: // Bite - Rank 11
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 52983: // Penance - Rank 2
    {
        sp->SP_coef_override = float(0.535000f);
    }break;
    case 52984: // Penance - Rank 3
    {
        sp->SP_coef_override = float(0.535000f);
    }break;
    case 52985: // Penance - Rank 4
    {
        sp->SP_coef_override = float(0.535000f);
    }break;
    case 52998: // Penance - Rank 2
    {
        sp->SP_coef_override = float(0.229000f);
    }break;
    case 52999: // Penance - Rank 3
    {
        sp->SP_coef_override = float(0.229000f);
    }break;
    case 53000: // Penance - Rank 4
    {
        sp->SP_coef_override = float(0.229000f);
    }break;
    case 53022: // Mind Sear - Rank 2
    {
        sp->SP_coef_override = float(0.714300f);
    }break;
    case 53188: // Starfall - Rank 2
    {
        sp->SP_coef_override = float(0.012000f);
    }break;
    case 53189: // Starfall - Rank 3
    {
        sp->SP_coef_override = float(0.012000f);
    }break;
    case 53190: // Starfall - Rank 4
    {
        sp->SP_coef_override = float(0.012000f);
    }break;
    case 53191: // Starfall - Rank 2
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 53194: // Starfall - Rank 3
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 53195: // Starfall - Rank 4
    {
        sp->SP_coef_override = float(0.050000f);
    }break;
    case 53223: // Typhoon - Rank 2
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 53225: // Typhoon - Rank 3
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 53226: // Typhoon - Rank 4
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 53248: // Wild Growth - Rank 2
    {
        sp->SP_coef_override = float(0.115050f);
    }break;
    case 53249: // Wild Growth - Rank 3
    {
        sp->SP_coef_override = float(0.115050f);
    }break;
    case 53251: // Wild Growth - Rank 4
    {
        sp->SP_coef_override = float(0.115050f);
    }break;
    case 53301: // Explosive Shot - Rank 1
    {
        sp->RAP_coef_override = float(0.140000f);
    }break;
    case 53308: // Entangling Roots - Rank 8
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 53339: // Mongoose Bite - Rank 6
    {
        sp->AP_coef_override = float(0.200000f);
    }break;
    case 53351: // Kill Shot - Rank 1
    {
        sp->RAP_coef_override = float(0.400000f);
    }break;
    case 53719: // Seal of the Martyr
    {
        sp->SP_coef_override = float(0.250000f);
    }break;
    case 53726: // Judgement of the Martyr - Rank 1
    {
        sp->SP_coef_override = float(0.250000f);
        sp->AP_coef_override = float(0.160000f);
    }break;
    case 55359: // Living Bomb - Rank 2
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 55360: // Living Bomb - Rank 3
    {
        sp->SP_coef_override = float(0.200000f);
    }break;
    case 55458: // Chain Heal - Rank 6
    {
        sp->SP_coef_override = float(0.800000f);
    }break;
    case 55459: // Chain Heal - Rank 7
    {
        sp->SP_coef_override = float(0.800000f);
    }break;
    case 55482: // Fire Breath - Rank 3
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 55483: // Fire Breath - Rank 4
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 55484: // Fire Breath - Rank 5
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 55485: // Fire Breath - Rank 6
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 55555: // Poison Spit - Rank 4
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 55556: // Poison Spit - Rank 5
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 55557: // Poison Spit - Rank 6
    {
        sp->RAP_coef_override = float(0.125000f);
    }break;
    case 55728: // Scorpid Poison - Rank 6
    {
        sp->RAP_coef_override = float(0.015000f);
    }break;
    case 57755: // Heroic Throw
    {
        sp->AP_coef_override = float(0.500000f);
    }break;
    case 57823: // Revenge - Rank 9
    {
        sp->AP_coef_override = float(0.207000f);
    }break;
    case 57964: // Instant Poison VIII - Rank 8
    {
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 57965: // Instant Poison IX - Rank 9
    {
        sp->AP_coef_override = float(0.100000f);
    }break;
    case 57969: // Deadly Poison VIII - Rank 8
    {
        sp->AP_coef_override = float(0.024000f);
    }break;
    case 57970: // Deadly Poison IX - Rank 9
    {
        sp->AP_coef_override = float(0.024000f);
    }break;
    case 57974: // Wound Poison VI - Rank 6
    {
        sp->AP_coef_override = float(0.008000f);
    }break;
    case 57975: // Wound Poison VII - Rank 7
    {
        sp->AP_coef_override = float(0.008000f);
    }break;
    case 58597: // Sacred Shield - Rank 1
    {
        sp->SP_coef_override = float(0.750000f);
    }break;
    case 58700: // Attack - Rank 8
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 58701: // Attack - Rank 9
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 58702: // Attack - Rank 10
    {
        sp->SP_coef_override = float(0.166700f);
    }break;
    case 58732: // Magma Totem - Rank 6
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 58735: // Magma Totem - Rank 7
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 58759: // Healing Stream Totem
    {
        sp->SP_coef_override = float(0.045000f);
    }break;
    case 58760: // Healing Stream Totem
    {
        sp->SP_coef_override = float(0.045000f);
    }break;
    case 58761: // Healing Stream Totem
    {
        sp->SP_coef_override = float(0.045000f);
    }break;
    case 58786: // Flametongue Weapon Proc - Rank 8
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 58787: // Flametongue Weapon Proc - Rank 9
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 58788: // Flametongue Weapon Proc - Rank 10
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 58797: // Frostbrand Attack - Rank 7
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 58798: // Frostbrand Attack - Rank 8
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 58799: // Frostbrand Attack - Rank 9
    {
        sp->SP_coef_override = float(0.100000f);
    }break;
    case 59092: // Dark Pact - Rank 5
    {
        sp->SP_coef_override = float(0.960000f);
    }break;
    case 59161: // Haunt - Rank 2
    {
        sp->SP_coef_override = float(0.479300f);
    }break;
    case 59163: // Haunt - Rank 3
    {
        sp->SP_coef_override = float(0.479300f);
    }break;
    case 59164: // Haunt - Rank 4
    {
        sp->SP_coef_override = float(0.479300f);
    }break;
    case 59170: // Chaos Bolt - Rank 2
    {
        sp->SP_coef_override = float(0.713900f);
    }break;
    case 59171: // Chaos Bolt - Rank 3
    {
        sp->SP_coef_override = float(0.713900f);
    }break;
    case 59172: // Chaos Bolt - Rank 4
    {
        sp->SP_coef_override = float(0.713900f);
    }break;
    case 60043: // Lava Burst - Rank 2
    {
        sp->SP_coef_override = float(0.571400f);
    }break;
    case 60051: // Explosive Shot - Rank 2
    {
        sp->RAP_coef_override = float(0.140000f);
    }break;
    case 60052: // Explosive Shot - Rank 3
    {
        sp->RAP_coef_override = float(0.140000f);
    }break;
    case 60053: // Explosive Shot - Rank 4
    {
        sp->RAP_coef_override = float(0.140000f);
    }break;
    case 60089: // Faerie Fire (Feral)
    {
        sp->AP_coef_override = float(0.050000f);
    }break;
    case 61005: // Kill Shot - Rank 2
    {
        sp->RAP_coef_override = float(0.400000f);
    }break;
    case 61006: // Kill Shot - Rank 3
    {
        sp->RAP_coef_override = float(0.400000f);
    }break;
    case 61295: // Riptide - Rank 1
    {
        sp->SP_coef_override = float(0.180000f);
    }break;
    case 61299: // Riptide - Rank 2
    {
        sp->SP_coef_override = float(0.180000f);
    }break;
    case 61300: // Riptide - Rank 3
    {
        sp->SP_coef_override = float(0.180000f);
    }break;
    case 61301: // Riptide - Rank 4
    {
        sp->SP_coef_override = float(0.180000f);
    }break;
    case 61384: // Typhoon - Rank 5
    {
        sp->SP_coef_override = float(0.193000f);
    }break;
    case 61650: // Fire Nova - Rank 8
    {
        sp->SP_coef_override = float(0.214200f);
    }break;
    case 61654: // Fire Nova - Rank 9
    {
        sp->SP_coef_override = float(0.214200f);
    }break;
    case 62124: // Hand of Reckoning
    {
        sp->SP_coef_override = float(0.085000f);
    }break;
    case 62606: // Savage Defense
    {
        sp->AP_coef_override = float(0.250000f);
    }break;
    }
    }

    //Settings for special cases
    // This is unused anymore but until we remove it completely just set the values.
    QueryResult* resultx = WorldDatabase.Query("SELECT * FROM spell_coef_override");
    if (resultx != NULL)
    {
        do
        {
            Field* f;
            f = resultx->Fetch();
            sp = dbcSpell.LookupEntryForced(f[0].GetUInt32());
            if (sp != NULL)
            {
                sp->Dspell_coef_override = f[2].GetFloat();
                sp->OTspell_coef_override = f[3].GetFloat();
            }
            else
                Log.Error("SpellCoefOverride", "Has nonexistent spell %u.", f[0].GetUInt32());
        } while (resultx->NextRow());
        delete resultx;
    }

	/* Ritual of Summoning summons a GameObject that triggers an inexistant spell.
	 * This will copy an existant Summon Player spell used for another Ritual Of Summoning
	 * to the one taught by Warlock trainers.
	 */
	sp = CheckAndReturnSpellEntry(7720);
	if(sp)
	{
		const uint32 ritOfSummId = 62330;
		CreateDummySpell(ritOfSummId);
		SpellEntry * ritOfSumm = dbcSpell.LookupEntryForced(ritOfSummId);
		if(ritOfSumm != NULL)
		{
			memcpy(ritOfSumm, sp, sizeof(SpellEntry));
			ritOfSumm->Id = ritOfSummId;
		}
	}
}