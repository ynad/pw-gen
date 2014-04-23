/**CHeaderFile*****************************************************************

   FileName    [generator.h]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Generator functions]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad, Diego]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-04-10]

******************************************************************************/


#ifndef GENERATOR_H_INCLUDED
#define GENERATOR_H_INCLUDED


/* Recursive generator - Single thread */
void generatorSingleR(unsigned char);

/* Iterative generator - Single thread - By Diego */
inline void generatorSingleI(unsigned char);

/* Recursive generator - CALC only */
void generatorCalcR(unsigned char);

/* Recursive generator - WRITE to file */
void generatorWriteR(unsigned char);

/* Iterative generator - CALC only - Based on Diego's */
inline void generatorCalcI(unsigned char);

/* Iterative generator - WRITE to file - Based on Diego's */
inline void generatorWriteI(unsigned char);


#endif // GENERATOR_H_INCLUDED

