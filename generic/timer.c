/* timer.c: Game timing functions.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	<stdlib.h>
#include	<stdio.h>
#include	"../gen.h"
#include	"../oshw.h"
#include	"generic.h"

/* By default, a second of game time lasts for 1000 milliseconds of
 * real time.
 */
static int	mspertick = 1000 / TICKS_PER_SECOND;

/* The tick counter.
 */
static int	utick = 0;

/* The time of the next tick.
 */
static int	nexttickat = 0;

/* A histogram of how many milliseconds the program spends sleeping
 * per tick.
 */
static int	showhistogram = FALSE;
static unsigned	hist[100];

/* Set the length (in real time) of a second of game time. A value of
 * zero selects the default length of one second.
 */
void settimersecond(int ms)
{
    mspertick = (ms ? ms : 1000) / TICKS_PER_SECOND;
}

/* Change the current timer setting. If action is positive, the timer
 * is started (or resumed). If action is negative, the timer is
 * stopped if it is running and the counter is reset to zero. If
 * action is zero, the timer is stopped if it is running, and the
 * counter remains at its current setting.
 */
void settimer(int action)
{
    if (action < 0) {
	nexttickat = 0;
	utick = 0;
    } else if (action > 0) {
	if (nexttickat < 0)
	    nexttickat = TW_GetTicks() - nexttickat;
	else
	    nexttickat = TW_GetTicks() + mspertick;
    } else {
	if (nexttickat > 0)
	    nexttickat = TW_GetTicks() - nexttickat;
    }
}

/* Return the number of ticks since the timer was last reset.
 */
int gettickcount(void)
{
    return (int)utick;
}

/* Put the program to sleep until the next timer tick. If we've
 * already missed a timer tick, then wait for the next one.
 */
int waitfortick(void)
{
    int	ms;

    ms = nexttickat - TW_GetTicks();
    if (showhistogram)
	if (ms < (int)(sizeof hist / sizeof *hist))
	    ++hist[ms >= 0 ? ms + 1 : 0];

    if (ms <= 0) {
	++utick;
	nexttickat += mspertick;
	return FALSE;
    }

    while (ms < 0)
	ms += mspertick;

    TW_Delay(ms);

    ++utick;
    nexttickat += mspertick;
    return TRUE;
}

/* Move to the next timer tick without waiting.
 */
int advancetick(void)
{
    return ++utick;
}

/* At shutdown time, display the histogram data on stdout.
 */
static void shutdown(void)
{
    unsigned long	n;
    int			i;

    settimer(-1);

    if (showhistogram) {
	n = 0;
	for (i = 0 ; i < (int)(sizeof hist / sizeof *hist) ; ++i)
	    n += hist[i];
	if (n) {
	    printf("Histogram of idle time (ms/tick)\n");
	    if (hist[0])
		printf("NEG: %.1f%%\n", (hist[0] * 100.0) / n);
	    for (i = 1 ; i < (int)(sizeof hist / sizeof *hist) ; ++i)
		if (hist[i])
		    printf("%3d: %.1f%%\n", i - 1, (hist[i] * 100.0) / n);
	}
    }
}

/* Initialize and reset the timer.
 */
int _generictimerinitialize(int _showhistogram)
{
    showhistogram = _showhistogram;
    atexit(shutdown);
    settimer(-1);
    return TRUE;
}
