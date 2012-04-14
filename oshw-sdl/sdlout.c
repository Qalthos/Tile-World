/* sdlout.c: Creating the program's displays.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"SDL.h"
#include	"sdlgen.h"
#include	"../err.h"
#include	"../state.h"

/* Space to leave between graphic objects.
 */
#define	MARGINW		8
#define	MARGINH		8

/* Size of the prompt icons.
 */
#define	PROMPTICONW	16
#define	PROMPTICONH	10

/* Erase a rectangle (useful for when a surface is locked).
 */
#define	fillrect(r)		(puttext((r), NULL, 0, PT_MULTILINE))

/* Structure for holding information about the message display.
 */
typedef	struct msgdisplayinfo {
    char		msg[64];	/* text of the message */
    unsigned int	msglen;		/* length of the message */
    unsigned long	until;		/* when to erase the message */
    unsigned long	bolduntil;	/* when to dim the message */
} msgdisplayinfo;

/* The message display.
 */
static msgdisplayinfo	msgdisplay;

/* Some prompting icons.
 */
static SDL_Surface     *prompticons = NULL;

/* TRUE means the program should attempt to run in fullscreen mode.
 */
static int		fullscreen = FALSE;

/* Coordinates specifying the placement of the various screen elements.
 */
static int		screenw, screenh;
static SDL_Rect		rinfoloc;
static SDL_Rect		locrects[8];

#define	displayloc	(locrects[0])
#define	titleloc	(locrects[1])
#define	infoloc		(locrects[2])
#define	invloc		(locrects[3])
#define	hintloc		(locrects[4])
#define	rscoreloc	(locrects[5])
#define	messageloc	(locrects[6])
#define	promptloc	(locrects[7])

/* TRUE means that the screen is in need of a full update.
 */
static int		fullredraw = TRUE;

/*
 * Display initialization functions.
 */

/* Set up a fontcolors structure, calculating the middle color from
 * the other two.
 */
static fontcolors makefontcolors(int rbkgnd, int gbkgnd, int bbkgnd,
				 int rtext, int gtext, int btext)
{
    fontcolors	colors;

    colors.c[0] = SDL_MapRGB(geng.screen->format, rbkgnd, gbkgnd, bbkgnd);
    colors.c[2] = SDL_MapRGB(geng.screen->format, rtext, gtext, btext);
    colors.c[1] = SDL_MapRGB(geng.screen->format, (rbkgnd + rtext) / 2,
						  (gbkgnd + gtext) / 2,
						  (bbkgnd + btext) / 2);
    return colors;
}

/* Create three simple icons, to be used when prompting the user.
 */
static int createprompticons(void)
{
    static Uint8 iconpixels[] = {
	0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,
	0,0,0,0,1,2,2,2,2,1,0,0,0,0,0,0,
	0,0,1,2,2,2,2,2,2,1,0,0,0,0,0,0,
	1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	0,0,1,2,2,2,2,2,2,1,0,0,0,0,0,0,
	0,0,0,0,1,2,2,2,2,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
	0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,
	0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,
	0,0,0,2,2,2,2,2,2,2,2,2,2,0,0,0,
	0,0,0,2,2,2,2,2,2,2,2,2,2,0,0,0,
	0,0,0,2,2,2,2,2,2,2,2,2,2,0,0,0,
	0,0,0,2,2,2,2,2,2,2,2,2,2,0,0,0,
	0,0,0,2,2,2,2,2,2,2,2,2,2,0,0,0,
	0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,
	0,0,0,0,0,2,2,2,2,2,2,0,0,0,0,0,
	0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,2,2,2,2,1,0,0,0,0,
	0,0,0,0,0,0,1,2,2,2,2,2,2,1,0,0,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,
	0,0,0,0,0,0,1,2,2,2,2,2,2,1,0,0,
	0,0,0,0,0,0,1,2,2,2,2,1,0,0,0,0,
	0,0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0
    };

    if (!prompticons) {
	prompticons = SDL_CreateRGBSurfaceFrom(iconpixels,
					       PROMPTICONW, 3 * PROMPTICONH,
					       8, PROMPTICONW, 0, 0, 0, 0);
	if (!prompticons) {
	    warn("couldn't create SDL surface: %s", SDL_GetError());
	    return FALSE;
	}
    }

    SDL_GetRGB(bkgndcolor(sdlg.dimtextclr), geng.screen->format,
	       &prompticons->format->palette->colors[0].r,
	       &prompticons->format->palette->colors[0].g,
	       &prompticons->format->palette->colors[0].b);
    SDL_GetRGB(halfcolor(sdlg.dimtextclr), geng.screen->format,
	       &prompticons->format->palette->colors[1].r,
	       &prompticons->format->palette->colors[1].g,
	       &prompticons->format->palette->colors[1].b);
    SDL_GetRGB(textcolor(sdlg.dimtextclr), geng.screen->format,
	       &prompticons->format->palette->colors[2].r,
	       &prompticons->format->palette->colors[2].g,
	       &prompticons->format->palette->colors[2].b);

    return TRUE;
}

/* Calculate the placements of all the separate elements of the
 * display.
 */
static int layoutscreen(void)
{
    static char const  *scoretext = "888  DRAWN AND QUARTERED"
				    "   88,888  8,888,888  8,888,888";
    static char const  *hinttext = "Total Score  ";
    static char const  *rscoretext = "88888888";
    static char const  *chipstext = "Chips";
    static char const  *timertext = " 88888";

    int			fullw, infow, rscorew, texth;

    if (geng.wtile <= 0 || geng.htile <= 0)
	return FALSE;

    puttext(&displayloc, scoretext, -1, PT_CALCSIZE);
    fullw = displayloc.w;
    texth = displayloc.h;
    puttext(&displayloc, hinttext, -1, PT_CALCSIZE);
    infow = displayloc.w;
    puttext(&displayloc, rscoretext, -1, PT_CALCSIZE);
    rscorew = displayloc.w;
    infow += rscorew;

    displayloc.x = MARGINW;
    displayloc.y = MARGINH;
    displayloc.w = NXTILES * geng.wtile;
    displayloc.h = NYTILES * geng.htile;
    geng.maploc = displayloc;

    titleloc.x = displayloc.x;
    titleloc.y = displayloc.y + displayloc.h + MARGINH;
    titleloc.w = displayloc.w;
    titleloc.h = texth;

    infoloc.x = displayloc.x + displayloc.w + MARGINW;
    infoloc.y = MARGINH;
    infoloc.w = 4 * geng.wtile;
    if (infoloc.w < infow)
	infoloc.w = infow;
    infoloc.h = 6 * texth;

    puttext(&rinfoloc, chipstext, -1, PT_CALCSIZE);
    rinfoloc.x = infoloc.x + rinfoloc.w + MARGINW;
    rinfoloc.y = infoloc.y + 3 * texth;
    puttext(&rinfoloc, timertext, -1, PT_CALCSIZE);
    rinfoloc.h = 2 * texth;

    invloc.x = infoloc.x;
    invloc.y = infoloc.y + infoloc.h + MARGINH;
    invloc.w = 4 * geng.wtile;
    invloc.h = 2 * geng.htile;

    screenw = infoloc.x + infoloc.w + MARGINW;
    if (screenw < fullw)
	screenw = fullw;
    screenh = titleloc.y + titleloc.h + MARGINH;

    promptloc.x = screenw - MARGINW - PROMPTICONW;
    promptloc.y = screenh - MARGINH - PROMPTICONH;
    promptloc.w = PROMPTICONW;
    promptloc.h = PROMPTICONH;

    messageloc.x = infoloc.x;
    messageloc.y = titleloc.y;
    messageloc.w = promptloc.x - messageloc.x - MARGINW;
    messageloc.h = titleloc.h;

    hintloc.x = infoloc.x;
    hintloc.y = invloc.y + invloc.h + MARGINH;
    hintloc.w = screenw - MARGINW - hintloc.x;
    hintloc.h = messageloc.y - hintloc.y;
    if (hintloc.y + hintloc.h + MARGINH > promptloc.y)
	hintloc.h = promptloc.y - MARGINH - hintloc.y;

    rscoreloc.x = hintloc.x + hintloc.w - rscorew;
    rscoreloc.y = hintloc.y + 2 * texth;
    rscoreloc.w = rscorew;
    rscoreloc.h = hintloc.h - 2 * texth;

    return TRUE;
}

/* Create or change the program's display surface.
 */
static int createdisplay(void)
{
    int	flags;

    if (geng.screen) {
	SDL_FreeSurface(geng.screen);
	geng.screen = NULL;
    }
    flags = SDL_SWSURFACE | SDL_ANYFORMAT;
    if (fullscreen)
	flags |= SDL_FULLSCREEN;
    if (!(geng.screen = SDL_SetVideoMode(screenw, screenh, 32, flags))) {
	errmsg(NULL, "cannot open %dx%d display: %s\n",
		     screenw, screenh, SDL_GetError());
	return FALSE;
    }
    if (geng.screen->w != screenw || geng.screen->h != screenh)
	warn("requested a %dx%d display, got %dx%d instead",
	     geng.screen->w, geng.screen->h);
    return TRUE;
}

/* Wipe the display.
 */
void cleardisplay(void)
{
    SDL_FillRect(geng.screen, NULL, bkgndcolor(sdlg.textclr));
    fullredraw = TRUE;
    geng.mapvieworigin = -1;
}

/*
 * Message display function.
 */

/* Refresh the message-display message. If update is TRUE, the screen
 * is updated immediately.
 */
static void displaymsg(int update)
{
    int	f;

    if (msgdisplay.until < SDL_GetTicks()) {
	*msgdisplay.msg = '\0';
	msgdisplay.msglen = 0;
	f = 0;
    } else {
	if (!msgdisplay.msglen)
	    return;
	f = PT_CENTER;
	if (msgdisplay.bolduntil < SDL_GetTicks())
	    f |= PT_DIM;
    }
    puttext(&messageloc, msgdisplay.msg, msgdisplay.msglen, f);
    if (update)
	SDL_UpdateRect(geng.screen, messageloc.x, messageloc.y,
				    messageloc.w, messageloc.h);
}

/* Change the current message-display message. msecs gives the number
 * of milliseconds to display the message, and bold specifies the
 * number of milliseconds to display the message highlighted.
 */
int setdisplaymsg(char const *msg, int msecs, int bold)
{
    if (!msg || !*msg) {
	*msgdisplay.msg = '\0';
	msgdisplay.msglen = 0;
	msgdisplay.until = 0;
	msgdisplay.bolduntil = 0;
    } else {
	msgdisplay.msglen = strlen(msg);
	if (msgdisplay.msglen >= sizeof msgdisplay.msg)
	    msgdisplay.msglen = sizeof msgdisplay.msg - 1;
	memcpy(msgdisplay.msg, msg, msgdisplay.msglen);
	msgdisplay.msg[msgdisplay.msglen] = '\0';
	msgdisplay.until = SDL_GetTicks() + msecs;
	msgdisplay.bolduntil = SDL_GetTicks() + bold;
    }
    displaymsg(TRUE);
    return TRUE;
}

/*
 * The main display functions.
 */

/* Create a string representing a decimal number.
 */
static char const *decimal(long number, int places)
{
    static char		buf[32];
    char	       *dest = buf + sizeof buf;
    unsigned long	n;

    n = number >= 0 ? (unsigned long)number : (unsigned long)-(number + 1) + 1;
    *--dest = '\0';
    do {
	*--dest = CHAR_MZERO + n % 10;
	n /= 10;
    } while (n);
    while (buf + sizeof buf - dest < places + 1)
	*--dest = CHAR_MZERO;
    if (number < 0)
	*--dest = '-';
    return dest;
}

/* Display an empty map view.
 */
static void displayshutter(void)
{
    SDL_Rect	rect;

    rect = displayloc;
    SDL_FillRect(geng.screen, &rect, halfcolor(sdlg.dimtextclr));
    ++rect.x;
    ++rect.y;
    rect.w -= 2;
    rect.h -= 2;
    SDL_FillRect(geng.screen, &rect, textcolor(sdlg.dimtextclr));
    ++rect.x;
    ++rect.y;
    rect.w -= 2;
    rect.h -= 2;
    SDL_FillRect(geng.screen, &rect, halfcolor(sdlg.dimtextclr));
    ++rect.x;
    ++rect.y;
    rect.w -= 2;
    rect.h -= 2;
    SDL_FillRect(geng.screen, &rect, bkgndcolor(sdlg.dimtextclr));
}

/* Render all the various nuggets of data that comprise the
 * information display. timeleft and besttime supply the current timer
 * value and the player's best recorded time as measured in seconds.
 * The level's title, number, password, and hint, the count of chips
 * needed, and the keys and boots in possession are all used as well
 * in creating the display.
 */
static void displayinfo(gamestate const *state, int timeleft, int besttime)
{
    SDL_Rect	rect, rrect;
    char	buf[512];
    int		n;

    puttext(&titleloc, state->game->name, -1, PT_CENTER);

    rect = infoloc;

    if (state->game->number) {
	sprintf(buf, "Level %d", state->game->number);
	puttext(&rect, buf, -1, PT_UPDATERECT);
    } else
	puttext(&rect, "", 0, PT_UPDATERECT);

    if (state->game->passwd && *state->game->passwd) {
	sprintf(buf, "Password: %s", state->game->passwd);
	puttext(&rect, buf, -1, PT_UPDATERECT);
    } else
	puttext(&rect, "", 0, PT_UPDATERECT);

    puttext(&rect, "", 0, PT_UPDATERECT);

    rrect.x = rinfoloc.x;
    rrect.w = rinfoloc.w;
    rrect.y = rect.y;
    rrect.h = rect.h;
    puttext(&rect, "Chips", 5, PT_UPDATERECT);
    puttext(&rect, "Time", 4, PT_UPDATERECT);
    puttext(&rrect, decimal(state->chipsneeded, 0), -1,
		    PT_RIGHT | PT_UPDATERECT);
    if (timeleft == TIME_NIL)
	puttext(&rrect, "---", -1, PT_RIGHT);
    else
	puttext(&rrect, decimal(timeleft, 0), -1, PT_RIGHT);
    if (state->stepping) {
	rrect.x += rrect.w;
	rrect.w = infoloc.x + infoloc.w - rrect.x;
	if (state->stepping < 4)
	    sprintf(buf, "   (+%d)", state->stepping);
	else if (state->stepping > 4)
	    sprintf(buf, "   (odd+%d)", state->stepping & 3);
	else
	    sprintf(buf, "   (odd)");
	puttext(&rrect, buf, -1, 0);
    }

    if (besttime != TIME_NIL) {
	if (timeleft == TIME_NIL)
	    sprintf(buf, "(Best time: %s)", decimal(besttime, 0));
	else
	    sprintf(buf, "Best time: %s", decimal(besttime, 0));
	n = (state->game->sgflags & SGF_REPLACEABLE) ? PT_DIM : 0;
	puttext(&rect, buf, -1, PT_UPDATERECT | n);
    }
    fillrect(&rect);

    for (n = 0 ; n < 4 ; ++n) {
	drawfulltileid(geng.screen,
		     invloc.x + n * geng.wtile, invloc.y,
		     (state->keys[n] ? Key_Red + n : Empty));
	drawfulltileid(geng.screen,
		     invloc.x + n * geng.wtile, invloc.y + geng.htile,
		     (state->boots[n] ? Boots_Ice + n : Empty));
    }

    if (state->statusflags & SF_INVALID) {
	puttext(&hintloc, "This level cannot be played.", -1, PT_MULTILINE);
    } else if (state->currenttime < 0 && state->game->unsolvable) {
	if (*state->game->unsolvable) {
	    sprintf(buf, "This level is reported to be unsolvable: %s.",
			 state->game->unsolvable);
	    puttext(&hintloc, buf, -1, PT_MULTILINE);
	} else {
	    puttext(&hintloc, "This level is reported to be unsolvable.", -1,
			      PT_MULTILINE);
	}
    } else if (state->statusflags & SF_SHOWHINT) {
	puttext(&hintloc, state->hinttext, -1, PT_MULTILINE | PT_CENTER);
    } else {
	fillrect(&hintloc);
    }

    fillrect(&promptloc);
}

/* Display a prompt icon in the lower right-hand corner. completed is
 * -1, 0, or +1, depending on which icon is being requested.
 */
static int displayprompticon(int completed)
{
    SDL_Rect	src;

    if (!prompticons)
	return FALSE;
    src.x = 0;
    src.y = (completed + 1) * PROMPTICONH;
    src.w = PROMPTICONW;
    src.h = PROMPTICONH;
    SDL_BlitSurface(prompticons, &src, geng.screen, &promptloc);
    SDL_UpdateRect(geng.screen, promptloc.x, promptloc.y,
				promptloc.w, promptloc.h);
    return TRUE;
}

/*
 * The exported functions.
 */

/* Set the four main colors used to render text on the display.
 */
void setcolors(long bkgnd, long text, long bold, long dim)
{
    int	bkgndr, bkgndg, bkgndb;

    if (bkgnd < 0)
	bkgnd = 0x000000;
    if (text < 0)
	text = 0xFFFFFF;
    if (bold < 0)
	bold = 0xFFFF00;
    if (dim < 0)
	dim = 0xC0C0C0;

    if (bkgnd == text || bkgnd == bold || bkgnd == dim) {
	errmsg(NULL, "one or more text colors matches the background color; "
		     "color scheme left unchanged.");
	return;
    }

    bkgndr = (bkgnd >> 16) & 255;
    bkgndg = (bkgnd >> 8) & 255;
    bkgndb = bkgnd & 255;

    sdlg.textclr = makefontcolors(bkgndr, bkgndg, bkgndb,
			(text >> 16) & 255, (text >> 8) & 255, text & 255);
    sdlg.dimtextclr = makefontcolors(bkgndr, bkgndg, bkgndb,
			(dim >> 16) & 255, (dim >> 8) & 255, dim & 255);
    sdlg.hilightclr = makefontcolors(bkgndr, bkgndg, bkgndb,
			(bold >> 16) & 255, (bold >> 8) & 255, bold & 255);

    createprompticons();
}

/* Create the game's display. state is a pointer to the gamestate
 * structure.
 */
int displaygame(gamestate const *state, int timeleft, int besttime)
{
    if (state->statusflags & SF_SHUTTERED) {
	displayshutter();
    } else {
	displaymapview(state, displayloc);
    }
    displayinfo(state, timeleft, besttime);
    displaymsg(FALSE);
    if (fullredraw) {
	SDL_UpdateRect(geng.screen, 0, 0, 0, 0);
	fullredraw = FALSE;
    } else {
	SDL_UpdateRects(geng.screen,
			sizeof locrects / sizeof *locrects, locrects);
    }
    return TRUE;
}

/* Update the display to acknowledge the end of game play. completed
 * is positive if the play was successful or negative if unsuccessful.
 * If the latter, then the other arguments can contain point values
 * that will be reported to the user.
 */
int displayendmessage(int basescore, int timescore, long totalscore,
		      int completed)
{
    SDL_Rect	rect;
    int		fullscore;

    if (totalscore) {
	fullscore = timescore + basescore;
	rect = hintloc;
	puttext(&rect, "Level Completed", -1, PT_CENTER);
	rect.y = rscoreloc.y;
	rect.h = rscoreloc.h;
	puttext(&rect, "Time Bonus", -1, PT_UPDATERECT);
	puttext(&rect, "Level Bonus", -1, PT_UPDATERECT);
	puttext(&rect, "Level Score", -1, PT_UPDATERECT);
	puttext(&rect, "Total Score", -1, PT_UPDATERECT);
	rect = rscoreloc;
	puttext(&rect, decimal(timescore, 4), -1, PT_RIGHT | PT_UPDATERECT);
	puttext(&rect, decimal(basescore, 5), -1, PT_RIGHT | PT_UPDATERECT);
	puttext(&rect, decimal(fullscore, 5), -1, PT_RIGHT | PT_UPDATERECT);
	puttext(&rect, decimal(totalscore, 7), -1, PT_RIGHT | PT_UPDATERECT);
	SDL_UpdateRect(geng.screen, hintloc.x, hintloc.y,
				    hintloc.w, hintloc.h);
    }
    displayprompticon(completed);
    return CmdNone;
}

/* Render a table on the display. title is a short string to let the
 * user know what they're looking at. completed determines the prompt
 * icon that will be displayed in the lower right-hand corner.
 */
int displaytable(char const *title, tablespec const *table, int completed)
{
    SDL_Rect	area;
    SDL_Rect   *cols;
    int		i, n;

    cleardisplay();

    area.x = MARGINW;
    area.y = screenh - MARGINH - sdlg.font.h;
    area.w = screenw - 2 * MARGINW;
    area.h = sdlg.font.h;
    puttext(&area, title, -1, 0);
    area.h = area.y - MARGINH;
    area.y = MARGINH;

    cols = measuretable(&area, table);
    for (i = table->rows, n = 0 ; i ; --i)
	drawtablerow(table, cols, &n, 0);
    free(cols);

    displayprompticon(completed);
    SDL_UpdateRect(geng.screen, 0, 0, 0, 0);
    return TRUE;
}

/* Render a table with embedded illustrations on the display. title is
 * a short string to display under the table. rows is an array of
 * count lines of text, each accompanied by one or two illustrations.
 * completed determines the prompt icon that will be displayed in the
 * lower right-hand corner.
 */
int displaytiletable(char const *title,
		     tiletablerow const *rows, int count, int completed)
{
    SDL_Rect	left, right;
    int		col, id, i;

    cleardisplay();

    left.x = MARGINW;
    left.y = screenh - MARGINH - sdlg.font.h;
    left.w = screenw - 2 * MARGINW;
    left.h = sdlg.font.h;
    puttext(&left, title, -1, 0);
    left.h = left.y - MARGINH;
    left.y = MARGINH;

    right = left;
    col = geng.wtile * 2 + MARGINW;
    right.x += col;
    right.w -= col;
    for (i = 0 ; i < count ; ++i) {
	if (rows[i].isfloor)
	    id = rows[i].item1;
	else
	    id = crtile(rows[i].item1, EAST);
	drawfulltileid(geng.screen, left.x + geng.wtile, left.y, id);
	if (rows[i].item2) {
	    if (rows[i].isfloor)
		id = rows[i].item2;
	    else
		id = crtile(rows[i].item2, EAST);
	    drawfulltileid(geng.screen, left.x, left.y, id);
	}
	left.y += geng.htile;
	left.h -= geng.htile;
	puttext(&right, rows[i].desc, -1, PT_MULTILINE | PT_UPDATERECT);
	if (left.y < right.y) {
	    left.y = right.y;
	    left.h = right.h;
	} else {
	    right.y = left.y;
	    right.h = left.h;
	}
    }

    displayprompticon(completed);

    SDL_UpdateRect(geng.screen, 0, 0, 0, 0);

    return TRUE;
}

/* Render a table as a scrollable list on the display. One row is
 * highlighted as the current selection, initially set by the integer
 * pointed to by idx. The callback function inputcallback is called
 * repeatedly to determine how to move the selection and when to
 * leave. The row selected when the function returns is returned to
 * the caller through idx.
 */
int displaylist(char const *title, tablespec const *table, int *idx,
		DisplayListType listtype, int (*inputcallback)(int*))
{
    SDL_Rect		area;
    SDL_Rect	       *cols;
    SDL_Rect	       *colstmp;
    int			linecount, itemcount, topitem, index;
    int			j, n;

    cleardisplay();
    area.x = MARGINW;
    area.y = screenh - MARGINH - sdlg.font.h;
    area.w = screenw - 2 * MARGINW;
    area.h = sdlg.font.h;
    puttext(&area, title, -1, 0);
    area.h = area.y - MARGINH;
    area.y = MARGINH;
    cols = measuretable(&area, table);
    if (!(colstmp = malloc(table->cols * sizeof *colstmp)))
	memerrexit();

    itemcount = table->rows - 1;
    topitem = 0;
    linecount = area.h / sdlg.font.h - 1;

    index = *idx;
    n = SCROLL_NOP;
    do {
	switch (n) {
	  case SCROLL_NOP:						break;
	  case SCROLL_UP:		--index;			break;
	  case SCROLL_DN:		++index;			break;
	  case SCROLL_HALFPAGE_UP:	index -= (linecount + 1) / 2;	break;
	  case SCROLL_HALFPAGE_DN:	index += (linecount + 1) / 2;	break;
	  case SCROLL_PAGE_UP:		index -= linecount;		break;
	  case SCROLL_PAGE_DN:		index += linecount;		break;
	  case SCROLL_ALLTHEWAY_UP:	index = 0;			break;
	  case SCROLL_ALLTHEWAY_DN:	index = itemcount - 1;		break;
	  default:			index = n;			break;
	}
	if (index < 0)
	    index = 0;
	else if (index >= itemcount)
	    index = itemcount - 1;
	if (linecount < itemcount) {
	    n = linecount / 2;
	    if (index < n)
		topitem = 0;
	    else if (index >= itemcount - n)
		topitem = itemcount - linecount;
	    else
		topitem = index - n;
	}

	n = 0;
	SDL_FillRect(geng.screen, &area, bkgndcolor(sdlg.textclr));
	memcpy(colstmp, cols, table->cols * sizeof *colstmp);
	drawtablerow(table, colstmp, &n, 0);
	for (j = 0 ; j < topitem ; ++j)
	    drawtablerow(table, NULL, &n, 0);
	for ( ; j < topitem + linecount && j < itemcount ; ++j)
	    drawtablerow(table, colstmp, &n, j == index ? PT_HILIGHT : 0);
	SDL_UpdateRect(geng.screen, 0, 0, 0, 0);

	n = SCROLL_NOP;
    } while ((*inputcallback)(&n));
    if (n)
	*idx = index;

    free(cols);
    free(colstmp);
    cleardisplay();
    return n;
}

/* Display a line of text, given by prompt, at the center of the display.
 * The callback function inputcallback is then called repeatedly to
 * obtain input characters, which are collected in input. maxlen sets an
 * upper limit to the length of the input so collected.
 */
int displayinputprompt(char const *prompt, char *input, int maxlen,
		       InputPromptType inputtype, int (*inputcallback)(void))
{
    SDL_Rect	area, promptrect, inputrect;
    int		len, ch;

    puttext(&inputrect, "W", 1, PT_CALCSIZE);
    inputrect.w *= maxlen + 1;
    puttext(&promptrect, prompt, -1, PT_CALCSIZE);
    area.h = inputrect.h + promptrect.h + 2 * MARGINH;
    if (inputrect.w > promptrect.w)
	area.w = inputrect.w;
    else
	area.w = promptrect.w;
    area.w += 2 * MARGINW;
    area.x = (screenw - area.w) / 2;
    area.y = (screenh - area.h) / 2;
    promptrect.x = area.x + MARGINW;
    promptrect.y = area.y + MARGINH;
    promptrect.w = area.w - 2 * MARGINW;
    inputrect.x = promptrect.x;
    inputrect.y = promptrect.y + promptrect.h;
    inputrect.w = promptrect.w;

    len = strlen(input);
    if (len > maxlen)
	len = maxlen;
    for (;;) {
	SDL_FillRect(geng.screen, &area, textcolor(sdlg.textclr));
	++area.x;
	++area.y;
	area.w -= 2;
	area.h -= 2;
	SDL_FillRect(geng.screen, &area, bkgndcolor(sdlg.textclr));
	--area.x;
	--area.y;
	area.w += 2;
	area.h += 2;
	puttext(&promptrect, prompt, -1, PT_CENTER);
	input[len] = '_';
	puttext(&inputrect, input, len + 1, PT_CENTER);
	input[len] = '\0';
	SDL_UpdateRect(geng.screen, area.x, area.y, area.w, area.h);
	ch = (*inputcallback)();
	if (ch == '\n' || ch < 0)
	    break;
	if (isprint(ch)) {
	    input[len] = ch;
	    if (len < maxlen)
		++len;
	    input[len] = '\0';
	} else if (ch == '\b') {
	    if (len)
		--len;
	    input[len] = '\0';
	} else if (ch == '\f') {
	    len = 0;
	    input[0] = '\0';
	} else {
	    /* no op */
	}
    }
    cleardisplay();
    return ch == '\n';
}

/* Create a display surface appropriate to the requirements of the
 * game.
 */
int creategamedisplay(void)
{
    if (!layoutscreen() || !createdisplay())
	return FALSE;
    cleardisplay();
    return TRUE;
}

/* Initialize the display with a generic surface capable of rendering
 * text.
 */
int _sdloutputinitialize(int _fullscreen)
{
    fullscreen = _fullscreen;

    screenw = 640;
    screenh = 480;
    promptloc.x = screenw - MARGINW - PROMPTICONW;
    promptloc.y = screenh - MARGINH - PROMPTICONH;
    promptloc.w = PROMPTICONW;
    promptloc.h = PROMPTICONH;
    createdisplay();
    cleardisplay();

    sdlg.textclr = makefontcolors(0, 0, 0, 255, 255, 255);
    sdlg.dimtextclr = makefontcolors(0, 0, 0, 192, 192, 192);
    sdlg.hilightclr = makefontcolors(0, 0, 0, 255, 255, 0);

    cleardisplay();

    if (!createprompticons())
	return FALSE;

    return TRUE;
}
