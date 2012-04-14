/* gen.h: General definitions belonging to no single module.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	_gen_h_
#define	_gen_h_

/* The standard Boolean values.
 */
#ifndef	TRUE
#define	TRUE	1
#endif
#ifndef	FALSE
#define	FALSE	0
#endif

/* Definition of the contents and layout of a table.
 *
 * The strings making up the contents of a table are each prefixed
 * with two characters that indicate the formatting of their cell. The
 * first character is a digit, usually "1", indicating the number of
 * columns that cell occupies. The second character indicates the
 * placement of the string in that cell: "-" to align to the left of
 * the cell, "+" to align to the right, "." to center the text, and
 * "!" to permit the cell to occupy multiple lines, with word
 * wrapping. At most one cell in a given row can be word-wrapped.
 */
typedef	struct tablespec {
    short	rows;		/* number of rows */
    short	cols;		/* number of columns */
    short	sep;		/* amount of space between columns */
    short	collapse;	/* the column to squeeze if necessary */
    char      **items;		/* the table's contents */
} tablespec;

/* The dimensions of a level.
 */
#define	CXGRID	32
#define	CYGRID	32

/* The four directions plus one non-direction.
 */
#define	NIL	0
#define	NORTH	1
#define	WEST	2
#define	SOUTH	4
#define	EAST	8

/* Translating directions to and from a two-bit representation. (Note
 * that NIL will map to the same value as NORTH.)
 */
#define	diridx(dir)	((0x30210 >> ((dir) * 2)) & 3)
#define	idxdir(idx)	(1 << ((idx) & 3))

/* The frequency of the gameplay timer. Note that "seconds" refers to
 * seconds in the game, which are not necessarily the same length as
 * real-time seconds.
 */
#define	TICKS_PER_SECOND	20

/* The gameplay timer's value is forced to remain within 23 bits.
 * Thus, gameplay of a single level cannot exceed 4 days 20 hours 30
 * minutes and 30.4 seconds.
 */
#define	MAXIMUM_TICK_COUNT	0x7FFFFF

/* A magic number used to indicate an undefined time value.
 */
#define	TIME_NIL		0x7FFFFFFF

#endif
