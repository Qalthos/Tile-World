/* cmdline.c: a reentrant version of getopt().
 *
 * Written 1997-2001 by Brian Raiter. This code is in the public
 * domain.
 */

#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"gen.h"
#include	"cmdline.h"

/* Initialize the state structure.
 */
void initoptions(cmdlineinfo *opt, int argc, char **argv, char const *list)
{
    opt->options = list;
    opt->argc = argc;
    opt->argv = argv;
    opt->index = 0;
    opt->argptr = NULL;
    opt->stop = FALSE;
}

/* Find the next option on the cmdline.
 */
int readoption(cmdlineinfo *opt)
{
    char const *str;

    if (!opt->options || !opt->argc || !opt->argv)
	return -1;

    /* If argptr is NULL or points to a \0, then we're done with this
     * argument, and it's time to move on to the next one (if any).
     */
  redo:
    if (!opt->argptr || !*opt->argptr) {
	if (opt->index >= opt->argc) {
	    opt->type = OPT_END;
	    return -1;
	}
	opt->argptr = opt->argv[opt->index];
	++opt->index;

	/* Special case: if the next argument is "--", we skip over it and
	 * stop looking for options for the rest of the cmdline.
	 */
	if (!opt->stop && opt->argptr && opt->argptr[0] == '-'
				      && opt->argptr[1] == '-'
				      && opt->argptr[2] == '\0') {
	    opt->argptr = NULL;
	    opt->stop = TRUE;
	    goto redo;
	}

	/* Arguments not starting with a '-' or appearing after a
	 * "--" argument are not options.
	 */
	if (*opt->argptr != '-' || opt->stop) {
	    opt->opt = 0;
	    opt->val = opt->argptr;
	    opt->type = OPT_NONOPTION;
	    opt->argptr = NULL;
	    return 0;
	}

	/* Check for special cases.
	 */
	++opt->argptr;
	if (!*opt->argptr) {			/* The "-" case. */
	    opt->opt = 0;
	    opt->val = opt->argptr - 1;
	    opt->type = OPT_DASH;
	    return 0;
	}
	if (*opt->argptr == '-') {
	    opt->opt = 0;			/* The "--foo" case. */
	    opt->val = opt->argptr - 1;
	    opt->type = OPT_LONG;
	    opt->argptr = NULL;
	    return '-';
	}
    }

    /* We are currently looking at the next option.
     */
    opt->type = OPT_OPTION;
    opt->opt = *opt->argptr;
    ++opt->argptr;

    /* Is it on the list? If so, does it expect a value to follow?
     */
    str = strchr(opt->options, opt->opt);
    if (!str) {
	opt->val = opt->argptr - 1;
	opt->type = OPT_BADOPTION;
	return '?';
    } else if (str[1] == ':') {
	if (*opt->argptr) {			/* Is the value here? */
	    opt->val = opt->argptr;
	    opt->argptr = NULL;
	} else {
	    if (opt->index >= opt->argc) {	/* Or in the next argument? */
		opt->val = NULL;
		opt->type = OPT_NOVALUE;
		return ':';
	    } else {
		opt->val = opt->argv[opt->index];
		++opt->index;
	    }
	}
    } else
	opt->val = NULL;

    return opt->opt;
}

/* Ignore the next argument on the cmdline.
 */
int skipoption(cmdlineinfo *opt)
{
    if (opt->index >= opt->argc)
	return -1;
    opt->val = opt->argv[opt->index];
    ++opt->index;
    return 0;
}
