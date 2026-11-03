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

// FILE: GUILayout.cpp ////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"

#include "GameClient/GUILayout.h"

#include "Common/AddonCompat.h"
#include "Common/FileSystem.h"
#include "Common/GlobalData.h"
#include "Common/OptionPreferences.h"
#include "GameClient/Display.h"

namespace gui
{

namespace
{
enum UniformScalingState
{
	UniformScalingState_Unknown,
	UniformScalingState_Enabled,
	UniformScalingState_Disabled,
};

UniformScalingState s_uniformScalingState = UniformScalingState_Unknown;

Real getAnchorOffset(LayoutAnchor anchor, Real displaySize, Real scaledSize)
{
	switch (anchor)
	{
	case LayoutAnchor_Min:
		return 0.0f;
	case LayoutAnchor_Max:
		return displaySize - scaledSize;
	case LayoutAnchor_Stretch:
	case LayoutAnchor_Center:
	default:
		return (displaySize - scaledSize) * 0.5f;
	}
}

} // namespace

Bool isUniformScalingEnabled()
{
	if (s_uniformScalingState == UniformScalingState_Unknown)
	{
		Bool enabled = TRUE;

		if (TheFileSystem != nullptr && addon::HasFullviewportDat())
		{
			// 'Control Bar Pro' Addons ship art for stretched widescreen layouts.
			enabled = FALSE;
		}
		else if (TheGlobalData != nullptr)
		{
			OptionPreferences optionPref;
			enabled = optionPref.getUniformUIScalingEnabled();
		}

		s_uniformScalingState = enabled ? UniformScalingState_Enabled : UniformScalingState_Disabled;
	}

	return s_uniformScalingState == UniformScalingState_Enabled;
}

void resetUniformScalingCache()
{
	s_uniformScalingState = UniformScalingState_Unknown;
}

void setUniformScalingEnabled(Bool enabled)
{
	s_uniformScalingState = enabled ? UniformScalingState_Enabled : UniformScalingState_Disabled;
}

LayoutTransform getLayoutTransform(Int creationWidth, Int creationHeight, LayoutAnchor anchorX, LayoutAnchor anchorY)
{
	LayoutTransform transform;
	transform.scaleX = 1.0f;
	transform.scaleY = 1.0f;
	transform.offsetX = 0.0f;
	transform.offsetY = 0.0f;

	if (TheDisplay == nullptr || creationWidth <= 0 || creationHeight <= 0)
		return transform;

	const Real displayWidth = (Real)TheDisplay->getWidth();
	const Real displayHeight = (Real)TheDisplay->getHeight();
	const Real stretchX = displayWidth / (Real)creationWidth;
	const Real stretchY = displayHeight / (Real)creationHeight;

	if (!isUniformScalingEnabled())
	{
		transform.scaleX = stretchX;
		transform.scaleY = stretchY;
		return transform;
	}

	const Real uniformScale = min(stretchX, stretchY);

	if (anchorX == LayoutAnchor_Stretch)
	{
		transform.scaleX = stretchX;
	}
	else
	{
		transform.scaleX = uniformScale;
		transform.offsetX = floorf(getAnchorOffset(anchorX, displayWidth, creationWidth * uniformScale));
	}

	if (anchorY == LayoutAnchor_Stretch)
	{
		transform.scaleY = stretchY;
	}
	else
	{
		transform.scaleY = uniformScale;
		transform.offsetY = floorf(getAnchorOffset(anchorY, displayHeight, creationHeight * uniformScale));
	}

	return transform;
}

LayoutTransform getDefaultLayoutTransform(Int creationWidth, Int creationHeight)
{
	return getLayoutTransform(creationWidth, creationHeight, LayoutAnchor_Center, LayoutAnchor_Center);
}

LayoutTransform getControlBarLayoutTransform(Int creationWidth, Int creationHeight)
{
	return getLayoutTransform(creationWidth, creationHeight, LayoutAnchor_Center, LayoutAnchor_Max);
}

Bool hasHorizontalMargins()
{
	const LayoutTransform transform = getDefaultLayoutTransform();
	return transform.offsetX >= 1.0f;
}

} // namespace gui
