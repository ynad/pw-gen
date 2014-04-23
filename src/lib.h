/**CHeaderFile*****************************************************************

   FileName    [lib.h]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Library functions]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-04-14]

******************************************************************************/


#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED


/* Clear memory and other stuff */
inline void freeExit();

/* CPU cores counter */
inline int procNumb();

/* Check current version with info on online repo */
inline int checkVersion();

#ifdef __linux__

/* Write data to pipe */
inline void writePipe(int *, double, double);

/* Read data from pipe */
inline double readPipe(int *, double *);

/* Pipe semaphore - Init */
inline void semInit(int *);

/* Pipe semaphore - Wait */
inline void semWait(int *);

/* Pipe semaphore - Signal */
inline void semSignal(int *);

/* Pipe semaphore - Destroy */
inline void semDestroy(int *);

/* Signals handler */
void sigHandler(int);

#endif


#endif // LIB_H_INCLUDED

