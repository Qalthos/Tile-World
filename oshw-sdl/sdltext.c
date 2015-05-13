/* sdltext.c: Font-rendering functions for SDL.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"SDL.h"
#include	"sdlgen.h"
#include	"../err.h"

/* Accept a bitmap as an 8-bit SDL surface and from it extract the
 * glyphs of a font. (See the documentation included in the Tile World
 * distribution for specifics regarding the bitmap layout.)
 */
static int makefontfromsurface(fontinfo *pf, SDL_Surface *surface)
{
    char		brk[267];
    unsigned char      *p;
    unsigned char      *dest;
    Uint8		foregnd, bkgnd;
    int			pitch, wsum;
    int			ch;
    int			x, y, x0, y0, w;

    if (surface->format->BytesPerPixel != 1)
	return FALSE;

    if (SDL_MUSTLOCK(surface))
	SDL_LockSurface(surface);

    pitch = surface->pitch;
    p = surface->pixels;
    foregnd = p[0];
    bkgnd = p[pitch];
    for (y = 1, p += pitch ; y < surface->h && *p == bkgnd ; ++y, p += pitch) ;
    pf->h = y - 1;

    wsum = 0;
    ch = 32;
    memset(pf->w, 0, sizeof pf->w);
    memset(brk, 0, sizeof brk);
    for (y = 0 ; y + pf->h < surface->h && ch < 256 ; y += pf->h + 1) {
	p = surface->pixels;
	p += y * pitch;
	x0 = 1;
	for (x = 1 ; x < surface->w ; ++x) {
	    if (p[x] == bkgnd)
		continue;
	    w = x - x0;
	    x0 = x + 1;
	    pf->w[ch] = w;
	    wsum += w;
	    ++ch;
	    if (ch == 127)
		ch = 144;
	    else if (ch == 154)
		ch = 160;
	    else if (ch == 256)
		break;
	}
	brk[ch] = 1;
    }

    if (!(pf->memory = calloc(wsum, pf->h)))
	memerrexit();

    x0 = 1;
    y0 = 1;
    dest = pf->memory;
    for (ch = 0 ; ch < 256 ; ++ch) {
	pf->bits[ch] = dest;
	if (pf->w[ch] == 0)
	    continue;
	if (brk[ch]) {
	    x0 = 1;
	    y0 += pf->h + 1;
	}
	p = surface->pixels;
	p += y0 * pitch + x0;
	for (y = 0 ; y < pf->h ; ++y, p += pitch)
	    for (x = 0 ; x < pf->w[ch] ; ++x, ++dest)
		*dest = p[x] == bkgnd ? 0 : p[x] == foregnd ? 2 : 1;
	x0 += pf->w[ch] + 1;
    }

    if (SDL_MUSTLOCK(surface))
	SDL_UnlockSurface(surface);

    return TRUE;
}

/* Given a text and a maximum horizontal space to occupy, return
 * the amount of vertial space needed to render the entire text with
 * word-wrapping.
 */
static int measuremltext(unsigned char const *text, int len, int maxwidth)
{
    int	brk, w, h, n;

    if (len < 0)
	len = strlen((char const*)text);
    h = 0;
    brk = 0;
    for (n = 0, w = 0 ; n < len ; ++n) {
	w += sdlg.font.w[text[n]];
	if (isspace(text[n])) {
	    brk = w;
	} else if (w > maxwidth) {
	    h += sdlg.font.h;
	    if (brk) {
		w -= brk;
		brk = 0;
	    } else {
		w = sdlg.font.w[text[n]];
		brk = 0;
	    }
	}
    }
    if (w)
	h += sdlg.font.h;
    return h;
}

/*
 * Render a single line of pixels of the given text to a locked
 * surface at scanline. w specifies the total number of pixels to
 * render. (Any pixels remaining after the last glyph has been
 * rendered are set to the background color.) y specifies the vertical
 * coordinate of the line to render relative to the font glyphs. A
 * separate function is supplied for each possible surface depth.
 */

static void *drawtextscanline8(Uint8 *scanline, int w, int y, Uint32 *clr,
			       unsigned char const *text, int len)
{
    unsigned char const	       *glyph;
    int				n, x;

    for (n = 0 ; n < len ; ++n) {
	glyph = sdlg.font.bits[text[n]];
	glyph += y * sdlg.font.w[text[n]];
	for (x = 0 ; w && x < sdlg.font.w[text[n]] ; ++x, --w)
	    scanline[x] = (Uint8)clr[glyph[x]];
	scanline += x;
    }
    while (w--)
	*scanline++ = (Uint8)clr[0];
    return scanline;
}

static void *drawtextscanline16(Uint16 *scanline, int w, int y, Uint32 *clr,
				unsigned char const *text, int len)
{
    unsigned char const	       *glyph;
    int				n, x;

    for (n = 0 ; n < len ; ++n) {
	glyph = sdlg.font.bits[text[n]];
	glyph += y * sdlg.font.w[text[n]];
	for (x = 0 ; w && x < sdlg.font.w[text[n]] ; ++x, --w)
	    scanline[x] = (Uint16)clr[glyph[x]];
	scanline += x;
    }
    while (w--)
	*scanline++ = (Uint16)clr[0];
    return scanline;
}

static void *drawtextscanline24(Uint8 *scanline, int w, int y, Uint32 *clr,
				unsigned char const *text, int len)
{
    unsigned char const	       *glyph;
    Uint32			c;
    int				n, x;

    for (n = 0 ; n < len ; ++n) {
	glyph = sdlg.font.bits[text[n]];
	glyph += y * sdlg.font.w[text[n]];
	for (x = 0 ; w && x < sdlg.font.w[text[n]] ; ++x, --w) {
	    c = clr[glyph[x]];
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	    *scanline++ = (Uint8)(c >> 16);
	    *scanline++ = (Uint8)(c >> 8);
	    *scanline++ = (Uint8)c;
#else
	    *scanline++ = (Uint8)c;
	    *scanline++ = (Uint8)(c >> 8);
	    *scanline++ = (Uint8)(c >> 16);
#endif
	}
    }
    c = clr[0];
    while (w--) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	*scanline++ = (Uint8)(c >> 16);
	*scanline++ = (Uint8)(c >> 8);
	*scanline++ = (Uint8)c;
#else
	*scanline++ = (Uint8)c;
	*scanline++ = (Uint8)(c >> 8);
	*scanline++ = (Uint8)(c >> 16);
#endif
    }
    return scanline;
}

static void *drawtextscanline32(Uint32 *scanline, int w, int y, Uint32 *clr,
				unsigned char const *text, int len)
{
    unsigned char const	       *glyph;
    int				n, x;

    for (n = 0 ; n < len ; ++n) {
	glyph = sdlg.font.bits[text[n]];
	glyph += y * sdlg.font.w[text[n]];
	for (x = 0 ; w && x < sdlg.font.w[text[n]] ; ++x, --w)
	    scanline[x] = clr[glyph[x]];
	scanline += x;
    }
    while (w--)
	*scanline++ = clr[0];
    return scanline;
}

/*
 * The main font-rendering functions.
 */

/* Draw a single line of text to the screen at the position given by
 * rect. The bitflags in the final argument control the placement of
 * text within rect and what colors to use.
 */
static void drawtext(SDL_Rect *rect, unsigned char const *text,
		     int len, int flags)
{
    Uint32     *clr;
    void       *p;
    void       *q;
    int		l, r;
    int		pitch, bpp, n, w, y;

    if (len < 0)
	len = text ? strlen((char const*)text) : 0;

    w = 0;
    for (n = 0 ; n < len ; ++n)
	w += sdlg.font.w[text[n]];
    if (flags & PT_CALCSIZE) {
	rect->h = sdlg.font.h;
	rect->w = w;
	return;
    }
    if (w >= rect->w) {
	w = rect->w;
	l = r = 0;
    } else if (flags & PT_RIGHT) {
	l = rect->w - w;
	r = 0;
    } else if (flags & PT_CENTER) {
	l = (rect->w - w) / 2;
	r = (rect->w - w) - l;
    } else {
	l = 0;
	r = rect->w - w;
    }

    if (flags & PT_DIM)
	clr = sdlg.dimtextclr.c;
    else if (flags & PT_HILIGHT)
	clr = sdlg.hilightclr.c;
    else
	clr = sdlg.textclr.c;

    pitch = geng.screen->pitch;
    bpp = geng.screen->format->BytesPerPixel;
    p = (unsigned char*)geng.screen->pixels + rect->y * pitch + rect->x * bpp;
    for (y = 0 ; y < sdlg.font.h && y < rect->h ; ++y) {
	switch (bpp) {
	  case 1:
	    q = drawtextscanline8(p, l, y, clr, NULL, 0);
	    q = drawtextscanline8(q, w, y, clr, text, len);
	    q = drawtextscanline8(q, r, y, clr, NULL, 0);
	    break;
	  case 2:
	    q = drawtextscanline16(p, l, y, clr, NULL, 0);
	    q = drawtextscanline16(q, w, y, clr, text, len);
	    q = drawtextscanline16(q, r, y, clr, NULL, 0);
	    break;
	  case 3:
	    q = drawtextscanline24(p, l, y, clr, NULL, 0);
	    q = drawtextscanline24(q, w, y, clr, text, len);
	    q = drawtextscanline24(q, r, y, clr, NULL, 0);
	    break;
	  case 4:
	    q = drawtextscanline32(p, l, y, clr, NULL, 0);
	    q = drawtextscanline32(q, w, y, clr, text, len);
	    q = drawtextscanline32(q, r, y, clr, NULL, 0);
	    break;
	}
	p = (unsigned char*)p + pitch;
    }

    if (flags & PT_UPDATERECT) {
	rect->y += y;
	rect->h -= y;
    }
}

/* Draw one or more lines of text to the screen at the position given by
 * rect. The text is broken up on whitespace whenever possible.
 */
static void drawmultilinetext(SDL_Rect *rect, unsigned char const *text,
			      int len, int flags)
{
    SDL_Rect	area;
    int		index, brkw, brkn;
    int		w, n;

    if (flags & PT_CALCSIZE) {
	rect->h = measuremltext(text, len, rect->w);
	return;
    }

    if (len < 0)
	len = strlen((char const*)text);

    area = *rect;
    brkw = brkn = 0;
    index = 0;
    for (n = 0, w = 0 ; n < len ; ++n) {
	w += sdlg.font.w[text[n]];
	if (isspace(text[n])) {
	    brkn = n;
	    brkw = w;
	} else if (w > rect->w) {
	    if (brkw) {
		drawtext(&area, text + index, brkn - index,
				 flags | PT_UPDATERECT);
		index = brkn + 1;
		w -= brkw;
	    } else {
		drawtext(&area, text + index, n - index,
				 flags | PT_UPDATERECT);
		index = n;
		w = sdlg.font.w[text[n]];
	    }
	    brkw = 0;
	}
    }
    if (w)
	drawtext(&area, text + index, len - index, flags | PT_UPDATERECT);
    if (flags & PT_UPDATERECT) {
	*rect = area;
    } else {
	while (area.h)
	    drawtext(&area, NULL, 0, PT_UPDATERECT);
    }
}

/*
 * The exported functions.
 */

/* Render a string of text.
 */
static void _puttext(SDL_Rect *rect, char const *text, int len, int flags)
{
    if (!sdlg.font.h)
	die("no font available! (how did I get this far?)");

    if (len < 0)
	len = text ? strlen(text) : 0;

    if (SDL_MUSTLOCK(geng.screen))
	SDL_LockSurface(geng.screen);

    if (flags & PT_MULTILINE)
	drawmultilinetext(rect, (unsigned char const*)text, len, flags);
    else
	drawtext(rect, (unsigned char const*)text, len, flags);

    if (SDL_MUSTLOCK(geng.screen))
	SDL_UnlockSurface(geng.screen);
}

/* Lay out the columns of the given table so that the entire table
 * fits within area (horizontally; no attempt is made to make it fit
 * vertically). Return an array of rectangles, one per column. This
 * function is essentially the same algorithm used within printtable()
 * in tworld.c
 */
static SDL_Rect *_measuretable(SDL_Rect const *area, tablespec const *table)
{
    SDL_Rect		       *colsizes;
    unsigned char const	       *p;
    int				sep, mlindex, mlwidth, diff;
    int				i, j, n, i0, c, w, x;

    if (!(colsizes = malloc(table->cols * sizeof *colsizes)))
	memerrexit();
    for (i = 0 ; i < table->cols ; ++i) {
	colsizes[i].x = 0;
	colsizes[i].y = area->y;
	colsizes[i].w = 0;
	colsizes[i].h = area->h;
    }

    mlindex = -1;
    mlwidth = 0;
    n = 0;
    for (j = 0 ; j < table->rows ; ++j) {
	for (i = 0 ; i < table->cols ; ++n) {
	    c = table->items[n][0] - '0';
	    if (c == 1) {
		w = 0;
		p = (unsigned char const*)table->items[n];
		for (p += 2 ; *p ; ++p)
		    w += sdlg.font.w[*p];
		if (table->items[n][1] == '!') {
		    if (w > mlwidth || mlindex != i)
			mlwidth = w;
		    mlindex = i;
		} else {
		    if (w > colsizes[i].w)
			colsizes[i].w = w;
		}
	    }
	    i += c;
	}
    }

    sep = sdlg.font.w[' '] * table->sep;
    w = -sep;
    for (i = 0 ; i < table->cols ; ++i)
	w += colsizes[i].w + sep;
    diff = area->w - w;
    if (diff < 0 && table->collapse >= 0) {
	w = -diff;
	if (colsizes[table->collapse].w < w)
	    w = colsizes[table->collapse].w - sdlg.font.w[' '];
	colsizes[table->collapse].w -= w;
	diff += w;
    }

    if (diff > 0) {
	n = 0;
	for (j = 0 ; j < table->rows && diff > 0 ; ++j) {
	    for (i = 0 ; i < table->cols ; ++n) {
		c = table->items[n][0] - '0';
		if (c > 1 && table->items[n][1] != '!') {
		    w = sep;
		    p = (unsigned char const*)table->items[n];
		    for (p += 2 ; *p ; ++p)
			w += sdlg.font.w[*p];
		    for (i0 = i ; i0 < i + c ; ++i0)
			w -= colsizes[i0].w + sep;
		    if (w > 0) {
			if (table->collapse >= i && table->collapse < i + c)
			    i0 = table->collapse;
			else if (mlindex >= i && mlindex < i + c)
			    i0 = mlindex;
			else
			    i0 = i + c - 1;
			if (w > diff)
			    w = diff;
			colsizes[i0].w += w;
			diff -= w;
			if (diff == 0)
			    break;
		    }
		}
		i += c;
	    }
	}
    }
    if (diff > 0 && mlindex >= 0 && colsizes[mlindex].w < mlwidth) {
	mlwidth -= colsizes[mlindex].w;
	w = mlwidth < diff ? mlwidth : diff;
	colsizes[mlindex].w += w;
	diff -= w;
    }

    x = 0;
    for (i = 0 ; i < table->cols && x < area->w ; ++i) {
	colsizes[i].x = area->x + x;
	x += colsizes[i].w + sep;
	if (x >= area->w)
	    colsizes[i].w = area->x + area->w - colsizes[i].x;
    }
    for ( ; i < table->cols ; ++i) {
	colsizes[i].x = area->x + area->w;
	colsizes[i].w = 0;
    }

    return colsizes;
}

/* Render a single row of a table to the screen, using cols to locate
 * the entries in the individual columns.
 */
static int _drawtablerow(tablespec const *table, SDL_Rect *cols,
			 int *row, int flags)
{
    SDL_Rect			rect;
    unsigned char const	       *p;
    int				c, f, n, i, y;

    if (!cols) {
	for (i = 0 ; i < table->cols ; i += table->items[(*row)++][0] - '0') ;
	return TRUE;
    }

    if (SDL_MUSTLOCK(geng.screen))
	SDL_LockSurface(geng.screen);

    y = cols[0].y;
    n = *row;
    for (i = 0 ; i < table->cols ; ++n) {
	p = (unsigned char const*)table->items[n];
	c = p[0] - '0';
	rect = cols[i];
	i += c;
	if (c > 1)
	    rect.w = cols[i - 1].x + cols[i - 1].w - rect.x;
	f = flags | PT_UPDATERECT;
	if (p[1] == '+')
	    f |= PT_RIGHT;
	else if (p[1] == '.')
	    f |= PT_CENTER;
	if (p[1] == '!')
	    drawmultilinetext(&rect, p + 2, -1, f);
	else
	    drawtext(&rect, p + 2, -1, f);
	if (rect.y > y)
	    y = rect.y;
    }

    if (SDL_MUSTLOCK(geng.screen))
	SDL_UnlockSurface(geng.screen);

    *row = n;
    for (i = 0 ; i < table->cols ; ++i) {
	cols[i].h -= y - cols[i].y;
	cols[i].y = y;
    }

    return TRUE;
}

/* Free the resources associated with a font.
 */
void freefont(void)
{
    if (sdlg.font.h) {
	free(sdlg.font.memory);
	sdlg.font.memory = NULL;
	sdlg.font.h = 0;
    }
}

/* Load the font contained in the given bitmap file. Error messages
 * will be displayed if complain is TRUE. The return value is TRUE if
 * the font was successfully retrieved.
 */
int loadfontfromfile(char const *filename, int complain)
{
    SDL_Surface	       *bmp;
    fontinfo		font;

    bmp = SDL_LoadBMP(filename);
    if (!bmp) {
	if (complain)
	    errmsg(filename, "can't load font bitmap: %s", SDL_GetError());
	return FALSE;
    }
    if (!makefontfromsurface(&font, bmp)) {
	if (complain)
	    errmsg(filename, "invalid font file");
	return FALSE;
    }
    SDL_FreeSurface(bmp);
    freefont();
    sdlg.font = font;
    return TRUE;
}

/* Initialize the module.
 */
int _sdltextinitialize(void)
{
    sdlg.font.h = 0;
    sdlg.puttextfunc = _puttext;
    sdlg.measuretablefunc = _measuretable;
    sdlg.drawtablerowfunc = _drawtablerow;
    return TRUE;
}
