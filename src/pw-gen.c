/**CFile***********************************************************************

   FileName    [pw-gen.c]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-05-24]

******************************************************************************/


/** Libraries and system dependencies **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#ifdef __linux__
    #include <unistd.h>
    #include <sys/wait.h>
    #include <fcntl.h>
#elif defined _WIN32 || defined _WIN64
    #define WIN32_LEAN_AND_MEAN
    #define WINZOZ
    #include <windows.h>
    #include <conio.h>
#else
    #error "Unknown platform"
#endif

/* Base header */
#include "pw-gen.h"
/* Library functions header */
#include "lib.h"
/* Generator functions header */
#include "generator.h"


/** Global variables **/
/* len=sequence lenght, nchars=number of character on current set (chars)
   left=left index of chars subset, right=right index of chars subset, procs=number of processes
 */
int len, nchars, left, right, procs;
double numSeq;      //counter for calculated sequences
char mode, dict[BUFF], *word, *pchars;   //mode=calc mode, dict=output filename, word=sequence, pchars=alternate/general set of chars
FILE *fout;

#ifdef __linux__
struct timespec tsBegin, tsEnd;   //monolitic time counter
double calcTime, lag;    //store calculation time and lag time (for process suspension)
int sigFlag, forks, sigSem[2];    //sigFlag=flag for father process, forks=number of actual forks to do, sigSem=pipe semaphore for signal handler

#elif defined WINZOZ
double timeBegin, timeEnd;   //time counters
#endif

char chars[]={'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
              'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
              '0','1','2','3','4','5','6','7','8','9',
			  '!','?','"','$','%','&','/','(',')','=','^','<','>','+','*','-',',',';','.',':','@','#','[',']','|',' '
              //'è','é','ò','ç','à','ì','°','ù','§','£','€'  must use wchar.h
			  ,'\n'
              };


/* Local prototypes */
static inline void forkProc(void (*generator)(unsigned char));
#ifdef __linux__
static inline int initFork(int *, pid_t **, int *, int *);
static inline void indexFork(int, int);
static inline void fatherFork(pid_t *, int *, int *);
#endif //__linux__



/** Main **/
int main(int argc, char *argv[])
{
#ifdef __linux__
	//set signal handler for SIGINT (Ctrl-C)
	signal(SIGINT, sigHandler);
#endif //__linux__

	//print program header
	printHeader();

	//arguments check
	argCheck(argc, argv);

	//set number of processes based on number of CPU cores found
	procs = procNumb();

	//set sequences stuff
	setSeq();
	//set operating mode
	setOper();

	//start generation
	if (mode == 1)
		forkProc(generatorWriteI);
	else
		forkProc(generatorCalcI);

	//results output
	printResults();

	freeExit();
	return (EXIT_SUCCESS);
}


/* Multi fork */
static inline void forkProc(void (*generator)(unsigned char))
{
#ifdef WINZOZ
	//open output file
    fileDict(1);
    fprintf(stdout, "Working...\n");
	timeBegin = ((double)clock())/CLOCKS_PER_SEC;
	generatorSingleI(0);
	timeEnd = ((double)clock())/CLOCKS_PER_SEC;

#elif defined __linux__
	int i, gap, rest=0, pData[2], pSem[2];
	pid_t *pid;
	errno = 0;

	//initialize suspension semaphore
	psemInit(sigSem);
	//set signal handler for SIGQUIT (Ctrl-\)
	signal(SIGQUIT, sigHandler);

	//if single core use single generator
	if (procs == 1)
		generator = generatorSingleI;

	//START main CRONOMETER
	clock_gettime(CLOCK_MONOTONIC, &tsBegin);
	//init stuff
	gap = initFork(&rest, &pid, pData, pSem);

	for (i=0; i<forks; i++) {
		//fork
		pid[i] = fork();
		if (pid[i] == 0) {
			//calculate current indexes
			indexFork(i, gap);
			//open output file in mode=1
			if (mode == 1)
				fileDict(i+1);
			//start watchThread
			watchThread(i+1);

			fprintf(stdout, "Starting worker %2d: left %2d, right %2d, seq %0.lf\n", i+1, left, right-1, SEQPART());
			//reSTART local CRONOMETER
			clock_gettime(CLOCK_MONOTONIC, &tsBegin);
			generator(0);
			//STOP local CRONOMETER
			clock_gettime(CLOCK_MONOTONIC, &tsEnd);
			//calculate elapsed time
			calcTime = SUMTIME(tsEnd, tsBegin) - lag;
			fprintf(stdout, "Finished worker %2d: %.0lf seq, %lf s, %lf seq/s\n", i+1, numSeq, calcTime, numSeq/calcTime);

			//save to pipe, use of pipe semaphore
			psemWait(pSem);
			writePipe(pData, numSeq, calcTime);
			psemSignal(pSem);
			free(pid);
			freeExit();
			//the child can exit
			exit (EXIT_SUCCESS);
		}
	}
	fatherFork(pid, pSem, pData);

#endif //__linux__
}


/* Linux specific functions */
#ifdef __linux__

/* Initialize stuff for process forking */
static inline int initFork(int *rest, pid_t **pid, int *pData, int *pSem)
{
	int gap;

	//init some stuff
	sigFlag = FALSE;
	forks = procs;
	//gap on number of chars and eventual rest
	gap = nchars / procs;
	*rest = nchars % procs;

	//one more fork if there's rest
	if (*rest != 0) {
		forks++;
		//and avoid rest bigger than standard gap
		while (*rest > gap) {
			forks++;
			*rest -= gap;
		}
	}

	//pids vector allocation
	if ((*pid = (pid_t *)malloc(forks * sizeof(pid_t))) == NULL) {
		fprintf(stderr, "Error allocating memory (%d): %s.\n", procs, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}

	//open pipe to store data calculated by forks
	if (pipe(pData) == -1) {
		fprintf(stderr, "Error opening pipe: %s.\n", strerror(errno));
		free(*pid);
		freeExit();
		exit (EXIT_FAILURE);
	}

	//initialize semaphore for pipe use
	psemInit(pSem);
	psemSignal(pSem);

	return gap;
}


/* Calculate indexes for job division */
static inline void indexFork(int i, int gap)
{
	//left and right indexes
	left = gap*i;
	right = gap*i + gap;
	//if right index is out of bounds, set it to nchars
	if (right > nchars)
		right = nchars;
}


/* Father process, wait and gather calculated data */
static inline void fatherFork(pid_t *pid, int *pSem, int *pData)
{
	int i;

	sigFlag = TRUE;
	//main process waits until all childs have finished
	for (i=0; i<forks; i++)
		waitpid(pid[i], NULL, 0);
	psemDestroy(pSem);

	//get calculated data (written on pipe)
	close(pData[1]);
	for (i=0; i<forks; i++)
		numSeq += readPipe(pData, &calcTime);
	close(pData[0]);
	free(pid);

#ifdef DEBUG
	//final output
	clock_gettime(CLOCK_MONOTONIC, &tsEnd);
	fprintf(stdout, "\nAll workers have finished:\n\tCalc time:\t%lf\n\tRun time:\t%lf\n\tTot time:\t%lf\n\tLag time:\t%lf\n", calcTime, SUMTIME(tsEnd, tsBegin) - lag, SUMTIME(tsEnd, tsBegin), lag);
#endif //DEBUG
}

#endif //__linux__

