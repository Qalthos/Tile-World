/* encoding.h: Functions to read the level data.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_encoding_h_
#define	_encoding_h_

#include	"state.h"

/* Initialize the gamestate by reading the level data from the setup.
 * FALSE is returned if the level data is invalid.
 */
extern int expandleveldata(gamestate *state);

/* Return the setup for a small level, created at runtime, that can be
 * displayed at the completion of a series.
 */
extern void getenddisplaysetup(gamestate *state);

#endif
