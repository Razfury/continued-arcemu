/*
* ArcScripts for ArcEmu MMORPG Server
* Copyright (C) 2009 ArcEmu Team <http://www.arcemu.org/>
* Copyright (C) 2008-2009 Sun++ Team <http://www.sunscripting.com/>
* Copyright (C) 2005-2007 Ascent Team <http://www.ascentemu.com/>
* Copyright (C) 2007-2008 Moon++ Team <http://www.moonplusplus.info/>
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

/* To-Do:
	Finish Kritkhir Encounter, needs more blizzlike, may need InstanceScript
	Anuburak
	Add's AI and trash
	*/

#include "Setup.h"

class InstanceAzjolNerubScript : public MoonInstanceScript
{
public:

    MOONSCRIPT_INSTANCE_FACTORY_FUNCTION(InstanceAzjolNerubScript, MoonInstanceScript);
    InstanceAzjolNerubScript(MapMgr* pMapMgr) : MoonInstanceScript(pMapMgr)
    {
        // Way to select bosses
        BuildEncounterMap();
        if (mEncounters.size() == 0)
            return;

        for (EncounterMap::iterator Iter = mEncounters.begin(); Iter != mEncounters.end(); ++Iter)
        {
            if ((*Iter).second.mState != State_Finished)
                continue;
        }
    }

    void SetData(uint32 pIndex, uint32 pData)
    {
        pData = 0;
    };

    void OnGameObjectPushToWorld(GameObject* pGameObject) { }

    void OnPlayerEnter(Player* player)
    {
        if (player)
        {
           // Hack! Load missing cells so the whole instance is spawned some scripts need this DO NOT REMOVE.
            player->GetMapMgr()->VisitCoordsInstance(533.01f, 678.32f, player);
        }
    }

    void SetInstanceData(uint32 pType, uint32 pIndex, uint32 pData)
    {
        if (pType != Data_EncounterState || pIndex == 0)
            return;

        EncounterMap::iterator Iter = mEncounters.find(pIndex);
        if (Iter == mEncounters.end())
            return;

        (*Iter).second.mState = (EncounterState)pData;
    }

    uint32 GetInstanceData(uint32 pType, uint32 pIndex)
    {
        if (pType != Data_EncounterState || pIndex == 0)
            return 0;

        EncounterMap::iterator Iter = mEncounters.find(pIndex);
        if (Iter == mEncounters.end())
            return 0;

        return (*Iter).second.mState;
    }

    void OnCreatureDeath(Creature* pCreature, Unit* pUnit)
    {
        EncounterMap::iterator Iter = mEncounters.find(pCreature->GetEntry());
        if (Iter == mEncounters.end())
            return;

        (*Iter).second.mState = State_Finished;

        return;
    }
};


//Krikthir The Gatewatcher BOSS
//Krikthir The Gatewatcher
enum KrikthirSpells
{
    SPELL_SUBBOSS_AGGRO_TRIGGER = 52343,
    SPELL_SWARM = 52440,
    SPELL_MIND_FLAY = 52586,
    SPELL_CURSE_OF_FATIGUE = 52592,
    SPELL_FRENZY = 28747
};

enum KrikthirEvents
{
    EVENT_KRIK_START_WAVE = 1,
    EVENT_KRIK_SUMMON = 2,
    EVENT_KRIK_MIND_FLAY = 3,
    EVENT_KRIK_CURSE = 4,
    EVENT_KRIK_HEALTH_CHECK = 5,
    EVENT_KRIK_ENTER_COMBAT = 6,
    EVENT_KILL_TALK = 7,
    EVENT_CALL_ADDS = 8,
    EVENT_KRIK_CHECK_EVADE = 9
};

enum KrikthirNpcs
{
    BOSS_KRIKTHIR = 28684,
    NPC_WATCHER_NARJIL = 28729,
    NPC_WATCHER_GASHRA = 28730,
    NPC_WATCHER_SILTHIK = 28731,
    NPC_WARRIOR = 28732,
    NPC_SKIRMISHER = 28734,
    NPC_SHADOWCASTER = 28733
};
class KrikthirAI : public AICreatureScript
{
    AI_CREATURE_SCRIPT_FUNCTION(KrikthirAI, AICreatureScript);
    KrikthirAI(Creature* pCreature) : AICreatureScript(pCreature)
    {
        maxSpawn = 0;
    }

    void OnCombatStop(Unit* pTarget)
    {
        RemoveAIUpdateEvent();
        events.Reset();
        /*_unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_WATCHER_NARJIL, 511.8f, 666.493f, 776.278f, 0.977f, true, false, 0, 0);
        _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_SHADOWCASTER, 518.13f, 667.0f, 775.74f, 1.0f, true, false, 0, 0);
        _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_WARRIOR, 506.75f, 670.7f, 776.24f, 0.92f, true, false, 0, 0);
        _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_WATCHER_GASHRA, 526.66f, 663.605f, 775.805f, 1.23f, true, false, 0, 0);
        _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_SKIRMISHER, 522.23f, 668.97f, 775.66f, 1.18f, true, false, 0, 0);
        _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_WARRIOR, 532.4f, 666.47f, 775.67f, 1.45f, true, false, 0, 0);
        _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_WATCHER_SILTHIK, 543.826f, 665.123f, 776.245f, 1.55f, true, false, 0, 0);
        _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_SKIRMISHER, 547.5f, 669.96f, 776.1f, 2.3f, true, false, 0, 0);
        _unit->GetMapMgr()->GetInterface()->SpawnCreature(NPC_SHADOWCASTER, 536.96f, 667.28f, 775.6f, 1.72f, true, false, 0, 0);*/
        ParentClass::OnCombatStop(pTarget);
    }

    void OnLoad()
    {
        RegisterAIUpdateEvent(1000);
        events.ScheduleEvent(10, 30000);
        ParentClass::OnLoad();
    }

    void EnterCombat(Unit* pTarget)
    {
        events.ScheduleEvent(EVENT_KRIK_HEALTH_CHECK, 1000);
        events.ScheduleEvent(EVENT_KRIK_MIND_FLAY, 13000);
        events.ScheduleEvent(EVENT_KRIK_SUMMON, 17000);
        events.ScheduleEvent(EVENT_KRIK_CURSE, 8000);
        events.ScheduleEvent(EVENT_CALL_ADDS, 1000);
        Emote("This kingdom belongs to the Scourge. Only the dead may enter!", Text_Yell, 14075);
        ParentClass::EnterCombat(pTarget);
    }

    void SpawnSwarmInsect()
    {
        uint32 randomX = _unit->GetPositionX() + urand(1, 16);
        uint32 randomY = _unit->GetPositionY() + urand(1, 8);
        Creature* swarmer = _unit->GetMapMgr()->GetInterface()->SpawnCreature(28735, randomX, randomY, _unit->GetPositionZ(), 0, true, false, 0, 0);
        if (swarmer)
            swarmer->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1, 0);
    }

    void UpdateAI()
    {
        if (!_unit->CombatStatus.IsInCombat())
        {
            events.Update(1000);
            if (events.ExecuteEvent() == 10)
            {
                switch (urand(0, 2))
                {
                case 0:
                    Emote("Keep an eye on the tunnel. We must not let anyone through!", Text_Yell, 14082);
                    break;
                case 1:
                    Emote("I hear footsteps. Be on your guard.", Text_Yell, 14083);
                    break;
                case 2:
                    Emote("I sense the living. Be ready.", Text_Yell, 14084);
                    break;
                }
                events.ScheduleEvent(10, 60000);
            }
            return;
        }

        events.Update(1000);
        if (_unit->IsCasting())
            return;

        switch (events.ExecuteEvent())
        {
        case EVENT_KRIK_HEALTH_CHECK:
            if (_unit->GetHealthPct() <= 10 && _AIPhase == 0)
            {
                events.PopEvent();
                _unit->CastSpell(_unit, SPELL_FRENZY, true);
                Announce("Kirk'thir the Gatewatcher goes into a frenzy!");
                _AIPhase = 1;
                break;
            }
            events.ScheduleEvent(EVENT_KRIK_HEALTH_CHECK, 1000);
            break;
        case EVENT_KRIK_SUMMON:
            Emote("Dinner time, my pets!", Text_Yell, 14086);
            maxSpawn = 0;
            _unit->CastSpell(_unit, SPELL_SWARM, false);
            events.ScheduleEvent(EVENT_KRIK_SUMMON, 25000);
            break;
        case EVENT_KRIK_MIND_FLAY:
            _unit->CastSpell(_unit->GetVictim(), SPELL_MIND_FLAY, false);
            events.ScheduleEvent(EVENT_KRIK_MIND_FLAY, 15000);
            break;
        case EVENT_KRIK_CURSE:
            if (Unit* target = GetRandomPlayerTarget())
                _unit->CastSpell(target, SPELL_CURSE_OF_FATIGUE, true);
            events.ScheduleEvent(EVENT_KRIK_CURSE, 10000);
            break;
        case EVENT_CALL_ADDS:
            if (maxSpawn != 50)
            {
                SpawnSwarmInsect();
                ++maxSpawn;
            }
            break;
        }
        ParentClass::UpdateAI();
    }

    void OnTargetDied(Unit* mKiller)
    {
        switch (urand(0,1))
        {
        case 0:
            Emote("You were foolish to come.", Text_Yell, 14077);
            break;
        case 1:
            Emote("As Anub'Arak commands!", Text_Yell, 14078);
            break;
        }
    }

    void JustDied(Unit* pKiller)
    {
        events.Reset();
        Emote("I should be grateful... but I long ago lost the capacity....", Text_Yell, 14087);

        GameObject* Doors = _unit->GetMapMgr()->GetInterface()->GetGameObjectNearestCoords(530.38f, 628.602f, 777.98f, 192395);
        if (Doors)
            Doors->Despawn(1000, 0);
        ParentClass::JustDied(pKiller);
    }
private:
    bool mEnraged;
    uint32 maxSpawn;
};

enum HadronoxSpells
{
	SPELL_ACID_CLOUD = 53400,
	H_SPELL_ACID_CLOUD = 59419,
	SPELL_LEECH_POISON = 53030,
	SPELL_LEECH_POISON_PCT = 53800,
	H_SPELL_LEECH_POISON = 59417,
	SPELL_PIERCE_ARMOR = 53418,
	SPELL_WEB_GRAB = 57731,
	H_SPELL_WEB_GRAB = 59421,
	SPELL_WEB_FRONT_DOORS = 53177,
	SPELL_WEB_SIDE_DOORS = 53185,

	// anubar crusher
	SPELL_FRENZY = 53801,
	SPELL_SMASH = 53318,
	H_SPELL_SMASH = 59346
};

enum HadronoxCreatures
{
	NPC_HADRONOX = 28921,
	NPC_ANUBAR_CHAMPION = 29117,
	NPC_ANUBAR_CRYPT_FIEND = 29118,
	NPC_ADD_CHAMPION = 29062,
	NPC_ADD_CRYPT_FIEND = 29063,
	NPC_ADD_NECROMANCER = 29064
};

enum HadronoxEvents
{
	EVENT_CLOUD = 1,
	EVENT_LEECH,
	EVENT_PIECRE,
	EVENT_GRAB,
	EVENT_SPAWN,
	EVENT_FORCEMOVE,
	EVENT_UNSTUCK // temporary
};

Location HadronoxWaypoints[14] =
{
	{565.681458f, 513.247803f, 698.7f, 0.0f},
	{578.135559f, 512.468811f, 698.5f, 0.0f},
	{596.820007f, 510.811249f, 694.8f, 0.0f},
	{608.183777f, 513.395508f, 695.5f, 0.0f},
	{618.232422f, 525.414185f, 697.0f, 0.0f},
	{620.192932f, 539.329041f, 706.3f, 0.0f},
	{615.690979f, 552.474915f, 713.2f, 0.0f},
	{607.791870f, 566.636841f, 720.1f, 0.0f},
	{599.256104f, 580.134399f, 724.7f, 0.0f},
	{591.600220f, 589.028748f, 730.7f, 0.0f},
	{587.181580f, 596.026489f, 739.5f, 0.0f},
	{577.790588f, 583.640930f, 727.9f, 0.0f},
	{559.274597f, 563.085999f, 728.7f, 0.0f},
	{528.960815f, 565.453613f, 733.2f, 0.0f}
};

Location AddWaypoints[6] =
{
	{582.961304f, 606.298645f, 739.3f, 0.0f},
	{590.240662f, 597.044556f, 739.2f, 0.0f},
	{600.471436f, 585.080200f, 726.2f, 0.0f},
	{611.900940f, 562.884094f, 718.9f, 0.0f},
	{615.533936f, 529.052002f, 703.3f, 0.0f},
	{606.844604f, 515.054199f, 696.2f, 0.0f}
};
//boss Hadronox
class HadronoxAI : public AICreatureScript
{
		AI_CREATURE_SCRIPT_FUNCTION(HadronoxAI, AICreatureScript);
		HadronoxAI(Creature* pCreature) : AICreatureScript(pCreature)
		{
		};
};

//Watcher Gashra.
#define CN_GASHRA 28730
class GashraAI : public AICreatureScript
{
		AI_CREATURE_SCRIPT_FUNCTION(GashraAI, AICreatureScript);
		GashraAI(Creature* pCreature) : AICreatureScript(pCreature)
		{};

        void EnterCombat(Unit* pTarget)
        {
            Unit* boss_kriththir = ForceCreatureFind(BOSS_KRIKTHIR);
            if (boss_kriththir)
            {
                boss_kriththir->SendChatMessage(14, 0, "We must hold the gate. Attack! Tear them limb from limb!");
                boss_kriththir->PlaySoundToSet(14080);
            }
            ParentClass::EnterCombat(pTarget);
        }

};

//Watcher Narjil
#define CN_NARJIL 28729
class NarjilAI : public AICreatureScript
{
		AI_CREATURE_SCRIPT_FUNCTION(NarjilAI, AICreatureScript);
		NarjilAI(Creature* pCreature) : AICreatureScript(pCreature)
		{};

        void EnterCombat(Unit* pTarget)
        {
            Unit* boss_kriththir = ForceCreatureFind(BOSS_KRIKTHIR);
            if (boss_kriththir)
            {
                boss_kriththir->SendChatMessage(14, 0, "The gate must be protected at all costs. Rip them to shreds!");
                boss_kriththir->PlaySoundToSet(14081);
            }
            ParentClass::EnterCombat(pTarget);
        }

};

//Watcher Silthik
#define CN_SILTHIK 28731
class SilthikAI : public AICreatureScript
{
		AI_CREATURE_SCRIPT_FUNCTION(SilthikAI, AICreatureScript);
		SilthikAI(Creature* pCreature) : AICreatureScript(pCreature)
		{};

        void EnterCombat(Unit* pTarget)
        {
            Unit* boss_kriththir = ForceCreatureFind(BOSS_KRIKTHIR);
            if (boss_kriththir)
            {
                boss_kriththir->SendChatMessage(14, 0, "We are besieged! Strike out and bring back their corpses!");
                boss_kriththir->PlaySoundToSet(14079);
            }

            events.ScheduleEvent(1, urand(4000, 9000));
            events.ScheduleEvent(2, urand(13000, 16000));
            events.ScheduleEvent(3, urand(18000, 21000));
            ParentClass::EnterCombat(pTarget);
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
                case 1:
                    if (IsHeroic())
                    {
                        _unit->DoCastVictim(59364);
                    }
                    else
                    {
                        _unit->DoCastVictim(52469);
                    }
                    events.ScheduleEvent(1, urand(9000, 12000));
                    break;
                case 2:
                    if (IsHeroic())
                    {
                        if (GetRandomPlayerTarget())
                            _unit->DoCast(GetRandomPlayerTarget(), 59366);
                    }
                    else
                    {
                        if (GetRandomPlayerTarget())
                            _unit->DoCast(GetRandomPlayerTarget(), 52493);
                    }
                    events.ScheduleEvent(2, urand(19000, 22000));
                    break;
                case 3:
                    if (GetRandomPlayerTarget() != _unit->GetCurrentVictim())
                    {
                        _unit->DoCast(GetRandomPlayerTarget(), 52086);
                    }
                    events.ScheduleEvent(3, urand(20000, 23000));
                    break;
                }
            }
            ParentClass::UpdateAI();
        }
};

//Anub'arak
enum AnubArakSpells
{
    SPELL_CARRION_BEETLES = 53520,
    SPELL_SUMMON_CARRION_BEETLES = 53521,
    SPELL_LEECHING_SWARM = 53467,
    SPELL_POUND = 53472,
    SPELL_POUND_DAMAGE = 53509,
    SPELL_IMPALE_PERIODIC = 53456,
    SPELL_EMERGE = 53500,
    SPELL_SUBMERGE = 53421,
    SPELL_SELF_ROOT = 42716,

    SPELL_SUMMON_DARTER = 53599,
    SPELL_SUMMON_ASSASSIN = 53610,
    SPELL_SUMMON_GUARDIAN = 53614,
    SPELL_SUMMON_VENOMANCER = 53615,
};

enum AnubArakMisc
{
    ACHIEV_TIMED_START_EVENT = 20381,

    EVENT_CHECK_HEALTH_25 = 1,
    EVENT_CHECK_HEALTH_50 = 2,
    EVENT_CHECK_HEALTH_75 = 3,
    EVENT_CARRION_BEETELS = 4,
    EVENT_LEECHING_SWARM = 5,
    EVENT_IMPALE = 6,
    EVENT_POUND = 7,
    EVENT_CLOSE_DOORS = 8,
    EVENT_EMERGE = 9,
    EVENT_SUMMON_VENOMANCER = 10,
    EVENT_SUMMON_DARTER = 11,
    EVENT_SUMMON_GUARDIAN = 12,
    EVENT_SUMMON_ASSASSINS = 13,
    EVENT_ENABLE_ROTATE = 14,
    ANUB_EVENT_KILL_TALK = 15
};
class AnubArakAI : public AICreatureScript
{
    AI_CREATURE_SCRIPT_FUNCTION(AnubArakAI, AICreatureScript);
    AnubArakAI(Creature* pCreature) : AICreatureScript(pCreature)
    {};

    void OnLoad()
    {
        Emote("I was king of this empire once, long ago. In life I stood as champion. In death I returned as conqueror. Now I protect the kingdom once more. Ironic, yes? ", Text_Yell, 14053);
        ParentClass::OnLoad();
    }

    void EnterCombat(Unit* pTarget)
    {
        Emote("Eternal agony awaits you!", Text_Yell, 14054);
        events.ScheduleEvent(EVENT_CARRION_BEETELS, 6500);
        events.ScheduleEvent(EVENT_LEECHING_SWARM, 20000);
        events.ScheduleEvent(EVENT_POUND, 15000);
        events.ScheduleEvent(EVENT_CHECK_HEALTH_75, 1000);
        events.ScheduleEvent(EVENT_CHECK_HEALTH_50, 1000);
        events.ScheduleEvent(EVENT_CHECK_HEALTH_25, 1000);
        events.ScheduleEvent(EVENT_CLOSE_DOORS, 5000);
        ParentClass::EnterCombat(pTarget);
    }

    void OnCombatStop(Unit* pTarget)
    {
        RemoveAIUpdateEvent();
        _unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2 | UNIT_FLAG_NOT_SELECTABLE);

		if (go_1)
			go_1->SetState(GAMEOBJECT_STATE_OPEN);
		if (go_2)
			go_2->SetState(GAMEOBJECT_STATE_OPEN);
		if (go_3)
			go_3->SetState(GAMEOBJECT_STATE_OPEN);

        ParentClass::OnCombatStop(pTarget);
    }

    void KilledUnit(Unit* pTarget)
    {
        Emote("Soon the master's voice will call to you.", Text_Yell, 14057);
        ParentClass::KilledUnit(pTarget);
    }

    void JustDied(Unit* pTarget)
    {
        Emote("Ahhh... RAAAAAGH! Never thought... I would be free of him....", Text_Yell, 14069);
        ParentClass::JustDied(pTarget);
    }

    void UpdateAI()
    {
        events.Update(1000);
        if (_unit->IsCasting())
            return;

        Unit* cr = NULL;

        switch (uint32 eventId = events.ExecuteEvent())
        {
        case EVENT_CLOSE_DOORS:
			if (go_1)
				go_1->SetState(GAMEOBJECT_STATE_CLOSED);
			if (go_2)
				go_2->SetState(GAMEOBJECT_STATE_CLOSED);
			if (go_3)
				go_3->SetState(GAMEOBJECT_STATE_CLOSED);
            break;
        case EVENT_CARRION_BEETELS:
            _unit->CastSpell(_unit, SPELL_CARRION_BEETLES, false);
            events.ScheduleEvent(EVENT_CARRION_BEETELS, 25000);
            break;
        case EVENT_LEECHING_SWARM:
            Emote("Uunak-hissss tik-k-k-k-k!", Text_Yell, 14067);
            _unit->CastSpell(_unit, SPELL_LEECHING_SWARM, false);
            events.ScheduleEvent(EVENT_LEECHING_SWARM, 20000);
            break;
        case EVENT_POUND:
            if (Unit* target = GetRandomPlayerTarget())
            {
                _unit->CastSpell(target, SPELL_POUND, false);
            }
            events.ScheduleEvent(EVENT_POUND, 18000);
            break;
        case EVENT_ENABLE_ROTATE:
            _unit->Unroot();
            break;
        case EVENT_CHECK_HEALTH_25:
        case EVENT_CHECK_HEALTH_50:
        case EVENT_CHECK_HEALTH_75:
            if (_unit->GetHealthPct() <= (eventId * 25))
            {
                Emote("Come forth, my brethren! Feast on their flesh!", Text_Yell, 14059);
                _unit->Root();
                _unit->GetAIInterface()->disable_melee = true;
                _unit->SetDisplayId(11686);
                _unit->CastSpell(_unit, SPELL_IMPALE_PERIODIC, true);
                //_unit->CastSpell(_unit, SPELL_SUBMERGE, false); // Not supported by core.
                _unit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2 | UNIT_FLAG_NOT_SELECTABLE);

                events.DelayEvents(46000, 0);
                events.ScheduleEvent(EVENT_EMERGE, 45000);
                events.ScheduleEvent(EVENT_SUMMON_ASSASSINS, 2000);
                events.ScheduleEvent(EVENT_SUMMON_GUARDIAN, 4000);
                events.ScheduleEvent(EVENT_SUMMON_ASSASSINS, 15000);
                events.ScheduleEvent(EVENT_SUMMON_VENOMANCER, 20000);
                events.ScheduleEvent(EVENT_SUMMON_DARTER, 30000);
                events.ScheduleEvent(EVENT_SUMMON_ASSASSINS, 35000);
                break;
            }
            events.ScheduleEvent(eventId, 500);
            break;
        case EVENT_EMERGE:
            //_unit->RemoveAura(SPELL_SUBMERGE); // Not supported by core.
            _unit->RemoveAura(SPELL_IMPALE_PERIODIC);
            _unit->Unroot();
            _unit->GetAIInterface()->disable_melee = false;
            _unit->SetDisplayId(_unit->GetNativeDisplayId());
            //_unit->CastSpell(_unit, SPELL_EMERGE, true); // Not supported
            _unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2 | UNIT_FLAG_NOT_SELECTABLE);
            break;
        case EVENT_SUMMON_ASSASSINS:
            cr = _unit->GetMapMgr()->GetInterface()->SpawnCreature(29214, 509.32f, 247.42f, 239.48f, 0.0f, true, false, 0, 0);
            cr->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1);
            cr = _unit->GetMapMgr()->GetInterface()->SpawnCreature(29214, 589.51f, 240.19f, 236.0f, 0.0f, true, false, 0, 0);
            cr->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1);
            break;
        case EVENT_SUMMON_DARTER:
            cr = _unit->GetMapMgr()->GetInterface()->SpawnCreature(29213, 509.32f, 247.42f, 239.48f, 0.0f, true, false, 0, 0);
            cr->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1);
            cr = _unit->GetMapMgr()->GetInterface()->SpawnCreature(29213, 589.51f, 240.19f, 236.0f, 0.0f, true, false, 0, 0);
            cr->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1);
            break;
        case EVENT_SUMMON_GUARDIAN:
            cr = _unit->GetMapMgr()->GetInterface()->SpawnCreature(29216, 550.34f, 316.00f, 234.30f, 0.0f, true, false, 0, 0);
            cr->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1);
            break;
        case EVENT_SUMMON_VENOMANCER:
            cr = _unit->GetMapMgr()->GetInterface()->SpawnCreature(29217, 550.34f, 316.00f, 234.30f, 0.0f, true, false, 0, 0);
            cr->GetAIInterface()->AttackReaction(GetRandomPlayerTarget(), 1);
            break;
        }
        ParentClass::UpdateAI();
    }
	private:
		GameObject* go_1 = _unit->GetMapMgr()->GetInterface()->GetGameObjectNearestCoords(_unit->GetPositionX(), _unit->GetPositionY(), _unit->GetPositionZ(), 192396);
		GameObject* go_2 = _unit->GetMapMgr()->GetInterface()->GetGameObjectNearestCoords(_unit->GetPositionX(), _unit->GetPositionY(), _unit->GetPositionZ(), 192397);
		GameObject* go_3 = _unit->GetMapMgr()->GetInterface()->GetGameObjectNearestCoords(_unit->GetPositionX(), _unit->GetPositionY(), _unit->GetPositionZ(), 192398);
};

void SetupAzjolNerub(ScriptMgr* mgr)
{
	//Bosses
	mgr->register_creature_script(BOSS_KRIKTHIR, &KrikthirAI::Create);
//	mgr->register_creature_script(BOSS_HADRONOX, &HadronoxAI::Create);
    mgr->register_creature_script(29120, &AnubArakAI::Create);

    mgr->register_instance_script(MAP_AZJOL_NERUB, &InstanceAzjolNerubScript::Create);

	// watchers
	mgr->register_creature_script(CN_GASHRA, &GashraAI::Create);
	mgr->register_creature_script(CN_NARJIL, &NarjilAI::Create);
	mgr->register_creature_script(CN_SILTHIK, &SilthikAI::Create);
}
