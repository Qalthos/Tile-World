/* series.h: Functions for finding and reading the series files.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_series_h_
#define	_series_h_

#include	"defs.h"

/* The directory containing the series files.
 */
extern char    *seriesdir;

/* The directory containing the configured data files.
 */
extern char    *seriesdatdir;

/* Load all levels of the given series.
 */
extern int readseriesfile(gameseries *series);

/* Release all resources associated with a gameseries structure.
 */
extern void freeseriesdata(gameseries *series);

/* Produce a list all available data files. pserieslist receives the
 * location of an array of gameseries structures, one per data file
 * successfully found. pcount points to a value that is filled in with
 * the number of the data files. table, if it is not NULL, is filled
 * in with a tabular representation of the list of data files, showing
 * the names of the files and which ruleset each uses, with the first
 * row of the table containing column headres. preferredfile
 * optionally provides the filename or pathname of a single data file.
 * If the preferred data file is found, it will be the only one
 * returned. FALSE is returned if no series files are found. An
 * unrecoverable error will cause the function to abort the program.
 */
extern int createserieslist(char const *preferredfile,
			    gameseries **pserieslist,
			    int *pcount, tablespec *table);

/* Make an independent copy of a single gameseries structure from
 * a list obtained from createserieslist().
 */
extern void getseriesfromlist(gameseries *dest,
			      gameseries const *list, int index);

/* Free the memory used by the table created in createserieslist().
 * Either serieslist or table can be NULL.
 */
extern void freeserieslist(gameseries *serieslist, int count,
			   tablespec *table);

/* A function for looking up a specific level in a series by number
 * and/or password. If number is -1, only the password will be
 * searched for; if passwd is NULL, only the number will be used.  The
 * function returns the index of the game in the series, or -1 if the
 * data could not be matched, or if it matched more than one level
 * (ugh).
 */
extern int findlevelinseries(gameseries const *series,
			     int number, char const *passwd);

#endif
