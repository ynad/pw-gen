/**CHeaderFile*****************************************************************

   FileName    [pw-gen.h]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Base header]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-04-04]

******************************************************************************/


#ifndef PWGEN_H_INCLUDED
#define PWGEN_H_INCLUDED


/* Version code - keep UPDATED! */
#define VERS "1.2.2"
#define BUILD "2014-04-04"

#define FALSE 0
#define TRUE 1
#define BUFF 256
#define NANO 1000000000

/* Chars numbers */
#define NUM_STD 62
#define NUM_ALL 88


/** Global variables - See pw-gen.c for details **/
extern int len, nchars, left, right;
extern double numSeq;
extern FILE *fout;
extern char mode, chars[], *word, *pchars;

#ifdef __linux__
extern double calcTime;
extern int sigFlag;
#endif


#endif // PWGEN_H_INCLUDED

