/* unslist.h: Functions to manage the list of unsolvable levels.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_unslist_h_
#define	_unslist_h_

#include	"defs.h"

/* Read the list of unsolvable levels from the given filename. If the
 * filename does not contain a path, then the function looks for the
 * file in the resource directory and the user's save directory.
 */
extern int loadunslistfromfile(char const *filename);

/* Look up the given level in the list of unsolvable levels. TRUE is
 * returned if the level is found in the list, FALSE otherwise. (Since
 * the setname is not supplied, this function is potentially less
 * reliable.) If the level is in the list and note is not NULL, then
 * the buffer it points to will receive a copy of the level's
 * annotation.
 */
extern int islevelunsolvable(gamesetup const *game, char *note);

/* Look up all the levels in the given series, and mark the ones that
 * appear in the list of unsolvable levels by initializing the
 * unsolvable field. Levels that do not appear in the list will have
 * their unsolvable fields explicitly set to NULL. The number of
 * unsolvable levels is returned.
 */
extern int markunsolvablelevels(gameseries *series);

/* Free all memory associated with the list of unsolvable levels.
 */
extern void clearunslist(void);

#endif
