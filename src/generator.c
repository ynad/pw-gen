/**CFile***********************************************************************

   FileName    [generator.c]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Generator functions]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad, Diego]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-09-25]

******************************************************************************/


/** Libraries and system dependencies **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Base header */
#include "pw-gen.h"
/* Library functions header */
#include "lib.h"
/* Generator functions header */
#include "generator.h"


/* Recursive generator - Single thread */
void generatorSingleR(unsigned char pos)
{
	int i;

	if (pos == len) {
		numSeq++;
		if (mode == 1)
			//write word
			fwrite(word, sizeof(char), len+1, fout);
		return;
	}
	for (i=0; i<nchars; i++) {
		word[pos] = pchars[i];
		generatorSingleR(pos+1);
	}
}


/* Iterative generator - Single thread - By Diego */
inline void generatorSingleI(unsigned char z)
{
	int pos, i, *num_chars;
	errno = 0;

	//index array for chars elements
	if ((num_chars = (int *)malloc(len * sizeof(int))) == NULL) {
		fprintf(stderr, "Error allocating memory (%d): %s\n", len, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}

	//initialize
	pos = 0;
	for (i=0; i<len; i++) {
		num_chars[i] = 0;
		word[i] = pchars[0];
	}

	while (word[0] != pchars[nchars]) {
		if (word[pos] != pchars[nchars]) {
			pos++;
			num_chars[pos] = 0;
			word[pos] = pchars[num_chars[pos]];

			while (pos == len-1 && word[pos] != pchars[nchars]) {
				if (mode == 1)
					//write word
					fwrite(word, sizeof(char), len+1, fout);
				numSeq++;
				num_chars[pos]++;
				word[pos] = pchars[num_chars[pos]];
			}
		}
		else {
			pos--;
			num_chars[pos]++;
			word[pos] = pchars[num_chars[pos]];
		}
	}
	free(num_chars);
}


/* Recursive generator - CALC only */
void generatorCalcR(unsigned char pos)
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
			generatorCalcR(pos+1);
		}
	}
	//others
	else {
		for (i=0; i<nchars; i++) {
			word[pos] = pchars[i];
			generatorCalcR(pos+1);
		}
	}
}


/* Recursive generator - WRITE to file */
void generatorWriteR(unsigned char pos)
{
	int i;

	//word completed
	if (pos == len) {
		numSeq++;
		//write word
		fwrite(word, sizeof(char), len+1, fout);
		return;
	}
	//first char
	if (pos == 0) {
		for (i=left; i<right; i++) {
			word[pos] = pchars[i];
			generatorWriteR(pos+1);
		}
	}
	//others
	else {
		for (i=0; i<nchars; i++) {
			word[pos] = pchars[i];
			generatorWriteR(pos+1);
		}
	}
}


/* Iterative generator - CALC only - Based on Diego's */
inline void generatorCalcI(unsigned char z)
{
	int pos, i, *num_chars;
	errno = 0;

	//index array for chars elements
	if ((num_chars = (int *)malloc(len * sizeof(int))) == NULL) {
		fprintf(stderr, "Error allocating memory (%d): %s\n", len, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}

	//initialize
	pos = 0;
	for (i=0; i<len; i++) {
		num_chars[i] = left;
		word[i] = pchars[left];
	}

	while (word[0] != pchars[right]) {
		if ((pos != 0 && word[pos] != pchars[nchars]) || (pos == 0 && word[pos] != pchars[right])) {
			pos++;
			num_chars[pos] = 0;
			word[pos] = pchars[num_chars[pos]];

			while (pos == len-1 && word[pos] != pchars[nchars]) {
				numSeq++;
				num_chars[pos]++;
				word[pos] = pchars[num_chars[pos]];
			}
		}
		else {
			pos--;
			num_chars[pos]++;
			word[pos] = pchars[num_chars[pos]];
		}
	}
	free(num_chars);
}


/* Iterative generator - WRITE to file - Based on Diego's */
inline void generatorWriteI(unsigned char z)
{
	int pos, i, *num_chars;
	errno = 0;

	//index array for chars elements
	if ((num_chars = (int *)malloc(len * sizeof(int))) == NULL) {
		fprintf(stderr, "Error allocating memory (%d): %s\n", len, strerror(errno));
		freeExit();
		exit (EXIT_FAILURE);
	}

	//initialize
	pos = 0;
	for (i=0; i<len; i++) {
		num_chars[i] = left;
		word[i] = pchars[left];
	}

	while (word[0] != pchars[right]) {
		if ((pos != 0 && word[pos] != pchars[nchars]) || (pos == 0 && word[pos] != pchars[right])) {
			pos++;
			num_chars[pos] = 0;
			word[pos] = pchars[num_chars[pos]];

			while (pos == len-1 && word[pos] != pchars[nchars]) {
				//write word
				fwrite(word, sizeof(char), len+1, fout);
				numSeq++;
				num_chars[pos]++;
				word[pos] = pchars[num_chars[pos]];
			}
		}
		else {
			pos--;
			num_chars[pos]++;
			word[pos] = pchars[num_chars[pos]];
		}
	}
	free(num_chars);
}

