/* score.h: Calculating and formatting the scores.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_score_h_
#define	_score_h_

#include	"defs.h"

/* Return the user's scores for a given level. The last three arguments
 * receive the base score for the level, the time bonus for the level,
 * and the total score for the series.
 */
extern int getscoresforlevel(gameseries const *series, int level,
			     int *base, int *bonus, long *total);

/* Produce a table showing the player's scores for the given series,
 * formatted in columns. Each level in the series is listed in a
 * separate row, with a header row and an extra row at the end giving
 * a grand total. The pointer pointed to by plevellist receives an
 * array of level indexes to match the rows of the table (less one for
 * the header row), or -1 if no level is displayed in that row. If
 * usepasswds is TRUE, levels for which the user has not learned the
 * password will either not be included or will show no title. FALSE
 * is returned if an error occurs.
 */
extern int createscorelist(gameseries const *series,
			   int usepasswds, char zchar,
			   int **plevellist, int *pcount, tablespec *table);

/* Produce a table showing the player's times for the given series. If
 * showpartial is zero, the times will be rounded to second values.
 * Otherwise, showpartial should be a power of ten, and the function
 * will attempt to provide fractional amounts to the appropriate
 * number of digits.
 */
extern int createtimelist(gameseries const *series,
			  int showpartial, char zchar,
			  int **plevellist, int *pcount, tablespec *table);

/* Free all memory allocated by the above functions.
 */
extern void freescorelist(int *plevellist, tablespec *table);
#define freetimelist freescorelist

#endif
