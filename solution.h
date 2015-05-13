/* solution.c: Functions for reading and writing the solution files.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_solution_h_
#define	HEADER_solution_h_

#include	"defs.h"
#include	"fileio.h"

/* A structure holding all the data needed to reconstruct a solution.
 */
typedef	struct solutioninfo {
    actlist		moves;		/* the actual moves of the solution */
    unsigned long	rndseed;	/* the PRNG's initial seed */
    unsigned long	flags;		/* other flags (currently unused) */
    unsigned char	rndslidedir;	/* random slide's initial direction */
    signed char		stepping;	/* the timer offset */
} solutioninfo;

/* The path of the directory containing the user's solution files.
 */
extern char    *savedir;

/* No file modification will be done unless this variable is FALSE.
 */
extern int	readonly;

/* Initialize or reinitialize list as empty.
 */
extern void initmovelist(actlist *list);

/* Append move to the end of list.
 */
extern void addtomovelist(actlist *list, action move);

/* Make to an independent copy of from.
 */
extern void copymovelist(actlist *to, actlist const *from);

/* Deallocate list.
 */
extern void destroymovelist(actlist *list);

/* Expand a level's solution data into the actual solution, including
 * the full list of moves. FALSE is returned if the solution is
 * invalid or absent.
 */
extern int expandsolution(solutioninfo *solution, gamesetup const *game);

/* Take the given solution and compress it, storing the compressed
 * data as part of the level's setup. FALSE is returned if an error
 * occurs. (It is not an error to compress the null solution.)
 */
extern int contractsolution(solutioninfo const *solution, gamesetup *game);

/* Read all the solutions for the given series into memory. FALSE is
 * returned if an error occurs. Note that it is not an error for the
 * solution file to not exist.
 */
extern int readsolutions(gameseries *series);

/* Write out all the solutions for the given series. The solution file
 * is created if it does not currently exist. The solution file's
 * directory is also created if it does not currently exist. (Nothing
 * is done if the directory's name has been unset, however.) FALSE is
 * returned if an error occurs.
 */
extern int savesolutions(gameseries *series);

/* Free all memory allocated for storing the game's solutions, and mark
 * the levels as being unsolved.
 */
extern void clearsolutions(gameseries *series);

/* Read the solution file at filename to see if it contains a set
 * name. If so, copy it into buffer and return its length in bytes (up
 * to 255). Zero is returned if the solution file contains no set
 * name. A negative value is returned if the file cannot be read or is
 * not a valid solution file.
 */
extern int loadsolutionsetname(char const *filename, char *buffer);

/* Produce a list of available solution files associated with the
 * given series (i.e. that have the name of the series as their
 * prefix). An array of filenames is returned through pfilelist, the
 * array's size is returned through pcount, and the table of the
 * filenames is returned through table. FALSE is returned if no table
 * was returned. If morethanone is TRUE, and less than two solution
 * files are found, FALSE is returned and the table is not created.
 */
extern int createsolutionfilelist(gameseries const *series, int morethanone,
				  char const ***pfilelist, int *pcount,
				  tablespec *table);

/* Free the memory allocated by createsolutionfilelist().
 */
extern void freesolutionfilelist(char const **filelist, tablespec *table);

#endif
