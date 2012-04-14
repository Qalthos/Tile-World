/* help.h: Displaying online help screens.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_help_h_
#define	_help_h_

/* The available help topics.
 */
enum {
    Help_None = 0,
    Help_First,
    Help_KeysDuringGame,
    Help_KeysBetweenGames,
    Help_FileListKeys,
    Help_ScoreListKeys,
    Help_ObjectsOfGame,
    Help_CmdlineOptions,
    Help_AboutGame
};

/* Help for the command-line options.
 */
extern tablespec const *yowzitch;

/* Version and license information.
 */
extern tablespec const *vourzhon;

/* Display online help screens for the game, using the given topic as
 * the default topic.
 */
extern void onlinemainhelp(int topic);

/* Display a single online help screen for the given topic.
 */
extern void onlinecontexthelp(int topic);

#endif
