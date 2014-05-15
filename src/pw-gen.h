/**CHeaderFile*****************************************************************

   FileName    [pw-gen.h]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Base header]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-05-15]

******************************************************************************/


#ifndef PWGEN_H_INCLUDED
#define PWGEN_H_INCLUDED


/* Version code - keep UPDATED! */
#define VERS "1.2.5"
#define BUILD "2014-05-15"

#define FALSE 0
#define TRUE 1
#define BUFF 256
#define NANO 1000000000

/* Chars numbers */
#define NUM_STD 62
#define NUM_ALL 88


/* Macros */
#define SUMTIME(tsEnd, tsBegin) (((double)tsEnd.tv_sec + (double)tsEnd.tv_nsec/NANO) - ((double)tsBegin.tv_sec + (double)tsBegin.tv_nsec/NANO))
#define SEQPART() (pow(nchars, len-1) * (right-left))


/** Global variables - See pw-gen.c for details **/
extern int len, nchars, left, right;
extern double numSeq;
extern char mode, dict[], *word, *pchars, chars[];
extern FILE *fout;

#ifdef __linux__
extern struct timespec tsBegin;
extern double calcTime, lag;
extern int sigFlag, procs, forks, sigSem[];
#endif //__linux__


#endif // PWGEN_H_INCLUDED

