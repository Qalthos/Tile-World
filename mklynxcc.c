/* mklynxcc.c: Written by Brian Raiter, 2001.
 *
 * This is a quick hack that can be used to make .dat files that use
 * the Lynx ruleset. Run the program, giving it the name of a .dat
 * file, and the program will modify the .dat file to mark it as being
 * for the Lynx ruleset.
 *
 * This source code is in the public domain.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>

#ifndef	TRUE
#define	TRUE	1
#endif
#ifndef	FALSE
#define	FALSE	0
#endif

/* The magic numbers of the .dat file.
 */
#define	CHIPS_SIGBYTE_1		0xAC
#define	CHIPS_SIGBYTE_2		0xAA
#define	CHIPS_SIGBYTE_3		0x02

/* Error return values.
 */
#define	ERR_BAD_CMDLINE		127
#define	ERR_NOT_DATFILE		126
#define	ERR_NOT_MS_DATFILE	125

/* The file to alter.
 */
static FILE		       *fp = NULL;
static char const	       *file = NULL;

/* Replace a byte in the file.
 */
static int changebyte(unsigned long pos, unsigned char byte)
{
    if (fseek(fp, pos, SEEK_SET) == 0 && fputc(byte, fp) != EOF)
	return TRUE;
    perror(file);
    return FALSE;
}

/* Return FALSE if the file is not a .dat file.
 */
static int sanitycheck(void)
{
    unsigned char	header[4];
    int			n;

    n = fread(header, 1, 4, fp);
    if (n < 0) {
	perror(file);
	return FALSE;
    }
    if (n < 4 || header[0] != CHIPS_SIGBYTE_1
	      || header[1] != CHIPS_SIGBYTE_2
	      || header[2] != CHIPS_SIGBYTE_3) {
	fprintf(stderr, "%s is not a valid .dat file\n", file);
	errno = ERR_NOT_DATFILE;
	return FALSE;
    }
    if (header[3] != 0) {
	if (header[3] == 1)
	    fprintf(stderr, "%s is already set for the Lynx ruleset\n", file);
	else
	    fprintf(stderr, "%s is not set for the MS ruleset\n", file);
	errno = ERR_NOT_MS_DATFILE;
	return FALSE;
    }
    rewind(fp);
    return TRUE;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int n;

    if (argc < 2) {
	fprintf(stderr, "Usage: %s DATFILE ...\n", argv[0]);
	return ERR_BAD_CMDLINE;
    }
    for (n = 1 ; n < argc ; ++n) {
	file = argv[n];
	if ((fp = fopen(file, "r+b")) == NULL) {
	    perror(file);
	    ret = errno;
	    continue;
	}
	if (sanitycheck() && changebyte(3, 1))
	    printf("%s was successfully altered\n", file);
	else
	    ret = errno;
	fclose(fp);
    }
    return ret;
}
