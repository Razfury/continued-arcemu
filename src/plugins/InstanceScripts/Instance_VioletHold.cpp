/*
* Copyright (C) 2015-2019 FrostwingCore Team <http://www.ArcEmu.org/>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Setup.h"
#include "Instance_VioletHold.h"

class VioletHoldScript : public MoonInstanceScript
{
public:
    MOONSCRIPT_INSTANCE_FACTORY_FUNCTION(VioletHoldScript, MoonInstanceScript);
    VioletHoldScript(MapMgr* pMapMgr) : MoonInstanceScript(pMapMgr)
    {
        GetInstance()->m_BossGUID1 = 0;
        GetInstance()->m_BossGUID2 = 0;
        GetInstance()->m_BossGUID3 = 0;
        GetInstance()->m_BossGUID4 = 0;
    };

    void OnPlayerEnter(Player* pPlayer)
    {
        if (pPlayer)
        {
            // Hack! Load missing cells so the whole instance is spawned some scripts need this DO NOT REMOVE.
            pPlayer->GetMapMgr()->VisitCoordsInstance(1889.27f, 746.88f, pPlayer);
        }
    };

};

void SetupVioletHold(ScriptMgr* mgr)
{
    mgr->register_instance_script(MAP_VIOLET_HOLD, &VioletHoldScript::Create);
}