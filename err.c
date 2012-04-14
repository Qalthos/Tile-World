/* err.c: Error handling and reporting.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdlib.h>
#include	<stdarg.h>
#include	"oshw.h"
#include	"err.h"

/* "Hidden" arguments to _warn, _errmsg, and _die.
 */
char const      *_err_cfile = NULL;
unsigned long	_err_lineno = 0;

/* Log a warning message.
 */
void _warn(char const *fmt, ...)
{
    va_list	args;

    va_start(args, fmt);
    usermessage(NOTIFY_LOG, NULL, _err_cfile, _err_lineno, fmt, args);
    va_end(args);
    _err_cfile = NULL;
    _err_lineno = 0;
}

/* Display an error message to the user.
 */
void _errmsg(char const *prefix, char const *fmt, ...)
{
    va_list	args;

    va_start(args, fmt);
    usermessage(NOTIFY_ERR, prefix, _err_cfile, _err_lineno, fmt, args);
    va_end(args);
    _err_cfile = NULL;
    _err_lineno = 0;
}

/* Display an error message to the user and exit.
 */
void _die(char const *fmt, ...)
{
    va_list	args;

    va_start(args, fmt);
    usermessage(NOTIFY_DIE, NULL, _err_cfile, _err_lineno, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}
