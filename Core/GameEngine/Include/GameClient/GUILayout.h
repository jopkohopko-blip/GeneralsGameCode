/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
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

// FILE: GUILayout.h //////////////////////////////////////////////////////////////////////////////
// Maps GUI coordinates that were authored for a creation resolution (usually 800x600) to the
// current display resolution.
//
// The original game stretched the GUI independently in x and y, which distorts every window,
// image and button on displays that are not 4:3. With uniform scaling the GUI is scaled by the
// same factor in both directions and the scaled 4:3 area is anchored on the display (centered by
// default), so wide displays show the GUI undistorted with the 3D world or the shell map visible
// to the sides.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Lib/BaseType.h"

namespace gui
{

enum LayoutAnchor
{
	LayoutAnchor_Stretch, ///< Fills the display on this axis, ignoring the aspect ratio. The original behavior.
	LayoutAnchor_Min,     ///< Sticks to the left or top edge of the display.
	LayoutAnchor_Center,  ///< Centered on the display.
	LayoutAnchor_Max,     ///< Sticks to the right or bottom edge of the display.
};

struct LayoutTransform
{
	Real scaleX;
	Real scaleY;
	Real offsetX;
	Real offsetY;

	Int toScreenX(Real x) const { return (Int)(x * scaleX + offsetX); }
	Int toScreenY(Real y) const { return (Int)(y * scaleY + offsetY); }
	Int toScreenWidth(Real w) const { return (Int)(w * scaleX); }
	Int toScreenHeight(Real h) const { return (Int)(h * scaleY); }
};

/// Returns whether the GUI keeps its aspect ratio. Can be turned off in Options.ini with
/// 'UniformUIScaling = no'. It is always off with 'Control Bar Pro' Addons, because they ship
/// their own widescreen art that expects the original stretching.
Bool isUniformScalingEnabled();

/// Re-reads the uniform scaling option on the next query.
void resetUniformScalingCache();

/// Forces uniform scaling on or off, for example for tools that edit layouts in screen space.
void setUniformScalingEnabled(Bool enabled);

/// Returns the transform for GUI content that was authored at the given creation resolution.
/// When uniform scaling is disabled, both axes are stretched like in the original game.
LayoutTransform getLayoutTransform(Int creationWidth, Int creationHeight, LayoutAnchor anchorX, LayoutAnchor anchorY);

/// Returns the transform for regular GUI content: uniformly scaled and centered.
LayoutTransform getDefaultLayoutTransform(Int creationWidth = 800, Int creationHeight = 600);

/// Returns the transform for the in-game Control Bar: uniformly scaled, centered horizontally and
/// sticking to the bottom of the display.
LayoutTransform getControlBarLayoutTransform(Int creationWidth = 800, Int creationHeight = 600);

/// Returns true when the uniformly scaled GUI leaves free space to the left and right of the
/// display, which happens on displays that are wider than 4:3.
Bool hasHorizontalMargins();

} // namespace gui
