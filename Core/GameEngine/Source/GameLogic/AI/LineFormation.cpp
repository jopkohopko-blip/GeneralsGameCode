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

// FILE: LineFormation.cpp ////////////////////////////////////////////////////////////////////////
// TheSuperHackers @feature Line formations and moving groups at the speed of their slowest member.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameLogic/LineFormation.h"

#include "Common/GlobalData.h"
#include "GameLogic/AI.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/TerrainLogic.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/BodyModule.h"

#include <algorithm>

namespace
{

/// Weapons reaching at least this far count as artillery.
const Real ARTILLERY_MIN_RANGE = 260.0f;
/// Units never spread further apart than this many slot widths, however long the line.
const Real MAX_SPREAD_IN_SLOTS = 4.0f;
/// Lines shorter than this are treated as a click: a block facing the move direction.
const Real MIN_LINE_LENGTH = 5.0f;
/// Free room around each unit, as a part of its diameter, plus a fixed minimum.
const Real SLOT_PADDING_FACTOR = 0.35f;
const Real SLOT_PADDING_MIN = 4.0f;
/// How far ahead of its slot a unit looks when it turns to face the formation's direction.
const Real FACING_LOOK_DISTANCE = 100.0f;

struct SortEntry
{
	Real lateral;
	Real depth;
	Int index;
	ObjectID id;
};

bool sortByLateral( const SortEntry &a, const SortEntry &b )
{
	if (a.lateral != b.lateral)
		return a.lateral < b.lateral;
	if (a.depth != b.depth)
		return a.depth < b.depth;
	return a.id < b.id;
}

Real dot2( const Coord2D &a, Real x, Real y )
{
	return a.x * x + a.y * y;
}

} // namespace

namespace LineFormation
{

//-------------------------------------------------------------------------------------------------
Bool isParticipant( const Object *obj )
{
	if (obj == nullptr || obj->isEffectivelyDead())
		return FALSE;

	if (obj->getAI() == nullptr)
		return FALSE;

	if (obj->isDisabledByType( DISABLED_HELD ))
		return FALSE; // riding in a transport

	if (obj->isKindOf( KINDOF_IMMOBILE ) || obj->isKindOf( KINDOF_STRUCTURE ))
		return FALSE;

	if (obj->isKindOf( KINDOF_AIRCRAFT ))
		return FALSE; // aircraft stack up over the target anyway

	if (!obj->isMobile())
		return FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
Role classify( const Object *obj )
{
	if (obj->isKindOf( KINDOF_INFANTRY ))
		return ROLE_INFANTRY;

	Bool hasWeapon = FALSE;
	Bool antiAir = FALSE;
	Real maxRange = 0.0f;
	Real minRange = 0.0f;

	for (Int slot = PRIMARY_WEAPON; slot < WEAPONSLOT_COUNT; ++slot)
	{
		const Weapon *weapon = obj->getWeaponInWeaponSlot( (WeaponSlotType)slot );
		if (weapon == nullptr)
			continue;

		const WeaponTemplate *weaponTemplate = weapon->getTemplate();
		if (weaponTemplate == nullptr)
			continue;

		hasWeapon = TRUE;

		if (weaponTemplate->getAntiMask() & (WEAPON_ANTI_AIRBORNE_VEHICLE | WEAPON_ANTI_AIRBORNE_INFANTRY))
			antiAir = TRUE;

		maxRange = std::max( maxRange, weaponTemplate->getUnmodifiedAttackRange() );
		minRange = std::max( minRange, weaponTemplate->getMinimumAttackRange() );
	}

	if (!hasWeapon)
		return ROLE_BACK; // dozers, supply trucks, ambulances

	if (antiAir)
		return ROLE_ANTI_AIR;

	if (minRange > 0.0f || maxRange >= ARTILLERY_MIN_RANGE)
		return ROLE_BACK;

	return ROLE_FRONT;
}

//-------------------------------------------------------------------------------------------------
Bool computeSlots( const std::vector<Object *> &units, const Coord3D &start, const Coord3D &end,
	Bool smartRoles, std::vector<Slot> &outSlots, Coord2D *outFacing )
{
	outSlots.clear();
	if (units.empty())
		return FALSE;

	// Where the units are now.
	Coord2D centroid;
	centroid.x = 0.0f;
	centroid.y = 0.0f;
	for (size_t i = 0; i < units.size(); ++i)
	{
		centroid.x += units[i]->getPosition()->x;
		centroid.y += units[i]->getPosition()->y;
	}
	centroid.x /= (Real)units.size();
	centroid.y /= (Real)units.size();

	Coord2D mid;
	mid.x = (start.x + end.x) * 0.5f;
	mid.y = (start.y + end.y) * 0.5f;

	Coord2D dir;
	dir.x = end.x - start.x;
	dir.y = end.y - start.y;
	Real lineLength = dir.length();

	Coord2D facing;
	if (lineLength < MIN_LINE_LENGTH)
	{
		// No real line: face the way the group is moving and form a block.
		facing.x = mid.x - centroid.x;
		facing.y = mid.y - centroid.y;
		if (facing.length() < 1.0f)
		{
			facing.x = 1.0f;
			facing.y = 0.0f;
		}
		facing.normalize();
		dir.x = facing.y;
		dir.y = -facing.x;
		lineLength = 0.0f;
	}
	else
	{
		dir.x /= lineLength;
		dir.y /= lineLength;
		facing.x = -dir.y;
		facing.y = dir.x;
		// Face away from where the group comes from, like Total War and Company of Heroes do.
		if (dot2( facing, mid.x - centroid.x, mid.y - centroid.y ) < 0.0f)
		{
			facing.x = -facing.x;
			facing.y = -facing.y;
		}
	}

	if (outFacing)
		*outFacing = facing;

	// Split the units into bands, front to back.
	std::vector<Object *> bands[ROLE_COUNT];
	for (size_t i = 0; i < units.size(); ++i)
	{
		Role role = smartRoles ? classify( units[i] ) : ROLE_FRONT;
		bands[role].push_back( units[i] );
	}

	Real depth = 0.0f;			// distance behind the line of the current rank
	Real prevHalfSlot = -1.0f;	// half slot of the previous band, negative for the first one

	for (Int b = 0; b < ROLE_COUNT; ++b)
	{
		const std::vector<Object *> &band = bands[b];
		const Int count = (Int)band.size();
		if (count == 0)
			continue;

		Real maxDiameter = 0.0f;
		for (Int i = 0; i < count; ++i)
			maxDiameter = std::max( maxDiameter, 2.0f * band[i]->getGeometryInfo().getBoundingCircleRadius() );

		const Real slotWidth = maxDiameter + std::max( SLOT_PADDING_MIN, maxDiameter * SLOT_PADDING_FACTOR );

		Int perRank;
		if (lineLength > 0.0f)
			perRank = (Int)(lineLength / slotWidth) + 1;
		else
			perRank = (Int)ceilf( sqrtf( (Real)count ) * 1.5f );
		perRank = std::max( 1, std::min( perRank, count ) );

		const Int ranks = (count + perRank - 1) / perRank;

		// Units in a single rank spread out along the whole line; full ranks use the same spacing.
		Real spacing = slotWidth;
		if (lineLength > 0.0f)
		{
			const Int unitsAcross = (ranks == 1) ? count : perRank;
			spacing = (unitsAcross > 1) ? std::min( lineLength / (Real)(unitsAcross - 1), slotWidth * MAX_SPREAD_IN_SLOTS ) : 0.0f;
		}

		if (prevHalfSlot >= 0.0f)
			depth += prevHalfSlot + slotWidth * 0.5f;

		// Make the slots.
		std::vector<SortEntry> slotOrder;
		std::vector<Coord3D> slotPos;
		for (Int r = 0; r < ranks; ++r)
		{
			const Int inRank = (r == ranks - 1) ? (count - r * perRank) : perRank;
			const Real width = (Real)(inRank - 1) * spacing;
			const Real rankDepth = depth + (Real)r * slotWidth;
			for (Int i = 0; i < inRank; ++i)
			{
				const Real lateral = -width * 0.5f + (Real)i * spacing;
				Coord3D pos;
				pos.x = mid.x + dir.x * lateral - facing.x * rankDepth;
				pos.y = mid.y + dir.y * lateral - facing.y * rankDepth;
				pos.z = 0.0f;

				SortEntry entry;
				entry.lateral = lateral;
				entry.depth = rankDepth;
				entry.index = (Int)slotPos.size();
				entry.id = INVALID_ID;
				slotOrder.push_back( entry );
				slotPos.push_back( pos );
			}
		}
		depth += (Real)(ranks - 1) * slotWidth;
		prevHalfSlot = slotWidth * 0.5f;

		// Hand the slots out left to right so the units do not cross each other's paths.
		std::vector<SortEntry> unitOrder;
		for (Int i = 0; i < count; ++i)
		{
			const Coord3D *pos = band[i]->getPosition();
			SortEntry entry;
			entry.lateral = dot2( dir, pos->x - mid.x, pos->y - mid.y );
			entry.depth = -dot2( facing, pos->x - mid.x, pos->y - mid.y );
			entry.index = i;
			entry.id = band[i]->getID();
			unitOrder.push_back( entry );
		}
		std::sort( slotOrder.begin(), slotOrder.end(), sortByLateral );
		std::sort( unitOrder.begin(), unitOrder.end(), sortByLateral );

		for (Int i = 0; i < count; ++i)
		{
			Slot slot;
			slot.obj = band[unitOrder[i].index];
			slot.pos = slotPos[slotOrder[i].index];
			outSlots.push_back( slot );
		}
	}

	// Put the slots on the ground.
	for (size_t i = 0; i < outSlots.size(); ++i)
	{
		Coord3D &pos = outSlots[i].pos;
		PathfindLayerEnum layer = TheTerrainLogic->getLayerForDestination( &pos );
		pos.z = TheTerrainLogic->getLayerHeight( pos.x, pos.y, layer );
	}

	return !outSlots.empty();
}

//-------------------------------------------------------------------------------------------------
static void collectMembers( AIGroup *group, std::vector<Object *> &participants, std::vector<Object *> *others )
{
	const VecObjectID &ids = group->getAllIDs();
	for (VecObjectID::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		Object *obj = TheGameLogic->findObjectByID( *it );
		if (obj == nullptr)
			continue;

		if (isParticipant( obj ))
			participants.push_back( obj );
		else if (others && obj->getAI() && !obj->isDisabledByType( DISABLED_HELD ) && obj->isMobile())
			others->push_back( obj );
	}
}

//-------------------------------------------------------------------------------------------------
static void applySpeedLimit( const std::vector<Object *> &units )
{
	if (units.size() < 2)
		return;

	Real slowest = FAST_AS_POSSIBLE;
	for (size_t i = 0; i < units.size(); ++i)
	{
		Object *obj = units[i];
		const AIUpdateInterface *ai = obj->getAI();
		const Real speed = ai->getCurLocomotorSpeed();
		if (speed <= 0.0f)
			continue;

		// Like AIGroup::recompute, a unit slowed by damage does not slow everyone else down.
		if (obj->getBodyModule() && !IS_CONDITION_BETTER( obj->getBodyModule()->getDamageState(), TheGlobalData->m_movementPenaltyDamageState ))
			continue;

		slowest = std::min( slowest, speed );
	}

	if (slowest >= FAST_AS_POSSIBLE)
		return;

	for (size_t i = 0; i < units.size(); ++i)
		units[i]->getAI()->setGroupSpeedLimit( slowest );
}

//-------------------------------------------------------------------------------------------------
void applyGroupSpeedLimit( AIGroup *group )
{
	if (group == nullptr)
		return;

	std::vector<Object *> units;
	collectMembers( group, units, nullptr );
	applySpeedLimit( units );
}

//-------------------------------------------------------------------------------------------------
void groupMoveToLine( AIGroup *group, const Coord3D &start, const Coord3D &end, Int flags, CommandSourceType cmdSource )
{
	if (group == nullptr)
		return;

	Coord3D mid;
	mid.x = (start.x + end.x) * 0.5f;
	mid.y = (start.y + end.y) * 0.5f;
	mid.z = (start.z + end.z) * 0.5f;

	std::vector<Object *> units;
	std::vector<Object *> others;
	collectMembers( group, units, &others );

	std::vector<Slot> slots;
	Coord2D facing;
	if (!computeSlots( units, start, end, (flags & FLAG_SMART_ROLES) != 0, slots, &facing ))
	{
		// Nothing to line up, e.g. only aircraft: do a normal move to the middle of the line.
		group->groupMoveToPosition( &mid, false, cmdSource );
		return;
	}

	const Bool moveTogether = (flags & FLAG_MOVE_TOGETHER) != 0 && slots.size() > 1;

	// Keep the shape on later moves by making it one of the engine's formations.
	FormationID formationID = moveTogether ? TheAI->getNextFormationID() : NO_FORMATION_ID;

	Coord2D slotCenter;
	slotCenter.x = 0.0f;
	slotCenter.y = 0.0f;
	for (size_t i = 0; i < slots.size(); ++i)
	{
		slotCenter.x += slots[i].pos.x;
		slotCenter.y += slots[i].pos.y;
		TheAI->pathfinder()->removeGoal( slots[i].obj );
	}
	slotCenter.x /= (Real)slots.size();
	slotCenter.y /= (Real)slots.size();

	for (size_t i = 0; i < slots.size(); ++i)
	{
		Object *obj = slots[i].obj;
		AIUpdateInterface *ai = obj->getAI();
		Coord3D dest = slots[i].pos;

		if (ai->isDoingGroundMovement())
		{
			TheAI->pathfinder()->adjustDestination( obj, ai->getLocomotorSet(), &dest, nullptr );
			TheAI->pathfinder()->updateGoal( obj, &dest, LAYER_GROUND );
		}

		Coord2D offset;
		offset.x = slots[i].pos.x - slotCenter.x;
		offset.y = slots[i].pos.y - slotCenter.y;
		obj->setFormationID( formationID );
		obj->setFormationOffset( offset );

		ai->aiMoveToPosition( &dest, cmdSource );

		Coord3D facePos = dest;
		facePos.x += facing.x * FACING_LOOK_DISTANCE;
		facePos.y += facing.y * FACING_LOOK_DISTANCE;
		ai->setPendingFormationFacing( &facePos, cmdSource );
	}

	// Aircraft and anything else that moves but has no place in the line.
	for (size_t i = 0; i < others.size(); ++i)
	{
		others[i]->setFormationID( NO_FORMATION_ID );
		others[i]->getAI()->aiMoveToPosition( &mid, cmdSource );
	}

	if (moveTogether)
	{
		std::vector<Object *> placed;
		for (size_t i = 0; i < slots.size(); ++i)
			placed.push_back( slots[i].obj );
		applySpeedLimit( placed );
	}
}

} // namespace LineFormation
