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
		// We only spawn on heroic modes.
		if (_unit->GetMapMgr()->GetMapId() == 619 && !IsHeroic())
		{
			_unit->SetVisible(true);
		}
		ParentClass::OnLoad();
	}
};

/*enum PrinceTaldaramSpells
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
    DATA_SPHERE_EVENT = 99999,
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
            if (pInstance->GetInstanceData(Data_EncounterState, DATA_SPHERE_EVENT) == State_Finished)
                DoAction(ACTION_FREE);
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

            //if (pInstance)
               // pInstance->HandleGameObject(pInstance->GetData64(DATA_PRINCE_TALDARAM_PLATFORM), true);
        }
    }

    void EnterCombat(Unit* who)
    {
        if (pInstance)
            pInstance->SetInstanceData(Data_EncounterState, _unit->GetEntry(), State_InProgress);

        Emote("I will feast on your remains.", Text_Yell, 14360);
        AddCombatEvents();

        _unit->RemoveAllAuras();
        _unit->InterruptSpell();

		_unit->SetFlag(UNIT_DYNAMIC_FLAGS, )
    }

    void AddCombatEvents()
    {
        events.Reset();
        events.AddCombatEvent(EVENT_PRINCE_FLAME_SPHERES, 10000);
        events.AddCombatEvent(EVENT_PRINCE_BLOODTHIRST, 10000);
        vanishTarget = 0;
        vanishDamage = 0;
    }
    
    void UpdateAI()
    {
        events.Update(1000);
        if (_unit->IsCasting())
            return;

        switch (events.GetEvent())
        {
        case EVENT_PRINCE_BLOODTHIRST:
        {
            _unit->CastSpell(_unit->GetVictim(), SPELL_BLOODTHIRST, false);
            events.RepeatEvent(10000);
            break;
        }
        case EVENT_PRINCE_FLAME_SPHERES:
        {
            _unit->CastSpell(_unit->GetVictim(), SPELL_CONJURE_FLAME_SPHERE, false);
            events.ReAddCombatEvent(EVENT_PRINCE_VANISH, 14000);
            Creature *cr;
            if (cr = _unit->GetMapMgr()->GetInterface()->SpawnCreature(CREATURE_FLAME_SPHERE, _unit->GetPositionX(), _unit->GetPositionY(), _unit->GetPositionZ() + 5.0f, 0.0f, true, false, 0, 0))
            {
                cr->Despawn(10000, 0);
                _unit->combatSummon1GUID = cr->GetGUID();
            }

            if (_unit->GetMapMgr()->iInstanceMode == Mode_Heroic)
            {
                if (cr = _unit->GetMapMgr()->GetInterface()->SpawnCreature(CREATURE_FLAME_SPHERE_1, _unit->GetPositionX(), _unit->GetPositionY(), _unit->GetPositionZ() + 5.0f, 0.0f, true, false, 0, 0))
                {
                    cr->Despawn(10000, 0);
                    _unit->combatSummon1GUID = cr->GetGUID();
                }

                if (cr = _unit->GetMapMgr()->GetInterface()->SpawnCreature(CREATURE_FLAME_SPHERE_2, _unit->GetPositionX(), _unit->GetPositionY(), _unit->GetPositionZ() + 5.0f, 0.0f, true, false, 0, 0))
                {
                    cr->Despawn(10000, 0);
                    _unit->combatSummon2GUID = cr->GetGUID();
                }
            }
            events.RepeatEvent(15000);
            break;
        }
        case EVENT_PRINCE_VANISH:
        {
            events.PopEvent();
            //Count alive players
            uint8 count = 0;
            Unit *pTarget;
            if (pInstance && pInstance->GetInstance()->GetPlayerCount() > 2)
                count = 3;

            //He only vanishes if there are 3 or more alive players
            if (count > 2)
            {
                Emote("Your heartbeat is music to my ears.", Text_Yell, 14361);
                //_unit->CastSpell(me, SPELL_VANISH, false);
                // HACK! When prince casts the above spell he resets in combat hackfix this until we find a way around it.
                

                events.CancelEvent(EVENT_PRINCE_FLAME_SPHERES);
                events.CancelEvent(EVENT_PRINCE_BLOODTHIRST);
                events.AddCombatEvent(EVENT_PRINCE_VANISH_RUN, 2499);
                if (Unit* pEmbraceTarget = GetRandomPlayerTarget())
                    vanishTarget = pEmbraceTarget->GetGUID();
            }
            break;
        }
        case EVENT_PRINCE_VANISH_RUN:
        {
            if (Unit *vT = ObjectAccessor::GetUnit(*me, vanishTarget))
            {
                _unit->UpdatePosition(vT->GetPositionX(), vT->GetPositionY(), vT->GetPositionZ(), _unit->GetAngle(vT), true);
                _unit->CastSpell(vT, SPELL_EMBRACE_OF_THE_VAMPYR, false);
                _unit->RemoveAura(SPELL_VANISH);
            }

            events.PopEvent();
            events.AddCombatEvent(EVENT_PRINCE_RESCHEDULE, 20000);
            break;
        }
        case EVENT_PRINCE_RESCHEDULE:
        {
            events.PopEvent();
            AddCombatEvents();
            break;
        }
        }
    }

    void DamageTaken(Unit* mAttacker, uint32 &damage)
    {
        if (vanishTarget)
        {
            vanishDamage += damage;
            if (vanishDamage > (uint32)DUNGEON_MODE(DATA_EMBRACE_DMG, DATA_EMBRACE_DMG_H))
            {
                AddCombatEvents();
                _unit->InterruptSpell();
            }
        }
        ParentClass::OnDamageTaken(mAttacker, damage);
    }

    void JustDied(Unit* killer)
    {
        _unit->DespawnCombatSummons();
        Talk(SAY_DEATH);

        ParentClass::JustDied(killer);
    }

    void KilledUnit(Unit * victim)
    {
        if (urand(0, 1))
            return;

        if (vanishTarget && victim->GetGUID() == vanishTarget)
            AddCombatEvents();

        Talk(SAY_SLAY);
        ParentClass::KilledUnit(victim);
    }

}; */


void SetupAhnkahet(ScriptMgr* mgr)
{
	mgr->register_creature_script(31104, &mob_ahnkahar_watcher::Create);
}