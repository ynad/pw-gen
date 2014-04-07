/**CHeaderFile*****************************************************************

   FileName    [generator.h]

   PackageName [Pw-Gen]

   Synopsis    [Pw-Gen - Sequences generator, Generator functions]

   Description [generator of alphanumeric sequences,
   to use with brute-force tools]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-04-03]

******************************************************************************/


#ifndef GENERATOR_H_INCLUDED
#define GENERATOR_H_INCLUDED


/* Basic recursive generator */
void generatorSingle(unsigned char);

/* Iterative single generator - UNDER DEVELOPMENT */
inline void generatorSingleI();

/* Recursive generator - CALC only */
void generatorCalc(unsigned char);

/* Recursive generator - WRITE to file */
void generatorWrite(unsigned char);

/* Iterative generator - By D - EXPERIMENTAL */
inline void generatorIterative(void);


#endif // GENERATOR_H_INCLUDED

