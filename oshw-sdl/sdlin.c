/* sdlin.c: Reading the keyboard.
 * 
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	"SDL.h"
#include	"sdlgen.h"
#include	"../defs.h"

/* Turn key-repeating on and off.
 */
int setkeyboardrepeat(int enable)
{
    if (enable)
	return SDL_EnableKeyRepeat(500, 75) == 0;
    else
	return SDL_EnableKeyRepeat(0, 0) == 0;
}

/* Initialization.
 */
int _sdlinputinitialize(void)
{
    if (!_genericinputinitialize())
	return FALSE;
	
    SDL_EnableUNICODE(TRUE);
    return TRUE;
}
