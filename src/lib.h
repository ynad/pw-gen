/**CHeaderFile*****************************************************************

   FileName    [lib.h]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Library functions]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-09-22]

******************************************************************************/


#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED


/* Print program header */
inline void printHeader();

/* Print program syntax */
inline void printSyntax(char *);

/* Print program results */
inline void printResults();

/* Set and alloc sequence and lenght */
inline void setSeq();

/* Set operating mode and output file */
inline void setOper();

/* Arguments and sintax check */
inline void argCheck(int argc, char **argv);

/* Read set of chars from file */
inline int readChars(char *);

/* Open output n-file */
inline void fileDict(int);

/* Clear memory and other stuff */
inline void freeExit();

/* CPU cores counter */
inline int procNumb();

/* Convert and print seconds to human legible format */
inline void secstoHuman(double, int);


/* Linux specific functions */
#ifdef __linux__

/* Print statistics related to file size and write speed */
inline void filesizeStats(double, double *);

/* Check current version with info on online repo */
inline int checkVersion();

/* Write data to pipe */
inline void writePipe(int *, double, double);

/* Read data from pipe */
inline double readPipe(int *, double *);

/* Pipe semaphore - Init */
inline void psemInit(int *);

/* Pipe semaphore - Wait */
inline void psemWait(int *);

/* Pipe semaphore - Signal */
inline void psemSignal(int *);

/* Pipe semaphore - Destroy */
inline void psemDestroy(int *);

/* Signals handler */
void sigHandler(int);

/* Watch thread caller */
inline void watchThread(int);

#endif //__linux__


#endif // LIB_H_INCLUDED

