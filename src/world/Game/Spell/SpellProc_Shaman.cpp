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

class EarthShieldSpellProc : public SpellProc
{
	SPELL_PROC_FACTORY_FUNCTION(EarthShieldSpellProc);

		bool ProcEffectOverride(Unit* victim, SpellEntry* CastingSpell, uint32 flag, uint32 dmg, uint32 abs, int* dmg_overwrite, uint32 weapon_damage_type)
		{
			int32 value = mOrigSpell->EffectBasePoints[0];
			dmg_overwrite[0] = value;

			return false;
		}

		void CastSpell(Unit* victim, SpellEntry* CastingSpell, int* dmg_overwrite)
		{
			Unit* caster = mTarget->GetMapMgr()->GetUnit(mCaster);
			if(caster == NULL)
				return;

			Spell* spell = sSpellFactoryMgr.NewSpell(caster, mSpell, true, NULL);
			SpellCastTargets targets(mTarget->GetGUID());
			spell->prepare(&targets);
		}

};

void SpellProcMgr::SetupShamman()
{
	AddById(379, &EarthShieldSpellProc::Create);
}
