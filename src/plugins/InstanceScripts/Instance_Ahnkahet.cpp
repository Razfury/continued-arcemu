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


void SetupAhnkahet(ScriptMgr* mgr)
{
}