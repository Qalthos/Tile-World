/* res.c: Functions for loading resources from external files.
 *
 * Copyright (C) 2001-2014 by Brian Raiter and Eric Schmidt, under the GNU
 * General Public License. No warranty. See COPYING for details.
 */

#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"defs.h"
#include	"fileio.h"
#include	"err.h"
#include	"oshw.h"
#include	"messages.h"
#include	"unslist.h"
#include	"res.h"

/*
 * The resource ID numbers
 */

#define	RES_IMG_BASE		0
#define	RES_IMG_TILES		(RES_IMG_BASE + 0)
#define	RES_IMG_FONT		(RES_IMG_BASE + 1)
#define	RES_IMG_LAST		RES_IMG_FONT

#define	RES_CLR_BASE		(RES_IMG_LAST + 1)
#define	RES_CLR_BKGND		(RES_CLR_BASE + 0)
#define	RES_CLR_TEXT		(RES_CLR_BASE + 1)
#define	RES_CLR_BOLD		(RES_CLR_BASE + 2)
#define	RES_CLR_DIM		(RES_CLR_BASE + 3)
#define	RES_CLR_LAST		RES_CLR_DIM

#define	RES_TXT_BASE		(RES_CLR_LAST + 1)
#define	RES_TXT_UNSLIST		(RES_TXT_BASE + 0)
#define RES_TXT_MESSAGE		(RES_TXT_BASE + 1)
#define	RES_TXT_LAST		RES_TXT_MESSAGE

#define	RES_SND_BASE		(RES_TXT_LAST + 1)
#define	RES_SND_CHIP_LOSES	(RES_SND_BASE + SND_CHIP_LOSES)
#define	RES_SND_CHIP_WINS	(RES_SND_BASE + SND_CHIP_WINS)
#define	RES_SND_TIME_OUT	(RES_SND_BASE + SND_TIME_OUT)
#define	RES_SND_TIME_LOW	(RES_SND_BASE + SND_TIME_LOW)
#define	RES_SND_DEREZZ		(RES_SND_BASE + SND_DEREZZ)
#define	RES_SND_CANT_MOVE	(RES_SND_BASE + SND_CANT_MOVE)
#define	RES_SND_IC_COLLECTED	(RES_SND_BASE + SND_IC_COLLECTED)
#define	RES_SND_ITEM_COLLECTED	(RES_SND_BASE + SND_ITEM_COLLECTED)
#define	RES_SND_BOOTS_STOLEN	(RES_SND_BASE + SND_BOOTS_STOLEN)
#define	RES_SND_TELEPORTING	(RES_SND_BASE + SND_TELEPORTING)
#define	RES_SND_DOOR_OPENED	(RES_SND_BASE + SND_DOOR_OPENED)
#define	RES_SND_SOCKET_OPENED	(RES_SND_BASE + SND_SOCKET_OPENED)
#define	RES_SND_BUTTON_PUSHED	(RES_SND_BASE + SND_BUTTON_PUSHED)
#define	RES_SND_TILE_EMPTIED	(RES_SND_BASE + SND_TILE_EMPTIED)
#define	RES_SND_WALL_CREATED	(RES_SND_BASE + SND_WALL_CREATED)
#define	RES_SND_TRAP_ENTERED	(RES_SND_BASE + SND_TRAP_ENTERED)
#define	RES_SND_BOMB_EXPLODES	(RES_SND_BASE + SND_BOMB_EXPLODES)
#define	RES_SND_WATER_SPLASH	(RES_SND_BASE + SND_WATER_SPLASH)
#define	RES_SND_SKATING_TURN	(RES_SND_BASE + SND_SKATING_TURN)
#define	RES_SND_BLOCK_MOVING	(RES_SND_BASE + SND_BLOCK_MOVING)
#define	RES_SND_SKATING_FORWARD	(RES_SND_BASE + SND_SKATING_FORWARD)
#define	RES_SND_SLIDING		(RES_SND_BASE + SND_SLIDING)
#define	RES_SND_SLIDEWALKING	(RES_SND_BASE + SND_SLIDEWALKING)
#define	RES_SND_ICEWALKING	(RES_SND_BASE + SND_ICEWALKING)
#define	RES_SND_WATERWALKING	(RES_SND_BASE + SND_WATERWALKING)
#define	RES_SND_FIREWALKING	(RES_SND_BASE + SND_FIREWALKING)
#define	RES_SND_LAST		RES_SND_FIREWALKING

#define	RES_COUNT		(RES_SND_LAST + 1)

/* Structure for enumerating the resource names.
 */
typedef	struct rcitem {
    char const *name;
    int		numeric;
} rcitem;

/* Union for storing the resource values.
 */
typedef union resourceitem {
    int		num;
    char	str[256];
} resourceitem;

/* The complete list of resource names.
 */
static rcitem rclist[RES_COUNT] = {
    { "tileimages",		FALSE },
    { "font",			FALSE },
    { "backgroundcolor",	FALSE },
    { "textcolor",		FALSE },
    { "boldtextcolor",		FALSE },
    { "dimtextcolor",		FALSE },
    { "unsolvablelist",		FALSE },
    { "endmessages",		FALSE },
    { "chipdeathsound",		FALSE },
    { "levelcompletesound",	FALSE },
    { "chipdeathbytimesound",	FALSE },
    { "ticksound",		FALSE },
    { "derezzsound",		FALSE },
    { "blockedmovesound",	FALSE },
    { "pickupchipsound",	FALSE },
    { "pickuptoolsound",	FALSE },
    { "thiefsound",		FALSE },
    { "teleportsound",		FALSE },
    { "opendoorsound",		FALSE },
    { "socketsound",		FALSE },
    { "switchsound",		FALSE },
    { "tileemptiedsound",	FALSE },
    { "wallcreatedsound",	FALSE },
    { "trapenteredsound",	FALSE },
    { "bombsound",		FALSE },
    { "splashsound",		FALSE },
    { "blockmovingsound",	FALSE },
    { "skatingforwardsound",	FALSE },
    { "skatingturnsound",	FALSE },
    { "slidingsound",		FALSE },
    { "slidewalkingsound",	FALSE },
    { "icewalkingsound",	FALSE },
    { "waterwalkingsound",	FALSE },
    { "firewalkingsound",	FALSE }
};

/* The complete collection of resource values.
 */
static resourceitem	allresources[Ruleset_Count][RES_COUNT];

/* The resource values for the current ruleset.
 */
static resourceitem    *resources = NULL;

/* The ruleset-independent resource values.
 */
static resourceitem    *globalresources = allresources[Ruleset_None];

/* The active ruleset.
 */
static int		currentruleset = Ruleset_None;

/* The directory containing all the resource files.
 */
char		       *resdir = NULL;

/* A few resources have non-empty default values.
 */
static void initresourcedefaults(void)
{
    strcpy(allresources[Ruleset_None][RES_IMG_TILES].str, "tiles.bmp");
    strcpy(allresources[Ruleset_None][RES_IMG_FONT].str, "font.bmp");
    strcpy(allresources[Ruleset_None][RES_CLR_BKGND].str, "000000");
    strcpy(allresources[Ruleset_None][RES_CLR_TEXT].str, "FFFFFF");
    strcpy(allresources[Ruleset_None][RES_CLR_BOLD].str, "FFFF00");
    strcpy(allresources[Ruleset_None][RES_CLR_DIM].str, "C0C0C0");
    memcpy(&allresources[Ruleset_MS], globalresources,
				sizeof allresources[Ruleset_MS]);
    memcpy(&allresources[Ruleset_Lynx], globalresources,
				sizeof allresources[Ruleset_Lynx]);
}

/* Iterate through the lines of the rc file, storing the values in the
 * allresources array. Lines consisting only of whitespace, or with an
 * octothorpe as the first non-whitespace character, are skipped over.
 * Lines containing a ruleset in brackets introduce ruleset-specific
 * resource values. Ruleset-independent values are copied into each of
 * the ruleset-specific entries. FALSE is returned if the rc file
 * could not be opened.
 */
static int readrcfile(void)
{
    resourceitem	item;
    fileinfo		file = {0};
    char		buf[256];
    char		name[256];
    char	       *p;
    int			ruleset;
    int			lineno, i, j;

    if (!openfileindir(&file, resdir, "rc", "r", "can't open"))
	return FALSE;

    ruleset = Ruleset_None;
    for (lineno = 1 ; ; ++lineno) {
	i = sizeof buf - 1;
	if (!filegetline(&file, buf, &i, NULL))
	    break;
	for (p = buf ; isspace(*p) ; ++p) ;
	if (!*p || *p == '#')
	    continue;
	if (sscanf(buf, "[%[^]]]", name) == 1) {
	    for (p = name ; (*p = tolower(*p)) != '\0' ; ++p) ;
	    if (!strcmp(name, "ms"))
		ruleset = Ruleset_MS;
	    else if (!strcmp(name, "lynx"))
		ruleset = Ruleset_Lynx;
	    else if (!strcmp(name, "all"))
		ruleset = Ruleset_None;
	    else
		warn("rc:%d: syntax error", lineno);
	    continue;
	}
	if (sscanf(buf, "%[^=]=%s", name, item.str) != 2) {
	    warn("rc:%d: syntax error", lineno);
	    continue;
	}
	for (p = name ; (*p = tolower(*p)) != '\0' ; ++p) ;
	for (i = sizeof rclist / sizeof *rclist - 1 ; i >= 0 ; --i)
	    if (!strcmp(name, rclist[i].name))
		break;
	if (i < 0) {
	    warn("rc:%d: illegal resource name \"%s\"", lineno, name);
	    continue;
	}
	if (rclist[i].numeric) {
	    i = atoi(item.str);
	    item.num = i;
	}
	allresources[ruleset][i] = item;
	if (ruleset == Ruleset_None)
	    for (j = Ruleset_None ; j < Ruleset_Count ; ++j)
		allresources[j][i] = item;
    }

    fileclose(&file, NULL);
    return TRUE;
}

/*
 * Resource-loading functions
 */

/* Parse the color-definition resource values.
 */
static int loadcolors(void)
{
    long	bkgnd, text, bold, dim;
    char       *end;

    bkgnd = strtol(resources[RES_CLR_BKGND].str, &end, 16);
    if (*end || bkgnd < 0 || bkgnd > 0xFFFFFF) {
	warn("rc: invalid color ID for background");
	bkgnd = -1;
    }
    text = strtol(resources[RES_CLR_TEXT].str, &end, 16);
    if (*end || text < 0 || text > 0xFFFFFF) {
	warn("rc: invalid color ID for text");
	text = -1;
    }
    bold = strtol(resources[RES_CLR_BOLD].str, &end, 16);
    if (*end || bold < 0 || bold > 0xFFFFFF) {
	warn("rc: invalid color ID for bold text");
	bold = -1;
    }
    dim = strtol(resources[RES_CLR_DIM].str, &end, 16);
    if (*end || dim < 0 || dim > 0xFFFFFF) {
	warn("rc: invalid color ID for dim text");
	dim = -1;
    }

    setcolors(bkgnd, text, bold, dim);
    return TRUE;
}

/* Attempt to load the tile images.
 */
static int loadimages(void)
{
    char       *path;
    int		f;

    f = FALSE;
    path = getpathbuffer();
    if (*resources[RES_IMG_TILES].str) {
	combinepath(path, resdir, resources[RES_IMG_TILES].str);
	f = loadtileset(path, TRUE);
    }
    if (!f && resources != globalresources
	   && *globalresources[RES_IMG_TILES].str) {
	combinepath(path, resdir, globalresources[RES_IMG_TILES].str);
	f = loadtileset(path, TRUE);
    }
    free(path);

    if (!f)
	errmsg(resdir, "no valid tilesets found");
    return f;
}

/* Load the font resource.
 */
static int loadfont(void)
{
    char       *path;
    int		f;

    f = FALSE;
    path = getpathbuffer();
    if (*resources[RES_IMG_FONT].str) {
	combinepath(path, resdir, resources[RES_IMG_FONT].str);
	f = loadfontfromfile(path, TRUE);
    }
    if (!f && resources != globalresources
	   && *globalresources[RES_IMG_FONT].str) {
	combinepath(path, resdir, globalresources[RES_IMG_FONT].str);
	f = loadfontfromfile(path, TRUE);
    }
    free(path);

    if (!f)
	errmsg(resdir, "no valid font found");
    return f;
}

/* Load the list of unsolvable levels.
 */
typedef int (*txtloader)(char const * fname);
static int loadtxtresource(int resid, txtloader loadfunc)
{
    char const *filename;

    if (*resources[resid].str)
	filename = resources[resid].str;
    else if (resources != globalresources
			&& *globalresources[resid].str)
	filename = globalresources[resid].str;
    else
	return FALSE;

    return loadfunc(filename);
}

/* Load all of the sound resources.
 */
static int loadsounds(void)
{
    char       *path;
    int		count;
    int		n, f;

    path = getpathbuffer();
    count = 0;
    for (n = 0 ; n < SND_COUNT ; ++n) {
	f = FALSE;
	if (*resources[RES_SND_BASE + n].str) {
	    combinepath(path, resdir, resources[RES_SND_BASE + n].str);
	    f = loadsfxfromfile(n, path);
	}
	if (!f && resources != globalresources
	       && *globalresources[RES_SND_BASE + n].str) {
	    combinepath(path, resdir, globalresources[RES_SND_BASE + n].str);
	    f = loadsfxfromfile(n, path);
	}
	if (f)
	    ++count;
    }
    free(path);
    return count;
}

/* Load all resources that are available. FALSE is returned if the
 * tile images could not be loaded. (Sounds are not required in order
 * to run, and by this point we should already have a valid font and
 * color scheme set.)
 */
int loadgameresources(int ruleset)
{
    currentruleset = ruleset;
    resources = allresources[ruleset];
    loadcolors();
    loadfont();
    if (!loadimages())
	return FALSE;
    if (loadsounds() == 0)
	setaudiosystem(FALSE);
    return TRUE;
}

/* Parse the rc file and load the font and color scheme. FALSE is returned
 * if an error occurs.
 */
int initresources(void)
{
    initresourcedefaults();
    resources = allresources[Ruleset_None];
    if (!readrcfile() || !loadcolors() || !loadfont())
	return FALSE;
    loadtxtresource(RES_TXT_UNSLIST, loadunslistfromfile);
    loadtxtresource(RES_TXT_MESSAGE, loadmessagesfromfile);
    return TRUE;
}

/* Free all resources.
 */
void freeallresources(void)
{
    int	n;
    freefont();
    freetileset();
    clearunslist();
    for (n = 0 ; n < SND_COUNT ; ++n)
	 freesfx(n);
}
