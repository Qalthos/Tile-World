/* random.h: The game's random-number generator.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_random_h_
#define	_random_h_

#include	"defs.h"

/* Create a fresh PRNG.
 */
extern prng createprng(void);

/* Mark an existing PRNG as beginning a new sequence.
 */
extern void resetprng(prng *gen);

/* Restart an existing PRNG upon a predetermined sequence.
 */
extern void restartprng(prng *gen, unsigned long initial);

/* Retrieve the original seed value of the current sequence.
 */
#define	getinitialseed(gen)	((gen)->initial)

/* Return a random integer between zero and three, inclusive.
 */
extern int random4(prng *gen);

/* Randomly select one of the three integer arguments as the return
 * value.
 */
extern int randomof3(prng *gen, int a, int b, int c);

/* Randomly permute an array of three integers.
 */
extern void randomp3(prng *gen, int *array);

/* Randomly permute an array of four integers.
 */
extern void randomp4(prng *gen, int *array);

#endif
