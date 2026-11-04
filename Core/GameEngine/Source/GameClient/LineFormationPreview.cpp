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

// FILE: LineFormationPreview.cpp /////////////////////////////////////////////////////////////////
// TheSuperHackers @feature Shows where the units will stand while the player drags a line formation.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameClient/LineFormationPreview.h"

#include "Common/GlobalData.h"
#include "GameClient/Display.h"
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"
#include "GameClient/View.h"
#include "GameLogic/LineFormation.h"
#include "GameLogic/Object.h"

#include <vector>

namespace
{

Bool s_shown = FALSE;
Coord3D s_start;
Coord3D s_end;

const Int RING_SEGMENTS = 10;

Color lineColor()		{ return GameMakeColor( 120, 255, 120, 220 ); }
Color slotColor()		{ return GameMakeColor( 120, 255, 120, 170 ); }
Color facingColor()	{ return GameMakeColor( 255, 255, 140, 220 ); }

void drawWorldLine( const Coord3D &a, const Coord3D &b, Real width, Color color )
{
	ICoord2D sa, sb;
	// Points slightly off screen are still useful for clipping by the display, so only skip if both fail.
	Bool okA = TheTacticalView->worldToScreen( &a, &sa );
	Bool okB = TheTacticalView->worldToScreen( &b, &sb );
	if (!okA || !okB)
		return;

	TheDisplay->drawLine( sa.x, sa.y, sb.x, sb.y, width, color );
}

void drawWorldRing( const Coord3D &center, Real radius, Color color )
{
	Coord3D prev;
	for (Int i = 0; i <= RING_SEGMENTS; ++i)
	{
		const Real angle = (Real)i * 2.0f * PI / (Real)RING_SEGMENTS;
		Coord3D p;
		p.x = center.x + radius * Cos( angle );
		p.y = center.y + radius * Sin( angle );
		p.z = center.z;
		if (i > 0)
			drawWorldLine( prev, p, 1.5f, color );
		prev = p;
	}
}

} // namespace

namespace LineFormationPreview
{

//-------------------------------------------------------------------------------------------------
void show( const Coord3D &start, const Coord3D &end )
{
	s_start = start;
	s_end = end;
	s_shown = TRUE;
}

//-------------------------------------------------------------------------------------------------
void hide()
{
	s_shown = FALSE;
}

//-------------------------------------------------------------------------------------------------
Bool isShown()
{
	return s_shown;
}

//-------------------------------------------------------------------------------------------------
void draw()
{
	if (!s_shown || TheTacticalView == nullptr || TheDisplay == nullptr || TheInGameUI == nullptr)
		return;

	std::vector<Object *> units;
	const DrawableList *selected = TheInGameUI->getAllSelectedDrawables();
	for (DrawableListCIt it = selected->begin(); it != selected->end(); ++it)
	{
		Object *obj = (*it)->getObject();
		if (LineFormation::isParticipant( obj ))
			units.push_back( obj );
	}

	std::vector<LineFormation::Slot> slots;
	Coord2D facing;
	if (!LineFormation::computeSlots( units, s_start, s_end, TheGlobalData->m_formationSmartRoles, slots, &facing ))
		return;

	// The dragged line, on the ground.
	Coord3D a = s_start;
	Coord3D b = s_end;
	if (!slots.empty())
	{
		a.z = slots.front().pos.z;
		b.z = slots.front().pos.z;
	}
	drawWorldLine( a, b, 2.0f, lineColor() );

	// Where each unit will stand.
	Coord3D center;
	center.zero();
	for (size_t i = 0; i < slots.size(); ++i)
	{
		const Real radius = slots[i].obj->getGeometryInfo().getBoundingCircleRadius();
		drawWorldRing( slots[i].pos, radius, slotColor() );
		center.x += slots[i].pos.x;
		center.y += slots[i].pos.y;
		center.z += slots[i].pos.z;
	}
	center.x /= (Real)slots.size();
	center.y /= (Real)slots.size();
	center.z /= (Real)slots.size();

	// Which way they will face: an arrow in front of the line.
	Coord3D mid;
	mid.x = (s_start.x + s_end.x) * 0.5f;
	mid.y = (s_start.y + s_end.y) * 0.5f;
	mid.z = center.z;

	const Real arrowLength = 40.0f;
	const Real headLength = 12.0f;
	Coord3D tip = mid;
	tip.x += facing.x * arrowLength;
	tip.y += facing.y * arrowLength;
	drawWorldLine( mid, tip, 2.0f, facingColor() );

	Coord3D headLeft = tip;
	headLeft.x += (-facing.x - facing.y) * headLength * 0.7071f;
	headLeft.y += (-facing.y + facing.x) * headLength * 0.7071f;
	Coord3D headRight = tip;
	headRight.x += (-facing.x + facing.y) * headLength * 0.7071f;
	headRight.y += (-facing.y - facing.x) * headLength * 0.7071f;
	drawWorldLine( tip, headLeft, 2.0f, facingColor() );
	drawWorldLine( tip, headRight, 2.0f, facingColor() );
}

} // namespace LineFormationPreview
