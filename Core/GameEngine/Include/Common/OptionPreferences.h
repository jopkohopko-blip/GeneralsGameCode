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

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
// FILE: OptionPreferences.h
// Author: Matthew D. Campbell, April 2002
// Description: Saving/Loading of option preferences
///////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "WW3D2/ww3d.h"
#include "WW3D2/texturefilter.h"

#include "Common/UserPreferences.h"

typedef UnsignedInt CursorCaptureMode;
typedef UnsignedInt ScreenEdgeScrollMode;

//-----------------------------------------------------------------------------
// OptionsPreferences options menu class
//-----------------------------------------------------------------------------
class OptionPreferences : public UserPreferences
{
public:
	OptionPreferences();
	virtual ~OptionPreferences() override;

	enum AntiAliasingMode CPP_11(: Int)
	{
		AntiAliasingMode_OFF = 0,
		AntiAliasingMode_MSAA_2X,
		AntiAliasingMode_MSAA_4X,
		AntiAliasingMode_MSAA_8X,
		AntiAliasingMode_Count
	};

	// @feature How the game window is presented. Stored as "WindowMode" in Options.ini.
	enum WindowMode CPP_11(: Int)
	{
		WindowMode_Fullscreen = 0, ///< Exclusive fullscreen (original behavior)
		WindowMode_Windowed, ///< Window with a frame
		WindowMode_Borderless, ///< Window without a frame (default)
		WindowMode_Count
	};

	Bool loadFromIniFile();

	WindowMode getWindowMode() const;
	void setWindowMode(WindowMode mode);
	static void applyWindowModeToGlobalData(WindowMode mode);

	// @feature Moves Options.ini files written by older builds to the modern graphics defaults once.
	// Only values that still match the old defaults are touched. Stored as "GraphicsDefaultsVersion".
	static void upgradeGraphicsDefaults();

	WW3D::MultiSampleModeEnum getAntiAliasing() const;
	TextureFilterClass::TextureFilterMode getTextureFilterMode() const;
	TextureFilterClass::AnisotropicFilterMode getTextureAnisotropyLevel() const;

	// Loading speed options
	Bool getBriefingMoviesEnabled() const;      ///< Options.ini "BriefingMovies": play the campaign load screen briefing movie
	Bool getKeepAssetsBetweenMapsEnabled() const; ///< Options.ini "KeepAssetsBetweenMaps": keep loaded models and textures cached between maps
	Bool getLogLoadTimesEnabled() const;        ///< Options.ini "LogLoadTimes": write LoadTimes.txt with per-stage map load timings
	Bool getSmartAIEnabled() const;             ///< Options.ini "SmartAI": smarter computer opponents in campaign/skirmish (default yes)
	Bool getBrutalAIEnabled() const;            ///< Options.ini "BrutalAI": "Hard" computer opponents become "Brutal" (default no)

	UnsignedInt getLANIPAddress();
	UnsignedInt getOnlineIPAddress();
	void setLANIPAddress(AsciiString IP);
	void setOnlineIPAddress(AsciiString IP);
	void setLANIPAddress(UnsignedInt IP);
	void setOnlineIPAddress(UnsignedInt IP);
	Bool getArchiveReplaysEnabled() const;
	Bool getAlternateMouseModeEnabled();
	Bool getRightMouseScrollWithAlternateMouseEnabled() const;
	Bool getRetaliationModeEnabled();
	Bool getDoubleClickAttackMoveEnabled();
	Bool getFormationDragEnabled() const;
	Bool getFormationSmartRolesEnabled() const;
	Bool getFormationMoveTogetherEnabled() const;
	Bool getSmartPathingEnabled() const;
	Int getJpegQuality() const;
	Real getScrollFactor();
	Bool getDrawScrollAnchor();
	Bool getMoveScrollAnchor();
	Real getCameraZoomOutScale() const;
	Bool getMiddleMouseScrollEnabled() const;
	Bool getSmoothScrollEnabled() const;
	Bool getCursorCaptureEnabledInWindowedGame() const;
	Bool getCursorCaptureEnabledInWindowedMenu() const;
	Bool getCursorCaptureEnabledInFullscreenGame() const;
	Bool getCursorCaptureEnabledInFullscreenMenu() const;
	CursorCaptureMode getCursorCaptureMode() const;
	Bool getScreenEdgeScrollEnabledInWindowedApp() const;
	Bool getScreenEdgeScrollEnabledInFullscreenApp() const;
	ScreenEdgeScrollMode getScreenEdgeScrollMode() const;
	Int getFirewallBehavior();
	Short getFirewallPortAllocationDelta();
	UnsignedShort getFirewallPortOverride();
	Bool getFirewallNeedToRefresh();
	Bool usesSystemMapDir();
	AsciiString getPreferred3DProvider();
	AsciiString getSpeakerType();
	Real getSoundVolume();
	Real get3DSoundVolume();
	Real getSpeechVolume();
	Real getMusicVolume();
	Real getMoneyTransactionVolume() const;
	Bool saveCameraInReplays();
	Bool useCameraInReplays();
	Bool getPlayerObserverEnabled() const;
	Int getStaticGameDetail();
	Int getIdealStaticGameDetail();
	Real getGammaValue();
	Int getTextureReduction();
	void getResolution(Int *xres, Int *yres);
	Bool get3DShadowsEnabled();
	Bool get2DShadowsEnabled();
	Bool getCloudShadowsEnabled();
	Bool getLightmapEnabled();
	Bool getSmoothWaterEnabled();
	Bool getTreesEnabled();
	Bool getExtraAnimationsDisabled();
	Bool getUseHeatEffects();
	Bool getDynamicLODEnabled();
	Bool getFPSLimitEnabled();
	Bool getBuildingOcclusionEnabled();
	Int getParticleCap();

	Int getCampaignDifficulty();
	void setCampaignDifficulty(Int diff);

	Int getNetworkLatencyFontSize();
	Int getRenderFpsFontSize();
	Int getSystemTimeFontSize();
	Int getGameTimeFontSize();
	Int getPlayerInfoListFontSize();

	Real getResolutionFontAdjustment();
	Bool getUniformUIScalingEnabled() const;

	Bool getShowMoneyPerMinute() const;
	Int getHealthBarMode() const;
	Bool getShowExperienceBars() const;
	Bool getShowPowerCooldownTimers() const;
	Bool getShowArmyCounter() const;
	Bool getShowIdleWorkerCount() const;
	void setHealthBarMode( Int mode );

	Real getGameWindowTransitionSpeedMultiplier() const;
};
