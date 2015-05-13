/* logic.h: Declarations for the game logic modules.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_logic_h_
#define	HEADER_logic_h_

#include	"state.h"

/* Turning macros.
 */
#define	left(dir)	((((dir) << 1) | ((dir) >> 3)) & 15)
#define	back(dir)	((((dir) << 2) | ((dir) >> 2)) & 15)
#define	right(dir)	((((dir) << 3) | ((dir) >> 1)) & 15)

/* One game logic engine.
 */
typedef	struct gamelogic gamelogic;
struct gamelogic {
    int		ruleset;		  /* the ruleset */
    gamestate  *state;			  /* ptr to the current game state */
    int	      (*initgame)(gamelogic*);	  /* prepare to play a game */
    int	      (*advancegame)(gamelogic*); /* advance the game one tick */
    int	      (*endgame)(gamelogic*);	  /* clean up after the game is done */
    void      (*shutdown)(gamelogic*);	  /* turn off the logic engine */
};

/* The available game logic engines.
 */
extern gamelogic *lynxlogicstartup(void);
extern gamelogic *mslogicstartup(void);

/* The high simluation fidelity flag: if true, the simulation should
 * forgo "standard play" in favor of being as true as possible to the
 * original source material.
 */
extern int	pedanticmode;

#endif
