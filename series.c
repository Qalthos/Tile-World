/* series.c: Functions for finding and reading the data files.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"defs.h"
#include	"err.h"
#include	"fileio.h"
#include	"solution.h"
#include	"unslist.h"
#include	"series.h"
#include	"oshw.h"

/* The signature bytes of the data files.
 */
#define	SIG_DATFILE		0xAAAC

#define	SIG_DATFILE_MS		0x0002
#define	SIG_DATFILE_LYNX	0x0102

/* The "signature bytes" of the configuration files.
 */
#define	SIG_DACFILE		0x656C6966

/* Mini-structure for passing data in and out of findfiles().
 */
typedef	struct seriesdata {
    gameseries *list;		/* the gameseries list */
    int		allocated;	/* number of gameseries currently allocated */
    int		count;		/* number of gameseries filled in */
    int		usedatdir;	/* TRUE if the file is in seriesdatdir. */
} seriesdata;

/* The directory containing the series files (data files and
 * configuration files).
 */
char	       *seriesdir = NULL;

/* The directory containing the configured data files.
 */
char	       *seriesdatdir = NULL;

/* Calculate a hash value for the given block of data.
 */
static unsigned long hashvalue(unsigned char const *data, unsigned int size)
{
    static unsigned long remainders[256] = {
	0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B,
	0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
	0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 0x4C11DB70, 0x48D0C6C7,
	0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
	0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3,
	0x709F7B7A, 0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
	0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58, 0xBAEA46EF,
	0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
	0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB,
	0xCEB42022, 0xCA753D95, 0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1,
	0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0,
	0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072,
	0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4,
	0x0808D07D, 0x0CC9CDCA, 0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE,
	0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08,
	0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
	0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC,
	0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6,
	0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 0xE0B41DE7, 0xE4750050,
	0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
	0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34,
	0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637,
	0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 0x4F040D56, 0x4BC510E1,
	0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53,
	0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5,
	0x3F9B762C, 0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
	0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E, 0xF5EE4BB9,
	0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,
	0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD,
	0xCDA1F604, 0xC960EBB3, 0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7,
	0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71,
	0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3,
	0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2,
	0x470CDD2B, 0x43CDC09C, 0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
	0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E,
	0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
	0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A,
	0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,
	0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 0xE3A1CBC1, 0xE760D676,
	0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4,
	0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
	0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,
	0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
    };

    unsigned long	accum;
    unsigned int	i, j;

    for (j = 0, accum = 0xFFFFFFFFUL ; j < size ; ++j) {
	i = ((accum >> 24) ^ data[j]) & 0x000000FF;
	accum = (accum << 8) ^ remainders[i];
    }
    return accum ^ 0xFFFFFFFFUL;
}

/*
 * Reading the data file.
 */

/* Examine the top of a data file and identify its type. FALSE is
 * returned if any header bytes appear to be invalid.
 */
static int readseriesheader(gameseries *series)
{
    unsigned short	val16;
    int			ruleset;

    if (!filereadint16(&series->mapfile, &val16, "not a valid data file"))
	return FALSE;
    if (val16 != SIG_DATFILE)
	return fileerr(&series->mapfile, "not a valid data file");
    if (!filereadint16(&series->mapfile, &val16, "not a valid data file"))
	return FALSE;
    switch (val16) {
      case SIG_DATFILE_MS:	ruleset = Ruleset_MS;		break;
      case SIG_DATFILE_LYNX:	ruleset = Ruleset_Lynx;		break;
      default:
	fileerr(&series->mapfile, "data file uses an unrecognized ruleset");
	return FALSE;
    }
    if (series->ruleset == Ruleset_None)
	series->ruleset = ruleset;
    if (!filereadint16(&series->mapfile, &val16, "not a valid data file"))
	return FALSE;
    series->count = val16;
    if (!series->count) {
	fileerr(&series->mapfile, "file contains no maps");
	return FALSE;
    }

    return TRUE;
}

/* Read a single level out of the given data file. The level's name,
 * password, and time limit are extracted from the data.
 */
static int readleveldata(fileinfo *file, gamesetup *game)
{
    unsigned char	       *data;
    unsigned char const	       *dataend;
    unsigned short		size;
    int				n;

    if (!filereadint16(file, &size, NULL))
	return FALSE;
    data = filereadbuf(file, size, "missing or invalid level data");
    if (!data)
	return FALSE;
    if (size < 2) {
	fileerr(file, "invalid level data");
	free(data);
	return FALSE;
    }
    game->levelsize = size;
    game->leveldata = data;
    dataend = game->leveldata + game->levelsize;

    game->number = data[0] | (data[1] << 8);
    if (size < 10)
	goto badlevel;
    game->time = data[2] | (data[3] << 8);
    game->besttime = TIME_NIL;
    game->passwd[0] = '\0';
    data += data[8] | (data[9] << 8);
    data += 10;
    if (data + 2 >= dataend)
	goto badlevel;
    data += data[0] | (data[1] << 8);
    data += 2;
    size = data[0] | (data[1] << 8);
    data += 2;
    if (data + size != dataend)
	warn("level %d: inconsistent size data (%d vs %d)",
	     game->number, dataend - data, size);

    while (data + 2 < dataend) {
	size = data[1];
	data += 2;
	if (size > dataend - data)
	    size = dataend - data;
	switch (data[-2]) {
	  case 1:
	    if (size > 1)
		game->time = data[0] | (data[1] << 8);
	    break;
	  case 3:
	    memcpy(game->name, data, size);
	    game->name[size] = '\0';
	    break;
	  case 6:
	    for (n = 0 ; n < size && n < 15 && data[n] ; ++n)
		game->passwd[n] = data[n] ^ 0x99;
	    game->passwd[n] = '\0';
	    break;
	  case 8:
	    warn("level %d: ignoring field 8 password", game->number);
	    break;
	}
	data += size;
    }
    if (!game->passwd[0] || strlen(game->passwd) != 4)
	goto badlevel;

    game->levelhash = hashvalue(game->leveldata, game->levelsize);
    return TRUE;

  badlevel:
    free(game->leveldata);
    game->levelsize = 0;
    game->leveldata = NULL;
    errmsg(file->name, "level %d: invalid level data", game->number);
    return FALSE;
}

/* Assuming that the series passed in is in fact the original
 * chips.dat file, this function undoes the changes that MS introduced
 * to the original Lynx levels. A rather "ad hack" way to accomplish
 * this, but it permits this fixup to occur without requiring the user
 * to perform a special one-time task. Four passwords are repaired, a
 * possibly missing wall in level 88 is restored, the beartrap wirings
 * of levels 99 and 111 are fixed, the layout changes to 121 and 127
 * are undone, and level 145 is removed.
 */
static int undomschanges(gameseries *series)
{
    struct { int num, pos, val; } *fixup, fixups[] = {
	{   5,  0x011D,  'P' ^ 0x99 },	{  95,  0x035F,  'W' ^ 0x99 },
	{   9,  0x032D,  'V' ^ 0x99 },	{  95,  0x0360,  'V' ^ 0x99 },
	{   9,  0x032E,  'U' ^ 0x99 },	{  95,  0x0361,  'H' ^ 0x99 },
	{  27,  0x01E7,  'D' ^ 0x99 },	{  95,  0x0362,  'Y' ^ 0x99 },
	{  87,  0x0148,  0x09 },	{ 120,  0x0195,  0x00 },
	{  98,  0x0340,  8 },		{  98,  0x0342,  14 },
	{  98,  0x034A,  23 },		{  98,  0x034C,  14 },
	{  98,  0x0354,  8 },		{  98,  0x0356,  16 },
	{  98,  0x035E,  23 },		{  98,  0x0360,  16 },
	{  98,  0x0368,  16 },		{  98,  0x036A,  18 },
	{  98,  0x0372,  6 },		{  98,  0x0374,  20 },
	{  98,  0x037C,  16 },		{  98,  0x037E,  20 },
	{  98,  0x0386,  23 },		{  98,  0x0388,  23 },
	{  98,  0x0390,  23 },		{  98,  0x0392,  25 },
	{ 110,  0x02B6,  22 },		{ 110,  0x02B8,  11 },
	{ 110,  0x02C0,  15 },		{ 110,  0x02C2,  6 },
	{ 126,  0x00B6,  0x00 },	{ 126,  0x00C2,  0x01 },
	{ 126,  0x01B6,  0x01 },	{ 126,  0x01C2,  0x00 },
	{ -1, -1, -1 }
    };

    if (series->count != 149)
	return FALSE;
    for (fixup = fixups ; fixup->num >= 0 ; ++fixup)
	if (series->games[fixup->num].levelsize <= fixup->pos)
	    return FALSE;

    free(series->games[144].leveldata);
    memmove(series->games + 144, series->games + 145,
	    4 * sizeof *series->games);
    --series->count;

    for (fixup = fixups ; fixup->num >= 0 ; ++fixup)
	series->games[fixup->num].leveldata[fixup->pos] = fixup->val;

    series->games[5].passwd[3] = 'P';
    series->games[9].passwd[0] = 'V';
    series->games[9].passwd[1] = 'U';
    series->games[27].passwd[3] = 'D';
    series->games[95].passwd[0] = 'W';
    series->games[95].passwd[1] = 'V';
    series->games[95].passwd[2] = 'H';
    series->games[95].passwd[3] = 'Y';

    return TRUE;
}

/*
 * Functions to read the data files.
 */

/* Load all levels from the given data file, and all of the user's
 * saved solutions.
 */
int readseriesfile(gameseries *series)
{
    int	n;

    if (series->gsflags & GSF_ALLMAPSREAD)
	return TRUE;
    if (series->count <= 0) {
	errmsg(series->filebase, "cannot read from empty level set");
	return FALSE;
    }

    if (!series->mapfile.fp) {
	if (!openfileindir(&series->mapfile, seriesdir,
			   series->mapfilename, "rb", "unknown error"))
	    return FALSE;
	if (!readseriesheader(series))
	    return FALSE;
    }

    xalloc(series->games, series->count * sizeof *series->games);
    memset(series->games + series->allocated, 0,
	   (series->count - series->allocated) * sizeof *series->games);
    series->allocated = series->count;
    n = 0;
    while (n < series->count && !filetestend(&series->mapfile)) {
	if (readleveldata(&series->mapfile, series->games + n))
	    ++n;
	else
	    --series->count;
    }
    fileclose(&series->mapfile, NULL);
    series->gsflags |= GSF_ALLMAPSREAD;
    if (series->gsflags & GSF_LYNXFIXES)
	undomschanges(series);
    markunsolvablelevels(series);
    readsolutions(series);
    readextensions(series);
    return TRUE;
}

/* Free all memory allocated for the given gameseries.
 */
void freeseriesdata(gameseries *series)
{
    gamesetup  *game;
    int		n;

    clearsolutions(series);

    fileclose(&series->mapfile, NULL);
    clearfileinfo(&series->mapfile);
    free(series->mapfilename);
    series->mapfilename = NULL;
    free(series->savefilename);
    series->savefilename = NULL;
    series->gsflags = 0;
    series->solheaderflags = 0;

    for (n = 0, game = series->games ; n < series->count ; ++n, ++game) {
	free(game->leveldata);
	game->leveldata = NULL;
	game->levelsize = 0;
    }
    free(series->games);
    series->games = NULL;
    series->allocated = 0;
    series->count = 0;

    series->ruleset = Ruleset_None;
    series->gsflags = 0;
    *series->filebase = '\0';
    *series->name = '\0';
}

/*
 * Reading the configuration file.
 */

/* Parse the lines of the given configuration file. The return value
 * is the name of the corresponding data file, or NULL if the
 * configuration file could not be read or contained a syntax error.
 */
static char *readconfigfile(fileinfo *file, gameseries *series)
{
    static char	datfilename[256];
    char	buf[256];
    char	name[256];
    char	value[256];
    char       *p;
    int		lineno, n;

    n = sizeof buf - 1;
    if (!filegetline(file, buf, &n, "invalid configuration file"))
	return NULL;
    if (sscanf(buf, "file = %s", datfilename) != 1) {
	fileerr(file, "bad filename in configuration file");
	return NULL;
    }
    for (lineno = 2 ; ; ++lineno) {
	n = sizeof buf - 1;
	if (!filegetline(file, buf, &n, NULL))
	    break;
	for (p = buf ; isspace(*p) ; ++p) ;
	if (!*p || *p == '#')
	    continue;
	if (sscanf(buf, "%[^= \t] = %s", name, value) != 2) {
	    fileerr(file, "invalid configuration file syntax");
	    return NULL;
	}
	for (p = name ; (*p = tolower(*p)) != '\0' ; ++p) ;
	if (!strcmp(name, "name")) {
	    sprintf(series->name, "%.*s", (int)(sizeof series->name - 1),
					  skippathname(value));
	} else if (!strcmp(name, "lastlevel")) {
	    n = (int)strtol(value, &p, 10);
	    if (*p || n <= 0) {
		fileerr(file, "invalid lastlevel in configuration file");
		return NULL;
	    }
	    series->final = n;
	} else if (!strcmp(name, "ruleset")) {
	    for (p = value ; (*p = tolower(*p)) != '\0' ; ++p) ;
	    if (strcmp(value, "ms") && strcmp(value, "lynx")) {
		fileerr(file, "invalid ruleset in configuration file");
		return NULL;
	    }
	    series->ruleset = *value == 'm' ? Ruleset_MS : Ruleset_Lynx;
	} else if (!strcmp(name, "usepasswords")) {
	    if (tolower(*value) == 'n')
		series->gsflags |= GSF_IGNOREPASSWDS;
	    else
		series->gsflags &= ~GSF_IGNOREPASSWDS;
	} else if (!strcmp(name, "fixlynx")) {
	    if (tolower(*value) == 'n')
		series->gsflags &= ~GSF_LYNXFIXES;
	    else
		series->gsflags |= GSF_LYNXFIXES;
	} else {
	    warn("line %d: directive \"%s\" unknown", lineno, name);
	    fileerr(file, "unrecognized setting in configuration file");
	    return NULL;
	}
    }

    return datfilename;
}

/*
 * Functions to locate the series files.
 */

/* Open the given file and read the information in the file header (or
 * the entire file if it is a configuration file), then allocate and
 * initialize a gameseries structure for the file and add it to the
 * list stored under the second argument. This function is used as a
 * findfiles() callback.
 */
static int getseriesfile(char *filename, void *data)
{
    fileinfo		file;
    seriesdata	       *sdata = (seriesdata*)data;
    gameseries	       *series;
    unsigned long	magic;
    char	       *datfilename;
    int			config, f;

    clearfileinfo(&file);
    if (!openfileindir(&file, seriesdir, filename, "rb", "unknown error"))
	return 0;
    if (!filereadint32(&file, &magic, "unexpected EOF")) {
	fileclose(&file, NULL);
	return 0;
    }
    filerewind(&file, NULL);
    if (magic == SIG_DACFILE) {
	config = TRUE;
    } else if ((magic & 0xFFFF) == SIG_DATFILE) {
	config = FALSE;
    } else {
	fileerr(&file, "not a valid data file or configuration file");
	fileclose(&file, NULL);
	return 0;
    }

    if (sdata->count >= sdata->allocated) {
	sdata->allocated = sdata->count + 1;
	xalloc(sdata->list, sdata->allocated * sizeof *sdata->list);
    }
    series = sdata->list + sdata->count;
    series->mapfilename = NULL;
    clearfileinfo(&series->savefile);
    series->savefilename = NULL;
    series->gsflags = 0;
    series->solheaderflags = 0;
    series->allocated = 0;
    series->count = 0;
    series->final = 0;
    series->ruleset = Ruleset_None;
    series->games = NULL;
    sprintf(series->filebase, "%.*s", (int)(sizeof series->filebase - 1),
                                      filename);
    sprintf(series->name, "%.*s", (int)(sizeof series->name - 1),
				  skippathname(filename));

    f = FALSE;
    if (config) {
	fileclose(&file, NULL);
	if (!openfileindir(&file, seriesdir, filename, "r", "unknown error"))
	    return 0;
	clearfileinfo(&series->mapfile);
	free(series->mapfilename);
	series->mapfilename = NULL;
	datfilename = readconfigfile(&file, series);
	fileclose(&file, NULL);
	if (datfilename) {
	    if (openfileindir(&series->mapfile, seriesdatdir,
			      datfilename, "rb", NULL))
		f = readseriesheader(series);
	    else
		warn("cannot use %s: %s unavailable", filename, datfilename);
	    fileclose(&series->mapfile, NULL);
	    clearfileinfo(&series->mapfile);
	    if (f)
		series->mapfilename = getpathforfileindir(seriesdatdir,
							  datfilename);
	}
    } else {
	series->mapfile = file;
	f = readseriesheader(series);
	fileclose(&series->mapfile, NULL);
	clearfileinfo(&series->mapfile);
	if (f)
	    series->mapfilename = getpathforfileindir(seriesdir, filename);
    }
    if (f)
	++sdata->count;
    return 0;
}

/* A callback function to compare two gameseries structures by
 * comparing their filenames.
 */
static int gameseriescmp(void const *a, void const *b)
{
    return
#ifdef WIN32
        stricmp
#else
        strcasecmp
#endif
           (((gameseries*)a)->name, ((gameseries*)b)->name);
}

/* Search the series directory and generate an array of gameseries
 * structures corresponding to the data files found there. The array
 * is returned through list, and the size of the array is returned
 * through count. If preferred is not NULL, then the array returned
 * will only contain the series with that string as its filename
 * (presuming it can be found). The program will be aborted if a
 * serious error occurs or if no series can be found.
 */
static int getseriesfiles(char const *preferred, gameseries **list, int *count)
{
    seriesdata	s;
    int		n;

    s.list = NULL;
    s.allocated = 0;
    s.count = 0;
    s.usedatdir = FALSE;
    if (preferred && *preferred && haspathname(preferred)) {
	if (getseriesfile((char*)preferred, &s) < 0)
	    return FALSE;
	if (!s.count) {
	    errmsg(preferred, "couldn't read data file");
	    return FALSE;
	}
	*seriesdir = '\0';
	s.list[0].gsflags |= GSF_NODEFAULTSAVE;
    } else {
	if (!*seriesdir)
	    return FALSE;
	if (!findfiles(seriesdir, &s, getseriesfile) || !s.count) {
	    errmsg(seriesdir, "directory contains no data files");
	    return FALSE;
	}
	if (preferred && *preferred) {
	    for (n = 0 ; n < s.count ; ++n) {
		if (!strcmp(s.list[n].name, preferred)) {
		    s.list[0] = s.list[n];
		    s.count = 1;
		    n = 0;
		    break;
		}
	    }
	    if (n == s.count) {
		errmsg(preferred, "no such data file");
		return FALSE;
	    }
	}
	if (s.count > 1)
	    qsort(s.list, s.count, sizeof *s.list, gameseriescmp);
    }
    *list = s.list;
    *count = s.count;
    return TRUE;
}

/* Produce a list of the series that are available for play. An array
 * of gameseries structures is returned through pserieslist, the size
 * of the array is returned through pcount, and a table of the the
 * filenames is returned through table. preferredfile, if not NULL,
 * limits the results to just the series with that filename.
 */
int createserieslist(char const *preferredfile, gameseries **pserieslist,
		     int *pcount, tablespec *table)
{
    static char const  *rulesetname[Ruleset_Count];
    gameseries	       *serieslist;
    char	      **ptrs;
    char	       *textheap;
    int			listsize;
    int			used, col, n, y;

    if (!getseriesfiles(preferredfile, &serieslist, &listsize))
	return FALSE;
    if (!table) {
	*pserieslist = serieslist;
	*pcount = listsize;
	return TRUE;
    }

    col = 8;
    for (n = 0 ; n < listsize ; ++n)
	if (col < (int)strlen(serieslist[n].name))
	    col = strlen(serieslist[n].name);
    if (col > 48)
	col = 48;

    rulesetname[Ruleset_Lynx] = "Lynx";
    rulesetname[Ruleset_MS] = "MS";
    ptrs = malloc((listsize + 1) * 2 * sizeof *ptrs);
    textheap = malloc((listsize + 1) * (col + 32));
    if (!ptrs || !textheap)
	memerrexit();

    n = 0;
    used = 0;
    ptrs[n++] = textheap + used;
    used += 1 + sprintf(textheap + used, "1-Filename");
    ptrs[n++] = textheap + used;
    used += 1 + sprintf(textheap + used, "1.Ruleset");
    for (y = 0 ; y < listsize ; ++y) {
	ptrs[n++] = textheap + used;
	used += 1 + sprintf(textheap + used,
			    "1-%-*s", col, serieslist[y].name);
	ptrs[n++] = textheap + used;
	used += 1 + sprintf(textheap + used,
			    "1.%s", rulesetname[serieslist[y].ruleset]);
    }

    *pserieslist = serieslist;
    *pcount = listsize;
    table->rows = listsize + 1;
    table->cols = 2;
    table->sep = 2;
    table->collapse = 0;
    table->items = ptrs;
    return TRUE;
}

/* Make an independent copy of a single gameseries structure from list.
 */
void getseriesfromlist(gameseries *dest, gameseries const *list, int index)
{
    int	n;

    *dest = list[index];
    n = strlen(list[index].mapfilename) + 1;
    if (!(dest->mapfilename = malloc(n)))
	memerrexit();
    memcpy(dest->mapfilename, list[index].mapfilename, n);
}

/* Free all memory allocated by the createserieslist() table.
 */
void freeserieslist(gameseries *list, int count, tablespec *table)
{
    int	n;

    if (list) {
	for (n = 0 ; n < count ; ++n)
	    free(list[n].mapfilename);
	free(list);
    }
    if (table) {
	free((void*)table->items[0]);
	free(table->items);
    }
}

/*
 * Miscellaneous functions
 */

/* A function for looking up a specific level in a series by number
 * and/or password.
 */
int findlevelinseries(gameseries const *series, int number, char const *passwd)
{
    int	i, n;

    n = -1;
    if (number) {
	for (i = 0 ; i < series->count ; ++i) {
	    if (series->games[i].number == number) {
		if (!passwd || !strcmp(series->games[i].passwd, passwd)) {
		    if (n >= 0)
			return -1;
		    n = i;
		}
	    }
	}
    } else if (passwd) {
	for (i = 0 ; i < series->count ; ++i) {
	    if (!strcmp(series->games[i].passwd, passwd)) {
		if (n >= 0)
		    return -1;
		n = i;
	    }
	}
    } else {
	return -1;
    }
    return n;
}
