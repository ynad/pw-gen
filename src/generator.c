/**CFile***********************************************************************

   FileName    [generator.c]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Generator functions]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-04-03]

******************************************************************************/


/** Libraries and system dependencies **/
#include <stdio.h>
#include <stdlib.h>

/* Base header */
#include "pw-gen.h"
/* Generator functions header */
#include "generator.h"


/* Basic recursive generator */
void generatorSingle(unsigned char pos)
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


/* Iterative single generator - UNDER DEVELOPMENT */
inline void generatorSingleI()
{
	int i, j, pos;

	for (i=0; i<nchars; i++) {
		for (pos=0; pos<=len; pos++) {
			for (j=0; j<nchars; j++) {
				if (pos == len) {
					numSeq++;
					printf("%s\n", word);
				}
				else {
					word[pos] = pchars[j];
				}
			}
		}
	}
}


/* Recursive generator - CALC only */
void generatorCalc(unsigned char pos)
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
	//others
	else {
		for (i=0; i<nchars; i++) {
			word[pos] = pchars[i];
			generatorCalc(pos+1);
		}
	}
}


/* Recursive generator - WRITE to file */
void generatorWrite(unsigned char pos)
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
	//others
	else {
		for (i=0; i<nchars; i++) {
			word[pos] = pchars[i];
			generatorWrite(pos+1);
		}
	}
}


/* Iterative generator - By D - EXPERIMENTAL */
inline void generatorIterative(void)
{
    int pos=0, i, *num_chars;

    if ((num_chars = (int *)malloc(len * sizeof(int))) == NULL) {
        printf("err. allocazione\n");
        exit (-1);
    }
    for (i=0; i<len; i++) {
        num_chars[i] = 0;
        word[i] = chars[0];
    }
    while (word[0] != '\n') {
		if (word[pos] != '\n') {
			num_chars[++pos] = 0;
			word[pos] = chars[num_chars[pos]];

			while (pos == len-1 && word[pos] != '\n') {
				//if(fout!=NULL)
				//fprintf(fout, "%s\n", word);
				numSeq++;
				num_chars[pos]++;
				word[pos] = chars[num_chars[pos]];            
			}
		}
		else {
			num_chars[--pos]++;
			word[pos] = chars[num_chars[pos]];
        }
    }
    free(num_chars);
}

