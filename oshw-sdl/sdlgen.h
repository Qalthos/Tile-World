/* sdlgen.h: The internal shared definitions of the SDL OS/hardware layer.
 * 
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_sdlgen_h_
#define	HEADER_sdlgen_h_

#include	"SDL.h"
#include	"../gen.h"
#include	"../oshw.h"
#include	"../generic/generic.h"

/* Structure to hold the definition of a font.
 */
typedef	struct fontinfo {
    signed char		h;		/* height of each character */
    signed char		w[256];		/* width of each character */
    void	       *memory;		/* memory allocated for the font */
    unsigned char      *bits[256];	/* pointers to each glyph */
} fontinfo;

/* Structure to hold a font's colors.
 */
typedef	struct fontcolors { Uint32 c[3]; } fontcolors;

#define	bkgndcolor(fc)	((fc).c[0])	/* the background color */
#define	halfcolor(fc)	((fc).c[1])	/* the antialiasing color */
#define	textcolor(fc)	((fc).c[2])	/* the main color of the glyphs */

/* Flags to the puttext function.
 */
#define	PT_CENTER	0x0001		/* center the text horizontally */
#define	PT_RIGHT	0x0002		/* right-align the text */
#define	PT_MULTILINE	0x0004		/* span lines & break at whitespace */
#define	PT_UPDATERECT	0x0008		/* return the unused area in rect */
#define	PT_CALCSIZE	0x0010		/* determine area needed for text */
#define	PT_DIM		0x0020		/* draw using the dim text color */
#define	PT_HILIGHT	0x0040		/* draw using the bold text color */

/*
 * Values global to this module. All the globals are placed in here,
 * in order to minimize pollution of the main module's namespace.
 */

typedef	struct oshwglobals
{
    /* 
     * Shared variables.
     */

    fontcolors		textclr;	/* color triplet for normal text */
    fontcolors		dimtextclr;	/* color triplet for dim text */
    fontcolors		hilightclr;	/* color triplet for bold text */
    fontinfo		font;		/* the font */

    /* 
     * Shared functions.
     */

     /* Display a line (or more) of text in the program's font. The
     * text is clipped to area if necessary. If area is taller than
     * the font, the topmost line is used. len specifies the number of
     * characters to render; -1 can be used if text is NUL-terminated.
     * flags is some combination of PT_* flags defined above. When the
     * PT_CALCSIZE flag is set, no drawing is done; instead the w and
     * h fields of area area changed to define the smallest rectangle
     * that encloses the text that would have been rendered. (If
     * PT_MULTILINE is also set, only the h field is changed.) If
     * PT_UPDATERECT is set instead, then the h field is changed, so
     * as to exclude the rectangle that was drawn in.
     */
    void (*puttextfunc)(SDL_Rect *area, char const *text, int len, int flags);

    /* Determine the widths necessary to display the columns of the
     * given table. area specifies an enclosing rectangle for the
     * complete table. The return value is an array of rectangles, one
     * for each column of the table. The rectangles y-coordinates and
     * heights are taken from area, and the x-coordinates and widths
     * are calculated so as to best render the columns of the table in
     * the given space. The caller has the responsibility of freeing
     * the returned array.
     */
    SDL_Rect *(*measuretablefunc)(SDL_Rect const *area,
				  tablespec const *table);

    /* Draw a single row of the given table. cols is an array of
     * rectangles, one for each column. Each rectangle is altered by
     * the function as per puttext's PT_UPDATERECT behavior. row
     * points to an integer indicating the first table entry of the
     * row to display; upon return, this value is updated to point to
     * the first entry following the row. flags can be set to PT_DIM
     * and/or PT_HIGHLIGHT; the values will be applied to every entry
     * in the row.
     */
    int (*drawtablerowfunc)(tablespec const *table, SDL_Rect *cols,
			    int *row, int flags);

} oshwglobals;

/* oshw's structure of globals.
 */
extern oshwglobals sdlg;

/* Some convenience macros for the above functions.
 */
#define	puttext			(*sdlg.puttextfunc)
#define	measuretable		(*sdlg.measuretablefunc)
#define	drawtablerow		(*sdlg.drawtablerowfunc)
#define	createscroll		(*sdlg.createscrollfunc)
#define	scrollmove		(*sdlg.scrollmovefunc)

/* The initialization functions for the various modules.
 */
extern int _sdlresourceinitialize(void);
extern int _sdltextinitialize(void);
extern int _sdlinputinitialize(void);
extern int _sdloutputinitialize(int fullscreen);

#endif
