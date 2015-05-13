/* err.h: Error handling and reporting.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_err_h_
#define	HEADER_err_h_

/* Simple macros for dealing with memory allocation simply.
 */
#define	memerrexit()	(die("out of memory"))
#define	x_alloc(p, n)	(((p) = realloc((p), (n))) || (memerrexit(), 0))

#ifdef __cplusplus
extern "C" {
#endif

/* Log an error message and continue.
 */
extern void warn_(char const *fmt, ...);

/* Display an error message.
 */
extern void errmsg_(char const *prefix, char const *fmt, ...);

/* Display an error message and abort.
 */
extern void die_(char const *fmt, ...);

#ifdef __cplusplus
}
#endif

/* A really ugly hack used to smuggle extra arguments into variadic
 * functions.
 */
extern char const      *err_cfile_;
extern unsigned long	err_lineno_;
#define	warn	(err_cfile_ = __FILE__, err_lineno_ = __LINE__, warn_)
#define	errmsg	(err_cfile_ = __FILE__, err_lineno_ = __LINE__, errmsg_)
#define	die	(err_cfile_ = __FILE__, err_lineno_ = __LINE__, die_)

#endif
