/* state.h: Definitions for embodying the state of a game in progress.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_state_h_
#define	_state_h_

#include	"defs.h"

/* The tiles that make up Chip's universe.
 */
enum
{
    Nothing		= 0,

    Empty		= 0x01,

    Slide_North		= 0x02,
    Slide_West		= 0x03,
    Slide_South		= 0x04,
    Slide_East		= 0x05,
    Slide_Random	= 0x06,
    Ice			= 0x07,
    IceWall_Northwest	= 0x08,
    IceWall_Northeast	= 0x09,
    IceWall_Southwest	= 0x0A,
    IceWall_Southeast	= 0x0B,
    Gravel		= 0x0C,
    Dirt		= 0x0D,
    Water		= 0x0E,
    Fire		= 0x0F,
    Bomb		= 0x10,
    Beartrap		= 0x11,
    Burglar		= 0x12,
    HintButton		= 0x13,

    Button_Blue		= 0x14,
    Button_Green	= 0x15,
    Button_Red		= 0x16,
    Button_Brown	= 0x17,
    Teleport		= 0x18,

    Wall		= 0x19,
    Wall_North		= 0x1A,
    Wall_West		= 0x1B,
    Wall_South		= 0x1C,
    Wall_East		= 0x1D,
    Wall_Southeast	= 0x1E,
    HiddenWall_Perm	= 0x1F,
    HiddenWall_Temp	= 0x20,
    BlueWall_Real	= 0x21,
    BlueWall_Fake	= 0x22,
    SwitchWall_Open	= 0x23,
    SwitchWall_Closed	= 0x24,
    PopupWall		= 0x25,

    CloneMachine	= 0x26,

    Door_Red		= 0x27,
    Door_Blue		= 0x28,
    Door_Yellow		= 0x29,
    Door_Green		= 0x2A,
    Socket		= 0x2B,
    Exit		= 0x2C,

    ICChip		= 0x2D,
    Key_Red		= 0x2E,
    Key_Blue		= 0x2F,
    Key_Yellow		= 0x30,
    Key_Green		= 0x31,
    Boots_Ice		= 0x32,
    Boots_Slide		= 0x33,
    Boots_Fire		= 0x34,
    Boots_Water		= 0x35,

    Block_Static	= 0x36,

    Drowned_Chip	= 0x37,
    Burned_Chip		= 0x38,
    Bombed_Chip		= 0x39,
    Exited_Chip		= 0x3A,
    Exit_Extra_1	= 0x3B,
    Exit_Extra_2	= 0x3C,

    Overlay_Buffer	= 0x3D,

    Floor_Reserved2	= 0x3E,
    Floor_Reserved1	= 0x3F,

    Chip		= 0x40,

    Block		= 0x44,

    Tank		= 0x48,
    Ball		= 0x4C,
    Glider		= 0x50,
    Fireball		= 0x54,
    Walker		= 0x58,
    Blob		= 0x5C,
    Teeth		= 0x60,
    Bug			= 0x64,
    Paramecium		= 0x68,

    Swimming_Chip	= 0x6C,
    Pushing_Chip	= 0x70,

    Entity_Reserved2	= 0x74,
    Entity_Reserved1	= 0x78,

    Water_Splash	= 0x7C,
    Bomb_Explosion	= 0x7D,
    Entity_Explosion	= 0x7E,
    Animation_Reserved1	= 0x7F
};

/* Macros to assist in identifying tile taxons.
 */
#define	isslide(f)	((f) >= Slide_North && (f) <= Slide_Random)
#define	isice(f)	((f) >= Ice && (f) <= IceWall_Southeast)
#define	isdoor(f)	((f) >= Door_Red && (f) <= Door_Green)
#define	iskey(f)	((f) >= Key_Red && (f) <= Key_Green)
#define	isboots(f)	((f) >= Boots_Ice && (f) <= Boots_Water)
#define	ismsspecial(f)	((f) >= Drowned_Chip && (f) <= Overlay_Buffer)
#define	isfloor(f)	((f) <= Floor_Reserved1)
#define	iscreature(f)	((f) >= Chip && (f) < Water_Splash)
#define	isanimation(f)	((f) >= Water_Splash && (f) <= Animation_Reserved1)

/* Macro for getting the tile ID of a creature with a specific direction.
 */
#define	crtile(id, dir)	((id) | diridx(dir))

/* Macros for decomposing a creature tile into ID and direction.
 */
#define	creatureid(id)		((id) & ~3)
#define	creaturedirid(id)	(idxdir((id) & 3))

/*
 * Substructures of the game state
 */

/* Two x,y-coordinates give the locations of a button and what it is
 * connected to.
 */
typedef	struct xyconn {
    short		from;		/* location of the button */
    short		to;		/* location of the trap/cloner */
} xyconn;

/* A tile on the map.
 */
typedef struct maptile {
    unsigned char	id;		/* identity of the tile */
    unsigned char	state;		/* internal state flags */
} maptile;

/* A location on the map.
 */
typedef	struct mapcell {
    maptile		top;		/* the upper tile */
    maptile		bot;		/* the lower tile */
} mapcell;

/* A creature.
 */
#if 0
typedef	struct creature {
    signed int		pos   : 11;	/* creature's location */
    signed int		dir   : 5;	/* current direction of creature */
    signed int		id    : 8;	/* type of creature */
    signed int		state : 8;	/* internal state value */
    signed int		hidden: 1;	/* TRUE if creature is invisible */
    signed int		moving: 5;	/* positional offset of creature */
    signed int		frame : 5;	/* explicit animation index */
    signed int		tdir  : 5;	/* internal state value */
} creature;
#else
typedef struct creature {
    short		pos;		/* creature's location */
    unsigned char	id;		/* type of creature */
    unsigned char	dir;		/* current direction of creature */
    signed char		moving;		/* positional offset of creature */
    signed char		frame;		/* explicit animation index */
    unsigned char	hidden;		/* TRUE if creature is invisible */
    unsigned char	state;		/* internal state value */
    unsigned char	tdir;		/* internal state value */
} creature;
#endif

/*
 * The game state structure proper.
 */

/* Ideally, everything that the gameplay module, the display module,
 * and both logic modules need to know about a game in progress is
 * in here.
 */
typedef struct gamestate {
    gamesetup	       *game;			/* the level specification */
    int			ruleset;		/* the ruleset for the game */
    int			replay;			/* playback move index */
    int			timelimit;		/* maximum time permitted */
    int			currenttime;		/* the current tick count */
    int			timeoffset;		/* offset for displayed time */
    short		currentinput;		/* the current keystroke */
    short		chipsneeded;		/* no. of chips still needed */
    short		xviewpos;		/* the visible part of the */
    short		yviewpos;		/*   map (ie, where Chip is) */
    short		keys[4];		/* keys collected */
    short		boots[4];		/* boots collected */
    short		statusflags;		/* flags (see below) */
    short		lastmove;		/* most recent move */
    unsigned char	initrndslidedir;	/* initial random-slide dir */
    signed char		stepping;		/* initial timer offset 0-7 */
    unsigned long	soundeffects;		/* the latest sound effects */
    actlist		moves;			/* the list of moves */
    prng		mainprng;		/* the main PRNG */
    creature	       *creatures;		/* the creature list */
    short		trapcount;		/* number of trap buttons */
    short		clonercount;		/* number of cloner buttons */
    short		crlistcount;		/* number of creatures */
    xyconn		traps[256];		/* list of trap wirings */
    xyconn		cloners[256];		/* list of cloner wirings */
    short		crlist[256];		/* list of creatures */
    char		hinttext[256];		/* text of the hint */
    mapcell		map[CXGRID * CYGRID];	/* the game's map */
    unsigned char	localstateinfo[256];	/* rule-specific state data */
} gamestate;

/* General status flags.
 */
#define	SF_NOSAVING		0x0001		/* solution won't be saved */
#define	SF_INVALID		0x0002		/* level is not playable */
#define	SF_BADTILES		0x0004		/* map has undefined tiles */
#define	SF_SHOWHINT		0x0008		/* display the hint text */
#define	SF_NOANIMATION		0x0010		/* suppress tile animation */
#define	SF_SHUTTERED		0x0020		/* hide map view */

/* Macros for the keys and boots.
 */
#define	redkeys(st)		((st)->keys[0])
#define	bluekeys(st)		((st)->keys[1])
#define	yellowkeys(st)		((st)->keys[2])
#define	greenkeys(st)		((st)->keys[3])
#define	iceboots(st)		((st)->boots[0])
#define	slideboots(st)		((st)->boots[1])
#define	fireboots(st)		((st)->boots[2])
#define	waterboots(st)		((st)->boots[3])

#endif
