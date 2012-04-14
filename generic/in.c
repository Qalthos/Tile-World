/* in.c: Reading the keyboard and mouse.
 * 
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	<string.h>
#include	"generic.h"
#include	"../gen.h"
#include	"../oshw.h"
#include	"../defs.h"
#include	"../err.h"

/* Structure describing a mapping of a key event to a game command.
 */
typedef	struct keycmdmap {
    int		scancode;	/* the key's scan code */
    int		shift;		/* the shift key's state */
    int		ctl;		/* the ctrl key's state */
    int		alt;		/* the alt keys' state */
    int		cmd;		/* the command */
    int		hold;		/* TRUE for repeating joystick-mode keys */
} keycmdmap;

/* Structure describing mouse activity.
 */
typedef struct mouseaction {
    int		state;		/* state of mouse action (KS_*) */
    int		x, y;		/* position of the mouse */
    int		button;		/* which button generated the event */
} mouseaction;

/* The possible states of keys.
 */
enum { KS_OFF = 0,		/* key is not currently pressed */
       KS_ON = 1,		/* key is down (shift-type keys only) */
       KS_DOWN,			/* key is being held down */
       KS_STRUCK,		/* key was pressed and released in one tick */
       KS_PRESSED,		/* key was pressed in this tick */
       KS_DOWNBUTOFF1,		/* key has been down since the previous tick */
       KS_DOWNBUTOFF2,		/* key has been down since two ticks ago */
       KS_DOWNBUTOFF3,		/* key has been down since three ticks ago */
       KS_REPEATING,		/* key is down and is now repeating */
       KS_count
};

/* The complete array of key states.
 */
static char		keystates[TWK_LAST];

/* The last mouse action.
 */
static mouseaction	mouseinfo;

/* TRUE if direction keys are to be treated as always repeating.
 */
static int		joystickstyle = FALSE;

/* The complete list of key commands recognized by the game while
 * playing. hold is TRUE for keys that are to be forced to repeat.
 * shift, ctl and alt are positive if the key must be down, zero if
 * the key must be up, or negative if it doesn't matter.
 */
static keycmdmap const gamekeycmds[] = {        
    { TWK_UP,                     0,  0,  0,   CmdNorth,              TRUE },
    { TWK_LEFT,                   0,  0,  0,   CmdWest,               TRUE },
    { TWK_DOWN,                   0,  0,  0,   CmdSouth,              TRUE },
    { TWK_RIGHT,                  0,  0,  0,   CmdEast,               TRUE },
    { TWK_KP8,                    0,  0,  0,   CmdNorth,              TRUE },
    { TWK_KP4,                    0,  0,  0,   CmdWest,               TRUE },
    { TWK_KP2,                    0,  0,  0,   CmdSouth,              TRUE },
    { TWK_KP6,                    0,  0,  0,   CmdEast,               TRUE },
#ifdef __TWPLUSPLUS
    { TWK_ESCAPE,                 0,  0,  0,   CmdQuitLevel,          FALSE },
#else
    { 'q',                        0,  0,  0,   CmdQuitLevel,          FALSE },
#endif
    { 'p',                        0, +1,  0,   CmdPrevLevel,          FALSE },
    { 'r',                        0, +1,  0,   CmdSameLevel,          FALSE },
    { 'n',                        0, +1,  0,   CmdNextLevel,          FALSE },
    { 'g',                        0, -1,  0,   CmdGotoLevel,          FALSE },
#ifndef __TWPLUSPLUS
    { 'q',                       +1,  0,  0,   CmdQuit,               FALSE },
#endif
    { TWK_PAGEUP,                -1, -1,  0,   CmdPrev10,             FALSE },
    { 'p',                        0,  0,  0,   CmdPrev,               FALSE },
    { 'r',                        0,  0,  0,   CmdSame,               FALSE },
    { 'n',                        0,  0,  0,   CmdNext,               FALSE },
    { TWK_PAGEDOWN,              -1, -1,  0,   CmdNext10,             FALSE },
    { TWK_BACKSPACE,             -1, -1,  0,   CmdPauseGame,          FALSE },
/* TEMP disabling help */
#ifndef __TWPLUSPLUS
    { '?',                       -1, -1,  0,   CmdHelp,               FALSE },
    { TWK_F1,                    -1, -1,  0,   CmdHelp,               FALSE },
#endif
    { 'o',                        0,  0,  0,   CmdStepping,           FALSE },
    { 'o',                       +1,  0,  0,   CmdSubStepping,        FALSE },
    { TWK_TAB,                    0, -1,  0,   CmdPlayback,           FALSE },
    { TWK_TAB,                   +1, -1,  0,   CmdCheckSolution,      FALSE },
    { 'i',                        0, +1,  0,   CmdPlayback,           FALSE },
    { 'i',                       +1, +1,  0,   CmdCheckSolution,      FALSE },
    { 'x',                        0, +1,  0,   CmdReplSolution,       FALSE },
    { 'x',                       +1, +1,  0,   CmdKillSolution,       FALSE },
    { 's',                        0,  0,  0,   CmdSeeScores,          FALSE },
    { 's',                        0, +1,  0,   CmdSeeSolutionFiles,   FALSE },
    { 'v',                       +1,  0,  0,   CmdVolumeUp,           FALSE },
    { 'v',                        0,  0,  0,   CmdVolumeDown,         FALSE },
    { TWK_RETURN,                -1, -1,  0,   CmdProceed,            FALSE },
    { TWK_KP_ENTER,              -1, -1,  0,   CmdProceed,            FALSE },
    { ' ',                       -1, -1,  0,   CmdProceed,            FALSE },
    { 'd',                        0,  0,  0,   CmdDebugCmd1,          FALSE },
    { 'd',                       +1,  0,  0,   CmdDebugCmd2,          FALSE },
    { TWK_UP,                    +1,  0,  0,   CmdCheatNorth,         TRUE },
    { TWK_LEFT,                  +1,  0,  0,   CmdCheatWest,          TRUE },
    { TWK_DOWN,                  +1,  0,  0,   CmdCheatSouth,         TRUE },
    { TWK_RIGHT,                 +1,  0,  0,   CmdCheatEast,          TRUE },
    { TWK_HOME,                  +1,  0,  0,   CmdCheatHome,          FALSE },
    { TWK_F2,                     0,  0,  0,   CmdCheatICChip,        FALSE },
    { TWK_F3,                     0,  0,  0,   CmdCheatKeyRed,        FALSE },
    { TWK_F4,                     0,  0,  0,   CmdCheatKeyBlue,       FALSE },
    { TWK_F5,                     0,  0,  0,   CmdCheatKeyYellow,     FALSE },
    { TWK_F6,                     0,  0,  0,   CmdCheatKeyGreen,      FALSE },
    { TWK_F7,                     0,  0,  0,   CmdCheatBootsIce,      FALSE },
    { TWK_F8,                     0,  0,  0,   CmdCheatBootsSlide,    FALSE },
    { TWK_F9,                     0,  0,  0,   CmdCheatBootsFire,     FALSE },
    { TWK_F10,                    0,  0,  0,   CmdCheatBootsWater,    FALSE },
    { TWK_CTRL_C,                -1, -1,  0,   CmdQuit,               FALSE },
    { TWK_F4,                     0,  0, +1,   CmdQuit,               FALSE },
#ifdef __TWPLUSPLUS
/* "Virtual" keys */
    { TWC_SEESCORES,              0,  0,  0,   CmdSeeScores,          FALSE },
    { TWC_SEESOLUTIONFILES,       0,  0,  0,   CmdSeeSolutionFiles,   FALSE },
    { TWC_QUITLEVEL,              0,  0,  0,   CmdQuitLevel,          FALSE },
    { TWC_QUIT,                   0,  0,  0,   CmdQuit,               FALSE },

    { TWC_PROCEED,                0,  0,  0,   CmdProceed,            FALSE },
    { TWC_PAUSEGAME,              0,  0,  0,   CmdPauseGame,          FALSE },
    { TWC_SAMELEVEL,              0,  0,  0,   CmdSameLevel,          FALSE },
    { TWC_NEXTLEVEL,              0,  0,  0,   CmdNextLevel,          FALSE },
    { TWC_PREVLEVEL,              0,  0,  0,   CmdPrevLevel,          FALSE },
    { TWC_GOTOLEVEL,              0,  0,  0,   CmdGotoLevel,          FALSE },

    { TWC_PLAYBACK,               0,  0,  0,   CmdPlayback,           FALSE },
    { TWC_PLAYBACK,               0, +1,  0,   CmdCheckSolution,      FALSE },
    { TWC_CHECKSOLUTION,          0,  0,  0,   CmdCheckSolution,      FALSE },
    { TWC_REPLSOLUTION,           0,  0,  0,   CmdReplSolution,       FALSE },
    { TWC_KILLSOLUTION,           0,  0,  0,   CmdKillSolution,       FALSE },
    { TWC_SEEK,                   0,  0,  0,   CmdSeek,               FALSE },

    { TWC_HELP,                   0,  0,  0,   CmdHelp,               FALSE },
#endif
    { 0, 0, 0, 0, 0, 0 }
};

/* The list of key commands recognized when the program is obtaining
 * input from the user.
 */
static keycmdmap const inputkeycmds[] = {
    { TWK_UP,                    -1, -1,  0,   CmdNorth,              FALSE },
    { TWK_LEFT,                  -1, -1,  0,   CmdWest,               FALSE },
    { TWK_DOWN,                  -1, -1,  0,   CmdSouth,              FALSE },
    { TWK_RIGHT,                 -1, -1,  0,   CmdEast,               FALSE },
    { TWK_BACKSPACE,             -1, -1,  0,   CmdWest,               FALSE },
    { ' ',                       -1, -1,  0,   CmdEast,               FALSE },
    { TWK_RETURN,                -1, -1,  0,   CmdProceed,            FALSE },
    { TWK_KP_ENTER,              -1, -1,  0,   CmdProceed,            FALSE },
    { TWK_ESCAPE,                -1, -1,  0,   CmdQuitLevel,          FALSE },
    { 'a',                       -1,  0,  0,   'a',                   FALSE },
    { 'b',                       -1,  0,  0,   'b',                   FALSE },
    { 'c',                       -1,  0,  0,   'c',                   FALSE },
    { 'd',                       -1,  0,  0,   'd',                   FALSE },
    { 'e',                       -1,  0,  0,   'e',                   FALSE },
    { 'f',                       -1,  0,  0,   'f',                   FALSE },
    { 'g',                       -1,  0,  0,   'g',                   FALSE },
    { 'h',                       -1,  0,  0,   'h',                   FALSE },
    { 'i',                       -1,  0,  0,   'i',                   FALSE },
    { 'j',                       -1,  0,  0,   'j',                   FALSE },
    { 'k',                       -1,  0,  0,   'k',                   FALSE },
    { 'l',                       -1,  0,  0,   'l',                   FALSE },
    { 'm',                       -1,  0,  0,   'm',                   FALSE },
    { 'n',                       -1,  0,  0,   'n',                   FALSE },
    { 'o',                       -1,  0,  0,   'o',                   FALSE },
    { 'p',                       -1,  0,  0,   'p',                   FALSE },
    { 'q',                       -1,  0,  0,   'q',                   FALSE },
    { 'r',                       -1,  0,  0,   'r',                   FALSE },
    { 's',                       -1,  0,  0,   's',                   FALSE },
    { 't',                       -1,  0,  0,   't',                   FALSE },
    { 'u',                       -1,  0,  0,   'u',                   FALSE },
    { 'v',                       -1,  0,  0,   'v',                   FALSE },
    { 'w',                       -1,  0,  0,   'w',                   FALSE },
    { 'x',                       -1,  0,  0,   'x',                   FALSE },
    { 'y',                       -1,  0,  0,   'y',                   FALSE },
    { 'z',                       -1,  0,  0,   'z',                   FALSE },
    { TWK_CTRL_C,                -1, -1,  0,   CmdQuit,               FALSE },
    { TWK_F4,                     0,  0, +1,   CmdQuit,               FALSE },
    { 0, 0, 0, 0, 0, 0 }
};

/* The current map of key commands.
 */
static keycmdmap const *keycmds = gamekeycmds;

/* A map of keys that can be held down simultaneously to produce
 * multiple commands.
 */
static int mergeable[CmdKeyMoveLast + 1];

/*
 * Running the keyboard's state machine.
 */

/* This callback is called whenever the state of any keyboard key
 * changes. It records this change in the keystates array. The key can
 * be recorded as being struck, pressed, repeating, held down, or down
 * but ignored, as appropriate to when they were first pressed and the
 * current behavior settings. Shift-type keys are always either on or
 * off.
 */
static void _keyeventcallback(int scancode, int down)
{
    switch (scancode) {
      case TWK_LSHIFT:
      case TWK_RSHIFT:
      case TWK_LCTRL:
      case TWK_RCTRL:
      case TWK_LALT:
      case TWK_RALT:
      case TWK_LMETA:
      case TWK_RMETA:
      case TWK_NUMLOCK:
      case TWK_CAPSLOCK:
      case TWK_MODE:
	keystates[scancode] = down ? KS_ON : KS_OFF;
	break;
      default:
	if (scancode < TWK_LAST) {
	    if (down) {
		keystates[scancode] = keystates[scancode] == KS_OFF ?
						KS_PRESSED : KS_REPEATING;
	    } else {
		keystates[scancode] = keystates[scancode] == KS_PRESSED ?
						KS_STRUCK : KS_OFF;
	    }
	}
	break;
    }
}

/* Initialize (or re-initialize) all key states.
 */
static void restartkeystates(void)
{
    uint8_t    *keyboard;
    int		count, n;

    memset(keystates, KS_OFF, sizeof keystates);
    keyboard = TW_GetKeyState(&count);
    if (count > TWK_LAST)
	count = TWK_LAST;
    for (n = 0 ; n < count ; ++n)
	if (keyboard[n])
	    _keyeventcallback(n, TRUE);
}

/* Update the key states. This is done at the start of each polling
 * cycle. The state changes that occur depend on the current behavior
 * settings.
 */
static void resetkeystates(void)
{
    /* The transition table for keys in joystick behavior mode.
     */
    static char const joystick_trans[KS_count] = {
	/* KS_OFF         => */	KS_OFF,
	/* KS_ON          => */	KS_ON,
	/* KS_DOWN        => */	KS_DOWN,
	/* KS_STRUCK      => */	KS_OFF,
	/* KS_PRESSED     => */	KS_DOWN,
	/* KS_DOWNBUTOFF1 => */	KS_DOWN,
	/* KS_DOWNBUTOFF2 => */	KS_DOWN,
	/* KS_DOWNBUTOFF3 => */	KS_DOWN,
	/* KS_REPEATING   => */	KS_DOWN
    };
    /* The transition table for keys in keyboard behavior mode.
     */
    static char const keyboard_trans[KS_count] = {
	/* KS_OFF         => */	KS_OFF,
	/* KS_ON          => */	KS_ON,
	/* KS_DOWN        => */	KS_DOWN,
	/* KS_STRUCK      => */	KS_OFF,
	/* KS_PRESSED     => */	KS_DOWNBUTOFF1,
	/* KS_DOWNBUTOFF1 => */	KS_DOWNBUTOFF2,
	/* KS_DOWNBUTOFF2 => */	KS_DOWN,
	/* KS_DOWNBUTOFF3 => */	KS_DOWN,
	/* KS_REPEATING   => */	KS_DOWN
    };

    char const *newstate;
    int		n;

    newstate = joystickstyle ? joystick_trans : keyboard_trans;
    for (n = 0 ; n < TWK_LAST ; ++n)
	keystates[n] = newstate[(int)keystates[n]];
}

/*
 * Mouse event functions.
 */

/* This callback is called whenever there is a state change in the
 * mouse buttons. Up events are ignored. Down events are stored to
 * be examined later.
 */
static void _mouseeventcallback(int xpos, int ypos, int button, int down)
{
    if (down) {
	mouseinfo.state = KS_PRESSED;
	mouseinfo.x = xpos;
	mouseinfo.y = ypos;
	mouseinfo.button = button;
    }
}

/* Return the command appropriate to the most recent mouse activity.
 */
static int retrievemousecommand(void)
{
    int	n;

    switch (mouseinfo.state) {
      case KS_PRESSED:
	mouseinfo.state = KS_OFF;
	if (mouseinfo.button == TW_BUTTON_WHEELDOWN)
	    return CmdNext;
	if (mouseinfo.button == TW_BUTTON_WHEELUP)
	    return CmdPrev;
	if (mouseinfo.button == TW_BUTTON_LEFT) {
	    n = windowmappos(mouseinfo.x, mouseinfo.y);
	    if (n >= 0) {
		mouseinfo.state = KS_DOWNBUTOFF1;
		return CmdAbsMouseMoveFirst + n;
	    }
	}
	break;
      case KS_DOWNBUTOFF1:
	mouseinfo.state = KS_DOWNBUTOFF2;
	return CmdPreserve;
      case KS_DOWNBUTOFF2:
	mouseinfo.state = KS_DOWNBUTOFF3;
	return CmdPreserve;
      case KS_DOWNBUTOFF3:
	mouseinfo.state = KS_OFF;
	return CmdPreserve;
    }
    return 0;
}

/*
 * Exported functions.
 */

/* Wait for any non-shift key to be pressed down, ignoring any keys
 * that may be down at the time the function is called. Return FALSE
 * if the key pressed is suggestive of a desire to quit.
 */
int anykey(void)
{
    int	n;

    resetkeystates();
    eventupdate(FALSE);
    for (;;) {
	resetkeystates();
	eventupdate(TRUE);
	for (n = 0 ; n < TWK_LAST ; ++n)
	    if (keystates[n] == KS_STRUCK || keystates[n] == KS_PRESSED
					  || keystates[n] == KS_REPEATING)
		return n != 'q' && n != TWK_ESCAPE;
    }
}

/* Poll the keyboard and return the command associated with the
 * selected key, if any. If no key is selected and wait is TRUE, block
 * until a key with an associated command is selected. In keyboard behavior
 * mode, the function can return CmdPreserve, indicating that if the key
 * command from the previous poll has not been processed, it should still
 * be considered active. If two mergeable keys are selected, the return
 * value will be the bitwise-or of their command values.
 */
int input(int wait)
{
    keycmdmap const    *kc;
    int			lingerflag = FALSE;
    int			cmd1, cmd, n;

    for (;;) {
	resetkeystates();
	eventupdate(wait);

	cmd1 = cmd = 0;
	for (kc = keycmds ; kc->scancode ; ++kc) {
	    n = keystates[kc->scancode];
	    if (!n)
		continue;
	    if (kc->shift != -1)
		if (kc->shift !=
			(keystates[TWK_LSHIFT] || keystates[TWK_RSHIFT]))
		    continue;
	    if (kc->ctl != -1)
		if (kc->ctl !=
			(keystates[TWK_LCTRL] || keystates[TWK_RCTRL]))
		    continue;
	    if (kc->alt != -1)
		if (kc->alt != (keystates[TWK_LALT] || keystates[TWK_RALT]))
		    continue;

	    if (n == KS_PRESSED || (kc->hold && n == KS_DOWN)) {
		if (!cmd1) {
		    cmd1 = kc->cmd;
		    if (!joystickstyle || cmd1 > CmdKeyMoveLast
				       || !mergeable[cmd1])
			return cmd1;
		} else {
		    if (cmd1 <= CmdKeyMoveLast
				&& (mergeable[cmd1] & kc->cmd) == kc->cmd)
			return cmd1 | kc->cmd;
		}
	    } else if (n == KS_STRUCK || n == KS_REPEATING) {
		cmd = kc->cmd;
	    } else if (n == KS_DOWNBUTOFF1 || n == KS_DOWNBUTOFF2) {
		lingerflag = TRUE;
	    }
	}
	if (cmd1)
	    return cmd1;
	if (cmd)
	    return cmd;
	cmd = retrievemousecommand();
	if (cmd)
	    return cmd;
	if (!wait)
	    break;
    }
    if (!cmd && lingerflag)
	cmd = CmdPreserve;
    return cmd;
}

/* Turn joystick behavior mode on or off. In joystick-behavior mode,
 * the arrow keys are always returned from input() if they are down at
 * the time of the polling cycle. Other keys are only returned if they
 * are pressed during a polling cycle (or if they repeat, if keyboard
 * repeating is on). In keyboard-behavior mode, the arrow keys have a
 * special repeating behavior that is kept synchronized with the
 * polling cycle.
 */
int setkeyboardarrowsrepeat(int enable)
{
    joystickstyle = enable;
    restartkeystates();
    return TRUE;
}

/* Turn input mode on or off. When input mode is on, the input key
 * command map is used instead of the game key command map.
 */
int setkeyboardinputmode(int enable)
{
    keycmds = enable ? inputkeycmds : gamekeycmds;
    return TRUE;
}

/* Given a pixel's coordinates, return the integer identifying the
 * tile's position in the map, or -1 if the pixel is not on the map view.
 */
static int _windowmappos(int x, int y)
{
    if (geng.mapvieworigin < 0)
	return -1;
    if (x < geng.maploc.x || y < geng.maploc.y)
	return -1;
    x = (x - geng.maploc.x) * 4 / geng.wtile;
    y = (y - geng.maploc.y) * 4 / geng.htile;
    if (x >= NXTILES * 4 || y >= NYTILES * 4)
	return -1;
    x = (x + geng.mapvieworigin % (CXGRID * 4)) / 4;
    y = (y + geng.mapvieworigin / (CXGRID * 4)) / 4;
    if (x < 0 || x >= CXGRID || y < 0 || y >= CYGRID) {
	warn("mouse moved off the map: (%d %d)", x, y);
	return -1;
    }	    
    return y * CXGRID + x;
}

/* Initialization.
 */
int _genericinputinitialize(void)
{
    geng.keyeventcallbackfunc = _keyeventcallback;
    geng.mouseeventcallbackfunc = _mouseeventcallback;
    geng.windowmapposfunc = _windowmappos;

    mergeable[CmdNorth] = mergeable[CmdSouth] = CmdWest | CmdEast;
    mergeable[CmdWest] = mergeable[CmdEast] = CmdNorth | CmdSouth;

    setkeyboardrepeat(TRUE);
    return TRUE;
}

/* Online help texts for the keyboard commands.
 */
tablespec const *keyboardhelp(int which)
{
    static char *ingame_items[] = {
	"1-arrows", "1-move Chip",
	"1-2 4 6 8 (keypad)", "1-also move Chip",
	"1-Q", "1-quit the current game",
	"1-Bkspc", "1-pause the game",
	"1-Ctrl-R", "1-restart the current level",
	"1-Ctrl-P", "1-jump to the previous level",
	"1-Ctrl-N", "1-jump to the next level",
	"1-V", "1-decrease volume",
	"1-Shift-V", "1-increase volume",
	"1-Ctrl-C", "1-exit the program",
	"1-Alt-F4", "1-exit the program"
    };
    static tablespec const keyhelp_ingame = { 11, 2, 4, 1, ingame_items };

    static char *twixtgame_items[] = {
	"1-P", "1-jump to the previous level",
	"1-N", "1-jump to the next level",
	"1-PgUp", "1-skip back ten levels",
	"1-PgDn", "1-skip ahead ten levels",
	"1-G", "1-go to a level using a password",
	"1-S", "1-see the scores for each level",
	"1-Tab", "1-playback saved solution",
	"1-Shift-Tab", "1-verify saved solution",
	"1-Ctrl-X", "1-replace existing solution",
	"1-Shift-Ctrl-X", "1-delete existing solution",
	"1-Ctrl-S", "1-see the available solution files",
	"1-O", "1-toggle between even-step and odd-step offset",
	"1-Shift-O", "1-increment stepping offset (Lynx only)",
	"1-V", "1-decrease volume",
	"1-Shift-V", "1-increase volume",
	"1-Q", "1-return to the file list",
	"1-Ctrl-C", "1-exit the program",
	"1-Alt-F4", "1-exit the program"
    };
    static tablespec const keyhelp_twixtgame = { 18, 2, 4, 1,
						 twixtgame_items };

    static char *scorelist_items[] = {
	"1-up down", "1-move selection",
	"1-PgUp PgDn", "1-scroll selection",
	"1-Enter Space", "1-select level",
	"1-Ctrl-S", "1-change solution file",
	"1-Q", "1-return to the last level",
	"1-Ctrl-C", "1-exit the program",
	"1-Alt-F4", "1-exit the program"
    };
    static tablespec const keyhelp_scorelist = { 7, 2, 4, 1, scorelist_items };

    static char *scroll_items[] = {
	"1-up down", "1-move selection",
	"1-PgUp PgDn", "1-scroll selection",
	"1-Enter Space", "1-select",
	"1-Q", "1-cancel",
	"1-Ctrl-C", "1-exit the program",
	"1-Alt-F4", "1-exit the program"
    };
    static tablespec const keyhelp_scroll = { 6, 2, 4, 1, scroll_items };

    switch (which) {
      case KEYHELP_INGAME:	return &keyhelp_ingame;
      case KEYHELP_TWIXTGAMES:	return &keyhelp_twixtgame;
      case KEYHELP_SCORELIST:	return &keyhelp_scorelist;
      case KEYHELP_FILELIST:	return &keyhelp_scroll;
    }

    return NULL;
}
