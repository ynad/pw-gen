/**CFile***********************************************************************

   FileName    [lib.c]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Library functions]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-06-04]

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
    #include <signal.h>
   	#include <pthread.h>
   	#include <semaphore.h>
   	#include <sys/stat.h>
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

//file extension for dictionary
#define EXTN "txt"
//thread's sleep time
#define SLEPTM 20
//maximum percentage for thread monitoring
#define MAXPERC 90.0
//megabyte
#define MEGA 1000000

//URL to repo and file containing version info
#define REPO "github.com/ynad/pw-gen/"
#define URLVERS "https://raw.github.com/ynad/pw-gen/master/VERSION"


#ifdef __linux__
/** Global variables **/
static pthread_t tid;
static int thrId;
static sem_t semThr;	//posix sem for threads

/* Local prototypes */
static void *threadRunner();
#endif //__linux__



/* Print program header */
inline void printHeader()
{
    fprintf(stdout, "\n =======================================================================\n");
    fprintf(stdout, "         Pw-Gen  |  Sequences generator - v. %s (%s)\n", VERS, BUILD);
    fprintf(stdout, " =======================================================================\n\n");
}


/* Print program syntax */
inline void printSyntax(char *argv0)
{
	fprintf(stderr, "Syntax: %s [mode] <seq-lenght> <chars-set>\n", argv0);
	fprintf(stderr, "\t[mode]:\t\t1 = Write to file\n\t\t\t2 = Calcs only\n"
			"\t<seq-lenght>:\tset sequence lenght (integer)\n"
			"\t<chars-set>:\tS = set SHORT set of chars [a-z,A-Z,0-9]\n\t\t\tF = set FULL set of chars (default)\n\t\t\tP = set PERSONALIZED set of chars from file\n");
}


/* Print program results */
inline void printResults()
{
		fprintf(stdout, "\nElapsed time:\t\t   ");
#ifdef WINZOZ
	secstoHuman(timeEnd-timeBegin, TRUE);
	fprintf(stdout, "\n");
	fprintf(stdout, "Sequences generated:\t   %.0lf\n", numSeq);
	fprintf(stdout, "Sequences/second:\t   %lf\n\n", numSeq/(timeEnd-timeBegin));

#elif defined __linux__
	secstoHuman(calcTime, TRUE);
	fprintf(stdout, "\n");
	fprintf(stdout, "Sequences generated:\t   %.0lf\n", numSeq);
	fprintf(stdout, "Sequences/second:\t   %lf\n\n", numSeq/calcTime);
#endif //__linux__
}


/* Set and alloc sequence and lenght */
inline void setSeq()
{
	char buff[BUFF];
#ifdef __linux__
    char c;
#endif // __linux__
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

#ifdef __linux__
		//check program updates
		fprintf(stdout, "\nDo you want to check for available updates? (requires internet connection!)  [Y-N]\n");
		do {
			if (fgets(buff, BUFF-1, stdin) == NULL) {
				fprintf(stderr, "Error reading input: %s.\n", strerror(errno));
				freeExit();
				exit (EXIT_FAILURE);
			}
			sscanf(buff, "%c", &c);
			c = toupper(c);
		} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));
		if (c == 'Y')
			checkVersion();
#endif //__linux__
	}
	//alloc word string (+1 for newline, used for printing to file)
	if ((word = (char *)malloc((len+1) * sizeof(char))) == NULL) {
		fprintf(stderr, "Error allocating memory (%d): %s.\n", len, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}
	word[len] = '\n';
	fprintf(stdout, "Lenght: %d, chars: %d, number of expected sequences: %.0lf\n", len, nchars, pow(nchars, len));
	fprintf(stdout, "Press ");
#ifdef __linux__
    fprintf(stdout, "Ctrl+\\ to pause or ");
#endif // __linux__
	fprintf(stdout, "Ctrl+C to exit.\n\n");
}


/* Set operating mode and output filename */
inline void setOper()
{
	char buff[BUFF];
	double size;
	errno = 0;

	//mode=1: write to file
	if (mode == 1) {
		fprintf(stdout, "Output file (NO file extension):\t");
		if (fgets(buff, BUFF-1, stdin) == NULL) {
			fprintf(stderr, "Error reading input: %s.\n", strerror(errno));
			freeExit();
			exit (EXIT_FAILURE);
		}
		sscanf(buff, "%s", dict);
		size = (len + 1) * pow(nchars, len) / MEGA;
		fprintf(stdout, "Expected total file size: %.2lf MB\n\n", size);
	}
	else
		fout = stdout;
}


/* Arguments and sintax check */
inline void argCheck(int argc, char **argv)
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
inline int readChars()
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


/* Open output n-file */
inline void fileDict(int n)
{
	//add file number and file extension
	sprintf(dict, "%s-%02d.%s", dict, n, EXTN);
	//and open it in binary mode
	if ((fout = fopen(dict, "wb")) == NULL) {
			fprintf(stderr, "Error opening file \"%s\": %s.\n", dict, strerror(errno));
			freeExit();
			exit (EXIT_FAILURE);
	}
}


/* Clear memory and other stuff */
inline void freeExit()
{
	if (pchars != chars)
		free(pchars);
	free(word);
	/* Disabled fclose() due to high load with big files - To re-enable uncomment also semaphore init in initFork() !
	if (mode == 1 && fout != NULL && fout != stdout) {
		//avoid program lock on file closing
#ifdef __linux__
		psemWait(fileSem);
#endif //__linux__
		fclose(fout);
#ifdef __linux__
		psemSignal(fileSem);
		psemDestroy(fileSem);
#endif //__linux__
	}*/
#ifdef __linux__
	psemDestroy(sigSem);
#endif //__linux__
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
#endif //_SC_NPROCESSORS_ONLN
#endif //WINZOZ

#ifdef _SC_NPROCESSORS_ONLN
	nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	if (nprocs < 1) {
#ifdef DEBUG
		fprintf(stderr, "Could not determine number of CPUs online:\n%s\n\n", strerror (errno));
#endif //DEBUG
		return nprocs;
	}
	nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
	if (nprocs_max < 1)	{
#ifdef DEBUG
		fprintf(stderr, "Could not determine number of CPUs configured:\n%s\n\n", strerror (errno));
#endif //DEBUG
		return nprocs_max;
	}
#ifdef DEBUG
	fprintf(stdout, "%d of %d processors online\n\n", nprocs, nprocs_max);
#endif //DEBUG
	return nprocs;
#else
#ifdef DEBUG
	fprintf(stderr, "Could not determine number of CPUs.\n\n");
#endif //DEBUG
	return -1;
#endif //_SC_NPROCESSORS_ONLN
}


/* Convert and print seconds to human legible format */
inline void secstoHuman(double sec, int precision)
{
	double s, m, h, d, y;			//values without rest
	double min, hour, day, year;	//values with rest

	//from seconds calculate eventual minutes, hours, days and years
	m = h = d = y = 0;
	min = hour = day = year = 0;
	s = fmod(sec, 60);
	if (sec >= 60) {
		m = (sec - s) / 60;
		if (m >= 60) {
			min = m;
			m = fmod(min, 60);
			h = (min - m) / 60;
			if (h >= 24) {
				hour = h;
				h = fmod(hour, 24);
				d = (hour - h) / 24;
				if (d >= 365)  {
					day = d;
					d = fmod(day, 365);
					y = (day - d) / 365;
				}
			}
		}
	}
	//output
	if (y > 0)
		fprintf(stdout, "%.0lf years ", y);
	if (d > 0)
		fprintf(stdout, "%.0lf days ", d);
	fprintf(stdout, "%02.0lf:%02.0lf:", h, m);
	//print full fractional part for high precision
	if (precision == TRUE)
		fprintf(stdout, "%02lf", s);
	else
		//printf may approximate values between 59.5 and 59.9 to 60
		fprintf(stdout, "%02.0lf", s);
}


/* Linux specific functions */
#ifdef __linux__

/* Print statistics related to file size and write speed */
inline void filesizeStats(double sec, double *oldSize)
{
	struct stat stt;
	double size, speed;

	stat(dict, &stt);
	size = (double)stt.st_size / MEGA;
	if (oldSize == NULL)
		speed = size / sec;
	else {
		speed = (size - (*oldSize)) / sec;
		*oldSize = size;
	}
	fprintf(stdout, ", size %.1lf MB (%.1lf MB/s)", size, speed);
}


/* Check current version with info on online repo */
inline int checkVersion()
{
	char cmd[BUFF], vers[BUFF], build[BUFF];
	FILE *fp;

	//clear error value
	errno = 0;

	//download file
	sprintf(cmd, "wget %s -O /tmp/VERSION -q", URLVERS);
	if (system(cmd) == -1) {
		fprintf(stderr, "Error executing system call: %s\n", strerror(errno));
		return (EXIT_FAILURE);
	}

	if ((fp = fopen("/tmp/VERSION", "r")) == NULL) {
		fprintf(stderr, "Error reading from file \"%s\": %s\n", "/tmp/VERSION", strerror(errno));
		return (EXIT_FAILURE);
	}
	if (fscanf(fp, "%s %s", vers, build) != 2) {
		fprintf(stderr, "No data collected or no internet connection.\n\n");
		vers[0] = '-';
		vers[1] = '\0';
		build[0] = '-';
		build[1] = '\0';
	}
	else {
		if (strcmp(vers, VERS) < 0 || (strcmp(vers, VERS) == 0 && strcmp(build, BUILD) < 0)) {
			fprintf(stdout, "Newer version is in use (local: %s [%s], repo: %s [%s]).\n\n", VERS, BUILD, vers, build);
		}
		else if (strcmp(vers, VERS) == 0 && strcmp(build, BUILD) == 0) {
			fprintf(stdout, "Up-to-date version is in use (%s [%s]).\n\n", VERS, BUILD);
		}
		else {
			fprintf(stdout, "Version in use: %s (%s), available: %s (%s)\nCheck \"%s\" for updates!\n\n", VERS, BUILD, vers, build, REPO);
		}
	}
	fclose(fp);
	if (system("rm -f /tmp/VERSION") == -1) {
		fprintf(stderr, "Error executing system call: %s\n", strerror(errno));
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}


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
inline void psemInit(int *s)
{
	errno = 0;
	if (pipe(s) == -1) {
		fprintf(stderr, "Error initializing semaphore: %s.\n", strerror(errno));
		exit (EXIT_FAILURE);
	}
}


/* Pipe semaphore - Wait */
inline void psemWait(int *s)
{
	char ch;
	errno = 0;
	if (read(s[0], &ch, sizeof(char)) != 1) {
		fprintf(stderr, "Error reading from semaphore: %s.\n", strerror(errno));
		exit (EXIT_FAILURE);
	}
}


/* Pipe semaphore - Signal */
inline void psemSignal(int *s)
{
	char ch='Z';
	errno = 0;
	if (write(s[1], &ch, sizeof(char)) != 1) {
		fprintf(stderr, "Error writing to semaphore: %s.\n", strerror(errno));
		exit (EXIT_FAILURE);
	}
}


/* Pipe semaphore - Destroy */
inline void psemDestroy(int *s)
{
	close(s[0]);
	close(s[1]);
}


/* Signals handler */
void sigHandler(int sig)
{
	int i;
	struct timespec ts1, ts2;

	//Ctrl-C: whole program exits
	if (sig == SIGINT) {
		//only father enters
		if (sigFlag == TRUE)
			fprintf(stderr, "\nReceived signal SIGINT (%d): exiting.\n", sig);
		//only childs
		else
			sem_destroy(&semThr);

		//pre-exit stuff
		freeExit();
		//now I can exit, threads will be stopped too
		exit (EXIT_FAILURE);

	}
	//Ctrl-\: whole program suspends
	else if (sig == SIGQUIT) {
		//store time for further evaluation
		clock_gettime(CLOCK_MONOTONIC, &ts1);
		//father process wait for user input and wake all childs
		if (sigFlag == TRUE) {
			fprintf(stderr, "\nReceived signal SIGQUIT (%d).\n", sig);
			fprintf(stderr, "Program suspended, press any key to continue...\n");
			getchar();
			fprintf(stderr, "Program restored...\n");

			//semaphore signal
			for (i=0; i<forks; i++)
				psemSignal(sigSem);
			clock_gettime(CLOCK_MONOTONIC, &ts2);
			lag += SUMTIME(ts2, ts1);
		}
		//childs wait for father signal
		else {
			//pause my thread
			pthread_kill(tid, SIGUSR1);
			//semaphore wait
			psemWait(sigSem);

			clock_gettime(CLOCK_MONOTONIC, &ts2);
			lag += SUMTIME(ts2, ts1);
			//wake up my thread and restart
			sem_post(&semThr);
		}
	}
	//signal used by fork's threads
	else if (sig == SIGUSR1) {
		//wait then restart
		sem_wait(&semThr);
	}
}


/* Watch thread caller */
inline void watchThread(int id)
{
    int ris;
    errno = 0;

    //set data
    thrId = id;
    //create thread
    ris = pthread_create(&tid, NULL, threadRunner, NULL);
    if (ris) {
      fprintf(stderr, "Error creating thread %d (%d): %s\n", id, ris, strerror(errno));
      freeExit();
      exit (EXIT_FAILURE);
    }
}


/* Watch thread */
static void *threadRunner()
{
    double locSeq, totSeq, perc, seqSec, totTime, oldSize;
    struct timespec timer;
    sigset_t sigSet;

    //reset thread's signals, to ignore previously settings
	sigemptyset(&sigSet);
	sigaddset(&sigSet, SIGINT);
	sigaddset(&sigSet, SIGQUIT);
	pthread_sigmask(SIG_BLOCK, &sigSet, NULL);

    //initialize thread semaphore
    sem_init(&semThr, 0, 0);
    //set signal handler (use SIGUSR1)
    signal(SIGUSR1, sigHandler);

    //init data
    oldSize = 0;
    totSeq = SEQPART();

    //short timeout
	sleep(SLEPTM);
	locSeq = numSeq;
	perc = (numSeq / totSeq) * 100;
    while (perc < MAXPERC) {
        //time elapsed until now
        clock_gettime(CLOCK_MONOTONIC, &timer);
        totTime = SUMTIME(timer, tsBegin) - lag;
        //stats
		seqSec = locSeq / SLEPTM;
		locSeq = numSeq;

        //print stats
        fprintf(stdout, "   Worker %2d: %5.2lf%% (%.0lf seq), ", thrId, perc, numSeq);
        secstoHuman(totTime, FALSE);
        fprintf(stdout, ", %.1lf seq/s, ETA ", seqSec);
        secstoHuman(totSeq/seqSec-totTime, FALSE);
        //in mode 1, print file size
        if (mode == 1)
        	filesizeStats(SLEPTM, &oldSize);
        fprintf(stdout, "\n");

        //sleep a while
		sleep(SLEPTM);
		//more stats
		locSeq = numSeq - locSeq;
		perc = (numSeq / totSeq) * 100;
    }

    sem_destroy(&semThr);
    pthread_exit(NULL);
}

#endif //__linux__

