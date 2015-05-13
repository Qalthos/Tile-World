/* err.c: Error handling and reporting.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdlib.h>
#include	<stdarg.h>
#include	"oshw.h"
#include	"err.h"

/* "Hidden" arguments to warn_, errmsg_, and die_.
 */
char const      *err_cfile_ = NULL;
unsigned long	err_lineno_ = 0;

/* Log a warning message.
 */
void warn_(char const *fmt, ...)
{
    va_list	args;

    va_start(args, fmt);
    usermessage(NOTIFY_LOG, NULL, err_cfile_, err_lineno_, fmt, args);
    va_end(args);
    err_cfile_ = NULL;
    err_lineno_ = 0;
}

/* Display an error message to the user.
 */
void errmsg_(char const *prefix, char const *fmt, ...)
{
    va_list	args;

    va_start(args, fmt);
    usermessage(NOTIFY_ERR, prefix, err_cfile_, err_lineno_, fmt, args);
    va_end(args);
    err_cfile_ = NULL;
    err_lineno_ = 0;
}

/* Display an error message to the user and exit.
 */
void die_(char const *fmt, ...)
{
    va_list	args;

    va_start(args, fmt);
    usermessage(NOTIFY_DIE, NULL, err_cfile_, err_lineno_, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}
