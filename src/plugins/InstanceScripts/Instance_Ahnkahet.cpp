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
#include "Instance_Ahnkahet.h"

class AhnkahetScript : public MoonInstanceScript
{
public:
	uint32		mPrinceTaldaramGUID;
	uint32		mElderNadoxGUID;
	uint32		mJedogaShadowseekerGUID;
	uint32		mHeraldVolazjGUID;

	MOONSCRIPT_INSTANCE_FACTORY_FUNCTION(AhnkahetScript, MoonInstanceScript);
	AhnkahetScript(MapMgr* pMapMgr) : MoonInstanceScript(pMapMgr)
	{
		mPrinceTaldaramGUID = 0;
		mElderNadoxGUID = 0;
		mJedogaShadowseekerGUID = 0;
		mHeraldVolazjGUID = 0;

		GetInstance()->m_BossGUID1 = 0;
		GetInstance()->m_BossGUID2 = 0;
		GetInstance()->m_BossGUID3 = 0;
		GetInstance()->m_BossGUID4 = 0;
	};

	void OnCreaturePushToWorld(Creature* pCreature)
	{
		switch (pCreature->GetEntry())
		{
		case CN_PRINCE_TALDARAM:
		{
			mPrinceTaldaramGUID = pCreature->GetLowGUID();
			GetInstance()->m_BossGUID1 = mPrinceTaldaramGUID;
			mEncounters.insert(EncounterMap::value_type(CN_PRINCE_TALDARAM, BossData(0, mPrinceTaldaramGUID)));
		}
		break;
		case CN_ELDER_NADOX:
		{
			mElderNadoxGUID = pCreature->GetLowGUID();
			GetInstance()->m_BossGUID2 = mElderNadoxGUID;
			mEncounters.insert(EncounterMap::value_type(CN_ELDER_NADOX, BossData(0, mElderNadoxGUID)));
		}
		break;
		case CN_JEDOGA_SHADOWSEEKER:
		{
			mJedogaShadowseekerGUID = pCreature->GetLowGUID();
			GetInstance()->m_BossGUID3 = mJedogaShadowseekerGUID;
			mEncounters.insert(EncounterMap::value_type(CN_JEDOGA_SHADOWSEEKER, BossData(0, mJedogaShadowseekerGUID)));
		}
		break;
		case CN_HERALD_VOLAZJ:
		{
			mHeraldVolazjGUID = pCreature->GetLowGUID();
			GetInstance()->m_BossGUID4 = mHeraldVolazjGUID;
			mEncounters.insert(EncounterMap::value_type(CN_HERALD_VOLAZJ, BossData(0, mHeraldVolazjGUID)));
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

	void OnPlayerEnter(Player* player)
	{
		if (player)
		{
			// Hack! Load missing cells so the whole instance is spawned some scripts need this DO NOT REMOVE.
			player->GetMapMgr()->VisitCoordsInstance(528.72f, -846.00f, player);
			player->UpdateVisibility();
		}
	}

	void OnCreatureDeath(Creature* pVictim, Unit* pKiller)
	{
		EncounterMap::iterator Iter = mEncounters.find(pVictim->GetEntry());
		if (Iter == mEncounters.end())
			return;

		(*Iter).second.mState = State_Finished;

		GameObject* pDoors = NULL;
		switch (pVictim->GetEntry())
		{
		case CN_PRINCE_TALDARAM:
		{
			SetInstanceData(Data_EncounterState, CN_PRINCE_TALDARAM, State_Finished);
		}
		break;
		case CN_ELDER_NADOX:
		{
			SetInstanceData(Data_EncounterState, CN_ELDER_NADOX, State_Finished);
		}
		break;
		case CN_JEDOGA_SHADOWSEEKER:
		{
			SetInstanceData(Data_EncounterState, CN_JEDOGA_SHADOWSEEKER, State_Finished);
		}
		break;
		case CN_HERALD_VOLAZJ:
		{
			SetInstanceData(Data_EncounterState, CN_HERALD_VOLAZJ, State_Finished);
		}
		break;
		};
	};
};

class mob_ahnkahar_watcher : public AICreatureScript
{
public:
	AI_CREATURE_SCRIPT_FUNCTION(mob_ahnkahar_watcher, AICreatureScript);
	mob_ahnkahar_watcher(Creature* pCreature) : AICreatureScript(pCreature)
	{
	}

	void OnLoad()
	{
		// We only show on heroic mode.
		if (_unit->GetMapMgr()->GetMapId() == MAP_AHN_KAHET && !IsHeroic())
		{
			_unit->SetVisible(false);
		}
		ParentClass::OnLoad();
	}
};

enum ElderNadoxSpells
{
	SPELL_BROOD_PLAGUE = 56130,
	H_SPELL_BROOD_RAGE = 59465,
	SPELL_ENRAGE = 26662, // Enraged if too far away from home
	SPELL_SUMMON_SWARMERS = 56119, // 2x 30178  -- 2x every 10secs
	SPELL_SUMMON_SWARM_GUARD = 56120, // 1x 30176  -- every 25%
	// Spells Adds
	SPELL_SPRINT = 56354,
	SPELL_GUARDIAN_AURA = 56151
};

enum ElderNadoxCreatures
{
	NPC_AHNKAHAR_SWARMER = 30178,
	NPC_AHNKAHAR_GUARDIAN = 30176
};

enum ElderNadoxEvents
{
	EVENT_PLAGUE = 1,
	EVENT_RAGE,
	EVENT_SUMMON_SWARMER,
	EVENT_CHECK_ENRAGE,
	EVENT_SPRINT,
	DATA_RESPECT_YOUR_ELDERS
};

class instanceboss_elder_nadox : public AICreatureScript
{
public:
	AI_CREATURE_SCRIPT_FUNCTION(instanceboss_elder_nadox, AICreatureScript);
	instanceboss_elder_nadox(Creature* pCreature) : AICreatureScript(pCreature)
	{}

	void OnLoad()
	{
		instance = GetInstanceScript();
		ParentClass::OnLoad();
	}

	void OnCombatStop(Unit* who)
	{
		events.Reset();
		_unit->DespawnCombatSummons();

		_mobPhase = 0;
		GuardianDied = false;

		if (instance)
			instance->SetInstanceData(Data_EncounterState, DATA_ELDER_NADOX_EVENT, State_NotStarted);
		ParentClass::OnCombatStop(who);
	}

	void EnterCombat(Unit* /*who*/)
	{
		Emote("The secrets of the deep shall remain hidden.", Text_Yell, 14033);

		_mobPhase = 0;

		if (instance)
			instance->SetInstanceData(Data_EncounterState, DATA_ELDER_NADOX_EVENT, State_InProgress);

		events.ScheduleEvent(EVENT_PLAGUE, 13 * IN_MILLISECONDS);
		events.ScheduleEvent(EVENT_SUMMON_SWARMER, 10 * IN_MILLISECONDS);

		if (IsHeroic())
		{
			events.ScheduleEvent(EVENT_RAGE, 12 * IN_MILLISECONDS);
			events.ScheduleEvent(EVENT_CHECK_ENRAGE, 5 * IN_MILLISECONDS);
		}
	}

	void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
	{
		if (summon->GetEntry() == NPC_AHNKAHAR_GUARDIAN)
			GuardianDied = true;
	}

	uint32 GetData(uint32 type)
	{
		if (type == DATA_RESPECT_YOUR_ELDERS)
			return !GuardianDied ? 1 : 0;

		return 0;
	}

	void KilledUnit(Unit* /*victim*/)
	{
		switch (urand(0, 2))
		{
		case 0:
			Emote("Sleep now, in the cold dark.", Text_Yell, 14036);
			break;
		case 1:
			Emote("For the Lich King!", Text_Yell, 14037);
			break;
		case 2:
			Emote("Perhaps we will be allies soon.", Text_Yell, 14038);
			break;
		}
	}

	void JustDied(Unit* /*killer*/)
	{
		Emote("Master... is my service... complete?", Text_Yell, 14039);

		if (_unit->GetMapMgr()->GetCreature(_unit->combatSummon1GUID) && !_unit->GetMapMgr()->GetCreature(_unit->combatSummon1GUID)->IsDead() && IsHeroic())
		{
			_unit->awardAchievement(2038);
		}

		_unit->DespawnCombatSummons();

		if (instance)
			instance->SetInstanceData(Data_EncounterState, DATA_ELDER_NADOX_EVENT, State_Finished);
	}

	void UpdateAI()
	{
		if (_unit->IsCasting())
			return;

		events.Update(1000);

		while (uint32 eventId = events.ExecuteEvent())
		{
			switch (eventId)
			{
			case EVENT_PLAGUE:
				_unit->DoCast(GetRandomPlayerTarget(), SPELL_BROOD_PLAGUE, true);
				events.ScheduleEvent(EVENT_PLAGUE, 15 * IN_MILLISECONDS);
				break;
			case EVENT_RAGE:
				if (Unit* swarmer = ForceCreatureFind(NPC_AHNKAHAR_SWARMER))
				{
					if (_unit->CalcDistance(_unit, swarmer) < 35)
						_unit->DoCast(swarmer, H_SPELL_BROOD_RAGE, true);
				}
				events.ScheduleEvent(EVENT_RAGE, urand(10 * IN_MILLISECONDS, 50 * IN_MILLISECONDS));
				break;
			case EVENT_SUMMON_SWARMER:
				_unit->DoCast(_unit, SPELL_SUMMON_SWARMERS);
				if (urand(1, 3) == 3) // 33% chance of dialog
					Emote("The young must not grow hungry...", Text_Yell, 14034);
				events.ScheduleEvent(EVENT_SUMMON_SWARMER, 10 * IN_MILLISECONDS);
				break;
			case EVENT_CHECK_ENRAGE: // Patch 3.0.8 (2009-01-20): Can no longer be kited safely up the stairs and away from his protectors. If he is kited, he will enrage with a five-minute Berserk buff and deal 25-30k damage per hit (on heroic).
				if (_unit->HasAura(SPELL_ENRAGE))
					return;
				if (_unit->GetPositionZ() < 24.0f)
					_unit->DoCast(_unit, SPELL_ENRAGE, true);
				events.ScheduleEvent(EVENT_CHECK_ENRAGE, 5 * IN_MILLISECONDS);
				break;
			default:
				break;
			}
		}

		//Patch 3.3.2 (2010-02-01): Elder Nadox now only gets one Ahn'Kahar Guardian during the encounter. 
		if (_unit->GetHealthPct() <= 50 && _mobPhase == 0)
		{
			Announce("An Ahn'kahar Guardian hatches!");
			_unit->DoCast(_unit, SPELL_SUMMON_SWARM_GUARD);
			if (ForceCreatureFind(30176))
			{
				_unit->combatSummon1GUID = ForceCreatureFind(30176)->GetGUID();
			}

			++_mobPhase;
		}

		ParentClass::UpdateAI();
	}

private:
	bool GuardianDied;
	uint8 _mobPhase;
	InstanceScript* instance;
};

//Script Status: 15%
//Notes: Needs scripted just has gameobject checks to be lowered.
enum PrinceTaldaramSpells
{
    SPELL_BLOODTHIRST = 55968, //Trigger Spell + add aura
    SPELL_CONJURE_FLAME_SPHERE = 55931,
    SPELL_FLAME_SPHERE_SPAWN_EFFECT = 55891,
    SPELL_FLAME_SPHERE_VISUAL = 55928,
    SPELL_FLAME_SPHERE_PERIODIC = 55926,
    SPELL_FLAME_SPHERE_PERIODIC_H = 59508,
    SPELL_FLAME_SPHERE_DEATH_EFFECT = 55947,
    SPELL_BEAM_VISUAL = 60342,
    SPELL_EMBRACE_OF_THE_VAMPYR = 55959,
    SPELL_EMBRACE_OF_THE_VAMPYR_H = 59513,
    SPELL_VANISH = 55964,
    CREATURE_FLAME_SPHERE = 30106,
    CREATURE_FLAME_SPHERE_1 = 31686,
    CREATURE_FLAME_SPHERE_2 = 31687,
};
enum PrinceTaldaramMisc
{
	DATA_SPHERE_EVENT1 = 99998,
    DATA_SPHERE_EVENT2 = 99999,
    DATA_EMBRACE_DMG = 20000,
    DATA_EMBRACE_DMG_H = 40000,
    DATA_SPHERE_DISTANCE = 30,
    ACTION_FREE = 1,
    ACTION_SPHERE = 2,
};
enum PrinceTaldaramEvent
{
    EVENT_PRINCE_FLAME_SPHERES = 1,
    EVENT_PRINCE_VANISH = 2,
    EVENT_PRINCE_BLOODTHIRST = 3,
    EVENT_PRINCE_VANISH_RUN = 4,
    EVENT_PRINCE_RESCHEDULE = 5,
};

#define DATA_GROUND_POSITION_Z             11.4f

 class instanceboss_taldaram : public AICreatureScript
{
public:
    AI_CREATURE_SCRIPT_FUNCTION(instanceboss_taldaram, AICreatureScript);
    instanceboss_taldaram(Creature* pCreature) : AICreatureScript(pCreature)
    {
        pInstance = GetInstanceScript();
        Initialize();
    }
    InstanceScript* pInstance;
    EventMap2 events;
    uint64 vanishTarget;
    uint32 vanishDamage;
    uint32 spawnDisplay;

    void Initialize()
    {
        vanishDamage = 0;
        vanishTarget = 0;
        spawnDisplay = 0;
    }

	void OnLoad()
	{
		RegisterAIUpdateEvent(1000);
		ParentClass::OnLoad();
	}

    void OnCombatStop(Unit* mAttacker)
    {
        if (_unit->GetPositionZ() > 15.0f)
            _unit->CastSpell(_unit, SPELL_BEAM_VISUAL, true);

        events.Reset();
        _unit->DespawnCombatSummons();
        vanishDamage = 0;
        vanishTarget = 0;
        spawnDisplay = 0;

        if (pInstance)
        {
            pInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_NotStarted);

            // Event not started
            if (pInstance->GetInstanceData(Data_EncounterState, DATA_SPHERE_EVENT1) == State_Finished && pInstance->GetInstanceData(Data_EncounterState, DATA_SPHERE_EVENT2) == State_Finished)
				_unit->SetPosition(_unit->GetPositionX(), _unit->GetPositionY(), DATA_GROUND_POSITION_Z, _unit->GetOrientation(), true);
        }
    }

    void DoAction(int32 param)
    {
        if (param == ACTION_FREE)
        {
            _unit->RemoveAllAuras();
            _unit->GetAIInterface()->SetWalk();
            _unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2 | UNIT_FLAG_NOT_SELECTABLE);

            _unit->SetPosition(_unit->GetPositionX(), _unit->GetPositionY(), DATA_GROUND_POSITION_Z, _unit->GetOrientation(), true);
        }
    }

    void EnterCombat(Unit* who)
    {
        if (pInstance)
            pInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_InProgress);

        Emote("I will feast on your remains.", Text_Yell, 14360);
        _unit->RemoveAllAuras();
        _unit->InterruptSpell();
    }
    
    void UpdateAI()
    {
		if (!_unit->CombatStatus.IsInCombat())
		{
			if (pInstance)
			{
				if (pInstance->GetInstanceData(Data_EncounterState, DATA_SPHERE_EVENT1) == State_Finished && pInstance->GetInstanceData(Data_EncounterState, DATA_SPHERE_EVENT2) == State_Finished)
				{
					DoAction(ACTION_FREE);
					Emote("Who dares enter the old kingdom? you outsiders will die!", Text_Yell, 0);
					RemoveAIUpdateEvent();
				}
			}
		}
		else
		{
			events.Update(1000);
			if (_unit->IsCasting())
				return;
		}
    }

    void DamageTaken(Unit* mAttacker, uint32 &damage)
    {
        ParentClass::OnDamageTaken(mAttacker, damage);
    }

    void JustDied(Unit* killer)
    {
        _unit->DespawnCombatSummons();
		Emote("Still I hunger, still I thirst.", Text_Yell, 0);

		GameObject* _bossDoor = _unit->GetMapMgr()->GetInterface()->GetGameObjectNearestCoords(443.06f, -325.156f, 104.023f, GO_TALDARAM_DOOR);
		if (_bossDoor)
			_bossDoor->Despawn(1000, 0);


        ParentClass::JustDied(killer);
    }

    void KilledUnit(Unit * victim)
    {
        if (urand(0, 1))
            return;

        //if (vanishTarget && victim->GetGUID() == vanishTarget)
            //AddCombatEvents();

		Emote("Bin dorei am'ovel.", Text_Yell, 0);
        ParentClass::KilledUnit(victim);
    }

};

class NerubianDevice1 : public GameObjectAIScript
{
public:
	NerubianDevice1(GameObject* goinstance) : GameObjectAIScript(goinstance) {}
	static GameObjectAIScript* Create(GameObject* GO) { return new NerubianDevice1(GO); }

	void OnActivate(Player* pPlayer)
	{
		if (pPlayer->GetMapId() != MAP_AHN_KAHET)
			return;

		if (pPlayer->GetMapMgr()->GetScript())
		{
			pPlayer->GetMapMgr()->GetScript()->SetInstanceData(Data_EncounterState, DATA_SPHERE_EVENT1, State_Finished);
		}
	}
};

class NerubianDevice2 : public GameObjectAIScript
{
public:
	NerubianDevice2(GameObject* goinstance) : GameObjectAIScript(goinstance) {}
	static GameObjectAIScript* Create(GameObject* GO) { return new NerubianDevice2(GO); }

	void OnActivate(Player* pPlayer)
	{
		if (pPlayer->GetMapId() != MAP_AHN_KAHET)
			return;

		if (pPlayer->GetMapMgr()->GetScript())
		{
			pPlayer->GetMapMgr()->GetScript()->SetInstanceData(Data_EncounterState, DATA_SPHERE_EVENT2, State_Finished);
		}
	}
};


void SetupAhnkahet(ScriptMgr* mgr)
{
	mgr->register_instance_script(MAP_AHN_KAHET, &AhnkahetScript::Create);

	mgr->register_gameobject_script(193094, &NerubianDevice1::Create);
	mgr->register_gameobject_script(193093, &NerubianDevice2::Create);

	mgr->register_creature_script(CN_ELDER_NADOX, &instanceboss_elder_nadox::Create);
	mgr->register_creature_script(CN_PRINCE_TALDARAM, &instanceboss_taldaram::Create);
	mgr->register_creature_script(CN_AHN_KAHAR_WATCHER, &mob_ahnkahar_watcher::Create);
}