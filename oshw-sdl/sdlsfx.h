/* sdlsfx.h: The internal shared definitions for sound effects using SDL.
 * Used in the SDL as well as the Qt OS/hardware layer.
 * 
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_sdlsfx_h_
#define	HEADER_sdlsfx_h_

#include	"../gen.h"
#include	"../oshw.h"

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

/* The initialization function for the sound module.
 */
OSHW_EXTERN int _sdlsfxinitialize(int silence, int soundbufsize);

#undef OSHW_EXTERN

#endif
