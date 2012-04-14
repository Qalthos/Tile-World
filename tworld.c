/* tworld.c: The top-level module.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"defs.h"
#include	"err.h"
#include	"series.h"
#include	"res.h"
#include	"play.h"
#include	"score.h"
#include	"solution.h"
#include	"unslist.h"
#include	"help.h"
#include	"oshw.h"
#include	"cmdline.h"
#include	"ver.h"

/* Bell-ringing macro.
 */
#define	bell()	(silence ? (void)0 : ding())

enum { Play_None, Play_Normal, Play_Back, Play_Verify };

/* The data needed to identify what level is being played.
 */
typedef	struct gamespec {
    gameseries	series;		/* the complete set of levels */
    int		currentgame;	/* which level is currently selected */
    int		playmode;	/* which mode to play */
    int		usepasswds;	/* FALSE if passwords are to be ignored */
    int		status;		/* final status of last game played */
    int		enddisplay;	/* TRUE if the final level was completed */
    int		melindacount;	/* count for Melinda's free pass */
} gamespec;

/* Structure used to hold data collected by initoptionswithcmdline().
 */
typedef	struct startupdata {
    char       *filename;	/* which data file to use */
    char       *savefilename;	/* an alternate solution file */
    int		levelnum;	/* a selected initial level */ 
    int		listdirs;	/* TRUE if directories should be listed */
    int		listseries;	/* TRUE if the files should be listed */
    int		listscores;	/* TRUE if the scores should be listed */
    int		listtimes;	/* TRUE if the times should be listed */
    int		batchverify;	/* TRUE to enter batch verification */
} startupdata;

/* Structure used to hold the complete list of available series.
 */
typedef	struct seriesdata {
    gameseries *list;		/* the array of available series */
    int		count;		/* size of arary */
    tablespec	table;		/* table for displaying the array */
} seriesdata;

/* TRUE suppresses sound and the console bell.
 */
static int	silence = FALSE;

/* TRUE means the program should attempt to run in fullscreen mode.
 */
static int	fullscreen = FALSE;

/* FALSE suppresses all password checking.
 */
static int	usepasswds = TRUE;

/* TRUE if the user requested an idle-time histogram.
 */
static int	showhistogram = FALSE;

/* Slowdown factor, used for debugging.
 */
static int	mudsucking = 1;

/* Frame-skipping disable flag.
 */
static int	noframeskip = FALSE;

/* The sound buffer scaling factor.
 */
static int	soundbufsize = -1;

/* The initial volume level.
 */
static int	volumelevel = -1;

/* The top of the stack of subtitles.
 */
static void   **subtitlestack = NULL;

/*
 * Text-mode output functions.
 */

/* Find a position to break a string inbetween words. The integer at
 * breakpos receives the length of the string prefix less than or
 * equal to len. The string pointer *str is advanced to the first
 * non-whitespace after the break. The original string pointer is
 * returned.
 */
static char *findstrbreak(char const **str, int maxlen, int *breakpos)
{
    char const *start;
    int		n;

  retry:
    start = *str;
    n = strlen(start);
    if (n <= maxlen) {
	*str += n;
	*breakpos = n;
    } else {
	n = maxlen;
	if (isspace(start[n])) {
	    *str += n;
	    while (isspace(**str))
		++*str;
	    while (n > 0 && isspace(start[n - 1]))
		--n;
	    if (n == 0)
		goto retry;
	    *breakpos = n;
	} else {
	    while (n > 0 && !isspace(start[n - 1]))
		--n;
	    if (n == 0) {
		*str += maxlen;
		*breakpos = maxlen;
	    } else {
		*str = start + n;
		while (n > 0 && isspace(start[n - 1]))
		    --n;
		if (n == 0)
		    goto retry;
		*breakpos = n;
	    }
	}
    }
    return (char*)start;
}

/* Render a table to the given file. This function encapsulates both
 * the process of determining the necessary widths for each column of
 * the table, and then sequentially rendering the table's contents to
 * a stream. On the first pass through the data, single-cell
 * non-word-wrapped entries are measured and each column sized to fit.
 * If the resulting table is too large for the given area, then the
 * collapsible column is reduced as necessary. If there is still
 * space, however, then the entries that span multiple cells are
 * measured in a second pass, and columns are grown to fit them as
 * well where needed. If there is still space after this, the column
 * containing word-wrapped entries may be expanded as well.
 */
void printtable(FILE *out, tablespec const *table)
{
    int const	maxwidth = 79;
    char const *mlstr;
    char const *p;
    int	       *colsizes;
    int		mlindex, mlwidth, mlpos;
    int		diff, pos;
    int		i, j, n, i0, c, w, z;

    if (!(colsizes = malloc(table->cols * sizeof *colsizes)))
	return;
    for (i = 0 ; i < table->cols ; ++i)
	colsizes[i] = 0;
    mlindex = -1;
    mlwidth = 0;
    n = 0;
    for (j = 0 ; j < table->rows ; ++j) {
	for (i = 0 ; i < table->cols ; ++n) {
	    c = table->items[n][0] - '0';
	    if (c == 1) {
		w = strlen(table->items[n] + 2);
		if (table->items[n][1] == '!') {
		    if (w > mlwidth || mlindex != i)
			mlwidth = w;
		    mlindex = i;
		} else {
		    if (w > colsizes[i])
			colsizes[i] = w;
		}
	    }
	    i += c;
	}
    }

    w = -table->sep;
    for (i = 0 ; i < table->cols ; ++i)
	w += colsizes[i] + table->sep;
    diff = maxwidth - w;

    if (diff < 0 && table->collapse >= 0) {
	w = -diff;
	if (colsizes[table->collapse] < w)
	    w = colsizes[table->collapse] - 1;
	colsizes[table->collapse] -= w;
	diff += w;
    }

    if (diff > 0) {
	n = 0;
	for (j = 0 ; j < table->rows && diff > 0 ; ++j) {
	    for (i = 0 ; i < table->cols ; ++n) {
		c = table->items[n][0] - '0';
		if (c > 1 && table->items[n][1] != '!') {
		    w = table->sep + strlen(table->items[n] + 2);
		    for (i0 = i ; i0 < i + c ; ++i0)
			w -= colsizes[i0] + table->sep;
		    if (w > 0) {
			if (table->collapse >= i && table->collapse < i + c)
			    i0 = table->collapse;
			else if (mlindex >= i && mlindex < i + c)
			    i0 = mlindex;
			else
			    i0 = i + c - 1;
			if (w > diff)
			    w = diff;
			colsizes[i0] += w;
			diff -= w;
			if (diff == 0)
			    break;
		    }
		}
		i += c;
	    }
	}
    }
    if (diff > 0 && mlindex >= 0 && colsizes[mlindex] < mlwidth) {
	mlwidth -= colsizes[mlindex];
	w = mlwidth < diff ? mlwidth : diff;
	colsizes[mlindex] += w;
	diff -= w;
    }

    n = 0;
    for (j = 0 ; j < table->rows ; ++j) {
	mlstr = NULL;
	mlwidth = mlpos = 0;
	pos = 0;
	for (i = 0 ; i < table->cols ; ++n) {
	    if (i)
		pos += fprintf(out, "%*s", table->sep, "");
	    c = table->items[n][0] - '0';
	    w = -table->sep;
	    while (c--)
		w += colsizes[i++] + table->sep;
	    if (table->items[n][1] == '-')
		fprintf(out, "%-*.*s", w, w, table->items[n] + 2);
	    else if (table->items[n][1] == '+')
		fprintf(out, "%*.*s", w, w, table->items[n] + 2);
	    else if (table->items[n][1] == '.') {
		z = (w - strlen(table->items[n] + 2)) / 2;
		if (z < 0)
		    z = w;
		fprintf(out, "%*.*s%*s",
			     w - z, w - z, table->items[n] + 2, z, "");
	    } else if (table->items[n][1] == '!') {
		mlwidth = w;
		mlpos = pos;
		mlstr = table->items[n] + 2;
		p = findstrbreak(&mlstr, w, &z);
		fprintf(out, "%.*s%*s", z, p, w - z, "");
	    }
	    pos += w;
	}
	fputc('\n', out);
	while (mlstr && *mlstr) {
	    p = findstrbreak(&mlstr, mlwidth, &w);
	    fprintf(out, "%*s%.*s\n", mlpos, "", w, p);
	}
    }
    free(colsizes);
}

/* Display directory settings.
 */
static void printdirectories(void)
{
    printf("Resource files read from:        %s\n", resdir);
    printf("Level sets read from:            %s\n", seriesdir);
    printf("Configured data files read from: %s\n", seriesdatdir);
    printf("Solution files saved in:         %s\n", savedir);
}

/*
 * Callback functions for oshw.
 */

/* An input callback that only accepts the characters Y and N.
 */
static int yninputcallback(void)
{
    switch (input(TRUE)) {
      case 'Y': case 'y':	return 'Y';
      case 'N': case 'n':	return 'N';
      case CmdWest:		return '\b';
      case CmdProceed:		return '\n';
      case CmdQuitLevel:	return -1;
      case CmdQuit:		exit(0);
    }
    return 0;
}

/* An input callback that accepts only alphabetic characters.
 */
static int keyinputcallback(void)
{
    int	ch;

    ch = input(TRUE);
    switch (ch) {
        case CmdWest:		return '\b';
        case CmdProceed:	return '\n';
        case CmdQuitLevel:	return -1;
        case CmdQuit:		exit(0);
        default:
	  if (isalpha(ch))
	      return toupper(ch);
    }
    return 0;
}

/* An input callback used while displaying a scrolling list.
 */
static int scrollinputcallback(int *move)
{
    int cmd;
    switch ((cmd = input(TRUE))) {
      case CmdPrev10:		*move = SCROLL_HALFPAGE_UP;	break;
      case CmdNorth:		*move = SCROLL_UP;		break;
      case CmdPrev:		*move = SCROLL_UP;		break;
      case CmdPrevLevel:	*move = SCROLL_UP;		break;
      case CmdSouth:		*move = SCROLL_DN;		break;
      case CmdNext:		*move = SCROLL_DN;		break;
      case CmdNextLevel:	*move = SCROLL_DN;		break;
      case CmdNext10:		*move = SCROLL_HALFPAGE_DN;	break;
      case CmdProceed:		*move = CmdProceed;		return FALSE;
      case CmdQuitLevel:	*move = CmdQuitLevel;		return FALSE;
      case CmdHelp:		*move = CmdHelp;		return FALSE;
      case CmdQuit:						exit(0);

    }
    return TRUE;
}

/* An input callback used while displaying the scrolling list of scores.
 */
static int scorescrollinputcallback(int *move)
{
    int cmd;
    switch ((cmd = input(TRUE))) {
      case CmdPrev10:		*move = SCROLL_HALFPAGE_UP;	break;
      case CmdNorth:		*move = SCROLL_UP;		break;
      case CmdPrev:		*move = SCROLL_UP;		break;
      case CmdPrevLevel:	*move = SCROLL_UP;		break;
      case CmdSouth:		*move = SCROLL_DN;		break;
      case CmdNext:		*move = SCROLL_DN;		break;
      case CmdNextLevel:	*move = SCROLL_DN;		break;
      case CmdNext10:		*move = SCROLL_HALFPAGE_DN;	break;
      case CmdProceed:		*move = CmdProceed;		return FALSE;
      case CmdSeeSolutionFiles:	*move = CmdSeeSolutionFiles;	return FALSE;
      case CmdQuitLevel:	*move = CmdQuitLevel;		return FALSE;
      case CmdHelp:		*move = CmdHelp;		return FALSE;
      case CmdQuit:						exit(0);
    }
    return TRUE;
}

/* An input callback used while displaying the scrolling list of solutions.
 */
static int solutionscrollinputcallback(int *move)
{
    int cmd;
    switch ((cmd = input(TRUE))) {
      case CmdPrev10:		*move = SCROLL_HALFPAGE_UP;	break;
      case CmdNorth:		*move = SCROLL_UP;		break;
      case CmdPrev:		*move = SCROLL_UP;		break;
      case CmdPrevLevel:	*move = SCROLL_UP;		break;
      case CmdSouth:		*move = SCROLL_DN;		break;
      case CmdNext:		*move = SCROLL_DN;		break;
      case CmdNextLevel:	*move = SCROLL_DN;		break;
      case CmdNext10:		*move = SCROLL_HALFPAGE_DN;	break;
      case CmdProceed:		*move = CmdProceed;		return FALSE;
      case CmdSeeScores:	*move = CmdSeeScores;		return FALSE;
      case CmdQuitLevel:	*move = CmdQuitLevel;		return FALSE;
      case CmdHelp:		*move = CmdHelp;		return FALSE;
      case CmdQuit:						exit(0);
    }
    return TRUE;
}

/*
 * Basic game activities.
 */

/* Return TRUE if the given level is a final level.
 */
static int islastinseries(gamespec const *gs, int index)
{
    return index == gs->series.count - 1
	|| gs->series.games[index].number == gs->series.final;
}

/* Return TRUE if the current level has a solution.
 */
static int issolved(gamespec const *gs, int index)
{
    return hassolution(gs->series.games + index);
}

/* Mark the current level's solution as replaceable.
 */
static void replaceablesolution(gamespec *gs, int change)
{
    if (change < 0)
	gs->series.games[gs->currentgame].sgflags ^= SGF_REPLACEABLE;
    else if (change > 0)
	gs->series.games[gs->currentgame].sgflags |= SGF_REPLACEABLE;
    else
	gs->series.games[gs->currentgame].sgflags &= ~SGF_REPLACEABLE;
}

/* Mark the current level's password as known to the user.
 */
static void passwordseen(gamespec *gs, int number)
{
    if (!(gs->series.games[number].sgflags & SGF_HASPASSWD)) {
	gs->series.games[number].sgflags |= SGF_HASPASSWD;
	savesolutions(&gs->series);
    }
}

/* Change the current level, ensuring that the user is not granted
 * access to a forbidden level. FALSE is returned if the specified
 * level is not available to the user.
 */
static int setcurrentgame(gamespec *gs, int n)
{
    if (n == gs->currentgame)
	return TRUE;
    if (n < 0 || n >= gs->series.count)
	return FALSE;

    if (gs->usepasswds)
	if (n > 0 && !(gs->series.games[n].sgflags & SGF_HASPASSWD)
		  && !issolved(gs, n -1))
	    return FALSE;

    gs->currentgame = n;
    gs->melindacount = 0;
    return TRUE;
}

/* Change the current level by a delta value. If the user cannot go to
 * that level, the "nearest" level in that direction is chosen
 * instead. FALSE is returned if the current level remained unchanged.
 */
static int changecurrentgame(gamespec *gs, int offset)
{
    int	sign, m, n;

    if (offset == 0)
	return FALSE;

    m = gs->currentgame;
    n = m + offset;
    if (n < 0)
	n = 0;
    else if (n >= gs->series.count)
	n = gs->series.count - 1;

    if (gs->usepasswds && n > 0) {
	sign = offset < 0 ? -1 : +1;
	for ( ; n >= 0 && n < gs->series.count ; n += sign) {
	    if (!n || (gs->series.games[n].sgflags & SGF_HASPASSWD)
		   || issolved(gs, n - 1)) {
		m = n;
		break;
	    }
	}
	n = m;
	if (n == gs->currentgame && offset != sign) {
	    n = gs->currentgame + offset - sign;
	    for ( ; n != gs->currentgame ; n -= sign) {
		if (n < 0 || n >= gs->series.count)
		    continue;
		if (!n || (gs->series.games[n].sgflags & SGF_HASPASSWD)
		       || issolved(gs, n - 1))
		    break;
	    }
	}
    }

    if (n == gs->currentgame)
	return FALSE;

    gs->currentgame = n;
    gs->melindacount = 0;
    return TRUE;
}

/* Return TRUE if Melinda is watching Chip's progress on this level --
 * i.e., if it is possible to earn a pass to the next level.
 */
static int melindawatching(gamespec const *gs)
{
    if (!gs->usepasswds)
	return FALSE;
    if (islastinseries(gs, gs->currentgame))
	return FALSE;
    if (gs->series.games[gs->currentgame + 1].sgflags & SGF_HASPASSWD)
	return FALSE;
    if (issolved(gs, gs->currentgame))
	return FALSE;
    return TRUE;
}

/*
 * The subtitle stack
 */

static void pushsubtitle(char const *subtitle)
{
    void      **stk;
    int		n;

    if (!subtitle)
	subtitle = "";
    n = strlen(subtitle) + 1;
    stk = NULL;
    xalloc(stk, sizeof(void**) + n);
    *stk = subtitlestack;
    subtitlestack = stk;
    memcpy(stk + 1, subtitle, n);
    setsubtitle(subtitle);
}

static void popsubtitle(void)
{
    void      **stk;

    if (subtitlestack) {
	stk = *subtitlestack;
	free(subtitlestack);
	subtitlestack = stk;
    }
    setsubtitle(subtitlestack ? (char*)(subtitlestack + 1) : NULL);
}

static void changesubtitle(char const *subtitle)
{
    int		n;

    if (!subtitle)
	subtitle = "";
    n = strlen(subtitle) + 1;
    xalloc(subtitlestack, sizeof(void**) + n);
    memcpy(subtitlestack + 1, subtitle, n);
    setsubtitle(subtitle);
}

/*
 *
 */

static void dohelp(int topic)
{
    pushsubtitle("Help");
    switch (topic) {
      case Help_First:
      case Help_FileListKeys:
      case Help_ScoreListKeys:
	onlinecontexthelp(topic);
	break;
      default:
	onlinemainhelp(topic);
	break;
    }
    popsubtitle();
}

/* Display a scrolling list of the available solution files, and allow
 * the user to select one. Return TRUE if the user selected a solution
 * file different from the current one. Do nothing if there is only
 * one solution file available. (If for some reason the new solution
 * file cannot be read, TRUE will still be returned, as the list of
 * solved levels will still need to be updated.)
 */
static int showsolutionfiles(gamespec *gs)
{
    tablespec		table;
    char const	      **filelist;
    int			readonly = FALSE;
    int			count, current, f, n;

    if (haspathname(gs->series.name) || (gs->series.savefilename
				&& haspathname(gs->series.savefilename))) {
	bell();
	return FALSE;
    } else if (!createsolutionfilelist(&gs->series, FALSE, &filelist,
				       &count, &table)) {
	bell();
	return FALSE;
    }

    current = -1;
    n = 0;
    if (gs->series.savefilename) {
	for (n = 0 ; n < count ; ++n)
	    if (!strcmp(filelist[n], gs->series.savefilename))
		break;
	if (n == count)
	    n = 0;
	else
	    current = n;
    }

    pushsubtitle(gs->series.name);
    for (;;) {
	f = displaylist("SOLUTION FILES", &table, &n,
			LIST_SOLUTIONFILES, solutionscrollinputcallback);
	if (f == CmdProceed) {
	    readonly = FALSE;
	    break;
	} else if (f == CmdSeeScores) {
	    readonly = TRUE;
	    break;
	} else if (f == CmdQuitLevel) {
	    n = -1;
	    break;
	} else if (f == CmdHelp) {
	    dohelp(Help_FileListKeys);
	}
    }
    popsubtitle();

    f = n >= 0 && n != current;
    if (f) {
	clearsolutions(&gs->series);
	if (!gs->series.savefilename)
	    gs->series.savefilename = getpathbuffer();
	sprintf(gs->series.savefilename, "%.*s", getpathbufferlen(),
						 filelist[n]);
	if (readsolutions(&gs->series)) {
	    if (readonly)
		gs->series.gsflags |= GSF_NOSAVING;
	} else {
	    bell();
	}
	n = gs->currentgame;
	gs->currentgame = 0;
	passwordseen(gs, 0);
	changecurrentgame(gs, n);
    }

    freesolutionfilelist(filelist, &table);
    return f;
}

/* Display the scrolling list of the user's current scores, and allow
 * the user to select a current level.
 */
static int showscores(gamespec *gs)
{
    tablespec	table;
    int	       *levellist;
    int		ret = FALSE;
    int		count, f, n;

  restart:
    if (!createscorelist(&gs->series, gs->usepasswds, CHAR_MZERO,
			 &levellist, &count, &table)) {
	bell();
	return ret;
    }
    for (n = 0 ; n < count ; ++n)
	if (levellist[n] == gs->currentgame)
	    break;
    pushsubtitle(gs->series.name);
    for (;;) {
	f = displaylist(gs->series.filebase, &table, &n,
			LIST_SCORES, scorescrollinputcallback);
	if (f == CmdProceed) {
	    n = levellist[n];
	    break;
	} else if (f == CmdSeeSolutionFiles) {
	    if (!(gs->series.gsflags & GSF_NODEFAULTSAVE)) {
		n = levellist[n];
		break;
	    }
	} else if (f == CmdQuitLevel) {
	    n = -1;
	    break;
	} else if (f == CmdHelp) {
	    dohelp(Help_ScoreListKeys);
	}
    }
    popsubtitle();
    freescorelist(levellist, &table);
    if (f == CmdSeeSolutionFiles) {
	setcurrentgame(gs, n);
	ret = showsolutionfiles(gs);
	goto restart;
    }
    if (n < 0)
	return ret;
    return setcurrentgame(gs, n) || ret;
}

/* Obtain a password from the user and move to the requested level.
 */
static int selectlevelbypassword(gamespec *gs)
{
    char	passwd[5] = "";
    int		n;

    setgameplaymode(BeginInput);
    n = displayinputprompt("Enter Password", passwd, 4,
			   INPUT_ALPHA, keyinputcallback);
    setgameplaymode(EndInput);
    if (!n)
	return FALSE;

    n = findlevelinseries(&gs->series, 0, passwd);
    if (n < 0) {
	bell();
	return FALSE;
    }
    passwordseen(gs, n);
    return setcurrentgame(gs, n);
}

/*
 * The game-playing functions.
 */

#define	leveldelta(n)	if (!changecurrentgame(gs, (n))) { bell(); continue; }

/* Get a key command from the user at the start of the current level.
 */
static int startinput(gamespec *gs)
{
    static int	lastlevel = -1;
    char	yn[2];
    int		cmd, n;

    if (gs->currentgame != lastlevel) {
	lastlevel = gs->currentgame;
	setstepping(0, FALSE);
    }
    drawscreen(TRUE);
    gs->playmode = Play_None;
    for (;;) {
	cmd = input(TRUE);
	if (cmd >= CmdMoveFirst && cmd <= CmdMoveLast) {
	    gs->playmode = Play_Normal;
	    return cmd;
	}
	switch (cmd) {
	  case CmdProceed:	gs->playmode = Play_Normal;	return cmd;
	  case CmdQuitLevel:					return cmd;
	  case CmdPrev10:	leveldelta(-10);		return CmdNone;
	  case CmdPrev:		leveldelta(-1);			return CmdNone;
	  case CmdPrevLevel:	leveldelta(-1);			return CmdNone;
	  case CmdNextLevel:	leveldelta(+1);			return CmdNone;
	  case CmdNext:		leveldelta(+1);			return CmdNone;
	  case CmdNext10:	leveldelta(+10);		return CmdNone;
	  case CmdStepping:	changestepping(4, TRUE);	break;
	  case CmdSubStepping:	changestepping(1, TRUE);	break;
	  case CmdVolumeUp:	changevolume(+2, TRUE);		break;
	  case CmdVolumeDown:	changevolume(-2, TRUE);		break;
	  case CmdHelp:		dohelp(Help_KeysBetweenGames);	break;
	  case CmdQuit:						exit(0);
	  case CmdPlayback:
	    if (prepareplayback()) {
		gs->playmode = Play_Back;
		return CmdProceed;
	    }
	    bell();
	    break;
	  case CmdCheckSolution:
	    if (prepareplayback()) {
		gs->playmode = Play_Verify;
		return CmdProceed;
	    }
	    bell();
	    break;
	  case CmdReplSolution:
	    if (issolved(gs, gs->currentgame))
		replaceablesolution(gs, -1);
	    else
		bell();
	    break;
	  case CmdKillSolution:
	    if (!issolved(gs, gs->currentgame)) {
		bell();
		break;
	    }
	    yn[0] = '\0';
	    setgameplaymode(BeginInput);
	    n = displayinputprompt("Really delete solution?",
				   yn, 1, INPUT_YESNO, yninputcallback);
	    setgameplaymode(EndInput);
	    if (n && *yn == 'Y')
		if (deletesolution())
		    savesolutions(&gs->series);
	    break;
	  case CmdSeeScores:
	    if (showscores(gs))
		return CmdNone;
	    break;
	  case CmdSeeSolutionFiles:
	    if (showsolutionfiles(gs))
		return CmdNone;
	    break;
	  case CmdGotoLevel:
	    if (selectlevelbypassword(gs))
		return CmdNone;
	    break;
	  default:
	    continue;
	}
	drawscreen(TRUE);
    }
}

/* Get a key command from the user at the completion of the current
 * level.
 */
static int endinput(gamespec *gs)
{
    char	yn[2];
    int		bscore = 0, tscore = 0;
    long	gscore = 0;
    int		n;
    int		cmd = CmdNone;

    if (gs->status < 0) {
	if (melindawatching(gs) && secondsplayed() >= 10) {
	    ++gs->melindacount;
	    if (gs->melindacount >= 10) {
		yn[0] = '\0';
		setgameplaymode(BeginInput);
		n = displayinputprompt("Skip level?", yn, 1,
				       INPUT_YESNO, yninputcallback);
		setgameplaymode(EndInput);
		if (n && *yn == 'Y') {
		    passwordseen(gs, gs->currentgame + 1);
		    changecurrentgame(gs, +1);
		}
		gs->melindacount = 0;
		return TRUE;
	    }
	}
    } else {
	getscoresforlevel(&gs->series, gs->currentgame,
			  &bscore, &tscore, &gscore);
    }

    cmd = displayendmessage(bscore, tscore, gscore, gs->status);

    for (;;) {
	if (cmd == CmdNone)
	    cmd = input(TRUE);
	switch (cmd) {
	  case CmdPrev10:	changecurrentgame(gs, -10);	return TRUE;
	  case CmdPrevLevel:	changecurrentgame(gs, -1);	return TRUE;
	  case CmdPrev:		changecurrentgame(gs, -1);	return TRUE;
	  case CmdSameLevel:					return TRUE;
	  case CmdSame:						return TRUE;
	  case CmdNextLevel:	changecurrentgame(gs, +1);	return TRUE;
	  case CmdNext:		changecurrentgame(gs, +1);	return TRUE;
	  case CmdNext10:	changecurrentgame(gs, +10);	return TRUE;
	  case CmdGotoLevel:	selectlevelbypassword(gs);	return TRUE;
	  case CmdPlayback:					return TRUE;
	  case CmdSeeScores:	showscores(gs);			return TRUE;
	  case CmdSeeSolutionFiles: showsolutionfiles(gs);	return TRUE;
	  case CmdKillSolution:					return TRUE;
	  case CmdHelp:		dohelp(Help_KeysBetweenGames);	return TRUE;
	  case CmdQuitLevel:					return FALSE;
	  case CmdQuit:						exit(0);
	  case CmdCheckSolution:
	  case CmdProceed:
	    if (gs->status > 0) {
		if (islastinseries(gs, gs->currentgame))
		    gs->enddisplay = TRUE;
		else
		    changecurrentgame(gs, +1);
	    }
	    return TRUE;
	  case CmdReplSolution:
	    if (issolved(gs, gs->currentgame))
		replaceablesolution(gs, -1);
	    else
		bell();
	    return TRUE;
	}
	cmd = CmdNone;
    }
}

/* Get a key command from the user at the completion of the current
 * series.
 */
static int finalinput(gamespec *gs)
{
    int	cmd;

    for (;;) {
	cmd = input(TRUE);
	switch (cmd) {
	  case CmdSameLevel:
	  case CmdSame:
	    return TRUE;
	  case CmdPrevLevel:
	  case CmdPrev:
	  case CmdNextLevel:
	  case CmdNext:
	    setcurrentgame(gs, 0);
	    return TRUE;
	  case CmdQuit:
	    exit(0);
	  default:
	    return FALSE;
	}
    }
}

/* Play the current level, using firstcmd as the initial key command,
 * and returning when the level's play ends. The return value is FALSE
 * if play ended because the user restarted or changed the current
 * level (indicating that the program should not prompt the user
 * before continuing). If the return value is TRUE, the gamespec
 * structure's status field will contain the return value of the last
 * call to doturn() -- i.e., positive if the level was completed
 * successfully, negative if the level ended unsuccessfully. Likewise,
 * the gamespec structure will be updated if the user ended play by
 * changing the current level.
 */
static int playgame(gamespec *gs, int firstcmd)
{
    int	render, lastrendered;
    int	cmd, n;

    cmd = firstcmd;
    if (cmd == CmdProceed)
	cmd = CmdNone;

    gs->status = 0;
    setgameplaymode(BeginPlay);
    render = lastrendered = TRUE;
    for (;;) {
	n = doturn(cmd);
	drawscreen(render);
	lastrendered = render;
	if (n)
	    break;
	render = waitfortick() || noframeskip;
	cmd = input(FALSE);
	if (cmd == CmdQuitLevel) {
	    quitgamestate();
	    n = -2;
	    break;
	}
	if (!(cmd >= CmdMoveFirst && cmd <= CmdMoveLast)) {
	    switch (cmd) {
	      case CmdPreserve:					break;
	      case CmdPrevLevel:		n = -1;		goto quitloop;
	      case CmdNextLevel:		n = +1;		goto quitloop;
	      case CmdSameLevel:		n = 0;		goto quitloop;
	      case CmdDebugCmd1:				break;
	      case CmdDebugCmd2:				break;
	      case CmdQuit:					exit(0);
	      case CmdVolumeUp:
		changevolume(+2, TRUE);
		cmd = CmdNone;
		break;
	      case CmdVolumeDown:
		changevolume(-2, TRUE);
		cmd = CmdNone;
		break;
	      case CmdPauseGame:
		setgameplaymode(SuspendPlayShuttered);
		drawscreen(TRUE);
		setdisplaymsg("(paused)", 1, 1);
		for (;;) {
		    switch (input(TRUE)) {
		      case CmdQuit:		exit(0);
		      case CmdPauseGame:	break;
		      default:			continue;
		    }
		    break;
		}
		setgameplaymode(ResumePlay);
		cmd = CmdNone;
		break;
	      case CmdHelp:
		setgameplaymode(SuspendPlay);
		dohelp(Help_KeysDuringGame);
		setgameplaymode(ResumePlay);
		cmd = CmdNone;
		break;
	      case CmdCheatNorth:     case CmdCheatWest:	break;
	      case CmdCheatSouth:     case CmdCheatEast:	break;
	      case CmdCheatHome:				break;
	      case CmdCheatKeyRed:    case CmdCheatKeyBlue:	break;
	      case CmdCheatKeyYellow: case CmdCheatKeyGreen:	break;
	      case CmdCheatBootsIce:  case CmdCheatBootsSlide:	break;
	      case CmdCheatBootsFire: case CmdCheatBootsWater:	break;
	      case CmdCheatICChip:				break;
	      default:
		cmd = CmdNone;
		break;
	    }
	}
    }
    if (!lastrendered)
	drawscreen(TRUE);
    setgameplaymode(EndPlay);
    if (n > 0)
	if (replacesolution())
	    savesolutions(&gs->series);
    gs->status = n;
    return TRUE;

  quitloop:
    if (!lastrendered)
	drawscreen(TRUE);
    quitgamestate();
    setgameplaymode(EndPlay);
    if (n)
	changecurrentgame(gs, n);
    return FALSE;
}

/* Play back the user's best solution for the current level in real
 * time. Other than the fact that this function runs from a
 * prerecorded series of moves, it has the same behavior as
 * playgame().
 */
static int playbackgame(gamespec *gs)
{
    int	render, lastrendered, n, cmd;
    int secondstoskip = -1, hideandseek = FALSE;
    
    secondstoskip = getreplaysecondstoskip();
    
    drawscreen(TRUE);

    gs->status = 0;
    setgameplaymode(BeginPlay);
    render = lastrendered = TRUE;
    for (;;) {
	n = doturn(CmdNone);
	hideandseek = (secondstoskip > 0  &&  secondsplayed() < secondstoskip);
	if (hideandseek) {
	    lastrendered = FALSE;
	} else {
	    drawscreen(render);
	    lastrendered = render;
	}
	if (n)
	    break;
	if (hideandseek) {
	    advancetick();
	    render = TRUE;
	    cmd = CmdNone;
	} else {
	    render = waitfortick() || noframeskip;
	    cmd = input(FALSE);
	}
	switch (cmd) {
	  case CmdSeek:
	  case CmdPrev10:
	  case CmdNext10:
	    if (cmd == CmdSeek) {
	        secondstoskip = getreplaysecondstoskip();
	    } else {
	        secondstoskip = secondsplayed() + ((cmd == CmdNext10) ? +10 : -10);
	    }
	    quitgamestate();
	    setgameplaymode(EndPlay);
	    gs->playmode = Play_None;
	    endgamestate();
	    initgamestate(gs->series.games + gs->currentgame,
			  gs->series.ruleset);
	    prepareplayback();
	    gs->playmode = Play_Back;
	    gs->status = 0;
	    setgameplaymode(BeginPlay);
	    lastrendered = FALSE;
	    break;
	  case CmdPrevLevel:	changecurrentgame(gs, -1);	goto quitloop;
	  case CmdNextLevel:	changecurrentgame(gs, +1);	goto quitloop;
	  case CmdSameLevel:					goto quitloop;
	  case CmdPlayback:					goto quitloop;
	  case CmdQuitLevel:					goto quitloop;
	  case CmdQuit:						exit(0);
	  case CmdVolumeUp:
	    changevolume(+2, TRUE);
	    break;
	  case CmdVolumeDown:
	    changevolume(-2, TRUE);
	    break;
	  case CmdPauseGame:
	    setgameplaymode(SuspendPlay);
	    setdisplaymsg("(paused)", 1, 1);
	    for (;;) {
		switch (input(TRUE)) {
		  case CmdQuit:		exit(0);
		  case CmdPauseGame:	break;
		  default:		continue;
		}
		break;
	    }
	    setgameplaymode(ResumePlay);
	    break;
	  case CmdHelp:
	    setgameplaymode(SuspendPlay);
	    dohelp(Help_None);
	    setgameplaymode(ResumePlay);
	    break;
	}
    }
    if (!lastrendered)
	drawscreen(TRUE);
    setgameplaymode(EndPlay);
    gs->playmode = Play_None;
    if (n < 0)
	replaceablesolution(gs, +1);
    if (n > 0) {
	if (checksolution())
	    savesolutions(&gs->series);
	if (islastinseries(gs, gs->currentgame))
	    n = 0;
    }
    gs->status = n;
    return TRUE;

  quitloop:
    if (!lastrendered)
	drawscreen(TRUE);
    quitgamestate();
    setgameplaymode(EndPlay);
    gs->playmode = Play_None;
    return FALSE;
}

/* Quickly play back the user's best solution for the current level
 * without rendering and without using the timer the keyboard. The
 * playback stops when the solution is finished or gameplay has
 * ended.
 */
static int verifyplayback(gamespec *gs)
{
    int	n;

    gs->status = 0;
    setdisplaymsg("Verifying ...", 1, 0);
    setgameplaymode(BeginVerify);
    for (;;) {
	n = doturn(CmdNone);
	if (n)
	    break;
	advancetick();
	switch (input(FALSE)) {
	  case CmdPrevLevel:	changecurrentgame(gs, -1);	goto quitloop;
	  case CmdNextLevel:	changecurrentgame(gs, +1);	goto quitloop;
	  case CmdSameLevel:					goto quitloop;
	  case CmdPlayback:					goto quitloop;
	  case CmdQuitLevel:					goto quitloop;
	  case CmdQuit:						exit(0);
	}
    }
    gs->playmode = Play_None;
    quitgamestate();
    drawscreen(TRUE);
    setgameplaymode(EndVerify);
    if (n < 0) {
	setdisplaymsg("Invalid solution!", 1, 1);
	replaceablesolution(gs, +1);
    }
    if (n > 0) {
	if (checksolution())
	    savesolutions(&gs->series);
	setdisplaymsg(NULL, 0, 0);
	if (islastinseries(gs, gs->currentgame))
	    n = 0;
    }
    gs->status = n;
    return TRUE;

  quitloop:
    setdisplaymsg(NULL, 0, 0);
    gs->playmode = Play_None;
    setgameplaymode(EndVerify);
    return FALSE;
}

/* Manage a single session of playing the current level, from start to
 * finish. A return value of FALSE indicates that the user is done
 * playing levels from the current series; otherwise, the gamespec
 * structure is updated as necessary upon return.
 */
static int runcurrentlevel(gamespec *gs)
{
    int ret = TRUE;
    int	cmd;
    int	valid, f;

    if (gs->enddisplay) {
	gs->enddisplay = FALSE;
	changesubtitle(NULL);
	setenddisplay();
	drawscreen(TRUE);
	displayendmessage(0, 0, 0, 0);
	endgamestate();
	return finalinput(gs);
    }

    valid = initgamestate(gs->series.games + gs->currentgame,
			  gs->series.ruleset);
    changesubtitle(gs->series.games[gs->currentgame].name);
    passwordseen(gs, gs->currentgame);
    if (!islastinseries(gs, gs->currentgame))
	if (!valid || gs->series.games[gs->currentgame].unsolvable)
	    passwordseen(gs, gs->currentgame + 1);

    cmd = startinput(gs);
    if (cmd == CmdQuitLevel) {
	ret = FALSE;
    } else {
	if (cmd != CmdNone) {
	    if (valid) {
		switch (gs->playmode) {
		  case Play_Normal:	f = playgame(gs, cmd);		break;
		  case Play_Back:	f = playbackgame(gs);		break;
		  case Play_Verify:	f = verifyplayback(gs);		break;
		  default:		f = FALSE;			break;
		}
		if (f)
		    ret = endinput(gs);
	    } else
		bell();
	}
    }

    endgamestate();
    return ret;
}

static int batchverify(gameseries *series, int display)
{
    gamesetup  *game;
    int		valid = 0, invalid = 0;
    int		i, f;

    batchmode = TRUE;

    for (i = 0, game = series->games ; i < series->count ; ++i, ++game) {
	if (!hassolution(game))
	    continue;
	if (initgamestate(game, series->ruleset) && prepareplayback()) {
	    setgameplaymode(BeginVerify);
	    while (!(f = doturn(CmdNone)))
		advancetick();
	    setgameplaymode(EndVerify);
	    if (f > 0) {
		++valid;
		checksolution();
	    } else {
		++invalid;
		game->sgflags |= SGF_REPLACEABLE;
		if (display)
		    printf("Solution for level %d is invalid\n", game->number);
	    }
	}
	endgamestate();
    }

    if (display) {
	if (valid + invalid == 0) {
	    printf("No solutions were found.\n");
	} else {
	    printf("  Valid solutions:%4d\n", valid);
	    printf("Invalid solutions:%4d\n", invalid);
	}
    }
    return invalid;
}

/*
 * Game selection functions
 */

/* Display the full selection of available series to the user as a
 * scrolling list, and permit one to be selected. When one is chosen,
 * pick one of levels to be the current level. All fields of the
 * gamespec structure are initiailzed. If autosel is TRUE, then the
 * function will skip the display if there is only one series
 * available. If defaultseries is not NULL, and matches the name of
 * one of the series in the array, then the scrolling list will be
 * initialized with that series selected. If defaultlevel is not zero,
 * and a level in the selected series that the user is permitted to
 * access matches it, then that level will be thhe initial current
 * level. The return value is zero if nothing was selected, negative
 * if an error occurred, or positive otherwise.
 */
static int selectseriesandlevel(gamespec *gs, seriesdata *series, int autosel,
				char const *defaultseries, int defaultlevel)
{
    int	okay, f, n;

    if (series->count < 1) {
	errmsg(NULL, "no level sets found");
	return -1;
    }

    okay = TRUE;
    if (series->count == 1 && autosel) {
	getseriesfromlist(&gs->series, series->list, 0);
    } else {
	n = 0;
	if (defaultseries) {
	    n = series->count;
	    while (n)
		if (!strcmp(series->list[--n].filebase, defaultseries))
		    break;
	}
	for (;;) {
	    f = displaylist("   Welcome to Tile World. Type ? or F1 for help.",
			    &series->table, &n, LIST_SERIES, scrollinputcallback);
	    if (f == CmdProceed) {
		getseriesfromlist(&gs->series, series->list, n);
		okay = TRUE;
		break;
	    } else if (f == CmdQuitLevel) {
		okay = FALSE;
		break;
	    } else if (f == CmdHelp) {
		pushsubtitle("Help");
		dohelp(Help_First);
		popsubtitle();
	    }
	}
    }
    freeserieslist(series->list, series->count, &series->table);
    if (!okay)
	return 0;

    if (!readseriesfile(&gs->series)) {
	errmsg(gs->series.filebase, "cannot read data file");
	freeseriesdata(&gs->series);
	return -1;
    }
    if (gs->series.count < 1) {
	errmsg(gs->series.filebase, "no levels found in data file");
	freeseriesdata(&gs->series);
	return -1;
    }

    gs->enddisplay = FALSE;
    gs->playmode = Play_None;
    gs->usepasswds = usepasswds && !(gs->series.gsflags & GSF_IGNOREPASSWDS);
    gs->currentgame = -1;
    gs->melindacount = 0;

    if (defaultlevel) {
	n = findlevelinseries(&gs->series, defaultlevel, NULL);
	if (n >= 0) {
	    gs->currentgame = n;
	    if (gs->usepasswds &&
			!(gs->series.games[n].sgflags & SGF_HASPASSWD))
		changecurrentgame(gs, -1);
	}
    }
    if (gs->currentgame < 0) {
	gs->currentgame = 0;
	for (n = 0 ; n < gs->series.count ; ++n) {
	    if (!issolved(gs, n)) {
		gs->currentgame = n;
		break;
	    }
	}
    }

    return +1;
}

/* Get the list of available series and permit the user to select one
 * to play. If lastseries is not NULL, use that series as the default.
 * The return value is zero if nothing was selected, negative if an
 * error occurred, or positive otherwise.
 */
static int choosegame(gamespec *gs, char const *lastseries)
{
    seriesdata	s;

    if (!createserieslist(NULL, &s.list, &s.count, &s.table))
	return -1;
    return selectseriesandlevel(gs, &s, FALSE, lastseries, 0);
}

/*
 * Initialization functions.
 */

/* Set the four directories that the program uses (the series
 * directory, the series data directory, the resource directory, and
 * the save directory).  Any or all of the arguments can be NULL,
 * indicating that the default value should be used. The environment
 * variables TWORLDDIR, TWORLDSAVEDIR, and HOME can define the default
 * values. If any or all of these are unset, the program will use the
 * default values it was compiled with.
 */
static void initdirs(char const *series, char const *seriesdat,
		     char const *res, char const *save)
{
    unsigned int	maxpath;
    char const	       *root = NULL;
    char const	       *dir;

    maxpath = getpathbufferlen();
    if (series && strlen(series) >= maxpath) {
	errmsg(NULL, "Data (-D) directory name is too long;"
		     " using default value instead");
	series = NULL;
    }
    if (seriesdat && strlen(seriesdat) >= maxpath) {
	errmsg(NULL, "Configured data (-C) directory name is too long;"
		     " using default value instead");
	seriesdat = NULL;
    }
    if (res && strlen(res) >= maxpath) {
	errmsg(NULL, "Resource (-R) directory name is too long;"
		     " using default value instead");
	res = NULL;
    }
    if (save && strlen(save) >= maxpath) {
	errmsg(NULL, "Save (-S) directory name is too long;"
		     " using default value instead");
	save = NULL;
    }
    if (!save && (dir = getenv("TWORLDSAVEDIR")) && *dir) {
	if (strlen(dir) < maxpath)
	    save = dir;
	else
	    warn("Value of environment variable TWORLDSAVEDIR is too long");
    }

    if (!res || !series || !seriesdat) {
	if ((dir = getenv("TWORLDDIR")) && *dir) {
	    if (strlen(dir) < maxpath - 8)
		root = dir;
	    else
		warn("Value of environment variable TWORLDDIR is too long");
	}
	if (!root) {
#ifdef ROOTDIR
	    root = ROOTDIR;
#else
	    root = ".";
#endif
	}
    }

    resdir = getpathbuffer();
    if (res)
	strcpy(resdir, res);
    else
	combinepath(resdir, root, "res");

    seriesdir = getpathbuffer();
    if (series)
	strcpy(seriesdir, series);
    else
	combinepath(seriesdir, root, "sets");

    seriesdatdir = getpathbuffer();
    if (seriesdat)
	strcpy(seriesdatdir, seriesdat);
    else
	combinepath(seriesdatdir, root, "data");

    savedir = getpathbuffer();
    if (!save) {
#ifdef SAVEDIR
	save = SAVEDIR;
#else
	if ((dir = getenv("HOME")) && *dir && strlen(dir) < maxpath - 8)
	    combinepath(savedir, dir, ".tworld");
	else
	    combinepath(savedir, root, "save");

#endif
    } else {
	strcpy(savedir, save);
    }
}

/* Parse the command-line options and arguments, and initialize the
 * user-controlled options.
 */
static int initoptionswithcmdline(int argc, char *argv[], startupdata *start)
{
    cmdlineinfo	opts;
    char const *optresdir = NULL;
    char const *optseriesdir = NULL;
    char const *optseriesdatdir = NULL;
    char const *optsavedir = NULL;
    char	buf[256];
    int		listdirs, pedantic;
    int		ch, n;
    char       *p;

    start->filename = getpathbuffer();
    *start->filename = '\0';
    start->savefilename = NULL;
    start->levelnum = 0;
    start->listseries = FALSE;
    start->listscores = FALSE;
    start->listtimes = FALSE;
    start->batchverify = FALSE;
    listdirs = FALSE;
    pedantic = FALSE;
    mudsucking = 1;
    soundbufsize = 0;
    volumelevel = -1;

    initoptions(&opts, argc - 1, argv + 1, "abD:dFfHhL:lm:n:PpqR:rS:stVv");
    while ((ch = readoption(&opts)) >= 0) {
	switch (ch) {
	  case 0:
	    if (start->savefilename && start->levelnum) {
		fprintf(stderr, "too many arguments: %s\n", opts.val);
		printtable(stderr, yowzitch);
		return FALSE;
	    }
	    if (!start->levelnum && (n = (int)strtol(opts.val, &p, 10)) > 0
				 && *p == '\0') {
		start->levelnum = n;
	    } else if (*start->filename) {
		start->savefilename = getpathbuffer();
		sprintf(start->savefilename, "%.*s", getpathbufferlen(),
						     opts.val);
	    } else {
		sprintf(start->filename, "%.*s", getpathbufferlen(), opts.val);
	    }
	    break;
	  case 'D':	optseriesdatdir = opts.val;			break;
	  case 'L':	optseriesdir = opts.val;			break;
	  case 'R':	optresdir = opts.val;				break;
	  case 'S':	optsavedir = opts.val;				break;
	  case 'H':	showhistogram = !showhistogram;			break;
	  case 'f':	noframeskip = !noframeskip;			break;
	  case 'F':	fullscreen = !fullscreen;			break;
	  case 'p':	usepasswds = !usepasswds;			break;
	  case 'q':	silence = !silence;				break;
	  case 'r':	readonly = !readonly;				break;
	  case 'P':	pedantic = !pedantic;				break;
	  case 'a':	++soundbufsize;					break;
	  case 'd':	listdirs = TRUE;				break;
	  case 'l':	start->listseries = TRUE;			break;
	  case 's':	start->listscores = TRUE;			break;
	  case 't':	start->listtimes = TRUE;			break;
	  case 'b':	start->batchverify = TRUE;			break;
	  case 'm':	mudsucking = atoi(opts.val);			break;
	  case 'n':	volumelevel = atoi(opts.val);			break;
	  case 'h':	printtable(stdout, yowzitch); 	   exit(EXIT_SUCCESS);
	  case 'v':	puts(VERSION);		 	   exit(EXIT_SUCCESS);
	  case 'V':	printtable(stdout, vourzhon); 	   exit(EXIT_SUCCESS);
	  case ':':
	    fprintf(stderr, "option requires an argument: -%c\n", opts.opt);
	    printtable(stderr, yowzitch);
	    return FALSE;
	  case '?':
	    fprintf(stderr, "unrecognized option: -%c\n", opts.opt);
	    printtable(stderr, yowzitch);
	    return FALSE;
	  default:
	    printtable(stderr, yowzitch);
	    return FALSE;
	}
    }

    if (pedantic)
	setpedanticmode();

    initdirs(optseriesdir, optseriesdatdir, optresdir, optsavedir);
    if (listdirs) {
	printdirectories();
	exit(EXIT_SUCCESS);
    }

    if (*start->filename && !start->savefilename) {
	if (loadsolutionsetname(start->filename, buf) > 0) {
	    start->savefilename = getpathbuffer();
	    strcpy(start->savefilename, start->filename);
	    strcpy(start->filename, buf);
	}
    }

    if (start->listscores || start->listtimes || start->batchverify
					      || start->levelnum)
	if (!*start->filename)
	    strcpy(start->filename, "chips.dat");

    return TRUE;
}

/* Run the initialization routines of oshw and the resource module.
 */
static int initializesystem(void)
{
#ifdef NDEBUG
    mudsucking = 1;
#endif
    setmudsuckingfactor(mudsucking);
    if (!oshwinitialize(silence, soundbufsize, showhistogram, fullscreen))
	return FALSE;
    if (!initresources())
	return FALSE;
    setkeyboardrepeat(TRUE);
    if (volumelevel >= 0)
	setvolume(volumelevel, FALSE);
    return TRUE;
}

/* Time for everyone to clean up and go home.
 */
static void shutdownsystem(void)
{
    shutdowngamestate();
    freeallresources();
    free(resdir);
    free(seriesdir);
    free(seriesdatdir);
    free(savedir);
}

/* Determine what to play. A list of available series is drawn up; if
 * only one is found, it is selected automatically. Otherwise, if the
 * listseries option is TRUE, the available series are displayed on
 * stdout and the program exits. Otherwise, if listscores or listtimes
 * is TRUE, the scores or times for a single series is display on
 * stdout and the program exits. (These options need to be checked for
 * before initializing the graphics subsystem.) Otherwise, the
 * selectseriesandlevel() function handles the rest of the work. Note
 * that this function is only called during the initial startup; if
 * the user returns to the series list later on, the choosegame()
 * function is called instead.
 */
static int choosegameatstartup(gamespec *gs, startupdata const *start)
{
    seriesdata	series;
    tablespec	table;
    int		n;

    if (!createserieslist(start->filename,
			  &series.list, &series.count, &series.table))
	return -1;

    free(start->filename);

    if (series.count <= 0) {
	errmsg(NULL, "no level sets found");
	return -1;
    }

    if (start->listseries) {
	printtable(stdout, &series.table);
	if (!series.count)
	    puts("(no files)");
	return 0;
    }

    if (series.count == 1) {
	if (start->savefilename)
	    series.list[0].savefilename = start->savefilename;
	if (!readseriesfile(series.list)) {
	    errmsg(series.list[0].filebase, "cannot read level set");
	    return -1;
	}
	if (start->batchverify) {
	    n = batchverify(series.list, !silence && !start->listtimes
						  && !start->listscores);
	    if (silence)
		exit(n > 100 ? 100 : n);
	    else if (!start->listtimes && !start->listscores)
		return 0;
	}
	if (start->listscores) {
	    if (!createscorelist(series.list, usepasswds, '0',
				 NULL, NULL, &table))
		return -1;
	    freeserieslist(series.list, series.count, &series.table);
	    printtable(stdout, &table);
	    freescorelist(NULL, &table);
	    return 0;
	}
	if (start->listtimes) {
	    if (!createtimelist(series.list,
				series.list->ruleset == Ruleset_MS ? 10 : 100,
				'0', NULL, NULL, &table))
		return -1;
	    freeserieslist(series.list, series.count, &series.table);
	    printtable(stdout, &table);
	    freetimelist(NULL, &table);
	    return 0;
	}
    }

    if (!initializesystem()) {
	errmsg(NULL, "cannot initialize program due to previous errors");
	return -1;
    }

    return selectseriesandlevel(gs, &series, TRUE, NULL, start->levelnum);
}

/*
 * The main function.
 */

int tworld(int argc, char *argv[])
{
    startupdata	start;
    gamespec	spec;
    char	lastseries[sizeof spec.series.filebase];
    int		f;

    if (!initoptionswithcmdline(argc, argv, &start))
	return EXIT_FAILURE;

    f = choosegameatstartup(&spec, &start);
    if (f < 0)
	return EXIT_FAILURE;
    else if (f == 0)
	return EXIT_SUCCESS;

    do {
	pushsubtitle(NULL);
	while (runcurrentlevel(&spec)) ;
	popsubtitle();
	cleardisplay();
	strcpy(lastseries, spec.series.filebase);
	freeseriesdata(&spec.series);
	f = choosegame(&spec, lastseries);
    } while (f > 0);

    shutdownsystem();
    return f == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
