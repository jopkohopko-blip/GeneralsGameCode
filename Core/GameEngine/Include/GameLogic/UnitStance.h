/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// TheSuperHackers @feature Unit stances that the player can give to selected units.
// They only change what a unit does on its own (auto targeting, chasing, retaliation).
// Explicit orders from the player always work in every stance.
enum UnitStance CPP_11(: Int)
{
	UNIT_STANCE_NORMAL = 0,     ///< Retail behaviour: fire at enemies in weapon range, retaliate when attacked
	UNIT_STANCE_AGGRESSIVE,     ///< Engage enemies in vision range and pursue them
	UNIT_STANCE_HOLD_POSITION,  ///< Fire at enemies in weapon range but never move on your own
	UNIT_STANCE_HOLD_FIRE,      ///< Never fire on your own; only on direct orders, attack move or guard

	UNIT_STANCE_COUNT
};
