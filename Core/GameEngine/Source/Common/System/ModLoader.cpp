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

// ModLoader.cpp //////////////////////////////////////////////////////////////
// See ModLoader.h for how mods are found, selected and ordered.
///////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/ModLoader.h"

#include "Common/GlobalData.h"
#include "Common/LocalFileSystem.h"
#include "Common/UserPreferences.h"

#include <map>
#include <stdio.h>

namespace
{
	const char* const MODS_FOLDER = "Mods\\";
	const char* const MODS_REPORT = "ModsLoaded.txt";
	const char* const MODS_README = "README.txt";

	Bool s_hasCommandLineList = FALSE;
	Bool s_commandLineNoMods = FALSE;
	AsciiString s_commandLineList;

	AsciiString s_selection;                   // the selection string that was used
	std::vector<AsciiString> s_looseRoots;     // load order, lowest priority first
	std::vector<AsciiString> s_loadedMods;     // load order, lowest priority first
	std::vector<AsciiString> s_missingMods;    // requested by name but not found
	std::vector<AsciiString> s_foundMods;      // everything found in the Mods folders

	typedef std::map<AsciiString, ModLoader::ModInfo> ModInfoMap; // lower case name -> mod

	AsciiString toLowerCopy(const AsciiString& s)
	{
		AsciiString lower = s;
		lower.toLower();
		return lower;
	}

	Bool isSeparator(char c)
	{
		return c == '\\' || c == '/';
	}

	// Adds every mod found in baseDir (which ends with a backslash) to the map.
	void scanModsFolder(const AsciiString& baseDir, ModInfoMap& mods)
	{
		if (TheLocalFileSystem == nullptr)
			return;

		FilenameList allFiles;
		TheLocalFileSystem->getFileListInDirectory(baseDir, AsciiString::TheEmptyString, "*", allFiles, TRUE);

		const Int baseLen = baseDir.getLength();

		// Mods found in this folder replace mods of the same name found in an earlier folder.
		ModInfoMap found;

		for (FilenameListIter it = allFiles.begin(); it != allFiles.end(); ++it)
		{
			const AsciiString& fullPath = *it;
			if (fullPath.getLength() <= baseLen)
				continue;

			const char* rel = fullPath.str() + baseLen;
			const char* sep = rel;
			while (*sep != '\0' && !isSeparator(*sep))
				++sep;

			if (*sep == '\0')
			{
				// A file directly in Mods\. Only .big archives are mods; README, reports etc. are ignored.
				AsciiString leaf = rel;
				if (!leaf.endsWithNoCase(".big"))
					continue;

				ModLoader::ModInfo info;
				info.name = leaf;
				info.name.truncateBy(4);
				info.path = fullPath;
				info.isArchive = TRUE;
				if (info.name.isNotEmpty())
					found[toLowerCopy(info.name)] = info;
			}
			else
			{
				// A file inside a mod folder.
				AsciiString name;
				name.set(rel, (int)(sep - rel));
				if (name.isEmpty())
					continue;

				const AsciiString key = toLowerCopy(name);
				ModInfoMap::iterator modIt = found.find(key);
				if (modIt == found.end())
				{
					ModLoader::ModInfo info;
					info.name = name;
					info.path = baseDir;
					info.path.concat(name);
					info.path.concat('\\');
					info.isArchive = FALSE;
					modIt = found.insert(std::make_pair(key, info)).first;
				}
				else if (modIt->second.isArchive)
				{
					// Both "X.big" and folder "X" exist: keep the folder, it is the more complete form.
					modIt->second.isArchive = FALSE;
					modIt->second.name = name;
					modIt->second.path = baseDir;
					modIt->second.path.concat(name);
					modIt->second.path.concat('\\');
				}
				modIt->second.files.insert(fullPath);
			}
		}

		for (ModInfoMap::iterator it = found.begin(); it != found.end(); ++it)
			mods[it->first] = it->second;
	}

	void writeReadmeIfMissing(const AsciiString& modsDir)
	{
		AsciiString readme = modsDir;
		readme.concat(MODS_README);
		if (TheLocalFileSystem->doesFileExist(readme.str()))
			return;

		FILE* fp = fopen(readme.str(), "w");
		if (fp == nullptr)
			return;

		fputs(
			"Mods folder\n"
			"===========\n"
			"\n"
			"Put mods here instead of copying them over the game files.\n"
			"\n"
			"A mod is either\n"
			"  - a folder, for example Mods\\MyMod\\, holding .big archives and/or loose\n"
			"    files laid out like the game data (Data\\INI\\..., Art\\Textures\\..., Maps\\...)\n"
			"  - or a single .big file, for example Mods\\MyMod.big\n"
			"\n"
			"This folder and the Mods folder next to the game executable are both used.\n"
			"Mod folder names must not contain dots.\n"
			"\n"
			"Choose mods in Options.ini (in the folder above this one):\n"
			"  Mods = All            every mod, alphabetically (default);\n"
			"                        names starting with _ are skipped, e.g. _MyMod\n"
			"  Mods = None           no mods\n"
			"  Mods = ModA; ModB     only these mods, ModB wins over ModA\n"
			"\n"
			"Command line: -mods \"ModA;ModB\" or -nomods (both override Options.ini).\n"
			"\n"
			"ModsLoaded.txt in this folder lists what was loaded at the last start.\n"
			"Saved games should be loaded with the same mods they were made with.\n",
			fp);
		fclose(fp);
	}
}

namespace ModLoader
{

void setCommandLineModList(const AsciiString& list)
{
	s_hasCommandLineList = TRUE;
	s_commandLineNoMods = FALSE;
	s_commandLineList = list;
}

void setCommandLineNoMods()
{
	s_hasCommandLineList = FALSE;
	s_commandLineNoMods = TRUE;
	s_commandLineList.clear();
}

void selectMods(ModInfoList& outMods)
{
	outMods.clear();
	s_foundMods.clear();
	s_missingMods.clear();

	ModInfoMap mods;

	// 1. Mods next to the game executable.
	scanModsFolder(MODS_FOLDER, mods);

	// 2. Mods in the user data folder, which win on name clashes.
	if (TheGlobalData != nullptr && TheGlobalData->getPath_UserData().isNotEmpty())
	{
		AsciiString userMods = TheGlobalData->getPath_UserData();
		userMods.concat(MODS_FOLDER);
		TheLocalFileSystem->createDirectory(userMods);
		writeReadmeIfMissing(userMods);
		scanModsFolder(userMods, mods);
	}

	for (ModInfoMap::const_iterator it = mods.begin(); it != mods.end(); ++it)
		s_foundMods.push_back(it->second.name);

	// Which mods to load.
	AsciiString selection;
	if (s_commandLineNoMods)
	{
		selection = "None";
	}
	else if (s_hasCommandLineList)
	{
		selection = s_commandLineList;
	}
	else if (TheGlobalData != nullptr)
	{
		UserPreferences prefs;
		prefs.load("Options.ini");
		selection = prefs.getAsciiString("Mods", "All");
	}
	selection.trim();
	if (selection.isEmpty())
		selection = "All";
	s_selection = selection;

	const AsciiString lower = toLowerCopy(selection);
	if (lower == "none" || lower == "off" || lower == "no" || lower == "0")
		return;

	if (lower == "all" || lower == "auto" || lower == "yes" || lower == "1")
	{
		// std::map keeps them sorted by lower case name.
		for (ModInfoMap::const_iterator it = mods.begin(); it != mods.end(); ++it)
		{
			if (it->second.name.startsWith("_"))
				continue;
			outMods.push_back(it->second);
		}
		return;
	}

	AsciiString tokenizer = selection;
	AsciiString token;
	while (tokenizer.nextToken(&token, ";,"))
	{
		token.trim();
		// Allow "Name.big" in the list as well.
		if (token.endsWithNoCase(".big"))
			token.truncateBy(4);
		if (token.isEmpty())
			continue;

		ModInfoMap::const_iterator it = mods.find(toLowerCopy(token));
		if (it == mods.end())
		{
			s_missingMods.push_back(token);
			continue;
		}

		Bool alreadyListed = FALSE;
		for (size_t i = 0; i < outMods.size(); ++i)
		{
			if (outMods[i].name.compareNoCase(it->second.name) == 0)
			{
				alreadyListed = TRUE;
				break;
			}
		}
		if (!alreadyListed)
			outMods.push_back(it->second);
	}
}

void registerLooseRoot(const AsciiString& root)
{
	s_looseRoots.push_back(root);
}

void registerLoadedMod(const AsciiString& name)
{
	s_loadedMods.push_back(name);
}

void writeReport()
{
	if (TheGlobalData == nullptr || TheGlobalData->getPath_UserData().isEmpty())
		return;

	AsciiString path = TheGlobalData->getPath_UserData();
	path.concat(MODS_FOLDER);
	path.concat(MODS_REPORT);

	FILE* fp = fopen(path.str(), "w");
	if (fp == nullptr)
		return;

	fprintf(fp, "Mods setting: %s\n", s_selection.str());
	if (s_commandLineNoMods)
		fprintf(fp, "(from the command line: -nomods)\n");
	else if (s_hasCommandLineList)
		fprintf(fp, "(from the command line: -mods)\n");
	else
		fprintf(fp, "(from Options.ini, key Mods)\n");

	fprintf(fp, "\nLoaded, lowest priority first (later mods win):\n");
	if (s_loadedMods.empty())
		fprintf(fp, "  (none)\n");
	for (size_t i = 0; i < s_loadedMods.size(); ++i)
		fprintf(fp, "  %d. %s\n", (int)(i + 1), s_loadedMods[i].str());

	if (!s_missingMods.empty())
	{
		fprintf(fp, "\nRequested but not found:\n");
		for (size_t i = 0; i < s_missingMods.size(); ++i)
			fprintf(fp, "  %s\n", s_missingMods[i].str());
	}

	fprintf(fp, "\nFound in the Mods folders:\n");
	if (s_foundMods.empty())
		fprintf(fp, "  (none)\n");
	for (size_t i = 0; i < s_foundMods.size(); ++i)
		fprintf(fp, "  %s\n", s_foundMods[i].str());

	fclose(fp);
}

Bool findLooseFile(const char* relativePath, AsciiString& outPath)
{
	if (relativePath == nullptr || TheLocalFileSystem == nullptr)
		return FALSE;

	for (std::vector<AsciiString>::const_reverse_iterator it = s_looseRoots.rbegin(); it != s_looseRoots.rend(); ++it)
	{
		AsciiString candidate = *it;
		candidate.concat(relativePath);
		if (TheLocalFileSystem->doesFileExist(candidate.str()))
		{
			outPath = candidate;
			return TRUE;
		}
	}
	return FALSE;
}

const std::vector<AsciiString>& getLoadedMods()
{
	return s_loadedMods;
}

} // namespace ModLoader
