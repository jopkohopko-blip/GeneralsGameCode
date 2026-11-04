/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
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

// FILE: LineFormation.h //////////////////////////////////////////////////////////////////////////
// TheSuperHackers @feature Line formations: drag with the order mouse button to line the selected
// units up along the dragged line, facing away from where they came from.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/GameType.h"
#include "Lib/BaseType.h"

#include <vector>

class AIGroup;
class Object;
enum CommandSourceType CPP_11(: Int);

namespace LineFormation
{

/// Flags carried by MSG_DO_FORMATION_MOVETO, so every player's logic does the same thing
/// regardless of the local Options.ini.
enum Flags CPP_11(: Int)
{
	FLAG_SMART_ROLES  = 0x01, ///< tanks in front, anti-air in the middle, infantry and artillery behind
	FLAG_MOVE_TOGETHER = 0x02, ///< move at the slowest member's speed and keep the shape on later moves
};

/// Where the units stand, front to back, when smart roles are on.
enum Role CPP_11(: Int)
{
	ROLE_FRONT,     ///< tanks and other direct fire vehicles
	ROLE_ANTI_AIR,  ///< vehicles that can shoot at aircraft
	ROLE_INFANTRY,
	ROLE_BACK,      ///< artillery, long range launchers and unarmed support vehicles

	ROLE_COUNT
};

struct Slot
{
	Object *obj;
	Coord3D pos;
};

/// True if the object takes a place in a line formation (mobile, ground based, has AI).
Bool isParticipant( const Object *obj );

/// Which band of the formation the object belongs to.
Role classify( const Object *obj );

/// Lays the units out along the line from start to end. The first rank stands on the line;
/// further ranks are placed behind it. outFacing is the unit direction the formation faces.
/// Returns false if there is nothing to place.
Bool computeSlots( const std::vector<Object *> &units, const Coord3D &start, const Coord3D &end,
	Bool smartRoles, std::vector<Slot> &outSlots, Coord2D *outFacing );

/// Game logic: orders the group into a line formation.
void groupMoveToLine( AIGroup *group, const Coord3D &start, const Coord3D &end, Int flags, CommandSourceType cmdSource );

/// Game logic: caps the speed of the ground units of the group to the slowest one, until their next order.
void applyGroupSpeedLimit( AIGroup *group );

} // namespace LineFormation
