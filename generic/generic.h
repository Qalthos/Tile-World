/* generic.h: The internal shared definitions of the generic layer.
 * 
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_generic_h_
#define	HEADER_generic_h_

#include "oshwbind.h"

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

struct gamestate;

/* The dimensions of the visible area of the map (in tiles).
 */
#define	NXTILES		9
#define	NYTILES		9

/*
 * Values global to this module. All the globals are placed in here,
 * in order to minimize pollution of the main module's namespace.
 */

typedef	struct genericglobals
{
    /* 
     * Shared variables.
     */

    short		wtile;		/* width of one tile in pixels */
    short		htile;		/* height of one tile in pixels */
    short		cptile;		/* size of one tile in pixels */
    TW_Surface	       *screen;		/* the display */
    TW_Rect		maploc;		/* location of the map in the window */

    /* Coordinates of the NW corner of the visible part of the map
     * (measured in quarter-tiles), or -1 if no map is currently visible.
     */
    int			mapvieworigin;

    /* 
     * Shared functions.
     */

    /* Process all pending events. If wait is TRUE and no events are
     * currently pending, the function blocks until an event arrives.
     */
    void (*eventupdatefunc)(int wait);

    /* A callback function, to be called every time a keyboard key is
     * pressed or released. scancode is an SDL key symbol. down is
     * TRUE if the key was pressed or FALSE if it was released.
     */
    void (*keyeventcallbackfunc)(int scancode, int down);

    /* A callback function, to be called when a mouse button is
     * pressed or released. xpos and ypos give the mouse's location.
     * button is the number of the mouse button. down is TRUE if the
     * button was pressed or FALSE if it was released.
     */
    void (*mouseeventcallbackfunc)(int xpos, int ypos, int button, int down);

    /* Given a pixel's coordinates, return an integer identifying the
     * tile on the map view display under that pixel, or -1 if the
     * pixel is not within the map view.
     */
    int (*windowmapposfunc)(int x, int y);

    /* Render the view of the visible area of the map to the display, with
     * the view position centered on the display as much as possible. The
     * gamestate's map and the list of creatures are consulted to
     * determine what to render.
     */
    void (*displaymapviewfunc)(struct gamestate const *state,
			       TW_Rect disploc);

    /* Draw a tile of the given id at the position (xpos, ypos).
     */
    void (*drawfulltileidfunc)(TW_Surface *dest, int xpos, int ypos, int id);

} genericglobals;

/* generic module's structure of globals.
 */
extern genericglobals geng;

/* Some convenience macros for the above functions.
 */
#define eventupdate		(*geng.eventupdatefunc)
#define	keyeventcallback	(*geng.keyeventcallbackfunc)
#define	mouseeventcallback	(*geng.mouseeventcallbackfunc)
#define	windowmappos		(*geng.windowmapposfunc)
#define	displaymapview		(*geng.displaymapviewfunc)
#define drawfulltileid		(*geng.drawfulltileidfunc)

/* The initialization functions for the various modules.
 */
OSHW_EXTERN int _generictimerinitialize(int showhistogram);
OSHW_EXTERN int _generictileinitialize(void);
OSHW_EXTERN int _genericinputinitialize(void);

#undef OSHW_EXTERN

#endif
