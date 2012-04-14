/* random.c: The game's random-number generator.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

/* 
 * This module is not here because I don't trust the C library's
 * random-number generator. (In fact, this module uses the linear
 * congruential generator, which is hardly impressive. But this isn't
 * strong cryptography; it's a game.) It is here simply because it is
 * necessary for the game to use the same generator FOREVER. In order
 * for playback of solutions to work correctly, the game must use the
 * same sequence of random numbers as when it was recorded. This would
 * fail if the playback occurred on a version compiled with a
 * different C library's generator. Thus, this module.
 */

#include	<stdlib.h>
#include	<time.h>
#include	"gen.h"
#include	"random.h"

/* The most recently generated random number is stashed here, so that
 * it can provide the initial seed of the next PRNG.
 */
static unsigned long	lastvalue = 0x80000000UL;

/* The standard linear congruential random-number generator needs no
 * introduction.
 */
static unsigned long nextvalue(unsigned long value)
{
    return ((value * 1103515245UL) + 12345UL) & 0x7FFFFFFFUL;
}

/* Move to the next pseudorandom number in the generator's series.
 */
static void nextrandom(prng *gen)
{
    if (gen->shared)
	gen->value = lastvalue = nextvalue(lastvalue);
    else
	gen->value = nextvalue(gen->value);
}

/* Create a new PRNG, reset to the shared sequence.
 */
prng createprng(void)
{
    prng gen;
    resetprng(&gen);
    return gen;
}

/* We start off a fresh series by taking the current time. A few
 * numbers are generated and discarded to work out any biases in the
 * seed value.
 */
void resetprng(prng *gen)
{
    if (lastvalue > 0x7FFFFFFFUL)
	lastvalue = nextvalue(nextvalue(nextvalue(nextvalue(time(NULL)))));
    gen->value = gen->initial = lastvalue;
    gen->shared = TRUE;
}

/* Reset a PRNG to an independent sequence.
 */
void restartprng(prng *gen, unsigned long seed)
{
    gen->value = gen->initial = seed & 0x7FFFFFFFUL;
    gen->shared = FALSE;
}

/* Use the top two bits to get a random number between 0 and 3.
 */
int random4(prng *gen)
{
    nextrandom(gen);
    return gen->value >> 29;
}

/* Randomly select an element from a list of three values.
 */
int randomof3(prng *gen, int a, int b, int c)
{
    int	n;

    nextrandom(gen);
    n = (int)((3.0 * (gen->value & 0x3FFFFFFFUL)) / (double)0x40000000UL);
    return n < 2 ? n < 1 ? a : b : c;
}

/* Randomly permute a list of three values. Two random numbers are
 * used, with the ranges [0,1] and [0,1,2].
 */
void randomp3(prng *gen, int *array)
{
    int	n, t;

    nextrandom(gen);
    n = gen->value >> 30;
    t = array[n];  array[n] = array[1];  array[1] = t;
    n = (int)((3.0 * (gen->value & 0x3FFFFFFFUL)) / (double)0x40000000UL);
    t = array[n];  array[n] = array[2];  array[2] = t;
}

/* Randomly permute a list of four values. Three random numbers are
 * used, with the ranges [0,1], [0,1,2], and [0,1,2,3].
 */
void randomp4(prng *gen, int *array)
{
    int	n, t;

    nextrandom(gen);
    n = gen->value >> 30;
    t = array[n];  array[n] = array[1];  array[1] = t;
    n = (int)((3.0 * (gen->value & 0x0FFFFFFFUL)) / (double)0x10000000UL);
    t = array[n];  array[n] = array[2];  array[2] = t;
    n = (gen->value >> 28) & 3;
    t = array[n];  array[n] = array[3];  array[3] = t;
}
