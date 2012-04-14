/* oshwbind.c: Binds the generic module to the SDL OS/hardware layer.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	"SDL.h"
#include	"../generic/generic.h"
#include	"../gen.h"
#include	"../err.h"

/* Create a fresh surface. If transparency is true, the surface is
 * created with 32-bit pixels, so as to ensure a complete alpha
 * channel. Otherwise, the surface is created with the same format as
 * the screen.
 */
TW_Surface *TW_NewSurface(int w, int h, int transparency)
{
    SDL_Surface	       *s;

    if (transparency) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	s = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA | SDL_RLEACCEL,
				 w, h, 32,
				 0xFF000000, 0x00FF0000,
				 0x0000FF00, 0x000000FF);
#else
	s = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA | SDL_RLEACCEL,
				 w, h, 32,
				 0x000000FF, 0x0000FF00,
				 0x00FF0000, 0xFF000000);
#endif
    } else {
	s = SDL_CreateRGBSurface(SDL_SWSURFACE,
				 w, h, geng.screen->format->BitsPerPixel,
				 geng.screen->format->Rmask,
				 geng.screen->format->Gmask,
				 geng.screen->format->Bmask,
				 geng.screen->format->Amask);
    }
    if (!s)
	die("couldn't create surface: %s", SDL_GetError());
    if (!transparency && geng.screen->format->palette)
	SDL_SetColors(s, geng.screen->format->palette->colors,
		      0, geng.screen->format->palette->ncolors);
    return s;
}

/* Return the color of the pixel at (x, y) on the given surface. (The
 * surface must be locked before calling this function.)
 */
uint32_t TW_PixelAt(TW_Surface *_s, int x, int y)
{
    SDL_Surface	       *s;
    s = (SDL_Surface*)_s;
    switch (s->format->BytesPerPixel) {
      case 1:
	return (Uint32)((Uint8*)s->pixels + y * s->pitch)[x];
      case 2:
	return (Uint32)((Uint16*)((Uint8*)s->pixels + y * s->pitch))[x];
      case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	return (Uint32)((((Uint8*)s->pixels + y * s->pitch)[x * 3 + 0] << 16)
		      | (((Uint8*)s->pixels + y * s->pitch)[x * 3 + 1] << 8)
		      | (((Uint8*)s->pixels + y * s->pitch)[x * 3 + 2] << 0));
#else
	return (Uint32)((((Uint8*)s->pixels + y * s->pitch)[x * 3 + 0] << 0)
		      | (((Uint8*)s->pixels + y * s->pitch)[x * 3 + 1] << 8)
		      | (((Uint8*)s->pixels + y * s->pitch)[x * 3 + 2] << 16));
#endif
      case 4:
	return ((Uint32*)((Uint8*)s->pixels + y * s->pitch))[x];
    }
    return 0;
}

/* Load the given bitmap file. If setscreenpalette is true, the screen palette
 * will be synchronized to the bitmap's palette.
 */
TW_Surface *TW_LoadBMP(char const *filename, int setscreenpalette)
{
    SDL_Surface	       *tiles = NULL;

    tiles = SDL_LoadBMP(filename);
    if (tiles && setscreenpalette) {
	if (tiles->format->palette && geng.screen->format->palette)
	    SDL_SetColors(geng.screen, tiles->format->palette->colors,
			  0, tiles->format->palette->ncolors);
    }

    return tiles;
}
