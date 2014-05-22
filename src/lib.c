/**CFile***********************************************************************

   FileName    [lib.c]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Library functions]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-05-22]

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
static void secstoHuman(double);
static void filesizeStats(double);
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
#ifdef WINZOZ
	fprintf(stdout, "\nElapsed time:\t\t   %lf sec\n", timeEnd-timeBegin);
	fprintf(stdout, "Sequences generated:\t   %.0lf\n", numSeq);
	fprintf(stdout, "Sequences/second:\t   %lf\n\n", numSeq/(timeEnd-timeBegin));

#elif defined __linux__
	fprintf(stdout, "\nElapsed time:\t\t   %lf sec\n", calcTime);
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
	if (fout != NULL && fout != stdout)
		fclose(fout);
	if (pchars != chars)
		free(pchars);
	free(word);
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


/* Linux specific functions */
#ifdef __linux__

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
		//only father prints
		if (sigFlag == TRUE)
			fprintf(stderr, "\nReceived signal SIGINT (%d): exiting.\n", sig);
		//only childs
		else
			sem_destroy(&semThr);

		//pre-exit stuff
		freeExit();

		//reset sigHandler and exit
		signal(SIGINT, SIG_DFL);
		raise(SIGINT);
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

			//wake up my thread and restart
			sem_post(&semThr);
			clock_gettime(CLOCK_MONOTONIC, &ts2);
			lag += SUMTIME(ts2, ts1);
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
    double totSeq, perc, loctime, seqSec;
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

    //calculate data
    perc = 0;
    totSeq = SEQPART();

    while (perc < MAXPERC) {
        sleep(SLEPTM);
        //time elapsed until now
        clock_gettime(CLOCK_MONOTONIC, &timer);
        loctime = SUMTIME(timer, tsBegin) - lag;
        //work progress
        perc = (numSeq / totSeq) * 100;
        seqSec = numSeq / loctime;

        //print stats
        fprintf(stdout, "   Worker %2d: %4.2lf%% (%.0lf seq), %.1lf s, %.1lf seq/s, ETA ", thrId, perc, numSeq, loctime, seqSec);
        secstoHuman(totSeq/seqSec-loctime);
        //in mode 1, print file size
        if (mode == 1)
        	filesizeStats(loctime);
        fprintf(stdout, "\n");
    }

    sem_destroy(&semThr);
    pthread_exit(NULL);
}


/* Convert and print seconds to human legible format */
static void secstoHuman(double sec)
{
	double s, m, h, d, y;

	//from seconds calculate eventual minutes, hours, days and years
	m = h = d = y = 0;
	s = fmod(sec, 60);
	if (sec >= 60) {
		m = sec / 60;
		if (m >= 60) {
			h = m / 60;
			m = fmod(m, 60);
			if (h >= 24) {
				d = h / 24;
				h = fmod(h, 24);
				if (d >= 365)  {
					y = d / 365;
					d = fmod(d, 365);
				}
			}
		}
	}
	//output
	if (y > 0)
		fprintf(stdout, "%.0lf years %.0lf days %02.0lf:%02.0lf:%02.0lf", y, d, h, m, s);
	else if (d > 0)
		fprintf(stdout, "%.0lf days %02.0lf:%02.0lf:%02.0lf", d, h, m, s);
	else
		fprintf(stdout, "%02.0lf:%02.0lf:%02.0lf", h, m, s);
}


/* Print statistics related to file size and write speed */
static void filesizeStats(double sec)
{
	struct stat stt;
	double size, speed;

	stat(dict, &stt);
	size = (double)stt.st_size / MEGA;
	speed = size / sec;

	fprintf(stdout, ", size %.1lf MB (%.1lf MB/s)", size, speed);
}

#endif //__linux__

