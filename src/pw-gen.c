/**CFile***********************************************************************

   FileName    [pw-gen.c]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-04-04]

******************************************************************************/


/** Libraries and system dependencies **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#ifdef __linux__
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <fcntl.h>
#elif defined _WIN32 || defined _WIN64
    #define WIN32_LEAN_AND_MEAN
    #define WINZOZ
    #include <windows.h>
    #include <conio.h>
    #define OUTCNT "C:\pw-gen.log"
#else
    #error "Unknown platform"
#endif

/* Base header */
#include "pw-gen.h"
/* Library functions header */
#include "lib.h"
/* Generator functions header */
#include "generator.h"


/* Macros */
#define SUMTIME(tsEnd, tsBegin) (((double)tsEnd.tv_sec + (double)tsEnd.tv_nsec/NANO) - ((double)tsBegin.tv_sec + (double)tsBegin.tv_nsec/NANO))


/** Global variables **/
/* len=sequence lenght, nchars=number of character on current set (chars)
   left=left index of chars subset, right=right index of chars subset,
   procs=number of core (thus processes to start)
 */
int len, nchars, left, right;
double numSeq;      //counter for calculated sequences
char mode, dict[BUFF], *word, *pchars;   //mode=calc mode, dict=output filename, word=sequence, pchars=alternate/general set of chars
FILE *fout;

#ifdef __linux__
struct timespec tsBegin, tsEnd;   //monolitic time counter
double calcTime;    //store calculation time
int sigFlag;
#elif defined WINZOZ
float timeBegin, timeEnd;   //time counter
#endif

char chars[]={'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
              'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
              '0','1','2','3','4','5','6','7','8','9',
			  '!','?','"','$','%','&','/','(',')','=','^','<','>','+','*','-',',',';','.',':','@','#','[',']','|',' '
              //'è','é','ò','ç','à','ì','°','ù','§','£','€'  must use wchar.h
			  ,'\n'
              };


/* Local prototypes */
static inline void forkProc(int, void (*generator)(unsigned char));
static inline void printSyntax(char *);
static inline void argCheck(int, char **);
static inline int readChars();
static inline void setSeq();
static inline void setOper();
static inline void printResults();


/** Main **/
int main(int argc, char *argv[])
{
	int procs=0;

#ifdef __linux__
	//set signal handler for SIGINT (Ctrl-C)
	signal(SIGINT, sigHandler);
#endif

	//arguments check
	argCheck(argc, argv);

	//set number of processes based on number CPU cores found (x2)
	procs = procNumb();
	//procs *= 4;
	//procs = 1;

	//set sequences stuff
	setSeq();
	//set operating mode
	setOper();

	//start generation
	if (mode == 1)
		forkProc(procs, generatorWrite);
	else
		forkProc(procs, generatorCalc);
	//numSeq = pow(nchars, len);

	//results output
	printResults();

	freeExit();
	return (EXIT_SUCCESS);
}


/* Multi fork */
static inline void forkProc(int procs, void (*generator)(unsigned char))
{
#ifdef WINZOZ
	timeBegin = ((float)clock())/CLOCKS_PER_SEC;
	generatorSingle(0);
	timeEnd = ((float)clock())/CLOCKS_PER_SEC;
#elif defined __linux__

	int i, gap, rest, forks, pDescr[2], pp[2];
	pid_t *pid;
	errno = 0;

	//if single core, run simple generation
	if (procs == 1) {
		fprintf(stdout, "Starting worker 0: left %2d, right %2d\n", 0, nchars);
		clock_gettime(CLOCK_MONOTONIC, &tsBegin);
		generatorSingle(0);
		clock_gettime(CLOCK_MONOTONIC, &tsEnd);
		calcTime = SUMTIME(tsEnd, tsBegin);
		fprintf(stdout, "Worker 0: numSeq %lf, sec %lf, seq/s %lf\n", numSeq, calcTime, numSeq/calcTime);
		return;
	}

	sigFlag = FALSE;
	//fork counter
	forks = procs;
	//gap on number of chars and eventual rest
	gap = nchars / procs;
	rest = nchars % procs;

	//one more fork if there's rest
	if (rest != 0) {
		forks++;
		//and avoid rest bigger than standard gap
		while (rest > gap) {
			forks++;
			rest -= gap;
		}
	}

	//pids vector allocation
	if ((pid = (pid_t *)malloc(forks * sizeof(pid_t))) == NULL) {
		fprintf(stderr, "Error allocating memory (%d): %s.\n", procs, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}

	//open pipe to store data calculated by forks
	if (pipe(pDescr) == -1) {
		fprintf(stderr, "Error opening pipe: %s.\n", strerror(errno));
		free(pid);
		freeExit();
		exit (EXIT_FAILURE);
	}

	//initialize pipe semaphore
	semInit(pp);
	semSignal(pp);

	//START CRONOMETER
	clock_gettime(CLOCK_MONOTONIC, &tsBegin);

	for (i=0; i<forks; i++) {
		//fork
		pid[i] = fork();
		if (pid[i] == 0) {
			//start-end indexes
			left = gap*i;
			right = gap*i + gap;
			//if right index is out of bounds, set it to nchars
			if (right > nchars)
				right = nchars;

			fprintf(stdout, "Starting worker %2d: left %2d, right %2d\n", i, left, right);
			generator(0);
			//STOP local CRONOMETER
			clock_gettime(CLOCK_MONOTONIC, &tsEnd);
			//calculate elapsed time
			calcTime = SUMTIME(tsEnd, tsBegin);
			fprintf(stdout, "Worker %2d: numSeq %lf, sec %lf, seq/s %lf\n", i, numSeq, calcTime, numSeq/calcTime);

			//save to pipe, use of pipe semaphore
			semWait(pp);
			writePipe(pDescr, numSeq, calcTime);
			semSignal(pp);
			free(pid);
			freeExit();
			//the child can exit
			exit (EXIT_SUCCESS);
		}
	}
	sigFlag = TRUE;
	//main process wait until all childs have finished
	for (i=0; i<forks; i++)
		waitpid(pid[i], NULL, 0);
	semDestroy(pp);

	//get calculated data (written on pipe)
	close(pDescr[1]);
	calcTime = 0;
	for (i=0; i<forks; i++)
		numSeq += readPipe(pDescr, &calcTime);
	close(pDescr[0]);
	free(pid);

#ifdef DEBUG
	//final output
	clock_gettime(CLOCK_MONOTONIC, &tsEnd);
	fprintf(stdout, "\nAll workers have finished:\n\tCalc time\t%lf\n\tRun time\t%lf\n", calcTime, SUMTIME(tsEnd, tsBegin));
#endif

#endif
}


/* Print program syntax */
static inline void printSyntax(char *argv0)
{
	fprintf(stderr, "Syntax: %s [mode] <seq-lenght> <chars-set>\n", argv0);
	fprintf(stderr, "\t[mode]:\t\t1 = Write to file\n\t\t\t2 = Calcs only\n"
			"\t<seq-lenght>:\tset sequence lenght (integer)\n"
			"\t<chars-set>:\tS = set SHORT set of chars [a-z,A-Z,0-9]\n\t\t\tF = set FULL set of chars (default)\n\t\t\tP = set PERSONALIZED set of chars from file\n");
}


/* Arguments and sintax check */
static inline void argCheck(int argc, char **argv)
{
	int l;

	//wrong sintax
	if (argc < 2) {
		printSyntax(argv[0]);
		exit (EXIT_FAILURE);
	}
	//set operating mode and initializations
	mode = atoi(argv[1]);
	nchars = NUM_ALL;
	len = 0;
	pchars = chars;
	numSeq = 0;
	left = 0;
	right = nchars;
	word = NULL;
	fout = NULL;
	//set lenght OR chars subset if defined
	if (argc == 3) {
		len = atoi(argv[2]);
		if (len > 0)
			return;
		else
			len = 0;
		if (toupper(argv[3][0]) == 'S')
			nchars = NUM_STD;
		else if (toupper(argv[3][0]) == 'P') {
			nchars = readChars();
		}
	}
	//set lenght AND chars subset if defined
	else if (argc == 4) {
		//check argv2
		if ((l = atoi(argv[2])) <= 0) {
			if (toupper(argv[2][0]) == 'S')
				nchars = NUM_STD;
			else if (toupper(argv[2][0]) == 'P')
				nchars = readChars();
		}
		else
			len = l;
		//check argv3
		if ((l = atoi(argv[3])) <= 0) {
			if (toupper(argv[3][0]) == 'S')
				nchars = NUM_STD;
			else if (toupper(argv[3][0]) == 'P')
				nchars = readChars();
		}
		else
			len = l;
	}
}


/* Read set of chars from file */
static inline int readChars()
{
	FILE *fp;
	char buff[BUFF], source[BUFF];
	int dim, i;
	errno = 0;

	//get filename
	fprintf(stdout, "\nInput chars source file:\t");
	if (fgets(source, BUFF-1, stdin) == NULL) {
		fprintf(stderr, "Error reading input: %s.\n", strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}
	if (source[strlen(source)-1] == '\n')
		source[strlen(source)-1] = '\0';
	if ((fp = fopen(source, "r")) == NULL) {
		fprintf(stderr, "Error opening file \"%s\": %s.\n", source, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}
	//calculate lenght and thus memory allocation
	dim = 0;
	while (fgets(buff, BUFF-1, fp) != NULL)
		dim++;
	rewind(fp);
	if ((pchars = (char *)malloc(dim * sizeof(char))) == NULL) {
		fprintf(stderr, "Error allocating memory (%d): %s.\n", dim, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}

	//acquisition of chars
	for (i=0; i<dim; i++) {
		if (fgets(buff, BUFF-1, fp) == NULL) {
			fprintf(stderr, "Error reading from file \"%s\": %s.\n", source, strerror(errno));
			freeExit();
			exit (EXIT_FAILURE);
		}
		if (sscanf(buff, "%c", &pchars[i]) != 1) {
			fprintf(stderr, "Error reading from file \"%s\": wrong format.\n", source);
			freeExit();
			exit(EXIT_FAILURE);
		}
	}
	return dim;
}


/* Set and alloc sequence, lenght, and counters */
static inline void setSeq()
{
	char buff[BUFF];
	errno = 0;

	if (len == 0) {
		fprintf(stdout, "\nSequence lenght:\t");
		do {
			if (fgets(buff, BUFF-1, stdin) == NULL) {
				fprintf(stderr, "Error reading input: %s.\n", strerror(errno));
				freeExit();
				exit (EXIT_FAILURE);
			}
			if (sscanf(buff, "%d", &len) != 1)
				len = 0;
		} while (len <= 0 && fprintf(stderr, "Positive integers only! Try again:\t"));
	}
	if ((word = (char *)malloc(len * sizeof(char))) == NULL) {
		fprintf(stderr, "Error allocating memory (%d): %s.\n", len, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}
}


/* Set operating mode and output file */
static inline void setOper()
{
	char buff[BUFF];
	errno = 0;

	//mode=1: write to file
	if (mode == 1) {
		fprintf(stdout, "\nOutput file:\t");
		if (fgets(buff, BUFF-1, stdin) == NULL) {
			fprintf(stderr, "Error reading input: %s.\n", strerror(errno));
			freeExit();
			exit (EXIT_FAILURE);
		}
		sscanf(buff, "%s", dict);
		if ((fout = fopen(dict, "w")) == NULL) {
			fprintf(stderr, "Error opening file \"%s\": %s.\n", dict, strerror(errno));
			freeExit();
			exit (EXIT_FAILURE);
		}
	}
	else
		fout = stdout;
}


/* Print program results */
static inline void printResults()
{
#ifdef WINZOZ
	fprintf(stdout, "\nElapsed time (seconds):\t   %f\n", timeEnd-timeBegin);
	fprintf(stdout, "Sequences generated:\t   %.0lf\n", numSeq);
	fprintf(stdout, "Sequences/second:\t   %lf\n\n", numSeq/(timeEnd-timeBegin));

#elif defined __linux__
	fprintf(stdout, "\nElapsed time:\t\t   %lf sec\n", calcTime);
	fprintf(stdout, "Sequences generated:\t   %.0lf\n", numSeq);
	fprintf(stdout, "Sequences/second:\t   %lf\n\n", numSeq/calcTime);
#endif
}

