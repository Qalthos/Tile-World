/* cmdline.h: a reentrant version of getopt().
 *
 * Written 1997-2001 by Brian Raiter. This code is in the public
 * domain.
 */

#ifndef	HEADER_cmdline_h_
#define	HEADER_cmdline_h_

/* Begin by calling initoptions with your argc-argv pair, a string
 * containing a list of valid options, and an cmdlineinfo structure.
 * In the string listing the valid options, a character can have a
 * colon after it to indicate that the option is to be followed with a
 * value. (If -: is a valid option, put it first in the string.) The
 * cmdlineinfo structure is used when calling readoption, and also
 * contains information about each option (analagous to the extern
 * variables in getopt). After this, call readoption repeatedly to
 * find and identify an option on the command line.
 *
 * For example:
 *
 *	initoptions(&opt, "cdL:rnx:", argc - 1, argv + 1);
 *
 * indicates that -c, -d, -r, and -n are legal boolean options, and that
 * -L and -x are legal options that include a value. If the cmdline was:
 *
 *	prog -dLfoo -n bar -x baz
 *
 * then the first three calls to readoption would find -d, -L, and -n
 * with -L having the value "foo" attached to it. Calling readoption a
 * fourth time would find the non-option argument "bar". The fifth
 * call would find -x with "baz" as its accompanying value. A sixth
 * call to readoption would return EOF, indicating the end of the
 * cmdline.
 *
 * Written by Brian Raiter, 1997, with minor modifications 1997-2001.
 * This code is in the public domain.
 */

/* Possible values for the type field, indicating the various kinds of
 * creatures to be found on the cmdline.
 */
enum {
    OPT_OPTION,		/* a legal option */
    OPT_NONOPTION,	/* a plain, non-option argument */
    OPT_BADOPTION,	/* an unrecognized option */
    OPT_NOVALUE,	/* a legal option without a value */
    OPT_LONG,		/* a GNU-style --long-option */
    OPT_DASH,		/* a bare "-" */
    OPT_END		/* end of the cmdline */
};

/* The cmdline info structure. The first three fields are supplied by
 * the caller via initoptions(). The next three fields provide
 * information back to the caller. The remaining fields are used
 * internally by readoption().
 */
typedef struct cmdlineinfo {
    char const *options;	/* the list of valid options */
    int		argc;		/* cmdline's argc */
    char      **argv;		/* cmdline's argv */
    int		opt;		/* the current option (character value) */
    int		type;		/* type of current option (see below) */
    char       *val;		/* value accompanying the option */
    int		index;		/* index of the next argument */
    char       *argptr;		/* ptr to the next option/argument */
    int		stop;		/* flag indicating no further options */
} cmdlineinfo;

/* initoptions() initializes the cmdline parsing. opt is an empty
 * cmdlineinfo structure to initialize. list is a string containing
 * the recognized options, in the same style as used by getopt. argc
 * and argv give the cmdline to parse. Note that argv[0] is NOT
 * automatically skipped; the caller should use argc - 1 and argv + 1.
 */
extern void initoptions(cmdlineinfo *opt,
			int argc, char **argv, char const* list);

/* readoption() locates and identifies the next option on the cmdline.
 * Returns the character of the option found, which is also stored in
 * opt->opt, and opt->type will be set to OPT_OPTION (and opt->val
 * will point to the option's associated value if appropriate), or
 * OPT_BADOPTION if the option character is not recognized, or
 * OPT_NOVALUE if the option requires a value which is not present. A
 * return value of 0 indicates that a non-option argument has been
 * found, in which case opt->type will contain OPT_NONOPTION or
 * OPT_DASH, and opt->val will contain a pointer to the argument. A
 * return value of EOF indicates that the entire cmdline has been
 * parsed.
 */
extern int readoption(cmdlineinfo *opt);

/* skipoption() causes the current cmdline argument to be skipped over
 * on the next call to readoption(). This is useful for situations
 * when an option has multiple values (i.e., a value with spaces). The
 * return value is 0, or EOF if the end of the cmdline is reached.
 */
extern int skipoption(cmdlineinfo *opt);

#endif
