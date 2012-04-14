/* encoding.c: Functions to read the level data.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"defs.h"
#include	"state.h"
#include	"err.h"
#include	"encoding.h"

/* Read a 16-bit value, stored little-endian, from the level data
 * stream.
 */
#define	readword(p)	((p)[0] | ((p)[1] << 8))

/* Read an x-y coordinate pair from the level data stream. Ensure that
 * an invalid x value always produces an invalid coordinate.
 */
#define	readpos(x, y)	(*(x) < CXGRID ? *(x) + CYGRID * *(y) : CXGRID*CYGRID)

/* Translation table for the codes used by the data file to define the
 * initial state of a level.
 */
static int const fileids[] = {
/* 00 empty space		*/	Empty,
/* 01 wall			*/	Wall,
/* 02 chip			*/	ICChip,
/* 03 water			*/	Water,
/* 04 fire			*/	Fire,
/* 05 invisible wall, perm.	*/	HiddenWall_Perm,
/* 06 blocked north		*/	Wall_North,
/* 07 blocked west		*/	Wall_West,
/* 08 blocked south		*/	Wall_South,
/* 09 blocked east		*/	Wall_East,
/* 0A block			*/	Block_Static,
/* 0B dirt			*/	Dirt,
/* 0C ice			*/	Ice,
/* 0D force south		*/	Slide_South,
/* 0E cloning block N		*/	crtile(Block, NORTH),
/* 0F cloning block W		*/	crtile(Block, WEST),
/* 10 cloning block S		*/	crtile(Block, SOUTH),
/* 11 cloning block E		*/	crtile(Block, EAST),
/* 12 force north		*/	Slide_North,
/* 13 force east		*/	Slide_East,
/* 14 force west		*/	Slide_West,
/* 15 exit			*/	Exit,
/* 16 blue door			*/	Door_Blue,
/* 17 red door			*/	Door_Red,
/* 18 green door		*/	Door_Green,
/* 19 yellow door		*/	Door_Yellow,
/* 1A SE ice slide		*/	IceWall_Southeast,
/* 1B SW ice slide		*/	IceWall_Southwest,
/* 1C NW ice slide		*/	IceWall_Northwest,
/* 1D NE ice slide		*/	IceWall_Northeast,
/* 1E blue block, tile		*/	BlueWall_Fake,
/* 1F blue block, wall		*/	BlueWall_Real,
/* 20 not used			*/	Overlay_Buffer,
/* 21 thief			*/	Burglar,
/* 22 socket			*/	Socket,
/* 23 green button		*/	Button_Green,
/* 24 red button		*/	Button_Red,
/* 25 switch block, closed	*/	SwitchWall_Closed,
/* 26 switch block, open	*/	SwitchWall_Open,
/* 27 brown button		*/	Button_Brown,
/* 28 blue button		*/	Button_Blue,
/* 29 teleport			*/	Teleport,
/* 2A bomb			*/	Bomb,
/* 2B trap			*/	Beartrap,
/* 2C invisible wall, temp.	*/	HiddenWall_Temp,
/* 2D gravel			*/	Gravel,
/* 2E pass once			*/	PopupWall,
/* 2F hint			*/	HintButton,
/* 30 blocked SE		*/	Wall_Southeast,
/* 31 cloning machine		*/	CloneMachine,
/* 32 force all directions	*/	Slide_Random,
/* 33 drowning Chip		*/	Drowned_Chip,
/* 34 burned Chip		*/	Burned_Chip,
/* 35 burned Chip		*/	Bombed_Chip,
/* 36 not used			*/	HiddenWall_Perm,
/* 37 not used			*/	HiddenWall_Perm,
/* 38 not used			*/	HiddenWall_Perm,
/* 39 Chip in exit		*/	Exited_Chip,
/* 3A exit - end game		*/	Exit_Extra_1,
/* 3B exit - end game		*/	Exit_Extra_2,
/* 3C Chip swimming N		*/	crtile(Swimming_Chip, NORTH),
/* 3D Chip swimming W		*/	crtile(Swimming_Chip, WEST),
/* 3E Chip swimming S		*/	crtile(Swimming_Chip, SOUTH),
/* 3F Chip swimming E		*/	crtile(Swimming_Chip, EAST),
/* 40 Bug N			*/	crtile(Bug, NORTH),
/* 41 Bug W			*/	crtile(Bug, WEST),
/* 42 Bug S			*/	crtile(Bug, SOUTH),
/* 43 Bug E			*/	crtile(Bug, EAST),
/* 44 Fireball N		*/	crtile(Fireball, NORTH),
/* 45 Fireball W		*/	crtile(Fireball, WEST),
/* 46 Fireball S		*/	crtile(Fireball, SOUTH),
/* 47 Fireball E		*/	crtile(Fireball, EAST),
/* 48 Pink ball N		*/	crtile(Ball, NORTH),
/* 49 Pink ball W		*/	crtile(Ball, WEST),
/* 4A Pink ball S		*/	crtile(Ball, SOUTH),
/* 4B Pink ball E		*/	crtile(Ball, EAST),
/* 4C Tank N			*/	crtile(Tank, NORTH),
/* 4D Tank W			*/	crtile(Tank, WEST),
/* 4E Tank S			*/	crtile(Tank, SOUTH),
/* 4F Tank E			*/	crtile(Tank, EAST),
/* 50 Glider N			*/	crtile(Glider, NORTH),
/* 51 Glider W			*/	crtile(Glider, WEST),
/* 52 Glider S			*/	crtile(Glider, SOUTH),
/* 53 Glider E			*/	crtile(Glider, EAST),
/* 54 Teeth N			*/	crtile(Teeth, NORTH),
/* 55 Teeth W			*/	crtile(Teeth, WEST),
/* 56 Teeth S			*/	crtile(Teeth, SOUTH),
/* 57 Teeth E			*/	crtile(Teeth, EAST),
/* 58 Walker N			*/	crtile(Walker, NORTH),
/* 59 Walker W			*/	crtile(Walker, WEST),
/* 5A Walker S			*/	crtile(Walker, SOUTH),
/* 5B Walker E			*/	crtile(Walker, EAST),
/* 5C Blob N			*/	crtile(Blob, NORTH),
/* 5D Blob W			*/	crtile(Blob, WEST),
/* 5E Blob S			*/	crtile(Blob, SOUTH),
/* 5F Blob E			*/	crtile(Blob, EAST),
/* 60 Paramecium N		*/	crtile(Paramecium, NORTH),
/* 61 Paramecium W		*/	crtile(Paramecium, WEST),
/* 62 Paramecium S		*/	crtile(Paramecium, SOUTH),
/* 63 Paramecium E		*/	crtile(Paramecium, EAST),
/* 64 Blue key			*/	Key_Blue,
/* 65 Red key			*/	Key_Red,
/* 66 Green key			*/	Key_Green,
/* 67 Yellow key		*/	Key_Yellow,
/* 68 Flippers			*/	Boots_Water,
/* 69 Fire boots		*/	Boots_Fire,
/* 6A Ice skates		*/	Boots_Ice,
/* 6B Suction boots		*/	Boots_Slide,
/* 6C Chip N			*/	crtile(Chip, NORTH),
/* 6D Chip W			*/	crtile(Chip, WEST),
/* 6E Chip S			*/	crtile(Chip, SOUTH),
/* 6F Chip E			*/	crtile(Chip, EAST)
};

/* Initialize the gamestate by reading the level data, in MS dat-file
 * format, from the state's setup.
 */
static int expandmsdatlevel(gamestate *state)
{
    gamesetup		       *setup;
    unsigned char const	       *data;
    unsigned char const	       *dataend;
    int				size, pos, id;
    int				i, n;

    memset(state->map, 0, sizeof state->map);
    state->trapcount = 0;
    state->clonercount = 0;
    state->crlistcount = 0;
    state->hinttext[0] = '\0';

    setup = state->game;
    if (setup->levelsize < 10)
	goto badlevel;
    data = setup->leveldata;
    dataend = data + setup->levelsize;

    if (readword(data) == 0)
	goto badlevel;
    state->chipsneeded = readword(data + 4);

    if (readword(data + 6) > 1)
	goto badlevel;
    size = readword(data + 8);
    data += 10;
    if (data + size + 2 > dataend)
	goto badlevel;
    for (n = pos = 0 ; n < size && pos < CXGRID * CYGRID ; ++n) {
	if (data[n] == 0xFF) {
	    i = data[++n];
	    id = data[++n];
	} else {
	    i = 1;
	    id = data[n];
	}
	if (id >= (int)(sizeof fileids / sizeof *fileids)) {
	    id = Wall;
	    state->statusflags |= SF_BADTILES;
	} else {
	    id = fileids[id];
	}
	while (i-- && pos < CXGRID * CYGRID)
	    state->map[pos++].top.id = id;
    }
    if (n < size)
	warn("level %d: %d extra bytes in upper map layer",
	     setup->number, size - n);
    else if (pos < CXGRID * CYGRID)
	warn("level %d: %d missing bytes in upper map layer",
	     setup->number, CXGRID * CYGRID - pos);
    data += size + 2;
    size = readword(data - 2);
    if (data + size > dataend)
	goto badlevel;
    for (n = pos = 0 ; n < size && pos < CXGRID * CYGRID ; ++n) {
	if (data[n] == 0xFF) {
	    i = data[++n];
	    id = data[++n];
	} else {
	    i = 1;
	    id = data[n];
	}
	if (id >= (int)(sizeof fileids / sizeof *fileids)) {
	    id = Wall;
	    state->statusflags |= SF_BADTILES;
	} else {
	    id = fileids[id];
	}
	while (i-- && pos < CXGRID * CYGRID)
	    state->map[pos++].bot.id = id;
    }
    if (n < size)
	warn("level %d: %d extra bytes in lower map layer",
	     setup->number, size - n);
    else if (pos < CXGRID * CYGRID)
	warn("level %d: %d missing bytes in lower map layer",
	     setup->number, CXGRID * CYGRID - pos);
    data += size;

    size = readword(data);
    data += 2;
    if (data + size != dataend)
	warn("level %d: inconsistent metadata", setup->number);
    while (data + 2 < dataend) {
	size = data[1];
	data += 2;
	if (data + size > dataend)
	    size = dataend - data;
	switch (data[-2]) {
	  case 1:
	    /* time */
	    break;
	  case 2:
	    if (size < 2)
		warn("level %d: ignoring field 2 data of size %d",
		     setup->number, size);
	    else
		state->chipsneeded = readword(data);
	    break;
	  case 3:
	    /* level name */
	    break;
	  case 4:
	    if (size % 10)
		warn("level %d: ignoring %d extra bytes at end of field 4",
		     setup->number, size % 10);
	    state->trapcount = size / 10;
	    for (i = 0 ; i < state->trapcount ; ++i) {
		state->traps[i].from = readpos(data + i * 10,
					       data + i * 10 + 2);
		state->traps[i].to = readpos(data + i * 10 + 4,
					     data + i * 10 + 6);
	    }
	    break;
	  case 5:
	    if (size % 8)
		warn("level %d: ignoring %d extra bytes at end of field 5",
		     setup->number, size % 8);
	    state->clonercount = size / 8;
	    for (i = 0 ; i < state->clonercount ; ++i) {
		state->cloners[i].from = readpos(data + i * 8,
						 data + i * 8 + 2);
		state->cloners[i].to = readpos(data + i * 8 + 4,
					       data + i * 8 + 6);
	    }
	    break;
	  case 6:
	    /* passwd */
	    break;
	  case 7:
	    memcpy(state->hinttext, data, size);
	    state->hinttext[size] = '\0';
	    break;
	  case 8:
	    /* field 8 passwd */
	    break;
	  case 10:
	    if (size % 2)
		warn("level %d: ignoring extra byte at end of field 10",
		     setup->number);
	    state->crlistcount = size / 2;
	    for (i = 0 ; i < state->crlistcount ; ++i)
		state->crlist[i] = readpos(data + i * 2, data + i * 2 + 1);
	    break;
	  default:
	    warn("level %d: ignoring unrecognized field %d (%d bytes)",
		 setup->number, data[-2], size);
	    break;
	}
	data += size;
    }

    return TRUE;

  badlevel:
    errmsg(NULL, "level %d: invalid data", setup->number);
    return FALSE;
}

/* Exported interface.
 */
int expandleveldata(gamestate *state)
{
    return expandmsdatlevel(state);
}

/* Return the setup for a small level to display at the completion of
 * a series.
 */
void getenddisplaysetup(gamestate *state)
{
    static unsigned char endingdata[] = {
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x6D, 0x00,
	0x15, 0x39, 0x39, 0x39, 0x15, 0x39, 0x39, 0x15, 0x39, 0xFF, 0x17, 0x00,
	0x39, 0x39, 0x15, 0x15, 0x15, 0x39, 0x39, 0x15, 0x39, 0xFF, 0x17, 0x00,
	0x39, 0x39, 0x15, 0x15, 0x15, 0xFF, 0x04, 0x39,       0xFF, 0x17, 0x00,
	0x15, 0x39, 0x39, 0x39, 0x15, 0x39, 0x39, 0x15, 0x39, 0xFF, 0x17, 0x00,
	0xFF, 0x04, 0x15, 0x6E, 0xFF, 0x04, 0x15,             0xFF, 0x17, 0x00,
	0xFF, 0x04, 0x39, 0x15, 0x39, 0x39, 0x39, 0x15,       0xFF, 0x17, 0x00,
	0x15, 0x39, 0x39, 0x15, 0x15, 0x39, 0x39, 0x15, 0x39, 0xFF, 0x17, 0x00,
	0x15, 0x39, 0x39, 0x15, 0x15, 0x39, 0x39, 0x39, 0x15, 0xFF, 0x17, 0x00,
	0xFF, 0x04, 0x39, 0x15, 0x39, 0x39, 0x15, 0x15,
	0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xF9, 0x00,
	0x10, 0x00,
	0xFF, 0x84, 0x00, 0x15, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
	0xFF, 0xFF, 0x00, 0xFF, 0x7E, 0x00,
	0x00, 0x00
    };

    static gamesetup	ending;

    ending.number = 1;
    ending.time = 0;
    ending.besttime = TIME_NIL;
    ending.sgflags = 0;
    ending.levelsize = sizeof endingdata;
    ending.leveldata = endingdata;
    ending.solutionsize = 0;
    ending.solutiondata = NULL;
    strcpy(ending.name, "CONGRATULATIONS!");
    ending.passwd[0] = '\0';

    state->game = &ending;
    expandmsdatlevel(state);
    ending.number = 0;
}
