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

// FILE: ControlBarMultiSelect.cpp ////////////////////////////////////////////////////////////////
// Author: Colin Day, March 2002
// Desc:   Context-sensitive GUI for when you select multiple objects.  What we do is show
//				 the commands that you can use between them all
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/MessageStream.h"
#include "Common/NameKeyGenerator.h"
#include "Common/ThingTemplate.h"
#include "GameClient/ControlBar.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameClient.h"
#include "GameClient/GadgetPushButton.h"
#include "GameClient/Gadget.h"
#include "GameClient/GameWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Keyboard.h"
#include "GameLogic/Object.h"




//-------------------------------------------------------------------------------------------------
/** Reset the common command data */
//-------------------------------------------------------------------------------------------------
void ControlBar::resetCommonCommandData()
{
	Int i;

	for( i = 0; i < MAX_COMMANDS_PER_SET; i++ )
	{
		m_commonCommands[ i ] = nullptr;
		//Clear out any remnant overlays.
		GadgetButtonDrawOverlayImage( m_commandWindows[ i ], nullptr );
	}

}

//-------------------------------------------------------------------------------------------------
/** add the common commands of this drawable to the common command set */
//-------------------------------------------------------------------------------------------------
void ControlBar::addCommonCommands( Drawable *draw, Bool firstDrawable )
{
	Int i;
	const CommandButton *command;

	// sanity
	if( draw == nullptr )
		return;

	Object* obj = draw->getObject();
	if (!obj)
		return;

	if (obj->isKindOf(KINDOF_IGNORED_IN_GUI)) // ignore these guys
		return;

	// get the command set of this drawable
	const CommandSet *commandSet = findCommandSet( obj->getCommandSetString() );
	if( commandSet == nullptr )
	{

		//
		// if there is no command set for this drawable, none of the selected drawables
		// can possibly have matching commands so we'll get rid of them all
		//
		for( i = 0; i < MAX_COMMANDS_PER_SET; i++ )
		{

			m_commonCommands[ i ] = nullptr;
			if (m_commandWindows[ i ])
			{
				m_commandWindows[ i ]->winHide( TRUE );
			}
			// After Every change to the m_commandWIndows, we need to show fill in the missing blanks with the images
	// removed from multiplayer branch
			//showCommandMarkers();

		}

		return;

	}


	//
	// easy case, if we're adding the first drawable we simply just add any of the commands
	// in its set that can be multi-select commands to the common command set
	//
	if( firstDrawable == TRUE )
	{

		// just add each command that is classified as a common command
		for( i = 0; i < MAX_COMMANDS_PER_SET; i++ )
		{
			// our implementation doesn't necessarily make use of the max possible command buttons
			if (! m_commandWindows[ i ]) continue;

			// get command
			command = commandSet->getCommandButton(i);

			// add if present and can be used in a multi select
			if( command && BitIsSet( command->getOptions(), OK_FOR_MULTI_SELECT ) == TRUE )
			{

				// put it in the common command set
				m_commonCommands[ i ] = command;

				// show and enable this control
				m_commandWindows[ i ]->winHide( FALSE );
				m_commandWindows[ i ]->winEnable( TRUE );

				// set the command into the control
				setControlCommand( m_commandWindows[ i ], command );

			}

		}

	}
	else
	{

		// go through each command one by one
		for( i = 0; i < MAX_COMMANDS_PER_SET; i++ )
		{

			// our implementation doesn't necessarily make use of the max possible command buttons
			if (! m_commandWindows[ i ]) continue;

			// get the command
			command = commandSet->getCommandButton(i);

			Bool attackMove = (command && command->getCommandType() == GUI_COMMAND_ATTACK_MOVE) ||
												(m_commonCommands[ i ] && m_commonCommands[ i ]->getCommandType() == GUI_COMMAND_ATTACK_MOVE);

			// Kris: When any units have attack move, they all get it. This is to allow
			// combat units to be selected with the odd dozer or pilot and still retain that ability.
			if( attackMove && !m_commonCommands[ i ] )
			{
				// put it in the common command set
				m_commonCommands[ i ] = command;

				// show and enable this control
				m_commandWindows[ i ]->winHide( FALSE );
				m_commandWindows[ i ]->winEnable( TRUE );

				// set the command into the control
				setControlCommand( m_commandWindows[ i ], command );
			}
			else if( command != m_commonCommands[ i ] && !attackMove )
			{
				//
				// if this command does not match the command that is in the common command set then
				// *neither* this command OR the command in the common command set are really common
				// commands, so we will remove the one that has been stored in the common set
				//

				// remove the common command
				m_commonCommands[ i ] = nullptr;

				//
				// hide the window control cause it should have been made visible from a command
				// that was placed in this common 'slot' earlier
				//
				m_commandWindows[ i ]->winHide( TRUE );
			}

		}

	}

	// After Every change to the m_commandWIndows, we need to show fill in the missing blanks with the images
	// removed from multiplayer branch
	//showCommandMarkers();

}

//-------------------------------------------------------------------------------------------------
/** Populate the visible command bar with commands that are common to all the objects
	* that are selected in the UI */
//-------------------------------------------------------------------------------------------------
void ControlBar::populateMultiSelect()
{
	Drawable *draw;
	Bool firstDrawable = TRUE;
	Bool portraitSet = FALSE;
	const Image *portrait = nullptr;
	Object *portraitObj = nullptr;

	// first reset the common command data
	resetCommonCommandData();

	// by default, hide all the controls in the command section
	for( Int i = 0; i < MAX_COMMANDS_PER_SET; i++ )
	{
		if (m_commandWindows[ i ])
		{
			m_commandWindows[ i ]->winHide( TRUE );
		}
	}

	// sanity
	DEBUG_ASSERTCRASH( TheInGameUI->getSelectCount() > 1,
										 ("populateMultiSelect: Can't populate multiselect context cause there are only '%d' things selected",
										  TheInGameUI->getSelectCount()) );

	// get the list of drawable IDs from the in game UI
	const DrawableList *selectedDrawables = TheInGameUI->getAllSelectedDrawables();

	// sanity
	DEBUG_ASSERTCRASH( selectedDrawables->empty() == FALSE, ("populateMultiSelect: Drawable list is empty") );

	// loop through all the selected drawables
	for( DrawableListCIt it = selectedDrawables->begin();
			 it != selectedDrawables->end(); ++it )
	{

		// get the drawable
		draw = *it;


		if (draw->getObject()->isKindOf(KINDOF_IGNORED_IN_GUI)) // ignore these guys
			continue;


		//
		// add command for this drawable, note that we also sanity check to make sure the
		// drawable has an object as all interesting drawables that we can select should
		// actually have an object underneath it so that we can do interesting things with
		// it ... otherwise we should have never selected it.
		// NOTE that we're not considering objects that are currently in the process of
		// being sold as those objects can't be issued anymore commands
		//
		if( draw && draw->getObject() &&
				!draw->getObject()->getStatusBits().test( OBJECT_STATUS_SOLD ) )
		{

			// add the common commands of this drawable to the common command set
			addCommonCommands( draw, firstDrawable );

			// not adding the first drawable anymore
			firstDrawable = FALSE;

			//
			// keep track of the portrait images, if all units selected have the same portrait
			// we will display it in the right HUD, otherwise we won't
			//
			if( portraitSet == FALSE )
			{

				portrait = draw->getTemplate()->getSelectedPortraitImage();
				portraitObj = draw->getObject();
				portraitSet = TRUE;

			}
			else if( draw->getTemplate()->getSelectedPortraitImage() != portrait )
				portrait = nullptr;

		}

	}

	// set the portrait image
	setPortraitByObject( portraitObj );

}

//-------------------------------------------------------------------------------------------------
/** Update logic for the multi select context sensitive GUI */
//-------------------------------------------------------------------------------------------------
void ControlBar::updateContextMultiSelect()
{
	Drawable *draw;
	Object *obj;
	const CommandButton *command;
	GameWindow *win;
	Int objectsThatCanDoCommand[ MAX_COMMANDS_PER_SET ];
	Int i;

	// zero the array that counts how many objects can do each command
	memset( objectsThatCanDoCommand, 0, sizeof( objectsThatCanDoCommand ) );

	// sanity
	DEBUG_ASSERTCRASH( TheInGameUI->getSelectCount() > 1,
										 ("updateContextMultiSelect: TheInGameUI only has '%d' things selected",
										  TheInGameUI->getSelectCount()) );

	// get the list of drawable IDs from the in game UI
	const DrawableList *selectedDrawables = TheInGameUI->getAllSelectedDrawables();

	// sanity
	DEBUG_ASSERTCRASH( selectedDrawables->empty() == FALSE, ("populateMultiSelect: Drawable list is empty") );

	// loop through all the selected drawable IDs
	for( DrawableListCIt it = selectedDrawables->begin();
			 it != selectedDrawables->end(); ++it )
	{

		// get the drawable from the ID
		draw = *it;

		if (draw->getObject()->isKindOf(KINDOF_IGNORED_IN_GUI)) // ignore these guys
			continue;


		// get the object
		obj = draw->getObject();

		// sanity
		if( obj == nullptr )
			continue;

		// for each of the visible command windows make sure the object can execute the command
		for( i = 0; i < MAX_COMMANDS_PER_SET; i++ )
		{

			// get the control window
			win = m_commandWindows[ i ];

			// our implementation doesn't necessarily make use of the max possible command buttons
			if (!win) continue;

			// don't consider hidden windows
			if( win->winIsHidden() == TRUE )
				continue;

			// get the command
			command = (const CommandButton *)GadgetButtonGetData(win);
			if( command == nullptr )
				continue;

			// can we do the command
			CommandAvailability availability = getCommandAvailability( command, obj, win );

			win->winClearStatus( WIN_STATUS_NOT_READY );
			win->winClearStatus( WIN_STATUS_ALWAYS_COLOR );

			// enable/disable the window control
			switch( availability )
			{
				case COMMAND_HIDDEN:
					win->winHide( TRUE );
					break;
				case COMMAND_RESTRICTED:
					win->winEnable( FALSE );
					break;
				case COMMAND_NOT_READY:
					win->winEnable( FALSE );
					win->winSetStatus( WIN_STATUS_NOT_READY );
					break;
				case COMMAND_CANT_AFFORD:
					win->winEnable( FALSE );
					win->winSetStatus( WIN_STATUS_ALWAYS_COLOR );
					break;
				default:
					win->winEnable( TRUE );
					break;
			}

			//If button is a CHECK_LIKE, then update it's status now.
			if( BitIsSet( command->getOptions(), CHECK_LIKE ) )
			{
				GadgetCheckLikeButtonSetVisualCheck( win, availability == COMMAND_ACTIVE );
			}

			if( availability == COMMAND_AVAILABLE || availability == COMMAND_ACTIVE )
					objectsThatCanDoCommand[ i ]++;

		}

	}

	//
	// for each command, if any objects can do the command we enable the window, otherwise
	// we disable it
	//
	for( i = 0; i < MAX_COMMANDS_PER_SET; i++ )
	{
		// our implementation doesn't necessarily make use of the max possible command buttons
		if (! m_commandWindows[ i ]) continue;

		// don't consider hidden commands
		if( m_commandWindows[ i ]->winIsHidden() == TRUE )
			continue;

		// don't consider slots that don't have commands
		if( m_commonCommands[ i ] == nullptr )
			continue;

		// check the count of objects that can do the command and enable/disable the control,
		if( objectsThatCanDoCommand[ i ] > 0 )
			m_commandWindows[ i ]->winEnable( TRUE );
		else
			m_commandWindows[ i ]->winEnable( FALSE );

	}

	// After Every change to the m_commandWIndows, we need to show fill in the missing blanks with the images
	// removed from multiplayer branch
	//showCommandMarkers();


}

//-------------------------------------------------------------------------------------------------
/** TheSuperHackers @feature Forget which unit types the build queue buttons represent */
//-------------------------------------------------------------------------------------------------
void ControlBar::clearMultiSelectTypes()
{
	for( Int i = 0; i < MAX_BUILD_QUEUE_BUTTONS; ++i )
	{
		m_multiSelectTypeButtons[ i ] = nullptr;
		m_multiSelectTypes[ i ] = nullptr;
	}
}

//-------------------------------------------------------------------------------------------------
/** TheSuperHackers @feature When several different unit types are selected, reuse the build queue
	* buttons to show one button per type with the number of selected units of that type. The most
	* numerous types come first. */
//-------------------------------------------------------------------------------------------------
void ControlBar::populateMultiSelectTypes()
{
	struct TypeCount
	{
		const ThingTemplate *thing;
		Int count;
	};

	clearMultiSelectTypes();

	GameWindow *queueParent = m_contextParent[ CP_BUILD_QUEUE ];
	if( queueParent == nullptr )
		return;

	// count the selected objects per type, keeping the order in which the types first appear
	std::vector<TypeCount> types;
	const DrawableList *selectedDrawables = TheInGameUI->getAllSelectedDrawables();
	for( DrawableListCIt it = selectedDrawables->begin(); it != selectedDrawables->end(); ++it )
	{
		const Drawable *draw = *it;
		const Object *obj = draw ? draw->getObject() : nullptr;
		if( obj == nullptr || obj->isKindOf( KINDOF_IGNORED_IN_GUI ) ||
				obj->getStatusBits().test( OBJECT_STATUS_SOLD ) || !obj->isLocallyControlled() )
			continue;

		// Use the Object's thing template, doing so will prevent weirdness for disguised vehicles.
		const ThingTemplate *thing = obj->getTemplate();
		size_t i = 0;
		for( ; i < types.size(); ++i )
		{
			if( types[ i ].thing == thing )
			{
				++types[ i ].count;
				break;
			}
		}
		if( i == types.size() )
		{
			TypeCount entry;
			entry.thing = thing;
			entry.count = 1;
			types.push_back( entry );
		}
	}

	// a selection of a single type already shows its portrait, nothing to choose from here
	if( types.size() < 2 )
	{
		queueParent->winHide( TRUE );
		return;
	}

	// bubble the most numerous types to the front, stable for equal counts
	for( size_t i = 1; i < types.size(); ++i )
	{
		for( size_t j = i; j > 0 && types[ j ].count > types[ j - 1 ].count; --j )
		{
			TypeCount tmp = types[ j ];
			types[ j ] = types[ j - 1 ];
			types[ j - 1 ] = tmp;
		}
	}

	static NameKeyType buildQueueIDs[ MAX_BUILD_QUEUE_BUTTONS ];
	static Bool idsInitialized = FALSE;
	if( idsInitialized == FALSE )
	{
		AsciiString buttonName;
		for( Int i = 0; i < MAX_BUILD_QUEUE_BUTTONS; ++i )
		{
			buttonName.format( "ControlBar.wnd:ButtonQueue%02d", i + 1 );
			buildQueueIDs[ i ] = TheNameKeyGenerator->nameToKey( buttonName );
		}
		idsInitialized = TRUE;
	}

	for( Int i = 0; i < MAX_BUILD_QUEUE_BUTTONS; ++i )
	{
		GameWindow *button = TheWindowManager->winGetWindowFromId( queueParent, buildQueueIDs[ i ] );
		if( button == nullptr )
			continue;

		// the button must not look like a cancel production command anymore
		GadgetButtonSetData( button, nullptr );
		GadgetButtonDrawOverlayImage( button, nullptr );

		if( i < (Int)types.size() )
		{
			const ThingTemplate *thing = types[ i ].thing;
			const Image *image = thing->getButtonImage();
			if( image == nullptr )
				image = thing->getSelectedPortraitImage();

			m_multiSelectTypeButtons[ i ] = button;
			m_multiSelectTypes[ i ] = thing;

			button->winEnable( TRUE );
			button->winSetStatus( WIN_STATUS_USE_OVERLAY_STATES | WIN_STATUS_RIGHT_CLICK );
			GadgetButtonSetEnabledImage( button, image );

			UnicodeString text;
			if( types[ i ].count > 1 )
				text.format( L"%d", types[ i ].count );
			GadgetButtonSetText( button, text );
		}
		else
		{
			button->winEnable( FALSE );
			button->winClearStatus( WIN_STATUS_USE_OVERLAY_STATES | WIN_STATUS_RIGHT_CLICK );
			GadgetButtonSetText( button, L"" );
		}
	}

	queueParent->winHide( FALSE );

	// the type buttons take the place of the portrait
	setPortraitByObject( nullptr );
}

//-------------------------------------------------------------------------------------------------
/** TheSuperHackers @feature A click on a unit type button of a mixed selection keeps only the
	* units of that type selected. Ctrl or Shift click, or a right click, removes that type from
	* the selection instead. Returns TRUE when the button was one of the type buttons. */
//-------------------------------------------------------------------------------------------------
Bool ControlBar::processMultiSelectTypeClick( GameWindow *button, GadgetGameMessage gadgetMessage )
{
	if( m_currContext != CB_CONTEXT_MULTI_SELECT || button == nullptr )
		return FALSE;

	const ThingTemplate *thing = nullptr;
	for( Int i = 0; i < MAX_BUILD_QUEUE_BUTTONS; ++i )
	{
		if( m_multiSelectTypeButtons[ i ] == button )
		{
			thing = m_multiSelectTypes[ i ];
			break;
		}
	}
	if( thing == nullptr )
		return FALSE;

	const Bool removeType = gadgetMessage == GBM_SELECTED_RIGHT ||
		( TheKeyboard && ( TheKeyboard->isCtrl() || TheKeyboard->isShift() ) );

	// copy the selection, it changes while we (de)select
	DrawableList selection = *TheInGameUI->getAllSelectedDrawables();

	if( removeType )
	{
		GameMessage *msg = TheMessageStream->appendMessage( GameMessage::MSG_REMOVE_FROM_SELECTED_GROUP );
		for( DrawableListIt it = selection.begin(); it != selection.end(); ++it )
		{
			Drawable *draw = *it;
			Object *obj = draw ? draw->getObject() : nullptr;
			if( obj && obj->getTemplate() == thing )
			{
				msg->appendObjectIDArgument( obj->getID() );
				TheInGameUI->deselectDrawable( draw );
			}
		}
	}
	else
	{
		DrawableList keep;
		for( DrawableListIt it = selection.begin(); it != selection.end(); ++it )
		{
			Drawable *draw = *it;
			Object *obj = draw ? draw->getObject() : nullptr;
			if( obj && obj->getTemplate() == thing && obj->isLocallyControlled() )
				keep.push_back( draw );
		}
		if( keep.empty() )
			return TRUE;

		TheInGameUI->deselectAllDrawables();

		GameMessage *msg = TheMessageStream->appendMessage( GameMessage::MSG_CREATE_SELECTED_GROUP );
		msg->appendBooleanArgument( TRUE );
		for( DrawableListIt it = keep.begin(); it != keep.end(); ++it )
		{
			Drawable *draw = *it;
			msg->appendObjectIDArgument( draw->getObject()->getID() );
			TheInGameUI->selectDrawable( draw );
		}
	}

	return TRUE;
}
