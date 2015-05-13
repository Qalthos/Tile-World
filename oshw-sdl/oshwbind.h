/* oshwbind.h: Binds the generic module to the SDL OS/hardware layer.
 * 
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_sdl_oshwbind_h_
#define	HEADER_sdl_oshwbind_h_

#include	"SDL.h"
#include	<stdint.h>

/* Constants
 */
enum {
	TW_ALPHA_TRANSPARENT	= SDL_ALPHA_TRANSPARENT,
	TW_ALPHA_OPAQUE		= SDL_ALPHA_OPAQUE
};

enum {
	TWK_BACKSPACE	= SDLK_BACKSPACE,
	TWK_TAB		= SDLK_TAB,
	TWK_RETURN	= SDLK_RETURN,
	TWK_KP_ENTER	= SDLK_KP_ENTER,
	TWK_ESCAPE	= SDLK_ESCAPE,

	TWK_UP		= SDLK_UP,
	TWK_LEFT	= SDLK_LEFT,
	TWK_DOWN	= SDLK_DOWN,
	TWK_RIGHT	= SDLK_RIGHT,

	TWK_KP8		= SDLK_KP8,
	TWK_KP4		= SDLK_KP4,
	TWK_KP2		= SDLK_KP2,
	TWK_KP6		= SDLK_KP6,

	TWK_INSERT	= SDLK_INSERT,
	TWK_DELETE	= SDLK_DELETE,
	TWK_HOME	= SDLK_HOME,
	TWK_END		= SDLK_END,
	TWK_PAGEUP	= SDLK_PAGEUP,
	TWK_PAGEDOWN	= SDLK_PAGEDOWN,

	TWK_F1		= SDLK_F1,
	TWK_F2		= SDLK_F2,
	TWK_F3		= SDLK_F3,
	TWK_F4		= SDLK_F4,
	TWK_F5		= SDLK_F5,
	TWK_F6		= SDLK_F6,
	TWK_F7		= SDLK_F7,
	TWK_F8		= SDLK_F8,
	TWK_F9		= SDLK_F9,
	TWK_F10		= SDLK_F10,

	TWK_LSHIFT	= SDLK_LSHIFT,
	TWK_RSHIFT	= SDLK_RSHIFT,
	TWK_LCTRL	= SDLK_LCTRL,
	TWK_RCTRL	= SDLK_RCTRL,
	TWK_LALT	= SDLK_LALT,
	TWK_RALT	= SDLK_RALT,
	TWK_LMETA	= SDLK_LMETA,
	TWK_RMETA	= SDLK_RMETA,
	TWK_CAPSLOCK	= SDLK_CAPSLOCK,
	TWK_NUMLOCK	= SDLK_NUMLOCK,
	TWK_SCROLLLOCK	= SDLK_SCROLLOCK,
	TWK_MODE	= SDLK_MODE,

	TWK_CTRL_C	= '\003',
	
	TWK_LAST	= SDLK_LAST
};

enum {
	TW_BUTTON_LEFT		= SDL_BUTTON_LEFT,
	TW_BUTTON_RIGHT		= SDL_BUTTON_RIGHT,
	TW_BUTTON_MIDDLE	= SDL_BUTTON_MIDDLE,
	TW_BUTTON_WHEELUP	= SDL_BUTTON_WHEELUP,
	TW_BUTTON_WHEELDOWN	= SDL_BUTTON_WHEELDOWN
};

/* Types
 */
typedef  SDL_Rect	TW_Rect;
typedef  SDL_Surface	TW_Surface;
 
/* Functions
 */
extern TW_Surface *TW_NewSurface(int w, int h, int transparency);
#define  TW_FreeSurface  SDL_FreeSurface
#define  TW_MUSTLOCK  SDL_MUSTLOCK
#define  TW_LockSurface  SDL_LockSurface
#define  TW_UnlockSurface  SDL_UnlockSurface
#define  TW_FillRect  SDL_FillRect
#define  TW_BlitSurface  SDL_BlitSurface
#define  TW_SetColorKey(s, c)  SDL_SetColorKey(s, SDL_SRCCOLORKEY, c)
#define  TW_ResetColorKey(s)  SDL_SetColorKey(s, 0, 0)
#define  TW_EnableAlpha(s)  SDL_SetAlpha(s, SDL_SRCALPHA | SDL_RLEACCEL, 0)
#define  TW_DisplayFormat  SDL_DisplayFormat
#define  TW_DisplayFormatAlpha  SDL_DisplayFormatAlpha
#define  TW_BytesPerPixel(s)  ((s)->format->BytesPerPixel)
extern uint32_t TW_PixelAt(TW_Surface *s, int x, int y);
#define  TW_MapRGB(s, r, g, b)  SDL_MapRGB((s)->format, r, g, b)
#define  TW_MapRGBA(s, r, g, b, a)  SDL_MapRGBA((s)->format, r, g, b, a)
extern TW_Surface *TW_LoadBMP(char const *filename, int setscreenpalette);

#define  TW_GetKeyState  SDL_GetKeyState

#define  TW_GetTicks  SDL_GetTicks
#define  TW_Delay  SDL_Delay

#define  TW_GetError  SDL_GetError

#endif
