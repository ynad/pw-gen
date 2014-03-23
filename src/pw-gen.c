/**CFile***********************************************************************

   FileName    [pw-gen.c]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-03-21]

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#ifdef __linux__
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <fcntl.h>
    #include <signal.h>
#elif defined _WIN32 || defined _WIN64
    #define WIN32_LEAN_AND_MEAN
    #define WINZOZ
    #include <windows.h>
    #include <conio.h>
    #define OUTCNT "C:\pw-gen.log"
#else
    #error "Unknown platform"
#endif


#define FALSE 0
#define TRUE 1
#define BUFF 256

//chars numbers
#define NUM_STD 62
#define NUM_ALL 88


/** Global variables **/
/* len=sequence lenght, nchars=number of character on current set (chars)
   left=left index of chars subset, right=right index of chars subset,
   procs=number of core (thus processes to start)
 */
int len, nchars, left, right, procs;
float timeBegin, timeEnd;   //time counter
double numSeq;   //counter for calculated sequences
FILE *fout;      //output file
char mode, dict[BUFF], *word, *pchars;   //mode=calc mode, dict=output filename, word=sequence, pchars=alternate/general set of chars

char chars[]={'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
              'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
              '0','1','2','3','4','5','6','7','8','9',
			  '!','?','"','$','%','&','/','(',')','=','^','<','>','+','*','-',',',';','.',':','@','#','[',']','|',' '
              //'è','é','ò','ç','à','ì','°','ù','§','£','€'  must use wchar.h
              };


/* Local prototypes */
static void forkProc(int, void (*generator)(unsigned char));
static void generatorSingle(unsigned char);
static void generatorCalc(unsigned char);
static void generatorWrite(unsigned char);
static inline void printSyntax(char *);
static inline void argCheck(int, char **);
static inline int readChars();
static inline void setSeq();
static inline void setOper();
static inline void freeExit();
static inline int procNumb();
#ifdef __linux__
static void sigHandler(int);
#endif


/* Main */
int main(int argc, char *argv[])
{
#ifdef __linux__
	//set signal handler for SIGINT (Ctrl-C)
	signal(SIGINT, sigHandler);
#endif

	//arguments check
	argCheck(argc, argv);

	//set number of processes based on number CPU cores found (x2)
	procs = procNumb();
	procs *= 2;

	//set sequences stuff
	setSeq();
	//set operating mode
	setOper();

	//start generation
	if (mode == 1)
		forkProc(procs, generatorWrite);
	else
		forkProc(procs, generatorCalc);

	//results output
	fprintf(stdout, "\nElapsed time (seconds):\t   %f\n", timeEnd-timeBegin);
	fprintf(stdout, "Sequences generated:\t   %.0lf\n", numSeq);
	fprintf(stdout, "Sequences/second:\t   %lf\n\n", numSeq/(timeEnd-timeBegin));

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

	int i, gap, rest, forks, pDescr[2];
	pid_t *pid;
	double sum;
	errno = 0;

	//if single core, run simple generation
	if (procs == 1) {
		timeBegin = ((float)clock())/CLOCKS_PER_SEC;
		generatorSingle(0);
		timeEnd = ((float)clock())/CLOCKS_PER_SEC;
		return;
	}

	if ((pid = (pid_t *)malloc(procs * sizeof(pid_t))) == NULL) {
		fprintf(stderr, "Error allocating memory (%d): %s.\n", procs, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}
	//fork counter
	forks = procs-1;
	//gap on number of chars
	gap = nchars / procs;
	//eventual rest on division by procs
	rest = nchars % procs;

	//open pipe to store calculated number of sequences
	if (pipe(pDescr) == -1) {
		fprintf(stderr, "Error opening pipe: %s.\n", strerror(errno));
		free(pid);
		freeExit();
		exit (EXIT_FAILURE);
	}

	//START CRONOMETER
	timeBegin = ((float)clock())/CLOCKS_PER_SEC;
	for (i=0; i<procs-1; i++) {
		//start-end indexes
		left = gap*i;
		right = gap*i + gap;
		//fork
		pid[i] = fork();
		if (pid[i] == 0) {
			fprintf(stdout, "Starting worker %2d: left %2d, right %2d\n", i, left, right);
			generator(0);
			//STOP local CRONOMETER
			timeEnd = ((float)clock())/CLOCKS_PER_SEC;
			fprintf(stdout, "Worker %2d: numSeq %lf, time %lf, seq/s %lf\n", i, numSeq, timeEnd-timeBegin, numSeq/(timeEnd-timeBegin));
			//save to pipe
			close(pDescr[0]);
			write(pDescr[1], &numSeq, sizeof(double));
			close(pDescr[1]);
			freeExit();
			//the child can exit
			exit(EXIT_SUCCESS);
		}
	}
	//new fork if there is rest
	if (rest != 0) {
		//start-end indexes
		left = nchars-rest;
		right = nchars;
		//fork
		pid[i] = fork();
		if(pid[i] == 0) {
			fprintf(stdout, "Starting worker %2d: left %2d, right %2d\n", i, left, right);
			generator(0);
			//STOP local CRONOMETER
			timeEnd = ((float)clock())/CLOCKS_PER_SEC;
			fprintf(stdout, "Worker %2d: numSeq %lf, time %lf, seq/s %lf\n", i, numSeq, timeEnd-timeBegin, numSeq/(timeEnd-timeBegin));
			//save to pipe
			close(pDescr[0]);
			write(pDescr[1], &numSeq, sizeof(double));
			close(pDescr[1]);
			freeExit();
			//the child can exit
			exit(EXIT_SUCCESS);
		}
		//increment counter of childs
		forks++;
	}
	//start last flow in main process
	left = gap*i;
	right = gap*i + gap;
	fprintf(stdout, "Starting worker %2d: left %2d, right %2d\n", forks, left, right);
	generator(0);

	//wait until all childs have finished
	for (i=0; i<forks; i++)
		waitpid(pid[i], NULL, 0);
	//STOP main CRONOMETER
	timeEnd = ((float)clock())/CLOCKS_PER_SEC;
	fprintf(stdout, "Worker %2d: numSeq %lf, time %lf, seq/s %lf\n", forks, numSeq, timeEnd-timeBegin, numSeq/(timeEnd-timeBegin));

	//sum calculated data (written on pipe)
	close(pDescr[1]);
	sum = 0;
	for (i=0; i<forks; i++) {
		read(pDescr[0], &sum, sizeof(double));
		numSeq += sum;
	}
	free(pid);
	close(pDescr[0]);
#endif
}


/* Basic recursive generator */
static void generatorSingle(unsigned char pos)
{
    int i;

    if (pos == len) {
        numSeq++;
        if (mode == 1)
            fprintf(fout, "%s\n", word);
        return;
    }
    for (i=0; i<nchars; i++) {
        word[pos] = pchars[i];
        generatorSingle(pos+1);
    }
}


/* Recursive generator - CALC only */
static void generatorCalc(unsigned char pos)
{
    int i;

	//word completed
    if (pos == len) {
        numSeq++;
        return;
    }
	//first char
	if (pos == 0) {
		for (i=left; i<right; i++) {
			word[pos] = pchars[i];
			generatorCalc(pos+1);
		}
	}
	else {
		for (i=0; i<nchars; i++) {
			word[pos] = pchars[i];
			generatorCalc(pos+1);
		}
	}
}


/* Recursive generator - WRITE to file */
static void generatorWrite(unsigned char pos)
{
    int i;

	//word completed
    if (pos == len) {
        numSeq++;
		fprintf(fout, "%s\n", word);
        return;
    }
	//first char
	if (pos == 0) {
		for (i=left; i<right; i++) {
			word[pos] = pchars[i];
			generatorWrite(pos+1);
		}
	}
	else {
		for (i=0; i<nchars; i++) {
			word[pos] = pchars[i];
			generatorWrite(pos+1);
		}
	}
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
	procs = 0;
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
	fgets(source, BUFF-1, stdin);
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
		fgets(buff, BUFF-1, fp);
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
			fgets(buff, BUFF-1, stdin);
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
		fgets(buff, BUFF-1, stdin);
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


/* Clear memory and other stuff */
static inline void freeExit()
{
	if (fout != NULL && fout != stdout)
		fclose(fout);
	if (pchars != chars)
		free(pchars);
	free(word);
}


/* CPU cores counter */
static inline int procNumb()
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


/* Signals handler */
#ifdef __linux__
static void sigHandler(int sig)
{
	if (sig == SIGINT)
		fprintf(stderr, "\nReceived signal SIGINT (%d): exiting.\n", sig);
	else
		fprintf(stderr, "\nReceived signal %d: exiting.\n", sig);

	//pre-exit stuff
	//fprintf(stdout, "Last word: \"%s\"\n", word);
	freeExit();

	//exit
	exit(EXIT_FAILURE);
}
#endif

