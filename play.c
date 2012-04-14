/* play.c: Top-level game-playing functions.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	<stdlib.h>
#include	<string.h>
#include	"defs.h"
#include	"err.h"
#include	"state.h"
#include	"encoding.h"
#include	"oshw.h"
#include	"res.h"
#include	"logic.h"
#include	"random.h"
#include	"solution.h"
#include	"unslist.h"
#include	"play.h"

/* The current state of the current game.
 */
static gamestate	state;

/* The current logic module.
 */
static gamelogic       *logic = NULL;

/* TRUE if the program is running without a user interface.
 */
int			batchmode = FALSE;

/* How much mud to make the timer suck (i.e., the slowdown factor).
 */
static int		mudsucking = 1;

/* Turn on the pedantry.
 */
void setpedanticmode(void)
{
    pedanticmode = TRUE;
}

/* Set the slowdown factor.
 */
int setmudsuckingfactor(int mud)
{
    if (mud < 1)
	return FALSE;
    mudsucking = mud;
    return TRUE;
}

/* Configure the game logic, and some of the OS/hardware layer, as
 * required for the given ruleset. Do nothing if the requested ruleset
 * is already the current ruleset.
 */
static int setrulesetbehavior(int ruleset)
{
    if (logic) {
	if (ruleset == logic->ruleset)
	    return TRUE;
	(*logic->shutdown)(logic);
	logic = NULL;
    }
    if (ruleset == Ruleset_None)
	return TRUE;

    switch (ruleset) {
      case Ruleset_Lynx:
	logic = lynxlogicstartup();
	if (!logic)
	    return FALSE;
	if (!batchmode)
	    setkeyboardarrowsrepeat(TRUE);
	settimersecond(1000 * mudsucking);
	break;
      case Ruleset_MS:
	logic = mslogicstartup();
	if (!logic)
	    return FALSE;
	if (!batchmode)
	    setkeyboardarrowsrepeat(FALSE);
	settimersecond(1100 * mudsucking);
	break;
      default:
	errmsg(NULL, "unknown ruleset requested (ruleset=%d)", ruleset);
	return FALSE;
    }

    if (!batchmode) {
	if (!loadgameresources(ruleset) || !creategamedisplay()) {
	    die("unable to proceed due to previous errors.");
	    return FALSE;
	}
    }

    logic->state = &state;
    return TRUE;
}

/* Initialize the current state to the starting position of the
 * given level.
 */
int initgamestate(gamesetup *game, int ruleset)
{
    if (!setrulesetbehavior(ruleset))
	die("unable to initialize the system for the requested ruleset");

    memset(state.map, 0, sizeof state.map);
    state.game = game;
    state.ruleset = ruleset;
    state.replay = -1;
    state.currenttime = -1;
    state.timeoffset = 0;
    state.currentinput = NIL;
    state.lastmove = NIL;
    state.initrndslidedir = NIL;
    state.stepping = -1;
    state.statusflags = 0;
    state.soundeffects = 0;
    state.timelimit = game->time * TICKS_PER_SECOND;
    initmovelist(&state.moves);
    resetprng(&state.mainprng);

    if (!expandleveldata(&state))
	return FALSE;

    return (*logic->initgame)(logic);
}

/* Change the current state to run from the recorded solution.
 */
int prepareplayback(void)
{
    solutioninfo	solution;

    if (!state.game->solutionsize)
	return FALSE;
    solution.moves.list = NULL;
    solution.moves.allocated = 0;
    if (!expandsolution(&solution, state.game) || !solution.moves.count)
	return FALSE;

    destroymovelist(&state.moves);
    state.moves = solution.moves;
    restartprng(&state.mainprng, solution.rndseed);
    state.initrndslidedir = solution.rndslidedir;
    state.stepping = solution.stepping;
    state.replay = 0;
    return TRUE;
}

/* Return the amount of time passed in the current game, in seconds.
 */
int secondsplayed(void)
{
    return (state.currenttime + state.timeoffset) / TICKS_PER_SECOND;
}

/* Change the system behavior according to the given gameplay mode.
 */
void setgameplaymode(int mode)
{
    switch (mode) {
      case BeginInput:
	setkeyboardinputmode(TRUE);
	break;
      case EndInput:
	setkeyboardinputmode(FALSE);
	break;
      case BeginPlay:
	setkeyboardrepeat(FALSE);
	settimer(+1);
	break;
      case EndPlay:
	setkeyboardrepeat(TRUE);
	settimer(-1);
	break;
      case BeginVerify:
	settimer(+1);
	break;
      case EndVerify:
	settimer(-1);
	break;
      case SuspendPlayShuttered:
	if (state.ruleset == Ruleset_MS)
	    state.statusflags |= SF_SHUTTERED;
      case SuspendPlay:
	setkeyboardrepeat(TRUE);
	settimer(0);
	setsoundeffects(0);
	break;
      case ResumePlay:
	setkeyboardrepeat(FALSE);
	settimer(+1);
	setsoundeffects(+1);
	state.statusflags &= ~SF_SHUTTERED;
	break;
    }
}

/* Alter the stepping. If display is true, update the screen to
 * reflect the change.
 */
int setstepping(int stepping, int display)
{
    char	msg[32], *p;

    state.stepping = stepping;
    if (display) {
	p = msg;
	p += sprintf(p, "%s-step", state.stepping & 4 ? "odd" : "even");
	if (state.stepping & 3)
	    p += sprintf(p, " +%d", state.stepping & 3);
	setdisplaymsg(msg, 500, 500);
    }
    return TRUE;
}

/* Alter the stepping by a delta. Force the stepping to be appropriate
 * to the current ruleset.
 */
int changestepping(int delta, int display)
{
    int	n;

    if (state.stepping < 0)
	state.stepping = 0;
    n = (state.stepping + delta) % 8;
    if (state.ruleset == Ruleset_MS)
	n &= ~3;
    if (state.stepping != n)
	return setstepping(n, display);
    return TRUE;
}

/* Advance the game one tick and update the game state. cmd is the
 * current keyboard command supplied by the user. The return value is
 * positive if the game was completed successfully, negative if the
 * game ended unsuccessfully, and zero otherwise.
 */
int doturn(int cmd)
{
    action	act;
    int		n;

    state.soundeffects &= ~((1 << SND_ONESHOT_COUNT) - 1);
    state.currenttime = gettickcount();
    if (state.currenttime >= MAXIMUM_TICK_COUNT) {
	errmsg(NULL, "timer reached its maximum of %d.%d hours; quitting now",
		     MAXIMUM_TICK_COUNT / (TICKS_PER_SECOND * 3600),
		     (MAXIMUM_TICK_COUNT / (TICKS_PER_SECOND * 360)) % 10);
	return -1;
    }
    if (state.replay < 0) {
	if (cmd != CmdPreserve)
	    state.currentinput = cmd;
    } else {
	if (state.replay < state.moves.count) {
	    if (state.currenttime > state.moves.list[state.replay].when)
		warn("Replay: Got ahead of saved solution: %d > %d!",
		     state.currenttime, state.moves.list[state.replay].when);
	    if (state.currenttime == state.moves.list[state.replay].when) {
		state.currentinput = state.moves.list[state.replay].dir;
		++state.replay;
	    }
	} else {
	    n = state.currenttime + state.timeoffset - 1;
	    if (n > state.game->besttime)
		return -1;
	}
    }

    n = (*logic->advancegame)(logic);

    if (state.replay < 0 && state.lastmove) {
	act.when = state.currenttime;
	act.dir = state.lastmove;
	addtomovelist(&state.moves, act);
	state.lastmove = NIL;
    }

    if (n)
	return n;

    return 0;
}

/* Update the display to show the current game state (including sound
 * effects, if any). If showframe is FALSE, then nothing is actually
 * displayed.
 */
int drawscreen(int showframe)
{
    int	currenttime;
    int timeleft, besttime;

    playsoundeffects(state.soundeffects);

    if (!showframe)
	return TRUE;

    currenttime = state.currenttime + state.timeoffset;
    if (hassolution(state.game))
	besttime = (state.game->time ? state.game->time : 999)
				- state.game->besttime / TICKS_PER_SECOND;
    else
	besttime = TIME_NIL;

    timeleft = TIME_NIL;
    if (state.game->time) {
	timeleft = state.game->time - currenttime / TICKS_PER_SECOND;
	if (timeleft <= 0) {
	    timeleft = 0;
	    setdisplaymsg("Out of time", 2, 2);
	}
    }

    return displaygame(&state, timeleft, besttime);
}

/* Stop game play and clean up.
 */
int quitgamestate(void)
{
    state.soundeffects = 0;
    setsoundeffects(-1);
    return TRUE;
}

/* Clean up after game play is over.
 */
int endgamestate(void)
{
    setsoundeffects(-1);
    return (*logic->endgame)(logic);
}

/* Close up shop.
 */
void shutdowngamestate(void)
{
    setrulesetbehavior(Ruleset_None);
    destroymovelist(&state.moves);
}

/* Initialize the current game state to a small level used for display
 * at the completion of a series.
 */
void setenddisplay(void)
{
    state.replay = -1;
    state.timelimit = 0;
    state.currenttime = -1;
    state.timeoffset = 0;
    state.chipsneeded = 0;
    state.currentinput = NIL;
    state.statusflags = 0;
    state.soundeffects = 0;
    getenddisplaysetup(&state);
    (*logic->initgame)(logic);
}

/*
 * Solution handling functions.
 */

/* Return TRUE if a solution exists for the given level.
 */
int hassolution(gamesetup const *game)
{
    return game->besttime != TIME_NIL;
}

/* Compare the most recent solution for the current game with the
 * user's best solution (if any). If this solution beats what's there,
 * or if the current solution has been marked as replaceable, then
 * replace it. TRUE is returned if the solution was replaced.
 */
int replacesolution(void)
{
    solutioninfo	solution;
    int			currenttime;

    if (state.statusflags & SF_NOSAVING)
	return FALSE;
    currenttime = state.currenttime + state.timeoffset;
    if (hassolution(state.game) && !(state.game->sgflags & SGF_REPLACEABLE)
				&& currenttime >= state.game->besttime)
	return FALSE;

    state.game->besttime = currenttime;
    state.game->sgflags &= ~SGF_REPLACEABLE;
    solution.moves = state.moves;
    solution.rndseed = getinitialseed(&state.mainprng);
    solution.flags = 0;
    solution.rndslidedir = state.initrndslidedir;
    solution.stepping = state.stepping;
    if (!contractsolution(&solution, state.game))
	return FALSE;

    return TRUE;
}

/* Delete the user's best solution for the current game. FALSE is
 * returned if no solution was present.
 */
int deletesolution(void)
{
    if (!hassolution(state.game))
	return FALSE;
    state.game->besttime = TIME_NIL;
    state.game->sgflags &= ~SGF_REPLACEABLE;
    free(state.game->solutiondata);
    state.game->solutionsize = 0;
    state.game->solutiondata = NULL;
    return TRUE;
}

/* Double-checks the timing for a solution that has just been played
 * back. If the timing is off, and the cause of the discrepancy can be
 * reasonably ascertained to be benign, the timing will be corrected
 * and TRUE is returned.
 */
int checksolution(void)
{
    int	currenttime;

    if (!hassolution(state.game))
	return FALSE;
    currenttime = state.currenttime + state.timeoffset;
    if (currenttime == state.game->besttime)
	return FALSE;
    warn("saved game has solution time of %d ticks, but replay took %d ticks",
	 state.game->besttime, currenttime);
    if (state.game->besttime == state.currenttime) {
	warn("difference matches clock offset; fixing.");
	state.game->besttime = currenttime;
	return TRUE;
    } else if (currenttime - state.game->besttime == 1) {
	warn("difference matches pre-0.10.1 error; fixing.");
	state.game->besttime = currenttime;
	return TRUE;
    }
    warn("reason for difference unknown.");
    state.game->besttime = currenttime;
    return FALSE;
}
