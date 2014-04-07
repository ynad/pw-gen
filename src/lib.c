/**CFile***********************************************************************

   FileName    [lib.c]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Library functions]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-04-03]

******************************************************************************/


/** Libraries and system dependencies **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __linux__
    #include <unistd.h>
    #include <signal.h>
#elif defined _WIN32 || defined _WIN64
    #define WIN32_LEAN_AND_MEAN
    #define WINZOZ
    #include <windows.h>
#else
    #error "Unknown platform"
#endif

/* Base header */
#include "pw-gen.h"
/* Library functions header */
#include "lib.h"


/* Clear memory and other stuff */
inline void freeExit()
{
	if (fout != NULL && fout != stdout)
		fclose(fout);
	if (pchars != chars)
		free(pchars);
	free(word);
}


/* CPU cores counter */
inline int procNumb()
{
	int nprocs = -1, nprocs_max = -1;

#ifdef WINZOZ
#ifndef _SC_NPROCESSORS_ONLN
	SYSTEM_INFO info;
	GetSystemInfo(&info);
#define sysconf(a) info.dwNumberOfProcessors
#define _SC_NPROCESSORS_ONLN
#endif
#endif

#ifdef _SC_NPROCESSORS_ONLN
	nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	if (nprocs < 1) {
		//fprintf(stderr, "Could not determine number of CPUs online:\n%s\n", strerror (errno));
		return nprocs;
	}
	nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
	if (nprocs_max < 1)	{
		//fprintf(stderr, "Could not determine number of CPUs configured:\n%s\n", strerror (errno));
		return nprocs_max;
	}
	//fprintf(stdout, "%d of %d processors online\n", nprocs, nprocs_max);
	return nprocs;
#else
	//fprintf(stderr, "Could not determine number of CPUs.\n");
	return -1;
#endif
}


#ifdef __linux__

/* Write data to pipe */
inline void writePipe(int *pDescr, double seq, double time)
{
	errno = 0;
	close(pDescr[0]);
	//write sequences calculated
	if (write(pDescr[1], &seq, sizeof(double)) == -1)
		fprintf(stderr, "Error writing to pipe: %s\n", strerror(errno));
	//write time elapsed
	if (write(pDescr[1], &time, sizeof(double)) == -1)
		fprintf(stderr, "Error writing to pipe: %s\n", strerror(errno));
	close(pDescr[1]);
}


/* Read data from pipe */
inline double readPipe(int *pDescr, double *time)
{
	double data=0, seq=0;
	errno = 0;
	//read sequences calculated
	if (read(pDescr[0], &data, sizeof(double)) == -1) {
		fprintf(stderr, "Error reading from pipe: %s\n", strerror(errno));
		data = 0;
	}
	seq += data;
	//read time elapsed
	if (read(pDescr[0], &data, sizeof(double)) == -1) {
		fprintf(stderr, "Error reading from pipe: %s\n", strerror(errno));
		data = 0;
	}
	if (*time < data)
		*time = data;
	return seq;
}


/* Pipe semaphore - Init */
inline void semInit(int *s)
{
	errno = 0;
	if (pipe(s) == -1) {
		fprintf(stderr, "Error initializing semaphore: %s.\n", strerror(errno));
		exit (EXIT_FAILURE);
	}
}


/* Pipe semaphore - Wait */
inline void semWait(int *s)
{
	char ch;
	errno = 0;
	if (read(s[0], &ch, sizeof(char)) != 1) {
		fprintf(stderr, "Error reading from semaphore: %s.\n", strerror(errno));
		exit (EXIT_FAILURE);
	}
}


/* Pipe semaphore - Signal */
inline void semSignal(int *s)
{
	char ch='Z';
	errno = 0;
	if (write(s[1], &ch, sizeof(char)) != 1) {
		fprintf(stderr, "Error writing to semaphore: %s.\n", strerror(errno));
		exit (EXIT_FAILURE);
	}
}


/* Pipe semaphore - Destroy */
inline void semDestroy(int *s)
{
	close(s[0]);
	close(s[1]);
}


/* Signals handler */
void sigHandler(int sig)
{
	if (sig == SIGINT && sigFlag == TRUE)
		fprintf(stderr, "\nReceived signal SIGINT (%d): exiting.\n", sig);

	//pre-exit stuff
	freeExit();

	//exit
	exit (EXIT_FAILURE);
}

#endif

