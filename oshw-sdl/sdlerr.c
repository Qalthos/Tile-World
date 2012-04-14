/* sdlerr.c: Notification functionality, not supplied by the SDL library.
 * 
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdarg.h>
#include	"sdlgen.h"

#ifdef WIN32

/*
 * Windows version
 */

#include	"windows.h"

/* Ring the bell already.
 */
void ding(void)
{
    MessageBeep(0);
}

/* Display a message box. If action is NOTIFY_LOG, the text of the
 * message is written to stderr instead.
 */
void usermessage(int action, char const *prefix,
		 char const *cfile, unsigned long lineno,
		 char const *fmt, va_list args)
{
    static char	errbuf[4096];
    char       *p;

    p = errbuf;

    if (prefix)
	p += sprintf(p, "%s: ", prefix);
    if (fmt)
	p += vsprintf(p, fmt, args);
    if (cfile)
	p += sprintf(p, " [%s:%lu]", cfile, lineno);

    switch (action) {
      case NOTIFY_DIE:
	MessageBox(NULL, errbuf, "Tile World", MB_ICONSTOP | MB_OK);
	break;
      case NOTIFY_ERR:
	MessageBox(NULL, errbuf, "Tile World", MB_ICONEXCLAMATION | MB_OK);
	break;
      case NOTIFY_LOG:
	fputs(errbuf, stderr);
	fputc('\n', stderr);
	fflush(stderr);
	break;
    }
}

#else

/*
 * Unix version
 */

/* Ring the bell already.
 */
void ding(void)
{
    fputc('\a', stderr);
    fflush(stderr);
}

/* Display a formatted message on stderr.
 */
void usermessage(int action, char const *prefix,
		 char const *cfile, unsigned long lineno,
		 char const *fmt, va_list args)
{
    fprintf(stderr, "%s: ", action == NOTIFY_DIE ? "FATAL" :
			    action == NOTIFY_ERR ? "error" : "warning");
    if (cfile)
	fprintf(stderr, "[%s:%lu] ", cfile, lineno);
    if (prefix)
	fprintf(stderr, "%s: ", prefix);
    if (fmt)
	vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    fflush(stderr);
}

#endif
