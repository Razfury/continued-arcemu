/*
* Continued-ArcEmu MMORPG Server
* Copyright (C) 2021 Continued-ArcEmu Team <https://github.com/c-arcemu/continued-arcemu>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Setup.h"

class DrakTharonKeepScript : public MoonInstanceScript
{
public:
	uint32		mTrollgoreGUID;
	uint32		mNovosGUID;
	uint32		mKingDredGUID;
	uint32		mTharonjaGUID;

	enum CreatureEntries
	{
		CN_TROLLGORE = 26630,
		CN_NOVOS_THE_SUMMONER = 26631,
		CN_KING_DRED = 27483,
		CN_THARONJA = 26632
	};

	MOONSCRIPT_INSTANCE_FACTORY_FUNCTION(DrakTharonKeepScript, MoonInstanceScript);
	DrakTharonKeepScript(MapMgr* pMapMgr) : MoonInstanceScript(pMapMgr)
	{
		mTrollgoreGUID = 0;
		mNovosGUID = 0;
		mKingDredGUID = 0;
		mTharonjaGUID = 0;

		GetInstance()->m_BossGUID1 = 0;
		GetInstance()->m_BossGUID2 = 0;
		GetInstance()->m_BossGUID3 = 0;
		GetInstance()->m_BossGUID4 = 0;
	};

	void OnCreaturePushToWorld(Creature* pCreature)
	{
		switch (pCreature->GetEntry())
		{
		case CN_TROLLGORE:
		{
			mTrollgoreGUID = pCreature->GetLowGUID();
			GetInstance()->m_BossGUID1 = mTrollgoreGUID;
			mEncounters.insert(EncounterMap::value_type(CN_TROLLGORE, BossData(0, mTrollgoreGUID)));
		}
		break;
		case CN_NOVOS_THE_SUMMONER:
		{
			mNovosGUID = pCreature->GetLowGUID();
			GetInstance()->m_BossGUID2 = mNovosGUID;
			mEncounters.insert(EncounterMap::value_type(CN_NOVOS_THE_SUMMONER, BossData(0, mNovosGUID)));
		}
		break;
		case CN_KING_DRED:
		{
			mKingDredGUID = pCreature->GetLowGUID();
			GetInstance()->m_BossGUID3 = mKingDredGUID;
			mEncounters.insert(EncounterMap::value_type(CN_KING_DRED, BossData(0, mKingDredGUID)));
		}
		break;
		case CN_THARONJA:
		{
			mTharonjaGUID = pCreature->GetLowGUID();
			GetInstance()->m_BossGUID4 = mTharonjaGUID;
			mEncounters.insert(EncounterMap::value_type(CN_THARONJA, BossData(0, mTharonjaGUID)));
		}
		break;
		};
	};

	/*void OnGameObjectPushToWorld(GameObject* pGameObject)
	{
		switch (pGameObject->GetEntry())
		{
		default:
			break;
		}

		ParentClass::OnGameObjectPushToWorld(pGameObject);
	};*/

	void SetInstanceData(uint32 pType, uint32 pIndex, uint32 pData)
	{
		EncounterMap::iterator Iter = mEncounters.find(pIndex);
		if (Iter == mEncounters.end())
			return;

		(*Iter).second.mState = (EncounterState)pData;
	};

	uint32 GetInstanceData(uint32 pType, uint32 pIndex)
	{
		if (pType != Data_EncounterState || pIndex == 0)
			return 0;

		EncounterMap::iterator Iter = mEncounters.find(pIndex);
		if (Iter == mEncounters.end())
			return 0;

		return (*Iter).second.mState;
	};

	void OnCreatureDeath(Creature* pVictim, Unit* pKiller)
	{
		EncounterMap::iterator Iter = mEncounters.find(pVictim->GetEntry());
		if (Iter == mEncounters.end())
			return;

		(*Iter).second.mState = State_Finished;

		GameObject* pDoors = NULL;
		switch (pVictim->GetEntry())
		{
		case CN_TROLLGORE:
		{
			SetInstanceData(Data_EncounterState, CN_TROLLGORE, State_Finished);
		}
		break;
		case CN_NOVOS_THE_SUMMONER:
		{
			SetInstanceData(Data_EncounterState, CN_NOVOS_THE_SUMMONER, State_Finished);
		}
		break;
		case CN_KING_DRED:
		{
			SetInstanceData(Data_EncounterState, CN_KING_DRED, State_Finished);
		}
		break;
		case CN_THARONJA:
		{
			SetInstanceData(Data_EncounterState, CN_THARONJA, State_Finished);
		}
		break;
		};
	};

};

/*
 Trollgore - TOO EASY!!
 TODO:
  - Whole corpses/consume thingo is wrong
 NOTES:
 Core doesn't support auras on corpses, we are currently unable to script this blizzlike
*/

enum TrollgoreSpells
{
	SPELL_INFECTED_WOUND = 49637,
	SPELL_CRUSH = 49639,
	SPELL_CORPSE_EXPLODE = 49555,
	SPELL_CORPSE_EXPLODE_DAMAGE = 49618,
	SPELL_CONSUME = 49380,
	SPELL_CONSUME_BUFF = 49381,
	SPELL_CONSUME_BUFF_H = 59805,

	SPELL_SUMMON_INVADER_A = 49456,
	SPELL_SUMMON_INVADER_B = 49457,
	SPELL_SUMMON_INVADER_C = 49458, // can't find any sniffs

	SPELL_INVADER_TAUNT = 49405
};

enum TrollgoreMisc
{
	NPC_TROLLGORE = 26630,

	DATA_CONSUMPTION_JUNCTION = 1,
	POINT_LANDING = 1
};

enum TrollgoreEvents
{
	EVENT_CONSUME = 1,
	EVENT_CRUSH,
	EVENT_INFECTED_WOUND,
	EVENT_CORPSE_EXPLODE,
	EVENT_SPAWN
};


Location const Landing = { -263.0534f, -660.8658f, 26.50903f, 0.0f };

class TrollgoreAI : public AICreatureScript
{
	AI_CREATURE_SCRIPT_FUNCTION(TrollgoreAI, AICreatureScript);
	TrollgoreAI(Creature* pCreature) : AICreatureScript(pCreature)
	{
		Initialize();
	};

	void Initialize()
	{
		_consumptionJunction = true;
	}

	void EnterCombat(Unit* pTarget)
	{
		Emote("Mo' guts! Mo' blood! Mo' food!", Text_Yell, 13181);
		events.ScheduleEvent(EVENT_CONSUME, 15000);
		events.ScheduleEvent(EVENT_CRUSH, urand(1000, 5000));
		events.ScheduleEvent(EVENT_INFECTED_WOUND, urand(10000, 60000));
		events.ScheduleEvent(EVENT_CORPSE_EXPLODE, 3000);
		events.ScheduleEvent(EVENT_SPAWN, urand(30000, 40000));
		ParentClass::EnterCombat(pTarget);
	};

	void KilledUnit(Unit* pVictim)
	{
		Emote("Me gonna carve you and eat you!", Text_Yell, 13185);
		ParentClass::KilledUnit(pVictim);
	}

	void JustDied(Unit* pKiller)
	{
		Emote("Braaaagh!", Text_Yell, 13183);
		ParentClass::JustDied(pKiller);
	}

	void OnCombatStop(Unit* pTarget)
	{
		if (mInstance)
			mInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_Performed);

		ParentClass::OnCombatStop(pTarget);
	};

	void UpdateAI()
	{
		events.Update(1000);

		if (_unit->IsCasting())
			return;

		while (uint32 eventId = events.ExecuteEvent())
		{
			switch (eventId)
			{
			case EVENT_CONSUME:
				Emote("So hungry! Must feed!", Text_Yell, 13182);
				_unit->DoCastAOE(SPELL_CONSUME);
				events.ScheduleEvent(EVENT_CONSUME, 15000);
				break;
			case EVENT_CRUSH:
				_unit->DoCastVictim(SPELL_CRUSH);
				events.ScheduleEvent(EVENT_CRUSH, urand(10000, 15000));
				break;
			case EVENT_INFECTED_WOUND:
				_unit->DoCastVictim(SPELL_INFECTED_WOUND);
				events.ScheduleEvent(EVENT_INFECTED_WOUND, urand(25000, 35000));
				break;
			/*case EVENT_CORPSE_EXPLODE:
				Talk(SAY_EXPLODE);
				DoCastAOE(SPELL_CORPSE_EXPLODE);
				events.ScheduleEvent(EVENT_CORPSE_EXPLODE, urand(15000, 19000));
				break;*/
			/*case EVENT_SPAWN:
				for (uint8 i = 0; i < 3; ++i)
					if (Creature* trigger = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_TROLLGORE_INVADER_SUMMONER_1 + i)))
						trigger->CastSpell(trigger, RAND(SPELL_SUMMON_INVADER_A, SPELL_SUMMON_INVADER_B, SPELL_SUMMON_INVADER_C), true, NULL, NULL, me->GetGUID());

				events.ScheduleEvent(EVENT_SPAWN, urand(30000, 40000));
				break;*/
			default:
				break;
			}
		}

		if (_consumptionJunction)
		{
			if (_unit->GetAuraStackCount(SPELL_CONSUME_BUFF) > 9 || _unit->GetAuraStackCount(SPELL_CONSUME_BUFF_H) > 9)
			{
				_consumptionJunction = false;
			}
		}

		ParentClass::UpdateAI();
	}


private:
	bool _consumptionJunction;
};

enum Spells_Novos
{
	SPELL_BEAM_CHANNEL = 52106,
	SPELL_ARCANE_BLAST = 49198,
	//SPELL_ARCANE_FIELD = 47346,
	SPELL_SUMMON_FETID_TROLL_CORPSE = 49103,
	SPELL_SUMMON_HULKING_CORPSE = 49104,
	SPELL_SUMMON_RISEN_SHADOWCASTER = 49105,
	SPELL_SUMMON_CRYSTAL_HANDLER = 49179,
	SPELL_DESPAWN_CRYSTAL_HANDLER = 51403,

	SPELL_SUMMON_MINIONS = 59910,
	SPELL_COPY_OF_SUMMON_MINIONS = 59933,
	SPELL_BLIZZARD = 49034,
	SPELL_FROSTBOLT = 49037,
	SPELL_TOUCH_OF_MISERY = 50090
};

enum Misc_Novos
{
	NPC_NOVOS_THE_SUMMONER = 26631,
	NPC_CRYSTAL_CHANNEL_TARGET = 26712,
	NPC_CRYSTAL_HANDLER = 26627,

	EVENT_SUMMON_FETID_TROLL = 1,
	EVENT_SUMMON_SHADOWCASTER = 2,
	EVENT_SUMMON_HULKING_CORPSE = 3,
	EVENT_SUMMON_CRYSTAL_HANDLER = 4,
	EVENT_CAST_OFFENSIVE_SPELL = 5,
	//	EVENT_KILL_TALK = 6,
	EVENT_CHECK_PHASE = 7,
	EVENT_SPELL_SUMMON_MINIONS = 8
};

class NovosTheSummonerAI : public AICreatureScript
{
	AI_CREATURE_SCRIPT_FUNCTION(NovosTheSummonerAI, AICreatureScript);
	NovosTheSummonerAI(Creature* pCreature) : AICreatureScript(pCreature)
	{
	}

	void OnLoad()
	{
		_unit->m_countHelper = 0;
		_unit->Root();
		ParentClass::OnLoad();
	}

	void EnterCombat(Unit* mAttacker)
	{
		RegisterAIUpdateEvent(1000);
		Emote("The chill that you feel is the herald of your doom! ", Text_Yell, 13173);
		events.ScheduleEvent(EVENT_SUMMON_FETID_TROLL, 3000);
		events.ScheduleEvent(EVENT_SUMMON_SHADOWCASTER, 9000);
		events.ScheduleEvent(EVENT_SUMMON_HULKING_CORPSE, 30000);
		events.ScheduleEvent(EVENT_SUMMON_CRYSTAL_HANDLER, 20000);
		events.ScheduleEvent(EVENT_CHECK_PHASE, 80000);

		_unit->CastSpell(_unit, SPELL_ARCANE_BLAST, true);
		_unit->CastSpell(_unit, SPELL_DESPAWN_CRYSTAL_HANDLER, true);
		_unit->CastSpell(_unit, SPELL_BEAM_CHANNEL, true);
		_unit->CastSpell(_unit, 47346, false);
		SpawnCreature(NPC_CRYSTAL_CHANNEL_TARGET, -378.40f, -813.13f, 59.74f, 0.0f);

		_unit->SetUInt64Value(UNIT_FIELD_TARGET, 0);
		//_unit->RemoveAllAuras();
		_unit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2);
		_unit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

		if (mInstance)
			mInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_InProgress);

		ParentClass::EnterCombat(mAttacker);
	}

	void JustDied(Unit* mKiller)
	{
		Emote("Your efforts... are in vain.", Text_Yell, 13174);
		ParentClass::JustDied(mKiller);
	}

	void KilledUnit(Unit* pTarget)
	{
		Emote("Such is the fate of all who oppose the Lich King.", Text_Yell, 13175);
		ParentClass::KilledUnit(pTarget);
	}

	void OnCombatStop(Unit* mAttacker)
	{
		_unit->m_countHelper = 0;
		_unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2);
		_unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
		if (mInstance)
			mInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_Performed);
		ParentClass::OnCombatStop(mAttacker);
	}

	void AIUpdate()
	{
		if (_unit->m_countHelper == 4)
		{
			_unit->RemoveAura(SPELL_BEAM_CHANNEL);
			_unit->m_countHelper = 5;
		}
		events.Update(1000);
		switch (events.ExecuteEvent())
		{
		case EVENT_SUMMON_FETID_TROLL:
			if (Creature* trigger = _unit->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(-378.40f, -813.13f, 59.74f, NPC_CRYSTAL_CHANNEL_TARGET))
				trigger->CastSpell(trigger, SPELL_SUMMON_FETID_TROLL_CORPSE, true);
			events.ScheduleEvent(EVENT_SUMMON_FETID_TROLL, 3000);
			break;
		case EVENT_SUMMON_HULKING_CORPSE:
			if (Creature* trigger = _unit->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(-378.40f, -813.13f, 59.74f, NPC_CRYSTAL_CHANNEL_TARGET))
				trigger->CastSpell(trigger, SPELL_SUMMON_HULKING_CORPSE, true);
			events.ScheduleEvent(EVENT_SUMMON_HULKING_CORPSE, 30000);
			break;
		case EVENT_SUMMON_SHADOWCASTER:
			if (Creature* trigger = _unit->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(-378.40f, -813.13f, 59.74f, NPC_CRYSTAL_CHANNEL_TARGET))
				trigger->CastSpell(trigger, SPELL_SUMMON_RISEN_SHADOWCASTER, true);
			events.ScheduleEvent(EVENT_SUMMON_SHADOWCASTER, 10000);
			break;
		case EVENT_SUMMON_CRYSTAL_HANDLER:
			if (_unit->m_countHelper < 4)
			{
				Emote("Bolster my defenses! Hurry, curse you!", Text_Yell, 13176);
				Announce("Novos the Summoner calls for assistance!");
				if (Creature* trigger = _unit->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(-378.40f, -813.13f, 59.74f, NPC_CRYSTAL_CHANNEL_TARGET))
					trigger->CastSpell(trigger, SPELL_SUMMON_CRYSTAL_HANDLER, true);
				events.ScheduleEvent(EVENT_SUMMON_CRYSTAL_HANDLER, 30000);
			}
			break;
		case EVENT_CHECK_PHASE:
			if (_unit->HasAura(SPELL_BEAM_CHANNEL))
			{
				events.ScheduleEvent(EVENT_CHECK_PHASE, 2000);
				break;
			}
			events.Reset();
			events.ScheduleEvent(EVENT_CAST_OFFENSIVE_SPELL, 3000);
			events.ScheduleEvent(EVENT_SPELL_SUMMON_MINIONS, 10000);
			_unit->Unroot();
			_unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2);
			_unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
			_unit->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1, 0);
			break;
		case EVENT_CAST_OFFENSIVE_SPELL:
			if (!_unit->IsCasting())
				if (Unit* target = GetRandomPlayerTarget()) // Needs a random Selection
					_unit->CastSpell(target, SPELL_BLIZZARD, false);

			events.ScheduleEvent(EVENT_CAST_OFFENSIVE_SPELL, 500);
			break;
		case EVENT_SPELL_SUMMON_MINIONS:
			if (_unit->hasStateFlag(0x00008000))
			{
				_unit->CastSpell(_unit, SPELL_SUMMON_MINIONS, false);
				events.ScheduleEvent(EVENT_SPELL_SUMMON_MINIONS, 15000);
				break;
			}
			events.ScheduleEvent(EVENT_SPELL_SUMMON_MINIONS, 500);
			break;
		}
		ParentClass::UpdateAI();
	}
};

#define CRYSTAL_HANDLER_ENTRY 26627

class CRYSTAL_HANDLER_AI : public CreatureAIScript
{
	public:
		ADD_CREATURE_FACTORY_FUNCTION(CRYSTAL_HANDLER_AI);

		CRYSTAL_HANDLER_AI(Creature* pCreature) : CreatureAIScript(pCreature)
		{
			heroic = (_unit->GetMapMgr()->iInstanceMode == MODE_HEROIC);
			spells.clear();
			/* SPELLS INIT */
			ScriptSpell* FlashofDarkness = new ScriptSpell;
			FlashofDarkness->normal_spellid = 49668;
			FlashofDarkness->heroic_spellid = 59004;
			FlashofDarkness->chance = 50;
			FlashofDarkness->timer = 4000;
			FlashofDarkness->time = 0;
			FlashofDarkness->target = SPELL_TARGET_CURRENT_ENEMY;
			spells.push_back(FlashofDarkness);
		}

		void EnterCombat(Unit* mTarget)
		{
			RegisterAIUpdateEvent(_unit->GetBaseAttackTime(MELEE));
		}

		void OnCombatStop(Unit* mTarget)
		{
			_unit->GetAIInterface()->setCurrentAgent(AGENT_NULL);
			_unit->GetAIInterface()->SetAIState(STATE_IDLE);
			RemoveAIUpdateEvent();
		}

		void JustDied(Unit*  mKiller)
		{
			RemoveAIUpdateEvent();
			Unit* Novos = _unit->GetMapMgr()->GetUnit(_unit->GetSummonedByGUID());
			if (Novos)
			{
				++Novos->m_countHelper;
			}
		}

		void UpdateAI()
		{
			if(spells.size() > 0)
			{
				for(uint8 i = 0; i < spells.size(); i++)
				{
					if(spells[i]->time < Arcemu::Shared::Util::getMSTime())
					{
						if(Rand(spells[i]->chance))
						{
							CastScriptSpell(spells[i]);
							spells[i]->time = Arcemu::Shared::Util::getMSTime() + spells[i]->timer;
						}
					}
				}
			}
		}

		void CastScriptSpell(ScriptSpell* spell)
		{
			_unit->Root();
			uint32 spellid = heroic ? spell->heroic_spellid : spell->normal_spellid;
			Unit* spelltarget = NULL;
			switch(spell->target)
			{
				case SPELL_TARGET_SELF:
					{
						spelltarget = _unit;
					}
					break;
				case SPELL_TARGET_GENERATE:
					{
						spelltarget = NULL;
					}
					break;
				case SPELL_TARGET_CURRENT_ENEMY:
					{
						spelltarget = _unit->GetAIInterface()->getNextTarget();
					}
					break;
				case SPELL_TARGET_RANDOM_PLAYER:
					{
						//spelltarget = _unit->GetRandomPlayerTarget();
					}
					break;
			}
			_unit->CastSpell(spelltarget , spellid, false);
			_unit->Unroot();
		}

		void Destroy()
		{
			for(uint32 i = 0; i < spells.size(); ++i)
			{
				if(spells[ i ] != NULL)
					delete spells[ i ];
			};

			spells.clear();

			delete this;
		};

	protected:
		bool heroic;
		vector< ScriptSpell* > spells;
};

class FetidTrollAI : public AICreatureScript
{
public:
	AI_CREATURE_SCRIPT_FUNCTION(FetidTrollAI, AICreatureScript);
	FetidTrollAI(Creature* pCreature) : AICreatureScript(pCreature)
	{
	}

	void OnLoad()
	{
		_unit->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1, 0);
		ParentClass::OnLoad();
	}
};

class HulkingCorpseAI : public AICreatureScript
{
public:
	AI_CREATURE_SCRIPT_FUNCTION(HulkingCorpseAI, AICreatureScript);
	HulkingCorpseAI(Creature* pCreature) : AICreatureScript(pCreature)
	{
	}

	void OnLoad()
	{
		_unit->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1, 0);
		ParentClass::OnLoad();
	}
};

class RisenShadowcasterAI : public AICreatureScript
{
public:
	AI_CREATURE_SCRIPT_FUNCTION(RisenShadowcasterAI, AICreatureScript);
	RisenShadowcasterAI(Creature* pCreature) : AICreatureScript(pCreature)
	{
	}

	void OnLoad()
	{
		_unit->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1, 0);
		ParentClass::OnLoad();
	}
};

enum KingDredSpells
{
	SPELL_BELLOWING_ROAR = 22686, // fears the group, can be resisted/dispelled
	SPELL_GRIEVOUS_BITE = 48920,
	SPELL_MANGLING_SLASH = 48873, // cast on the current tank, adds debuf
	SPELL_FEARSOME_ROAR = 48849,
	SPELL_PIERCING_SLASH = 48878, // debuff --> Armor reduced by 75%
	SPELL_RAPTOR_CALL = 59416, // dummy
	SPELL_GUT_RIP = 49710,
	SPELL_REND = 13738
};

enum DredCreatureInfo
{
	// King Dred
	NPC_DRAKKARI_GUTRIPPER = 26641,
	NPC_DRAKKARI_SCYTHECLAW = 26628,
	NPC_KING_DRED = 27483
};

enum KingDredMisc
{
	ACTION_RAPTOR_KILLED = 1,
	DATA_RAPTORS_KILLED = 2
};

enum KingDredEvents
{
	EVENT_BELLOWING_ROAR = 1,
	EVENT_GRIEVOUS_BITE,
	EVENT_MANGLING_SLASH,
	EVENT_FEARSOME_ROAR,
	EVENT_PIERCING_SLASH,
	EVENT_RAPTOR_CALL
};

class KING_DRED_AI : public AICreatureScript
{
	public:
		AI_CREATURE_SCRIPT_FUNCTION(KING_DRED_AI, AICreatureScript);
		KING_DRED_AI(Creature* pCreature) : AICreatureScript(pCreature)
		{
		}

		void Initialize()
		{
			raptorsKilled = 0;
		}

		void EnterCombat(Unit* mTarget)
		{
			RegisterAIUpdateEvent(1000);

			events.ScheduleEvent(EVENT_BELLOWING_ROAR, 33000);
			events.ScheduleEvent(EVENT_GRIEVOUS_BITE, 20000);
			events.ScheduleEvent(EVENT_MANGLING_SLASH, 18500);
			events.ScheduleEvent(EVENT_FEARSOME_ROAR, urand(10000, 20000));
			events.ScheduleEvent(EVENT_PIERCING_SLASH, 17000);
			events.ScheduleEvent(EVENT_RAPTOR_CALL, urand(20000, 25000));

			if (mInstance)
				mInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_InProgress);
		}

		void OnCombatStop(Unit* mAttacker)
		{
			Initialize();

			if (mInstance)
				mInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_Performed);
			ParentClass::OnCombatStop(mAttacker);
		}

		void DoAction(int32 action)
		{
			if (action == ACTION_RAPTOR_KILLED)
				++raptorsKilled;
		}

		void JustDied(Unit*  mKiller)
		{
		}

		void UpdateAI()
		{
			events.Update(1000);

			if (_unit->IsCasting())
				return;

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_BELLOWING_ROAR:
					_unit->DoCastAOE(SPELL_BELLOWING_ROAR);
					events.ScheduleEvent(EVENT_BELLOWING_ROAR, 33000);
					break;
				case EVENT_GRIEVOUS_BITE:
					_unit->DoCastVictim(SPELL_GRIEVOUS_BITE);
					events.ScheduleEvent(EVENT_GRIEVOUS_BITE, 20000);
					break;
				case EVENT_MANGLING_SLASH:
					_unit->DoCastVictim(SPELL_MANGLING_SLASH);
					events.ScheduleEvent(EVENT_MANGLING_SLASH, 18500);
					break;
				case EVENT_FEARSOME_ROAR:
					_unit->DoCastAOE(SPELL_FEARSOME_ROAR);
					events.ScheduleEvent(EVENT_FEARSOME_ROAR, urand(10000, 20000));
					break;
				case EVENT_PIERCING_SLASH:
					_unit->DoCastVictim(SPELL_PIERCING_SLASH);
					events.ScheduleEvent(EVENT_PIERCING_SLASH, 17000);
					break;
				case EVENT_RAPTOR_CALL:
					_unit->DoCastVictim(SPELL_RAPTOR_CALL);
					if (RandomUInt(1) == 1)
					{
						Unit* Gutripper = _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_DRAKKARI_GUTRIPPER, -522.02f, -718.89f, 30.26f, 2.41f, true, false, 0, 0);
						if (Gutripper)
						{
							if (GetRandomPlayerTarget())
							{
								Gutripper->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1, 0);
							}
						}
					}
					else
					{
						Unit* Scytheclaw = _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_DRAKKARI_SCYTHECLAW, -522.02f, -718.89f, 30.26f, 2.41f, true, false, 0, 0);
						if (Scytheclaw)
						{
							if (GetRandomPlayerTarget())
							{
								Scytheclaw->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1, 0);
							}
						}
					}
					events.ScheduleEvent(EVENT_RAPTOR_CALL, urand(20000, 25000));
					break;
				default:
					break;
				}
			}
			ParentClass::UpdateAI();
		}
private:
	uint8 raptorsKilled;
};

class npc_drakkari_gutripper : public AICreatureScript
{
    public:
	    AI_CREATURE_SCRIPT_FUNCTION(npc_drakkari_gutripper, AICreatureScript);
	    npc_drakkari_gutripper(Creature* pCreature) : AICreatureScript(pCreature)
	    {
	    }

	    void EnterCombat(Unit* mTarget)
	    {
		    events.ScheduleEvent(1, urand(10000, 15000));
		    ParentClass::EnterCombat(mTarget);
	    }
		void UpdateAI() override
		{
			events.Update(1000);

			if (_unit->IsCasting())
				return;

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{
				case 1:
					_unit->DoCastVictim(SPELL_GUT_RIP, false);
					break;
				}
			}
			ParentClass::UpdateAI();
		}

		void JustDied(Unit* killer) override
		{
			if (Creature* Dred = _unit->GetMapMgr()->GetCreature(instance->GetInstance()->m_BossGUID3))
				Dred->GetAIInterface()->DoAction(ACTION_RAPTOR_KILLED);
			ParentClass::JustDied(killer);
		}
private:
	InstanceScript* instance;
};

class npc_drakkari_scytheclaw : public AICreatureScript
{
public:
	AI_CREATURE_SCRIPT_FUNCTION(npc_drakkari_scytheclaw, AICreatureScript);
	npc_drakkari_scytheclaw(Creature* pCreature) : AICreatureScript(pCreature)
	{
	}

	void EnterCombat(Unit* mTarget)
	{
		events.ScheduleEvent(1, urand(10000, 15000));
		ParentClass::EnterCombat(mTarget);
	}
	void UpdateAI() override
	{
		events.Update(1000);

		if (_unit->IsCasting())
			return;

		while (uint32 eventId = events.ExecuteEvent())
		{
			switch (eventId)
			{
			case 1:
				_unit->DoCastVictim(SPELL_REND, false);
				break;
			}
		}
		ParentClass::UpdateAI();
	}

	void JustDied(Unit* killer) override
	{
		if (Creature* Dred = _unit->GetMapMgr()->GetCreature(instance->GetInstance()->m_BossGUID3))
			Dred->GetAIInterface()->DoAction(ACTION_RAPTOR_KILLED);
		ParentClass::JustDied(killer);
	}
private:
	InstanceScript* instance;
};

/*
 * Known Issues: Spell 49356 and 53463 will be interrupted for an unknown reason
 */

enum TharonjaSpells
{
	// Skeletal Spells (phase 1)
	SPELL_CURSE_OF_LIFE = 49527,
	SPELL_RAIN_OF_FIRE = 49518,
	SPELL_SHADOW_VOLLEY = 49528,
	SPELL_DECAY_FLESH = 49356, // cast at end of phase 1, starts phase 2
	// Flesh Spells (phase 2)
	SPELL_GIFT_OF_THARON_JA = 52509,
	SPELL_CLEAR_GIFT_OF_THARON_JA = 53242,
	SPELL_EYE_BEAM = 49544,
	SPELL_LIGHTNING_BREATH = 49537,
	SPELL_POISON_CLOUD = 49548,
	SPELL_RETURN_FLESH = 53463, // Channeled spell ending phase two and returning to phase 1. This ability will stun the party for 6 seconds.
	SPELL_ACHIEVEMENT_CHECK = 61863,
	SPELL_FLESH_VISUAL = 52582,
	SPELL_DUMMY = 49551
};

enum TharonjaEvents
{
	EVENT_CURSE_OF_LIFE = 1,
	EVENT_RAIN_OF_FIRE,
	EVENT_SHADOW_VOLLEY,

	EVENT_EYE_BEAM,
	EVENT_LIGHTNING_BREATH,
	EVENT_POISON_CLOUD,

	EVENT_DECAY_FLESH,
	EVENT_GOING_FLESH,
	EVENT_RETURN_FLESH,
	EVENT_GOING_SKELETAL
};

enum TharonjaModels
{
	MODEL_FLESH = 27073,
	NPC_PROPHET_THARONJA = 26632
};


class THE_PROPHET_THARONJA : public AICreatureScript
{
public:
	AI_CREATURE_SCRIPT_FUNCTION(THE_PROPHET_THARONJA, AICreatureScript);
	THE_PROPHET_THARONJA(Creature* pCreature) : AICreatureScript(pCreature)
	{}

	void EnterCombat(Unit* who)
	{
		RegisterAIUpdateEvent(1000);
		Emote("Tharon'ja sees all! The work of mortals shall not end the eternal dynasty!", Text_Yell, 13862);
		orginalmodelid = _unit->GetDisplayId();

		events.ScheduleEvent(EVENT_DECAY_FLESH, 20000);
		events.ScheduleEvent(EVENT_CURSE_OF_LIFE, 1000);
		events.ScheduleEvent(EVENT_RAIN_OF_FIRE, urand(14000, 18000));
		events.ScheduleEvent(EVENT_SHADOW_VOLLEY, urand(8000, 10000));

		if (mInstance)
			mInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_InProgress);
		ParentClass::EnterCombat(who);
	}

	void OnCombatStop(Unit* mAttacker)
	{
		if (mInstance)
			mInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_Performed);
		ParentClass::OnCombatStop(mAttacker);
	}

	void KilledUnit(Unit* who)
	{
		switch (urand(0, 1))
		{
		case 0:
			Emote("As Tharon'ja predicted!", Text_Yell, 13863);
			break;
		case 1:
			Emote("As it was written!", Text_Yell, 13864);
			break;
		}
		ParentClass::KilledUnit(who);
	}

	void JustDied(Unit* killer)
	{
		Emote("Im...impossible! Tharon'ja is eternal! Tharon'ja...is...", Text_Yell, 13869);
		_unit->DoCastAOE(SPELL_CLEAR_GIFT_OF_THARON_JA, true);
		_unit->DoCastAOE(SPELL_ACHIEVEMENT_CHECK, true);
		ParentClass::JustDied(killer);
	}

	void UpdateAI()
	{
		events.Update(1000);

		if (_unit->IsCasting())
			return;

		while (uint32 eventId = events.ExecuteEvent())
		{
			switch (eventId)
			{
			case EVENT_CURSE_OF_LIFE:
				if (Unit* target = GetRandomPlayerTarget())
					_unit->DoCast(target, SPELL_CURSE_OF_LIFE);
				events.ScheduleEvent(EVENT_CURSE_OF_LIFE, urand(10000, 15000));
				return;
			case EVENT_SHADOW_VOLLEY:
				_unit->DoCastVictim(SPELL_SHADOW_VOLLEY);
				events.ScheduleEvent(EVENT_SHADOW_VOLLEY, urand(8000, 10000));
				return;
			case EVENT_RAIN_OF_FIRE:
				if (Unit* target = GetRandomPlayerTarget())
					_unit->DoCast(target, SPELL_RAIN_OF_FIRE);
				events.ScheduleEvent(EVENT_RAIN_OF_FIRE, urand(14000, 18000));
				return;
			case EVENT_LIGHTNING_BREATH:
				if (Unit* target = GetRandomPlayerTarget())
					_unit->DoCast(target, SPELL_LIGHTNING_BREATH);
				events.ScheduleEvent(EVENT_LIGHTNING_BREATH, urand(6000, 7000));
				return;
			case EVENT_EYE_BEAM:
				if (Unit* target = GetRandomPlayerTarget())
					_unit->DoCast(target, SPELL_EYE_BEAM);
				events.ScheduleEvent(EVENT_EYE_BEAM, urand(4000, 6000));
				return;
			case EVENT_POISON_CLOUD:
				_unit->DoCastAOE(SPELL_POISON_CLOUD);
				events.ScheduleEvent(EVENT_POISON_CLOUD, urand(10000, 12000));
				return;
			case EVENT_DECAY_FLESH:
				_unit->DoCastAOE(SPELL_DECAY_FLESH);
				events.ScheduleEvent(EVENT_GOING_FLESH, 6000);
				return;
			case EVENT_GOING_FLESH:
				Emote("No! A taste...all too brief!", Text_Yell, 13867);
				_unit->SetDisplayId(MODEL_FLESH);
				_unit->DoCastAOE(SPELL_GIFT_OF_THARON_JA, true);
				_unit->DoCast(_unit, SPELL_FLESH_VISUAL, true);
				_unit->DoCast(_unit, SPELL_DUMMY, true);

				events.Reset();
				events.ScheduleEvent(EVENT_RETURN_FLESH, 20000);
				events.ScheduleEvent(EVENT_LIGHTNING_BREATH, urand(3000, 4000));
				events.ScheduleEvent(EVENT_EYE_BEAM, urand(4000, 8000));
				events.ScheduleEvent(EVENT_POISON_CLOUD, urand(6000, 7000));
				break;
			case EVENT_RETURN_FLESH:
				_unit->DoCastAOE(SPELL_RETURN_FLESH);
				events.ScheduleEvent(EVENT_GOING_SKELETAL, 6000);
				return;
			case EVENT_GOING_SKELETAL:
				Emote("Your flesh serves Tharon'ja now!", Text_Yell, 13865);
				_unit->SetDisplayId(orginalmodelid);
				_unit->DoCastAOE(SPELL_CLEAR_GIFT_OF_THARON_JA, true);

				events.Reset();
				events.ScheduleEvent(EVENT_DECAY_FLESH, 20000);
				events.ScheduleEvent(EVENT_CURSE_OF_LIFE, 1000);
				events.ScheduleEvent(EVENT_RAIN_OF_FIRE, urand(14000, 18000));
				events.ScheduleEvent(EVENT_SHADOW_VOLLEY, urand(8000, 10000));
				break;
			default:
				break;
			}
		}
	}
	protected:
		uint32 orginalmodelid = 0;
};


void SetupDrakTharonKeep(ScriptMgr* mgr)
{
	//////////////////////////////////////////
	// TRASH MOBS
	//////////////////////////////////////////
	mgr->register_creature_script(NPC_DRAKKARI_GUTRIPPER, &npc_drakkari_gutripper::Create);
	mgr->register_creature_script(NPC_DRAKKARI_SCYTHECLAW, &npc_drakkari_scytheclaw::Create);
	mgr->register_creature_script(27597, &HulkingCorpseAI::Create);
	mgr->register_creature_script(27600, &RisenShadowcasterAI::Create);
	mgr->register_creature_script(27598, &FetidTrollAI::Create);

	//Instance
	mgr->register_instance_script(MAP_DRAK_THARON_KEEP, &DrakTharonKeepScript::Create);

	//////////////////////////////////////////
	// BOSSES
	//////////////////////////////////////////
	mgr->register_creature_script(NPC_TROLLGORE, &TrollgoreAI::Create);
	mgr->register_creature_script(NPC_NOVOS_THE_SUMMONER, &NovosTheSummonerAI::Create);
	mgr->register_creature_script(CRYSTAL_HANDLER_ENTRY, &CRYSTAL_HANDLER_AI::Create);
	mgr->register_creature_script(NPC_KING_DRED, &KING_DRED_AI::Create);
	mgr->register_creature_script(NPC_PROPHET_THARONJA, &THE_PROPHET_THARONJA::Create);

}
