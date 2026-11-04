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

// ModLoader.h ////////////////////////////////////////////////////////////////
// Loads mods from a "Mods" folder without replacing any game files.
//
// A mod is either
//   - a sub folder of Mods\ that holds .big archives and/or loose files laid out
//     like the game data (Data\INI\..., Art\Textures\..., Maps\...), or
//   - a single .big archive placed directly in Mods\.
//
// Mods are looked up in two places, the second one winning on name clashes:
//   <game folder>\Mods\
//   <user data folder>\Mods\   (Documents\Command and Conquer Generals Zero Hour Data\Mods)
//
// Which mods load, and in which order, comes from (first match wins):
//   command line  -nomods            no mods
//                 -mods "A;B"        exactly these mods, in this order
//   Options.ini   Mods = None        no mods
//                 Mods = A; B        exactly these mods, in this order
//                 Mods = All         (default) every mod, alphabetically,
//                                    except names starting with '_'
// Mods later in the list win over earlier ones; all mods win over game files.
// The result is written to <user data folder>\Mods\ModsLoaded.txt.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/AsciiString.h"
#include "Common/FileSystem.h"

#include <vector>

namespace ModLoader
{
	struct ModInfo
	{
		AsciiString name;      ///< display name (folder name, or .big file name without extension)
		AsciiString path;      ///< folder path with trailing backslash, or the .big file path
		Bool isArchive;        ///< TRUE if the mod is a single .big file
		FilenameList files;    ///< for folder mods: every file inside the folder (full paths)
	};

	typedef std::vector<ModInfo> ModInfoList;

	/// Command line overrides (parsed before the file system starts).
	void setCommandLineModList(const AsciiString& list);
	void setCommandLineNoMods();

	/// Finds the mods to load, in load order (lowest priority first).
	void selectMods(ModInfoList& outMods);

	/// Remembers a loose-file mod folder (with trailing backslash) in load order.
	void registerLooseRoot(const AsciiString& root);

	/// Remembers that a mod was loaded, for the report and the window title.
	void registerLoadedMod(const AsciiString& name);

	/// Writes Mods\ModsLoaded.txt to the user data folder.
	void writeReport();

	/// Looks for a loose mod file by its game-relative path (e.g. "Data\\Movies\\Intro.bik"),
	/// highest priority mod first. For code that needs a real disk path (movies, cursors).
	Bool findLooseFile(const char* relativePath, AsciiString& outPath);

	/// Names of the loaded mods in load order.
	const std::vector<AsciiString>& getLoadedMods();
}
