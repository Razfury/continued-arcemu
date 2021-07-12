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
#include "ScriptMgr.h"

uint32 creatureEntry;

class creature_combat_spells : public AICreatureScript
{
public:
    AI_CREATURE_SCRIPT_FUNCTION(creature_combat_spells, AICreatureScript);
    creature_combat_spells(Creature* pCreature) : AICreatureScript(pCreature)
    {}
    void SetUpCombatSpells()
    {
        startcd_min = 0;
        startcd_max = 0;
        combatcd_min = 0;
        combatcd_max = 0;
        startcd_min2 = 0;
        startcd_max2 = 0;
        combatcd_min2 = 0;
        combatcd_max2 = 0;
        startcd_min3 = 0;
        startcd_max3 = 0;
        combatcd_min3 = 0;
        combatcd_max3 = 0;
        startcd_min4 = 0;
        startcd_max4 = 0;
        combatcd_min4 = 0;
        combatcd_max4 = 0;
        startcd_min5 = 0;
        startcd_max5 = 0;
        combatcd_min5 = 0;
        combatcd_max5 = 0;
        startcd_min6 = 0;
        startcd_max6 = 0;
        combatcd_min6 = 0;
        combatcd_max6 = 0;

        target = 0;
        target1 = 0;
        target2 = 0;
        target3 = 0;
        target4 = 0;
        target5 = 0;
        target6 = 0;

        spellid0 = 0;
        spellid1 = 0;
        spellid2 = 0;
        spellid3 = 0;
        spellid4 = 0;
        spellid5 = 0;
        spellid6 = 0;

        modes = 0;

        QueryResult* result = WorldDatabase.Query("SELECT * FROM creature_spells WHERE creature_spells.entry = %u", _unit->GetEntry());
        if (result)
        {
            uint32 Count = 0;
            do
            {
                Field* fields = result->Fetch();

                if (result->GetRowCount() > 6) // Right now we don'tr support rows higher then 6 so return
                {
                    sLog.Error("CreatureSpells.cpp", "creature: %u has more then 6 row entries that we don't support", _unit->GetEntry());
                    return;
                }

                if (Count == 0)
                {
                    startcd_min = fields[1].GetUInt32();
                    startcd_max = fields[2].GetUInt32();
                    combatcd_min = fields[3].GetUInt32();
                    combatcd_max = fields[4].GetUInt32();

                    spellid0 = fields[5].GetUInt32();
                    if (spellid0 == 0)
                    {
                        sLog.Error("CreatureSpells.cpp", "Creature: %u has invalid spellid0 returning as a crash would occur.", _unit->GetEntry());
                        return;
                    }

                    modes = fields[7].GetUInt32();

                    if (startcd_min == 0 && startcd_max == 0 && combatcd_min == 0 && combatcd_max == 0)
                    {
                        // This is usually an on aggro only (1 time cast) so don't schedule any events
                        _unit->DoCastVictim(spellid0);
                    }
                    target = fields[6].GetUInt32();
                }
                if (Count == 1)
                {
                    startcd_min1 = fields[1].GetUInt32();
                    startcd_max1 = fields[2].GetUInt32();
                    combatcd_min1 = fields[3].GetUInt32();
                    combatcd_max1 = fields[4].GetUInt32();

                    spellid1 = fields[5].GetUInt32();
                    if (spellid1 == 0)
                    {
                        sLog.Error("CreatureSpells.cpp", "Creature: %u has invalid spellid1 returning as a crash would occur.", _unit->GetEntry());
                        return;
                    }

                    modes = fields[7].GetUInt32();
                    target1 = fields[6].GetUInt32();

                    if (startcd_min1 == 0 && startcd_max1 == 0 && combatcd_min1 == 0 && combatcd_max1 == 0)
                    {
                        // This is usually an on aggro only (1 time cast) so don't schedule any events
                        if (target1 == 1)
                        {
                            _unit->DoCastVictim(spellid1);
                        }
                        if (target1 == 0)
                        {
                            _unit->DoCast(_unit, spellid1);
                        }
                    }
                }
                if (Count == 2)
                {
                    startcd_min2 = fields[1].GetUInt32();
                    startcd_max2 = fields[2].GetUInt32();
                    combatcd_min2 = fields[3].GetUInt32();
                    combatcd_max2 = fields[4].GetUInt32();

                    spellid2 = fields[5].GetUInt32();
                    if (spellid2 == 0)
                    {
                        sLog.Error("CreatureSpells.cpp", "Creature: %u has invalid spellid2 returning as a crash would occur.", _unit->GetEntry());
                        return;
                    }

                    modes = fields[7].GetUInt32();
                    target2 = fields[6].GetUInt32();

                    if (startcd_min2 == 0 && startcd_max2 == 0 && combatcd_min2 == 0 && combatcd_max2 == 0)
                    {
                        // This is usually an on aggro only (1 time cast) so don't schedule any events
                        if (target2 == 1)
                        {
                            _unit->DoCastVictim(spellid2);
                        }
                        if (target2 == 0)
                        {
                            _unit->DoCast(_unit, spellid2);
                        }
                    }
                }
                if (Count == 3)
                {
                    startcd_min3 = fields[1].GetUInt32();
                    startcd_max3 = fields[2].GetUInt32();
                    combatcd_min3 = fields[3].GetUInt32();
                    combatcd_max3 = fields[4].GetUInt32();

                    spellid3 = fields[5].GetUInt32();
                    if (spellid3 == 0)
                    {
                        sLog.Error("CreatureSpells.cpp", "Creature: %u has invalid spellid3 returning as a crash would occur.", _unit->GetEntry());
                        return;
                    }

                    modes = fields[7].GetUInt32();
                    target3 = fields[6].GetUInt32();

                    if (startcd_min3 == 0 && startcd_max3 == 0 && combatcd_min3 == 0 && combatcd_max3 == 0)
                    {
                        // This is usually an on aggro only (1 time cast) so don't schedule any events
                        if (target3 == 1)
                        {
                            _unit->DoCastVictim(spellid3);
                        }
                        if (target3 == 0)
                        {
                            _unit->DoCast(_unit, spellid3);
                        }
                    }
                }
                if (Count == 4)
                {
                    startcd_min4 = fields[1].GetUInt32();
                    startcd_max4 = fields[2].GetUInt32();
                    combatcd_min4 = fields[3].GetUInt32();
                    combatcd_max4 = fields[4].GetUInt32();

                    spellid4 = fields[5].GetUInt32();
                    if (spellid4 == 0)
                    {
                        sLog.Error("CreatureSpells.cpp", "Creature: %u has invalid spellid4 returning as a crash would occur.", _unit->GetEntry());
                        return;
                    }

                    modes = fields[7].GetUInt32();
                    target4 = fields[6].GetUInt32();
                    if (startcd_min4 == 0 && startcd_max4 == 0 && combatcd_min4 == 0 && combatcd_max4 == 0)
                    {
                        // This is usually an on aggro only (1 time cast) so don't schedule any events
                        if (target4 == 1)
                        {
                            _unit->DoCastVictim(spellid4);
                        }
                        if (target4 == 0)
                        {
                            _unit->DoCast(_unit, spellid4);
                        }
                    }
                }
                if (Count == 5)
                {
                    startcd_min5 = fields[1].GetUInt32();
                    startcd_max5 = fields[2].GetUInt32();
                    combatcd_min5 = fields[3].GetUInt32();
                    combatcd_max5 = fields[4].GetUInt32();

                    spellid5 = fields[5].GetUInt32();
                    if (spellid5 == 0)
                    {
                        sLog.Error("CreatureSpells.cpp", "Creature: %u has invalid spellid5 returning as a crash would occur.", _unit->GetEntry());
                        return;
                    }

                    modes = fields[7].GetUInt32();
                    target5 = fields[6].GetUInt32();

                    if (startcd_min5 == 0 && startcd_max5 == 0 && combatcd_min5 == 0 && combatcd_max5 == 0)
                    {
                        // This is usually an on aggro only (1 time cast) so don't schedule any events
                        if (target5 == 1)
                        {
                            _unit->DoCastVictim(spellid5);
                        }
                        if (target5 == 0)
                        {
                            _unit->DoCast(_unit, spellid5);
                        }
                    }
                }
                if (Count == 6)
                {
                    startcd_min6 = fields[1].GetUInt32();
                    startcd_max6 = fields[2].GetUInt32();
                    combatcd_min6 = fields[3].GetUInt32();
                    combatcd_max6 = fields[4].GetUInt32();

                    spellid6 = fields[5].GetUInt32();
                    if (spellid6 == 0)
                    {
                        sLog.Error("CreatureSpells.cpp", "Creature: %u has invalid spellid6 returning as a crash would occur.", _unit->GetEntry());
                        return;
                    }

                    modes = fields[7].GetUInt32();
                    target6 = fields[6].GetUInt32();

                    if (startcd_min6 == 0 && startcd_max6 == 0 && combatcd_min6 == 0 && combatcd_max6 == 0)
                    {
                        // This is usually an on aggro only (1 time cast) so don't schedule any events
                        if (target6 == 1)
                        {
                            _unit->DoCastVictim(spellid6);
                        }
                        if (target6 == 0)
                        {
                            _unit->DoCast(_unit, spellid6);
                        }
                    }
                }

                //Don't schedule events for spells that require heroic mode
                if (modes == 1 && !IsHeroic())
                    return;

                switch (Count)
                {
                case 0:
                    if (startcd_min >= 0 && startcd_max > 0)
                    {
                        events.ScheduleEvent(1 + Count, urand(startcd_min, startcd_max));
                    }
                    else
                    {
                        events.ScheduleEvent(1 + Count, urand(combatcd_min, combatcd_max));
                    }
                    break;
                case 1:
                    if (startcd_min1 >= 0 && startcd_max1 > 0)
                    {
                        events.ScheduleEvent(1 + Count, urand(startcd_min1, startcd_max1));
                    }
                    else
                    {
                        events.ScheduleEvent(1 + Count, urand(combatcd_min1, combatcd_max1));
                    }
                    break;
                case 2:
                    if (startcd_min2 >= 0 && startcd_max2 > 0)
                    {
                        events.ScheduleEvent(1 + Count, urand(startcd_min2, startcd_max2));
                    }
                    else
                    {
                        events.ScheduleEvent(1 + Count, urand(combatcd_min2, combatcd_max2));
                    }
                    break;
                case 3:
                    if (startcd_min3 >= 0 && startcd_max3 > 0)
                    {
                        events.ScheduleEvent(1 + Count, urand(startcd_min3, startcd_max3));
                    }
                    else
                    {
                        events.ScheduleEvent(1 + Count, urand(combatcd_min3, combatcd_max3));
                    }
                    break;
                case 4:
                    if (startcd_min4 >= 0 && startcd_max4 > 0)
                    {
                        events.ScheduleEvent(1 + Count, urand(startcd_min4, startcd_max4));
                    }
                    else
                    {
                        events.ScheduleEvent(1 + Count, urand(combatcd_min4, combatcd_max4));
                    }
                    break;
                case 5:
                    if (startcd_min5 >= 0 && startcd_max5 > 0)
                    {
                        events.ScheduleEvent(1 + Count, urand(startcd_min5, startcd_max5));
                    }
                    else
                    {
                        events.ScheduleEvent(1 + Count, urand(combatcd_min5, combatcd_max5));
                    }
                    break;
                case 6:
                    if (startcd_min6 >= 0 && startcd_max6 > 0)
                    {
                        events.ScheduleEvent(1 + Count, urand(startcd_min6, startcd_max6));
                    }
                    else
                    {
                        events.ScheduleEvent(1 + Count, urand(combatcd_min6, combatcd_max6));
                    }
                    break;
                default:
                    break;
                }

                ++Count;

            } while (result->NextRow());

            delete result;
        }
    }

    void OnLoad()
    {
        if (!_unit->IsPet())
        {
            _unit->GetProto()->castable_spells.clear();
        }
        ParentClass::OnLoad();
    }

    void EnterCombat(Unit* who)
    {
        SetUpCombatSpells();
        ParentClass::EnterCombat(who);
    }

    void UpdateAI()
    {
        events.Update(1000);

        if (_unit->IsCasting())
            return;

        Unit* spellTar = NULL;

        //Unsupported targets so return
        if ((target == 6) | (target == 7) | (target == 10) | (target == 11) | (target == 12) | (target == 13) | (target == 14) | (target > 18))
        {
            sLog.Error("CreatureSpells.cpp", "Unsupported targettype for creature %u, type %u please correct this in DB.", _unit->GetEntry(), target);
            return;
        }

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case 1:
                if (target == 0) // Cast on ourselves
                {
                    _unit->DoCast(_unit, spellid0, false);

                    if (combatcd_min >= 0 && combatcd_max > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(1, urand(combatcd_min, combatcd_max));
                    }
                }
                if ((target == 1) | (target == 2) | (target == 3)) // Cast on our current target (2 & 3 not supported yet but should be highest/lowest aggro)
                {
                    _unit->DoCastVictim(spellid0, false);
                    if (combatcd_min >= 0 && combatcd_max > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(1, urand(combatcd_min, combatcd_max));
                    }
                }
                if ((target == 4) | (target == 8)) // Cast on random target
                {
                    if (_unit->getThreatListRandomTarget())
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid0);
                    if (combatcd_min >= 0 && combatcd_max > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(1, urand(combatcd_min, combatcd_max));
                    }
                }
                if ((target == 5) | (target == 9)) // Cast on random target not current target
                {
                    if (_unit->GetVictim() == _unit->getThreatListRandomTarget()) // If the random player is our current target reschedule
                    {
                        if (combatcd_min >= 0 && combatcd_max > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(1, urand(combatcd_min, combatcd_max));
                        }
                    }
                    else
                    {
                        if (_unit->getThreatListRandomTarget())
                        {
                            _unit->DoCast(_unit->getThreatListRandomTarget(), spellid0);
                        }

                        if (combatcd_min >= 0 && combatcd_max > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(1, urand(combatcd_min, combatcd_max));
                        }
                    }
                }
                if (target == 15) // Cast summon spell on self
                {
                    _unit->DoCast(_unit, spellid0, false);

                    if (combatcd_min >= 0 && combatcd_max > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(1, urand(combatcd_min, combatcd_max));
                    }
                }
                if (target == 16) // Cast on mana user random
                {
                    if (_unit->getThreatListRandomTarget() && _unit->getThreatListRandomTarget()->GetPowerType() == POWER_TYPE_MANA)
                    {
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid0);
                    }

                    if (combatcd_min >= 0 && combatcd_max > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(1, urand(combatcd_min, combatcd_max));
                    }
                }
                if (target == 17) // Cast AoE Spell
                {
                    _unit->DoCastAOE(spellid0);

                    if (combatcd_min >= 0 && combatcd_max > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(1, urand(combatcd_min, combatcd_max));
                    }
                }
                if (target == 18) // Cast damaging ranged spell 
                {
                    if (Unit* tar = _unit->getThreatListRandomTarget())
                    {
                        float distance = _unit->CalcDistance(tar->GetPositionX(), tar->GetPositionY(), tar->GetPositionZ());
                        if (distance >= 6.0f)
                        {
                            _unit->DoCast(tar, spellid0);
                        }
                    }
                    if (combatcd_min >= 0 && combatcd_max > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(1, urand(combatcd_min, combatcd_max));
                    }
                }
                break;
            case 2:
                if (target1 == 0) // Cast on ourselves
                {
                    _unit->DoCast(_unit, spellid1, false);

                    if (combatcd_min1 >= 0 && combatcd_max1 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(2, urand(combatcd_min1, combatcd_max1));
                    }
                }
                if ((target1 == 1) | (target1 == 2) | (target1 == 3)) // Cast on our current target (2 & 3 not supported yet but should be highest/lowest aggro)
                {
                    _unit->DoCastVictim(spellid1, false);
                    if (combatcd_min1 >= 0 && combatcd_max1 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(2, urand(combatcd_min1, combatcd_max1));
                    }
                }
                if ((target1 == 4) | (target1 == 8)) // Cast on random target
                {
                    if (_unit->getThreatListRandomTarget())
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid1);
                    if (combatcd_min1 >= 0 && combatcd_max1 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(2, urand(combatcd_min1, combatcd_max1));
                    }
                }
                if ((target1 == 5) | (target1 == 9)) // Cast on random target not current target
                {
                    if (_unit->GetVictim() == _unit->getThreatListRandomTarget()) // If the random player is our current target reschedule
                    {
                        if (combatcd_min1 >= 0 && combatcd_max1 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(2, urand(combatcd_min1, combatcd_max1));
                        }
                    }
                    else
                    {
                        if (_unit->getThreatListRandomTarget())
                        {
                            _unit->DoCast(_unit->getThreatListRandomTarget(), spellid1);
                        }

                        if (combatcd_min1 >= 0 && combatcd_max1 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(2, urand(combatcd_min1, combatcd_max1));
                        }
                    }
                }
                if (target1 == 15) // Cast summon spell on self
                {
                    _unit->DoCast(_unit, spellid1, false);

                    if (combatcd_min1 >= 0 && combatcd_max1 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(2, urand(combatcd_min1, combatcd_max1));
                    }
                }
                if (target1 == 16) // Cast on mana user random
                {
                    if (_unit->getThreatListRandomTarget() && _unit->getThreatListRandomTarget()->GetPowerType() == POWER_TYPE_MANA)
                    {
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid1);
                    }

                    if (combatcd_min1 >= 0 && combatcd_max1 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(2, urand(combatcd_min1, combatcd_max1));
                    }
                }
                if (target1 == 17) // Cast AoE Spell
                {
                    _unit->DoCastAOE(spellid1);

                    if (combatcd_min1 >= 0 && combatcd_max1 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(2, urand(combatcd_min1, combatcd_max1));
                    }
                }
                if (target1 == 18) // Cast damaging ranged spell 
                {
                    if (Unit* tar = _unit->getThreatListRandomTarget())
                    {
                        float distance = _unit->CalcDistance(tar->GetPositionX(), tar->GetPositionY(), tar->GetPositionZ());
                        if (distance >= 6.0f)
                        {
                            _unit->DoCast(tar, spellid1);
                        }
                    }
                    if (combatcd_min1 >= 0 && combatcd_max1 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(2, urand(combatcd_min1, combatcd_max1));
                    }
                }
                break;
            case 3:
                if (target2 == 0) // Cast on ourselves
                {
                    _unit->DoCast(_unit, spellid2, false);

                    if (combatcd_min2 >= 0 && combatcd_max2 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(3, urand(combatcd_min2, combatcd_max2));
                    }
                }
                if ((target2 == 1) | (target2 == 2) | (target2 == 3)) // Cast on our current target (2 & 3 not supported yet but should be highest/lowest aggro)
                {
                    _unit->DoCastVictim(spellid2, false);
                    if (combatcd_min2 >= 0 && combatcd_max2 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(3, urand(combatcd_min2, combatcd_max2));
                    }
                }
                if ((target2 == 4) | (target2 == 8)) // Cast on random target
                {
                    if (_unit->getThreatListRandomTarget())
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid2);
                    if (combatcd_min2 >= 0 && combatcd_max2 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(3, urand(combatcd_min2, combatcd_max2));
                    }
                }
                if ((target2 == 5) | (target2 == 9)) // Cast on random target not current target
                {
                    if (_unit->GetVictim() == _unit->getThreatListRandomTarget()) // If the random player is our current target reschedule
                    {
                        if (combatcd_min2 >= 0 && combatcd_max2 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(3, urand(combatcd_min2, combatcd_max2));
                        }
                    }
                    else
                    {
                        if (_unit->getThreatListRandomTarget())
                        {
                            _unit->DoCast(_unit->getThreatListRandomTarget(), spellid2);
                        }

                        if (combatcd_min2 >= 0 && combatcd_max2 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(3, urand(combatcd_min2, combatcd_max2));
                        }
                    }
                }
                if (target2 == 15) // Cast summon spell on self
                {
                    _unit->DoCast(_unit, spellid2, false);

                    if (combatcd_min2 >= 0 && combatcd_max2 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(3, urand(combatcd_min2, combatcd_max2));
                    }
                }
                if (target2 == 16) // Cast on mana user random
                {
                    if (_unit->getThreatListRandomTarget() && _unit->getThreatListRandomTarget()->GetPowerType() == POWER_TYPE_MANA)
                    {
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid2);
                    }

                    if (combatcd_min2 >= 0 && combatcd_max2 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(3, urand(combatcd_min2, combatcd_max2));
                    }
                }
                if (target2 == 17) // Cast AoE Spell
                {
                    _unit->DoCastAOE(spellid2);

                    if (combatcd_min2 >= 0 && combatcd_max2 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(3, urand(combatcd_min2, combatcd_max2));
                    }
                }
                if (target2 == 18) // Cast damaging ranged spell 
                {
                    if (Unit* tar = _unit->getThreatListRandomTarget())
                    {
                        float distance = _unit->CalcDistance(tar->GetPositionX(), tar->GetPositionY(), tar->GetPositionZ());
                        if (distance >= 6.0f)
                        {
                            _unit->DoCast(tar, spellid2);
                        }
                    }
                    if (combatcd_min2 >= 0 && combatcd_max2 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(3, urand(combatcd_min2, combatcd_max2));
                    }
                }
                break;
            case 4:
                if (target3 == 0) // Cast on ourselves
                {
                    _unit->DoCast(_unit, spellid3, false);

                    if (combatcd_min3 >= 0 && combatcd_max3 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(4, urand(combatcd_min3, combatcd_max3));
                    }
                }
                if ((target3 == 1) | (target3 == 2) | (target3 == 3)) // Cast on our current target (2 & 3 not supported yet but should be highest/lowest aggro)
                {
                    _unit->DoCastVictim(spellid3, false);
                    if (combatcd_min3 >= 0 && combatcd_max3 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(4, urand(combatcd_min3, combatcd_max3));
                    }
                }
                if ((target3 == 4) | (target3 == 8)) // Cast on random target
                {
                    if (_unit->getThreatListRandomTarget())
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid3);
                    if (combatcd_min3 >= 0 && combatcd_max3 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(4, urand(combatcd_min3, combatcd_max3));
                    }
                }
                if ((target3 == 5) | (target3 == 9)) // Cast on random target not current target
                {
                    if (_unit->GetVictim() == _unit->getThreatListRandomTarget()) // If the random player is our current target reschedule
                    {
                        if (combatcd_min3 >= 0 && combatcd_max3 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(4, urand(combatcd_min3, combatcd_max3));
                        }
                    }
                    else
                    {
                        if (_unit->getThreatListRandomTarget())
                        {
                            _unit->DoCast(_unit->getThreatListRandomTarget(), spellid3);
                        }

                        if (combatcd_min3 >= 0 && combatcd_max3 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(4, urand(combatcd_min3, combatcd_max3));
                        }
                    }
                }
                if (target3 == 15) // Cast summon spell on self
                {
                    _unit->DoCast(_unit, spellid3, false);

                    if (combatcd_min3 >= 0 && combatcd_max3 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(4, urand(combatcd_min3, combatcd_max3));
                    }
                }
                if (target3 == 16) // Cast on mana user random
                {
                    if (_unit->getThreatListRandomTarget() && _unit->getThreatListRandomTarget()->GetPowerType() == POWER_TYPE_MANA)
                    {
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid3);
                    }

                    if (combatcd_min3 >= 0 && combatcd_max3 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(4, urand(combatcd_min3, combatcd_max3));
                    }
                }
                if (target3 == 17) // Cast AoE Spell
                {
                    _unit->DoCastAOE(spellid3);

                    if (combatcd_min3 >= 0 && combatcd_max3 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(4, urand(combatcd_min3, combatcd_max3));
                    }
                }
                if (target3 == 18) // Cast damaging ranged spell 
                {
                    if (Unit* tar = _unit->getThreatListRandomTarget())
                    {
                        float distance = _unit->CalcDistance(tar->GetPositionX(), tar->GetPositionY(), tar->GetPositionZ());
                        if (distance >= 6.0f)
                        {
                            _unit->DoCast(tar, spellid3);
                        }
                    }
                    if (combatcd_min3 >= 0 && combatcd_max3 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(4, urand(combatcd_min3, combatcd_max3));
                    }
                }
                break;
            case 5:
                if (target4 == 0) // Cast on ourselves
                {
                    _unit->DoCast(_unit, spellid4, false);

                    if (combatcd_min4 >= 0 && combatcd_max4 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(5, urand(combatcd_min4, combatcd_max4));
                    }
                }
                if ((target4 == 1) | (target4 == 2) | (target4 == 3)) // Cast on our current target (2 & 3 not supported yet but should be highest/lowest aggro)
                {
                    _unit->DoCastVictim(spellid4, false);
                    if (combatcd_min4 >= 0 && combatcd_max4 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(5, urand(combatcd_min4, combatcd_max4));
                    }
                }
                if ((target4 == 4) | (target4 == 8)) // Cast on random target
                {
                    if (_unit->getThreatListRandomTarget())
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid4);
                    if (combatcd_min4 >= 0 && combatcd_max4 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(5, urand(combatcd_min4, combatcd_max4));
                    }
                }
                if ((target4 == 5) | (target4 == 9)) // Cast on random target not current target
                {
                    if (_unit->GetVictim() == _unit->getThreatListRandomTarget()) // If the random player is our current target reschedule
                    {
                        if (combatcd_min4 >= 0 && combatcd_max4 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(5, urand(combatcd_min4, combatcd_max4));
                        }
                    }
                    else
                    {
                        if (_unit->getThreatListRandomTarget())
                        {
                            _unit->DoCast(_unit->getThreatListRandomTarget(), spellid4);
                        }

                        if (combatcd_min4 >= 0 && combatcd_max4 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(5, urand(combatcd_min4, combatcd_max4));
                        }
                    }
                }
                if (target4 == 15) // Cast summon spell on self
                {
                    _unit->DoCast(_unit, spellid4, false);

                    if (combatcd_min4 >= 0 && combatcd_max4 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(5, urand(combatcd_min4, combatcd_max4));
                    }
                }
                if (target4 == 16) // Cast on mana user random
                {
                    if (_unit->getThreatListRandomTarget() && _unit->getThreatListRandomTarget()->GetPowerType() == POWER_TYPE_MANA)
                    {
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid4);
                    }

                    if (combatcd_min4 >= 0 && combatcd_max4 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(5, urand(combatcd_min4, combatcd_max4));
                    }
                }
                if (target4 == 17) // Cast AoE Spell
                {
                    _unit->DoCastAOE(spellid4);

                    if (combatcd_min4 >= 0 && combatcd_max4 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(5, urand(combatcd_min4, combatcd_max4));
                    }
                }
                if (target4 == 18) // Cast damaging ranged spell 
                {
                    if (Unit* tar = _unit->getThreatListRandomTarget())
                    {
                        float distance = _unit->CalcDistance(tar->GetPositionX(), tar->GetPositionY(), tar->GetPositionZ());
                        if (distance >= 6.0f)
                        {
                            _unit->DoCast(tar, spellid4);
                        }
                    }
                    if (combatcd_min4 >= 0 && combatcd_max4 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(5, urand(combatcd_min4, combatcd_max4));
                    }
                }
                break;
            case 6:
                if (target5 == 0) // Cast on ourselves
                {
                    _unit->DoCast(_unit, spellid5, false);

                    if (combatcd_min5 >= 0 && combatcd_max5 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(6, urand(combatcd_min5, combatcd_max5));
                    }
                }
                if ((target5 == 1) | (target5 == 2) | (target5 == 3)) // Cast on our current target (2 & 3 not supported yet but should be highest/lowest aggro)
                {
                    _unit->DoCastVictim(spellid5, false);
                    if (combatcd_min5 >= 0 && combatcd_max5 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(6, urand(combatcd_min5, combatcd_max5));
                    }
                }
                if ((target5 == 4) | (target5 == 8)) // Cast on random target
                {
                    if (_unit->getThreatListRandomTarget())
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid5);
                    if (combatcd_min5 >= 0 && combatcd_max5 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(6, urand(combatcd_min5, combatcd_max5));
                    }
                }
                if ((target5 == 5) | (target5 == 9)) // Cast on random target not current target
                {
                    if (_unit->GetVictim() == _unit->getThreatListRandomTarget()) // If the random player is our current target reschedule
                    {
                        if (combatcd_min5 >= 0 && combatcd_max5 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(6, urand(combatcd_min5, combatcd_max5));
                        }
                    }
                    else
                    {
                        if (_unit->getThreatListRandomTarget())
                        {
                            _unit->DoCast(_unit->getThreatListRandomTarget(), spellid5);
                        }

                        if (combatcd_min5 >= 0 && combatcd_max5 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(6, urand(combatcd_min5, combatcd_max5));
                        }
                    }
                }
                if (target5 == 15) // Cast summon spell on self
                {
                    _unit->DoCast(_unit, spellid5, false);

                    if (combatcd_min5 >= 0 && combatcd_max5 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(6, urand(combatcd_min5, combatcd_max5));
                    }
                }
                if (target5 == 16) // Cast on mana user random
                {
                    if (_unit->getThreatListRandomTarget() && _unit->getThreatListRandomTarget()->GetPowerType() == POWER_TYPE_MANA)
                    {
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid5);
                    }

                    if (combatcd_min5 >= 0 && combatcd_max5 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(6, urand(combatcd_min5, combatcd_max5));
                    }
                }
                if (target5 == 17) // Cast AoE Spell
                {
                    _unit->DoCastAOE(spellid5);

                    if (combatcd_min5 >= 0 && combatcd_max5 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(6, urand(combatcd_min5, combatcd_max5));
                    }
                }
                if (target5 == 18) // Cast damaging ranged spell 
                {
                    if (Unit* tar = _unit->getThreatListRandomTarget())
                    {
                        float distance = _unit->CalcDistance(tar->GetPositionX(), tar->GetPositionY(), tar->GetPositionZ());
                        if (distance >= 6.0f)
                        {
                            _unit->DoCast(tar, spellid5);
                        }
                    }
                    if (combatcd_min5 >= 0 && combatcd_max5 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(6, urand(combatcd_min5, combatcd_max5));
                    }
                }
                break;
            case 7:
                if (target6 == 0) // Cast on ourselves
                {
                    _unit->DoCast(_unit, spellid6, false);

                    if (combatcd_min6 >= 0 && combatcd_max6 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(7, urand(combatcd_min6, combatcd_max6));
                    }
                }
                if ((target6 == 1) | (target6 == 2) | (target6 == 3)) // Cast on our current target (2 & 3 not supported yet but should be highest/lowest aggro)
                {
                    _unit->DoCastVictim(spellid6, false);
                    if (combatcd_min6 >= 0 && combatcd_max6 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(7, urand(combatcd_min6, combatcd_max6));
                    }
                }
                if ((target6 == 4) | (target6 == 8)) // Cast on random target
                {
                    if (_unit->getThreatListRandomTarget())
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid6);
                    if (combatcd_min6 >= 0 && combatcd_max6 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(7, urand(combatcd_min6, combatcd_max6));
                    }
                }
                if ((target6 == 5) | (target6 == 9)) // Cast on random target not current target
                {
                    if (_unit->GetVictim() == _unit->getThreatListRandomTarget()) // If the random player is our current target reschedule
                    {
                        if (combatcd_min6 >= 0 && combatcd_max6 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(7, urand(combatcd_min6, combatcd_max6));
                        }
                    }
                    else
                    {
                        if (_unit->getThreatListRandomTarget())
                        {
                            _unit->DoCast(_unit->getThreatListRandomTarget(), spellid6);
                        }

                        if (combatcd_min6 >= 0 && combatcd_max6 > 0) // Check if we can cast this again in combat
                        {
                            events.ScheduleEvent(7, urand(combatcd_min6, combatcd_max6));
                        }
                    }
                }
                if (target6 == 15) // Cast summon spell on self
                {
                    _unit->DoCast(_unit, spellid6, false);

                    if (combatcd_min6 >= 0 && combatcd_max6 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(7, urand(combatcd_min6, combatcd_max6));
                    }
                }
                if (target6 == 16) // Cast on mana user random
                {
                    if (_unit->getThreatListRandomTarget() && _unit->getThreatListRandomTarget()->GetPowerType() == POWER_TYPE_MANA)
                    {
                        _unit->DoCast(_unit->getThreatListRandomTarget(), spellid6);
                    }

                    if (combatcd_min6 >= 0 && combatcd_max6 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(7, urand(combatcd_min6, combatcd_max6));
                    }
                }
                if (target6 == 17) // Cast AoE Spell
                {
                    _unit->DoCastAOE(spellid6);

                    if (combatcd_min6 >= 0 && combatcd_max6 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(7, urand(combatcd_min6, combatcd_max6));
                    }
                }
                if (target6 == 18) // Cast damaging ranged spell 
                {
                    if (Unit* tar = _unit->getThreatListRandomTarget())
                    {
                        float distance = _unit->CalcDistance(tar->GetPositionX(), tar->GetPositionY(), tar->GetPositionZ());
                        if (distance >= 6.0f)
                        {
                            _unit->DoCast(tar, spellid6);
                        }
                    }
                    if (combatcd_min6 >= 0 && combatcd_max6 > 0) // Check if we can cast this again in combat
                    {
                        events.ScheduleEvent(7, urand(combatcd_min6, combatcd_max6));
                    }
                }
                break;
                			default:
                                break;
            }
        }

        ParentClass::UpdateAI();
    }
protected:
    uint32 startcd_min;
    uint32 startcd_max;
    uint32 combatcd_min;
    uint32 combatcd_max;
    uint32 startcd_min1;
    uint32 startcd_max1;
    uint32 combatcd_min1;
    uint32 combatcd_max1;
    uint32 startcd_min2;
    uint32 startcd_max2;
    uint32 combatcd_min2;
    uint32 combatcd_max2;
    uint32 startcd_min3;
    uint32 startcd_max3;
    uint32 combatcd_min3;
    uint32 combatcd_max3;
    uint32 startcd_min4;
    uint32 startcd_max4;
    uint32 combatcd_min4;
    uint32 combatcd_max4;
    uint32 startcd_min5;
    uint32 startcd_max5;
    uint32 combatcd_min5;
    uint32 combatcd_max5;
    uint32 startcd_min6;
    uint32 startcd_max6;
    uint32 combatcd_min6;
    uint32 combatcd_max6;
    uint32 target;
    uint32 target1;
    uint32 target2;
    uint32 target3;
    uint32 target4;
    uint32 target5;
    uint32 target6;
    uint32 spellid0;
    uint32 spellid1;
    uint32 spellid2;
    uint32 spellid3;
    uint32 spellid4;
    uint32 spellid5;
    uint32 spellid6;
    uint32 modes;
    uint32 spellpos;
    uint32 c_entry;
};

void SetupCreatureSpells(ScriptMgr* mgr)
{
    QueryResult* result = WorldDatabase.Query("SELECT * FROM creature_spells");
        if (result)
        {
            do
            {
                CreatureCreateMap _creatures;
                Field* fields = result->Fetch();
                uint32 creature_r_entry;
                creature_r_entry = fields[0].GetUInt32();
                if (!mgr->has_creature_script(creature_r_entry)) // Check if we already have a creature script if so don't register it again.
                {
                    mgr->register_creature_script(creature_r_entry, &creature_combat_spells::Create);
                }
            } while (result->NextRow());

            delete result; // Free up memory after registering.
        }
}