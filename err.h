/* err.h: Error handling and reporting.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	_err_h_
#define	_err_h_

/* Simple macros for dealing with memory allocation simply.
 */
#define	memerrexit()	(die("out of memory"))
#define	xalloc(p, n)	(((p) = realloc((p), (n))) || (memerrexit(), NULL))

#ifdef __cplusplus
extern "C" {
#endif

/* Log an error message and continue.
 */
extern void _warn(char const *fmt, ...);

/* Display an error message.
 */
extern void _errmsg(char const *prefix, char const *fmt, ...);

/* Display an error message and abort.
 */
extern void _die(char const *fmt, ...);

#ifdef __cplusplus
}
#endif

/* A really ugly hack used to smuggle extra arguments into variadic
 * functions.
 */
extern char const      *_err_cfile;
extern unsigned long	_err_lineno;
#define	warn	(_err_cfile = __FILE__, _err_lineno = __LINE__, _warn)
#define	errmsg	(_err_cfile = __FILE__, _err_lineno = __LINE__, _errmsg)
#define	die	(_err_cfile = __FILE__, _err_lineno = __LINE__, _die)

#endif
