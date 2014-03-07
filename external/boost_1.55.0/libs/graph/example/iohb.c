//  (C) Copyright Jeremy Siek 2004 
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

/*
Fri Aug 15 16:29:47 EDT 1997

                       Harwell-Boeing File I/O in C
                                V. 1.0

           National Institute of Standards and Technology, MD.
                             K.A. Remington

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the Author nor the Institution (National Institute of Standards
 and Technology) make any representations about the suitability of this 
 software for any purpose. This software is provided "as is" without 
 expressed or implied warranty.
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

                         ---------------------
                         INTERFACE DESCRIPTION
                         ---------------------
  ---------------
  QUERY FUNCTIONS
  ---------------

  FUNCTION:

  int readHB_info(const char *filename, int *M, int *N, int *nz,
  char **Type, int *Nrhs)

  DESCRIPTION:

  The readHB_info function opens and reads the header information from
  the specified Harwell-Boeing file, and reports back the number of rows
  and columns in the stored matrix (M and N), the number of nonzeros in
  the matrix (nz), the 3-character matrix type(Type), and the number of
  right-hand-sides stored along with the matrix (Nrhs).  This function
  is designed to retrieve basic size information which can be used to 
  allocate arrays.

  FUNCTION:

  int  readHB_header(FILE* in_file, char* Title, char* Key, char* Type, 
                    int* Nrow, int* Ncol, int* Nnzero, int* Nrhs,
                    char* Ptrfmt, char* Indfmt, char* Valfmt, char* Rhsfmt, 
                    int* Ptrcrd, int* Indcrd, int* Valcrd, int* Rhscrd, 
                    char *Rhstype)

  DESCRIPTION:

  More detailed than the readHB_info function, readHB_header() reads from 
  the specified Harwell-Boeing file all of the header information.  


  ------------------------------
  DOUBLE PRECISION I/O FUNCTIONS
  ------------------------------
  FUNCTION:

  int readHB_newmat_double(const char *filename, int *M, int *N, *int nz,
  int **colptr, int **rowind,  double**val)

  int readHB_mat_double(const char *filename, int *colptr, int *rowind,
  double*val)


  DESCRIPTION:

  This function opens and reads the specified file, interpreting its
  contents as a sparse matrix stored in the Harwell/Boeing standard
  format.  (See readHB_aux_double to read auxillary vectors.)
        -- Values are interpreted as double precision numbers. --

  The "mat" function uses _pre-allocated_ vectors to hold the index and 
  nonzero value information.

  The "newmat" function allocates vectors to hold the index and nonzero
  value information, and returns pointers to these vectors along with
  matrix dimension and number of nonzeros.

  FUNCTION:

  int readHB_aux_double(const char* filename, const char AuxType, double b[])

  int readHB_newaux_double(const char* filename, const char AuxType, double** b)

  DESCRIPTION:

  This function opens and reads from the specified file auxillary vector(s).
  The char argument Auxtype determines which type of auxillary vector(s)
  will be read (if present in the file).

                  AuxType = 'F'   right-hand-side 
                  AuxType = 'G'   initial estimate (Guess)
                  AuxType = 'X'   eXact solution

  If Nrhs > 1, all of the Nrhs vectors of the given type are read and 
  stored in column-major order in the vector b.

  The "newaux" function allocates a vector to hold the values retrieved.
  The "mat" function uses a _pre-allocated_ vector to hold the values.

  FUNCTION:

  int writeHB_mat_double(const char* filename, int M, int N, 
                        int nz, const int colptr[], const int rowind[], 
                        const double val[], int Nrhs, const double rhs[], 
                        const double guess[], const double exact[],
                        const char* Title, const char* Key, const char* Type, 
                        char* Ptrfmt, char* Indfmt, char* Valfmt, char* Rhsfmt,
                        const char* Rhstype)

  DESCRIPTION:

  The writeHB_mat_double function opens the named file and writes the specified
  matrix and optional auxillary vector(s) to that file in Harwell-Boeing
  format.  The format arguments (Ptrfmt,Indfmt,Valfmt, and Rhsfmt) are
  character strings specifying "Fortran-style" output formats -- as they
  would appear in a Harwell-Boeing file.  They are used to produce output
  which is as close as possible to what would be produced by Fortran code,
  but note that "D" and "P" edit descriptors are not supported.
  If NULL, the following defaults will be used:
                    Ptrfmt = Indfmt = "(8I10)"
                    Valfmt = Rhsfmt = "(4E20.13)"

  -----------------------
  CHARACTER I/O FUNCTIONS 
  -----------------------
  FUNCTION: 

  int readHB_mat_char(const char* filename, int colptr[], int rowind[], 
                                           char val[], char* Valfmt)
  int readHB_newmat_char(const char* filename, int* M, int* N, int* nonzeros, 
                          int** colptr, int** rowind, char** val, char** Valfmt)

  DESCRIPTION: 

  This function opens and reads the specified file, interpreting its
  contents as a sparse matrix stored in the Harwell/Boeing standard
  format.  (See readHB_aux_char to read auxillary vectors.)
              -- Values are interpreted as char strings.     --
  (Used to translate exact values from the file into a new storage format.)

  The "mat" function uses _pre-allocated_ arrays to hold the index and 
  nonzero value information.

  The "newmat" function allocates char arrays to hold the index 
  and nonzero value information, and returns pointers to these arrays 
  along with matrix dimension and number of nonzeros.

  FUNCTION:

  int readHB_aux_char(const char* filename, const char AuxType, char b[])
  int readHB_newaux_char(const char* filename, const char AuxType, char** b, 
                         char** Rhsfmt)

  DESCRIPTION:

  This function opens and reads from the specified file auxillary vector(s).
  The char argument Auxtype determines which type of auxillary vector(s)
  will be read (if present in the file).

                  AuxType = 'F'   right-hand-side 
                  AuxType = 'G'   initial estimate (Guess)
                  AuxType = 'X'   eXact solution

  If Nrhs > 1, all of the Nrhs vectors of the given type are read and 
  stored in column-major order in the vector b.

  The "newaux" function allocates a character array to hold the values 
                retrieved.
  The "mat" function uses a _pre-allocated_ array to hold the values.

  FUNCTION:

  int writeHB_mat_char(const char* filename, int M, int N, 
                        int nz, const int colptr[], const int rowind[], 
                        const char val[], int Nrhs, const char rhs[], 
                        const char guess[], const char exact[], 
                        const char* Title, const char* Key, const char* Type, 
                        char* Ptrfmt, char* Indfmt, char* Valfmt, char* Rhsfmt,
                        const char* Rhstype)

  DESCRIPTION:

  The writeHB_mat_char function opens the named file and writes the specified
  matrix and optional auxillary vector(s) to that file in Harwell-Boeing
  format.  The format arguments (Ptrfmt,Indfmt,Valfmt, and Rhsfmt) are
  character strings specifying "Fortran-style" output formats -- as they
  would appear in a Harwell-Boeing file.  Valfmt and Rhsfmt must accurately 
  represent the character representation of the values stored in val[] 
  and rhs[]. 

  If NULL, the following defaults will be used for the integer vectors:
                    Ptrfmt = Indfmt = "(8I10)"
                    Valfmt = Rhsfmt = "(4E20.13)"


*/

/*---------------------------------------------------------------------*/
/* If zero-based indexing is desired, _SP_base should be set to 0      */
/* This will cause indices read from H-B files to be decremented by 1  */
/*             and indices written to H-B files to be incremented by 1 */
/*            <<<  Standard usage is _SP_base = 1  >>>                 */
#ifndef _SP_base
#define _SP_base 1
#endif
/*---------------------------------------------------------------------*/

#include "iohb.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

char* substr(const char* S, const int pos, const int len);
void upcase(char* S);
void IOHBTerminate(char* message);

int readHB_info(const char* filename, int* M, int* N, int* nz, char** Type, 
                                                      int* Nrhs)
{
/****************************************************************************/
/*  The readHB_info function opens and reads the header information from    */
/*  the specified Harwell-Boeing file, and reports back the number of rows  */
/*  and columns in the stored matrix (M and N), the number of nonzeros in   */
/*  the matrix (nz), and the number of right-hand-sides stored along with   */
/*  the matrix (Nrhs).                                                      */
/*                                                                          */
/*  For a description of the Harwell Boeing standard, see:                  */
/*            Duff, et al.,  ACM TOMS Vol.15, No.1, March 1989              */
/*                                                                          */
/*    ----------                                                            */
/*    **CAVEAT**                                                            */
/*    ----------                                                            */
/*  **  If the input file does not adhere to the H/B format, the  **        */
/*  **             results will be unpredictable.                 **        */
/*                                                                          */
/****************************************************************************/
    FILE *in_file;
    int Ptrcrd, Indcrd, Valcrd, Rhscrd; 
    int Nrow, Ncol, Nnzero;
    char* mat_type;
    char Title[73], Key[9], Rhstype[4];
    char Ptrfmt[17], Indfmt[17], Valfmt[21], Rhsfmt[21];

    mat_type = *Type;
    if ( mat_type == NULL ) IOHBTerminate("Insufficient memory for mat_typen");
    
    if ( (in_file = fopen( filename, "r")) == NULL ) {
       fprintf(stderr,"Error: Cannot open file: %s\n",filename);
       return 0;
    }

    readHB_header(in_file, Title, Key, mat_type, &Nrow, &Ncol, &Nnzero, Nrhs,
                  Ptrfmt, Indfmt, Valfmt, Rhsfmt, 
                  &Ptrcrd, &Indcrd, &Valcrd, &Rhscrd, Rhstype);
    fclose(in_file);
    *Type = mat_type;
    *(*Type+3) = (char) NULL;
    *M    = Nrow;
    *N    = Ncol;
    *nz   = Nnzero;
    if (Rhscrd == 0) {*Nrhs = 0;}

/*  In verbose mode, print some of the header information:   */
/*
    if (verbose == 1)
    {
        printf("Reading from Harwell-Boeing file %s (verbose on)...\n",filename);
        printf("  Title: %s\n",Title);
        printf("  Key:   %s\n",Key);
        printf("  The stored matrix is %i by %i with %i nonzeros.\n", 
                *M, *N, *nz );
        printf("  %i right-hand--side(s) stored.\n",*Nrhs);
    }
*/
 
    return 1;

}



int readHB_header(FILE* in_file, char* Title, char* Key, char* Type, 
                    int* Nrow, int* Ncol, int* Nnzero, int* Nrhs,
                    char* Ptrfmt, char* Indfmt, char* Valfmt, char* Rhsfmt, 
                    int* Ptrcrd, int* Indcrd, int* Valcrd, int* Rhscrd, 
                    char *Rhstype)
{
/*************************************************************************/
/*  Read header information from the named H/B file...                   */
/*************************************************************************/
    int Totcrd,Neltvl,Nrhsix;
    char line[BUFSIZ];

/*  First line:   */
    fgets(line, BUFSIZ, in_file);
    if ( sscanf(line,"%*s") < 0 ) 
        IOHBTerminate("iohb.c: Null (or blank) first line of HB file.\n");
    (void) sscanf(line, "%72c%8[^\n]", Title, Key);
    *(Key+8) = (char) NULL;
    *(Title+72) = (char) NULL;

/*  Second line:  */
    fgets(line, BUFSIZ, in_file);
    if ( sscanf(line,"%*s") < 0 ) 
        IOHBTerminate("iohb.c: Null (or blank) second line of HB file.\n");
    if ( sscanf(line,"%i",&Totcrd) != 1) Totcrd = 0;
    if ( sscanf(line,"%*i%i",Ptrcrd) != 1) *Ptrcrd = 0;
    if ( sscanf(line,"%*i%*i%i",Indcrd) != 1) *Indcrd = 0;
    if ( sscanf(line,"%*i%*i%*i%i",Valcrd) != 1) *Valcrd = 0;
    if ( sscanf(line,"%*i%*i%*i%*i%i",Rhscrd) != 1) *Rhscrd = 0;

/*  Third line:   */
    fgets(line, BUFSIZ, in_file);
    if ( sscanf(line,"%*s") < 0 ) 
        IOHBTerminate("iohb.c: Null (or blank) third line of HB file.\n");
    if ( sscanf(line, "%3c", Type) != 1) 
        IOHBTerminate("iohb.c: Invalid Type info, line 3 of Harwell-Boeing file.\n");
    upcase(Type);
    if ( sscanf(line,"%*3c%i",Nrow) != 1) *Nrow = 0 ;
    if ( sscanf(line,"%*3c%*i%i",Ncol) != 1) *Ncol = 0 ;
    if ( sscanf(line,"%*3c%*i%*i%i",Nnzero) != 1) *Nnzero = 0 ;
    if ( sscanf(line,"%*3c%*i%*i%*i%i",&Neltvl) != 1) Neltvl = 0 ;

/*  Fourth line:  */
    fgets(line, BUFSIZ, in_file);
    if ( sscanf(line,"%*s") < 0 ) 
        IOHBTerminate("iohb.c: Null (or blank) fourth line of HB file.\n");
    if ( sscanf(line, "%16c",Ptrfmt) != 1)
        IOHBTerminate("iohb.c: Invalid format info, line 4 of Harwell-Boeing file.\n"); 
    if ( sscanf(line, "%*16c%16c",Indfmt) != 1)
        IOHBTerminate("iohb.c: Invalid format info, line 4 of Harwell-Boeing file.\n"); 
    if ( sscanf(line, "%*16c%*16c%20c",Valfmt) != 1) 
        IOHBTerminate("iohb.c: Invalid format info, line 4 of Harwell-Boeing file.\n"); 
    sscanf(line, "%*16c%*16c%*20c%20c",Rhsfmt);
    *(Ptrfmt+16) = (char) NULL;
    *(Indfmt+16) = (char) NULL;
    *(Valfmt+20) = (char) NULL;
    *(Rhsfmt+20) = (char) NULL;
   
/*  (Optional) Fifth line: */
    if (*Rhscrd != 0 )
    { 
       fgets(line, BUFSIZ, in_file);
       if ( sscanf(line,"%*s") < 0 ) 
           IOHBTerminate("iohb.c: Null (or blank) fifth line of HB file.\n");
       if ( sscanf(line, "%3c", Rhstype) != 1) 
         IOHBTerminate("iohb.c: Invalid RHS type information, line 5 of Harwell-Boeing file.\n");
       if ( sscanf(line, "%*3c%i", Nrhs) != 1) *Nrhs = 0;
       if ( sscanf(line, "%*3c%*i%i", &Nrhsix) != 1) Nrhsix = 0;
    }
    return 1;
}


int readHB_mat_double(const char* filename, int colptr[], int rowind[], 
                                                                 double val[])
{
/****************************************************************************/
/*  This function opens and reads the specified file, interpreting its      */
/*  contents as a sparse matrix stored in the Harwell/Boeing standard       */
/*  format and creating compressed column storage scheme vectors to hold    */
/*  the index and nonzero value information.                                */
/*                                                                          */
/*    ----------                                                            */
/*    **CAVEAT**                                                            */
/*    ----------                                                            */
/*  Parsing real formats from Fortran is tricky, and this file reader       */
/*  does not claim to be foolproof.   It has been tested for cases when     */
/*  the real values are printed consistently and evenly spaced on each      */
/*  line, with Fixed (F), and Exponential (E or D) formats.                 */
/*                                                                          */
/*  **  If the input file does not adhere to the H/B format, the  **        */
/*  **             results will be unpredictable.                 **        */
/*                                                                          */
/****************************************************************************/
    FILE *in_file;
    int i,j,ind,col,offset,count,last,Nrhs;
    int Ptrcrd, Indcrd, Valcrd, Rhscrd;
    int Nrow, Ncol, Nnzero, Nentries;
    int Ptrperline, Ptrwidth, Indperline, Indwidth;
    int Valperline, Valwidth, Valprec;
    int Valflag;           /* Indicates 'E','D', or 'F' float format */
    char* ThisElement;
    char Title[73], Key[8], Type[4], Rhstype[4];
    char Ptrfmt[17], Indfmt[17], Valfmt[21], Rhsfmt[21];
    char line[BUFSIZ];

    if ( (in_file = fopen( filename, "r")) == NULL ) {
       fprintf(stderr,"Error: Cannot open file: %s\n",filename);
       return 0;
    }

    readHB_header(in_file, Title, Key, Type, &Nrow, &Ncol, &Nnzero, &Nrhs,
                  Ptrfmt, Indfmt, Valfmt, Rhsfmt,
                  &Ptrcrd, &Indcrd, &Valcrd, &Rhscrd, Rhstype);

/*  Parse the array input formats from Line 3 of HB file  */
    ParseIfmt(Ptrfmt,&Ptrperline,&Ptrwidth);
    ParseIfmt(Indfmt,&Indperline,&Indwidth);
    if ( Type[0] != 'P' ) {          /* Skip if pattern only  */
    ParseRfmt(Valfmt,&Valperline,&Valwidth,&Valprec,&Valflag);
    }

/*  Read column pointer array:   */

    offset = 1-_SP_base;  /* if base 0 storage is declared (via macro definition), */
                          /* then storage entries are offset by 1                  */

    ThisElement = (char *) malloc(Ptrwidth+1);
    if ( ThisElement == NULL ) IOHBTerminate("Insufficient memory for ThisElement.");
    *(ThisElement+Ptrwidth) = (char) NULL;
    count=0;
    for (i=0;i<Ptrcrd;i++)
    {
       fgets(line, BUFSIZ, in_file);
       if ( sscanf(line,"%*s") < 0 ) 
         IOHBTerminate("iohb.c: Null (or blank) line in pointer data region of HB file.\n");
       col =  0;
       for (ind = 0;ind<Ptrperline;ind++)
       {
          if (count > Ncol) break;
          strncpy(ThisElement,line+col,Ptrwidth);
  /* ThisElement = substr(line,col,Ptrwidth); */
          colptr[count] = atoi(ThisElement)-offset;
          count++; col += Ptrwidth;
       }
    }
    free(ThisElement);

/*  Read row index array:  */

    ThisElement = (char *) malloc(Indwidth+1);
    if ( ThisElement == NULL ) IOHBTerminate("Insufficient memory for ThisElement.");
    *(ThisElement+Indwidth) = (char) NULL;
    count = 0;
    for (i=0;i<Indcrd;i++)
    {
       fgets(line, BUFSIZ, in_file);
       if ( sscanf(line,"%*s") < 0 ) 
         IOHBTerminate("iohb.c: Null (or blank) line in index data region of HB file.\n");
       col =  0;
       for (ind = 0;ind<Indperline;ind++)
       {
          if (count == Nnzero) break;
          strncpy(ThisElement,line+col,Indwidth);
/*        ThisElement = substr(line,col,Indwidth); */
          rowind[count] = atoi(ThisElement)-offset;
          count++; col += Indwidth;
       }
    }
    free(ThisElement);

/*  Read array of values:  */

    if ( Type[0] != 'P' ) {          /* Skip if pattern only  */

       if ( Type[0] == 'C' ) Nentries = 2*Nnzero;
           else Nentries = Nnzero;

    ThisElement = (char *) malloc(Valwidth+1);
    if ( ThisElement == NULL ) IOHBTerminate("Insufficient memory for ThisElement.");
    *(ThisElement+Valwidth) = (char) NULL;
    count = 0;
    for (i=0;i<Valcrd;i++)
    {
       fgets(line, BUFSIZ, in_file);
       if ( sscanf(line,"%*s") < 0 ) 
         IOHBTerminate("iohb.c: Null (or blank) line in value data region of HB file.\n");
       if (Valflag == 'D')  {
          while( strchr(line,'D') ) *strchr(line,'D') = 'E';
/*           *strchr(Valfmt,'D') = 'E'; */
       }
       col =  0;
       for (ind = 0;ind<Valperline;ind++)
       {
          if (count == Nentries) break;
          strncpy(ThisElement,line+col,Valwidth);
          /*ThisElement = substr(line,col,Valwidth);*/
          if ( Valflag != 'F' && strchr(ThisElement,'E') == NULL ) { 
             /* insert a char prefix for exp */
             last = strlen(ThisElement);
             for (j=last+1;j>=0;j--) {
                ThisElement[j] = ThisElement[j-1];
                if ( ThisElement[j] == '+' || ThisElement[j] == '-' ) {
                   ThisElement[j-1] = Valflag;                    
                   break;
                }
             }
          }
          val[count] = atof(ThisElement);
          count++; col += Valwidth;
       }
    }
    free(ThisElement);
    }

    fclose(in_file);
    return 1;
}

int readHB_newmat_double(const char* filename, int* M, int* N, int* nonzeros, 
                         int** colptr, int** rowind, double** val)
{
        int Nrhs;
        char *Type;

        readHB_info(filename, M, N, nonzeros, &Type, &Nrhs);

        *colptr = (int *)malloc((*N+1)*sizeof(int));
        if ( *colptr == NULL ) IOHBTerminate("Insufficient memory for colptr.\n");
        *rowind = (int *)malloc(*nonzeros*sizeof(int));
        if ( *rowind == NULL ) IOHBTerminate("Insufficient memory for rowind.\n");
        if ( Type[0] == 'C' ) {
/*
   fprintf(stderr, "Warning: Reading complex data from HB file %s.\n",filename);
   fprintf(stderr, "         Real and imaginary parts will be interlaced in val[].\n");
*/
           /* Malloc enough space for real AND imaginary parts of val[] */
           *val = (double *)malloc(*nonzeros*sizeof(double)*2);
           if ( *val == NULL ) IOHBTerminate("Insufficient memory for val.\n");
        } else {
           if ( Type[0] != 'P' ) {   
             /* Malloc enough space for real array val[] */
             *val = (double *)malloc(*nonzeros*sizeof(double));
             if ( *val == NULL ) IOHBTerminate("Insufficient memory for val.\n");
           }
        }  /* No val[] space needed if pattern only */
        return readHB_mat_double(filename, *colptr, *rowind, *val);

}

int readHB_aux_double(const char* filename, const char AuxType, double b[])
{
/****************************************************************************/
/*  This function opens and reads the specified file, placing auxillary     */
/*  vector(s) of the given type (if available) in b.                        */
/*  Return value is the number of vectors successfully read.                */
/*                                                                          */
/*                AuxType = 'F'   full right-hand-side vector(s)            */
/*                AuxType = 'G'   initial Guess vector(s)                   */
/*                AuxType = 'X'   eXact solution vector(s)                  */
/*                                                                          */
/*    ----------                                                            */
/*    **CAVEAT**                                                            */
/*    ----------                                                            */
/*  Parsing real formats from Fortran is tricky, and this file reader       */
/*  does not claim to be foolproof.   It has been tested for cases when     */
/*  the real values are printed consistently and evenly spaced on each      */
/*  line, with Fixed (F), and Exponential (E or D) formats.                 */
/*                                                                          */
/*  **  If the input file does not adhere to the H/B format, the  **        */
/*  **             results will be unpredictable.                 **        */
/*                                                                          */
/****************************************************************************/
    FILE *in_file;
    int i,j,n,maxcol,start,stride,col,last,linel;
    int Ptrcrd, Indcrd, Valcrd, Rhscrd;
    int Nrow, Ncol, Nnzero, Nentries;
    int Nrhs, nvecs, rhsi;
    int Rhsperline, Rhswidth, Rhsprec;
    int Rhsflag;
    char *ThisElement;
    char Title[73], Key[9], Type[4], Rhstype[4];
    char Ptrfmt[17], Indfmt[17], Valfmt[21], Rhsfmt[21];
    char line[BUFSIZ];

    if ((in_file = fopen( filename, "r")) == NULL) {
      fprintf(stderr,"Error: Cannot open file: %s\n",filename);
      return 0;
     }

    readHB_header(in_file, Title, Key, Type, &Nrow, &Ncol, &Nnzero, &Nrhs,
                  Ptrfmt, Indfmt, Valfmt, Rhsfmt,
                  &Ptrcrd, &Indcrd, &Valcrd, &Rhscrd, Rhstype);

    if (Nrhs <= 0)
    {
      fprintf(stderr, "Warn: Attempt to read auxillary vector(s) when none are present.\n");
      return 0;
    }
    if (Rhstype[0] != 'F' )
    {
      fprintf(stderr,"Warn: Attempt to read auxillary vector(s) which are not stored in Full form.\n");
      fprintf(stderr,"       Rhs must be specified as full. \n");
      return 0;
    }

/* If reading complex data, allow for interleaved real and imaginary values. */ 
    if ( Type[0] == 'C' ) {
       Nentries = 2*Nrow;
     } else {
       Nentries = Nrow;
    }

    nvecs = 1;
    
    if ( Rhstype[1] == 'G' ) nvecs++;
    if ( Rhstype[2] == 'X' ) nvecs++;

    if ( AuxType == 'G' && Rhstype[1] != 'G' ) {
      fprintf(stderr, "Warn: Attempt to read auxillary Guess vector(s) when none are present.\n");
      return 0;
    }
    if ( AuxType == 'X' && Rhstype[2] != 'X' ) {
      fprintf(stderr, "Warn: Attempt to read auxillary eXact solution vector(s) when none are present.\n");
      return 0;
    }

    ParseRfmt(Rhsfmt, &Rhsperline, &Rhswidth, &Rhsprec,&Rhsflag);
    maxcol = Rhsperline*Rhswidth;

/*  Lines to skip before starting to read RHS values... */
    n = Ptrcrd + Indcrd + Valcrd;

    for (i = 0; i < n; i++)
      fgets(line, BUFSIZ, in_file);

/*  start  - number of initial aux vector entries to skip   */
/*           to reach first  vector requested               */
/*  stride - number of aux vector entries to skip between   */
/*           requested vectors                              */
    if ( AuxType == 'F' ) start = 0;
    else if ( AuxType == 'G' ) start = Nentries;
    else start = (nvecs-1)*Nentries;
    stride = (nvecs-1)*Nentries;

    fgets(line, BUFSIZ, in_file);
    linel= strchr(line,'\n')-line;
    col = 0;
/*  Skip to initial offset */

    for (i=0;i<start;i++) {
       if ( col >=  ( maxcol<linel?maxcol:linel ) ) {
           fgets(line, BUFSIZ, in_file);
           linel= strchr(line,'\n')-line;
           col = 0;
       }
       col += Rhswidth;
    }
    if (Rhsflag == 'D')  {
       while( strchr(line,'D') ) *strchr(line,'D') = 'E';
    }

/*  Read a vector of desired type, then skip to next */
/*  repeating to fill Nrhs vectors                   */

  ThisElement = (char *) malloc(Rhswidth+1);
  if ( ThisElement == NULL ) IOHBTerminate("Insufficient memory for ThisElement.");
  *(ThisElement+Rhswidth) = (char) NULL;
  for (rhsi=0;rhsi<Nrhs;rhsi++) {

    for (i=0;i<Nentries;i++) {
       if ( col >= ( maxcol<linel?maxcol:linel ) ) {
           fgets(line, BUFSIZ, in_file);
           linel= strchr(line,'\n')-line;
           if (Rhsflag == 'D')  {
              while( strchr(line,'D') ) *strchr(line,'D') = 'E';
           }
           col = 0;
       }
       strncpy(ThisElement,line+col,Rhswidth);
       /*ThisElement = substr(line, col, Rhswidth);*/
          if ( Rhsflag != 'F' && strchr(ThisElement,'E') == NULL ) { 
             /* insert a char prefix for exp */
             last = strlen(ThisElement);
             for (j=last+1;j>=0;j--) {
                ThisElement[j] = ThisElement[j-1];
                if ( ThisElement[j] == '+' || ThisElement[j] == '-' ) {
                   ThisElement[j-1] = Rhsflag;                    
                   break;
                }
             }
          }
       b[i] = atof(ThisElement);
       col += Rhswidth;
    }
 
/*  Skip any interleaved Guess/eXact vectors */

    for (i=0;i<stride;i++) {
       if ( col >= ( maxcol<linel?maxcol:linel ) ) {
           fgets(line, BUFSIZ, in_file);
           linel= strchr(line,'\n')-line;
           col = 0;
       }
       col += Rhswidth;
    }

  }
  free(ThisElement);
    

    fclose(in_file);
    return Nrhs;
}

int readHB_newaux_double(const char* filename, const char AuxType, double** b)
{
        int Nrhs,M,N,nonzeros;
        char *Type;

        readHB_info(filename, &M, &N, &nonzeros, &Type, &Nrhs);
        if ( Nrhs <= 0 ) {
          fprintf(stderr,"Warn: Requested read of aux vector(s) when none are present.\n");
          return 0;
        } else { 
          if ( Type[0] == 'C' ) {
            fprintf(stderr, "Warning: Reading complex aux vector(s) from HB file %s.",filename);
            fprintf(stderr, "         Real and imaginary parts will be interlaced in b[].");
            *b = (double *)malloc(M*Nrhs*sizeof(double)*2);
            if ( *b == NULL ) IOHBTerminate("Insufficient memory for rhs.\n");
            return readHB_aux_double(filename, AuxType, *b);
          } else {
            *b = (double *)malloc(M*Nrhs*sizeof(double));
            if ( *b == NULL ) IOHBTerminate("Insufficient memory for rhs.\n");
            return readHB_aux_double(filename, AuxType, *b);
          }
        }
}

int writeHB_mat_double(const char* filename, int M, int N, 
                        int nz, const int colptr[], const int rowind[], 
                        const double val[], int Nrhs, const double rhs[], 
                        const double guess[], const double exact[],
                        const char* Title, const char* Key, const char* Type, 
                        char* Ptrfmt, char* Indfmt, char* Valfmt, char* Rhsfmt,
                        const char* Rhstype)
{
/****************************************************************************/
/*  The writeHB function opens the named file and writes the specified      */
/*  matrix and optional right-hand-side(s) to that file in Harwell-Boeing   */
/*  format.                                                                 */
/*                                                                          */
/*  For a description of the Harwell Boeing standard, see:                  */
/*            Duff, et al.,  ACM TOMS Vol.15, No.1, March 1989              */
/*                                                                          */
/****************************************************************************/
    FILE *out_file;
    int i,j,entry,offset,acount,linemod;
    int totcrd, ptrcrd, indcrd, valcrd, rhscrd;
    int nvalentries, nrhsentries;
    int Ptrperline, Ptrwidth, Indperline, Indwidth;
    int Rhsperline, Rhswidth, Rhsprec;
    int Rhsflag;
    int Valperline, Valwidth, Valprec;
    int Valflag;           /* Indicates 'E','D', or 'F' float format */
    char pformat[16],iformat[16],vformat[19],rformat[19];

    if ( Type[0] == 'C' ) {
         nvalentries = 2*nz;
         nrhsentries = 2*M;
    } else {
         nvalentries = nz;
         nrhsentries = M;
    }

    if ( filename != NULL ) {
       if ( (out_file = fopen( filename, "w")) == NULL ) {
         fprintf(stderr,"Error: Cannot open file: %s\n",filename);
         return 0;
       }
    } else out_file = stdout;

    if ( Ptrfmt == NULL ) Ptrfmt = "(8I10)";
    ParseIfmt(Ptrfmt,&Ptrperline,&Ptrwidth);
    sprintf(pformat,"%%%dd",Ptrwidth);
    ptrcrd = (N+1)/Ptrperline;
    if ( (N+1)%Ptrperline != 0) ptrcrd++;
   
    if ( Indfmt == NULL ) Indfmt =  Ptrfmt;
    ParseIfmt(Indfmt,&Indperline,&Indwidth);
    sprintf(iformat,"%%%dd",Indwidth);
    indcrd = nz/Indperline;
    if ( nz%Indperline != 0) indcrd++;

    if ( Type[0] != 'P' ) {          /* Skip if pattern only  */
      if ( Valfmt == NULL ) Valfmt = "(4E20.13)";
      ParseRfmt(Valfmt,&Valperline,&Valwidth,&Valprec,&Valflag);
      if (Valflag == 'D') *strchr(Valfmt,'D') = 'E';
      if (Valflag == 'F')
         sprintf(vformat,"%% %d.%df",Valwidth,Valprec);
      else
         sprintf(vformat,"%% %d.%dE",Valwidth,Valprec);
      valcrd = nvalentries/Valperline;
      if ( nvalentries%Valperline != 0) valcrd++;
    } else valcrd = 0;

    if ( Nrhs > 0 ) {
       if ( Rhsfmt == NULL ) Rhsfmt = Valfmt;
       ParseRfmt(Rhsfmt,&Rhsperline,&Rhswidth,&Rhsprec, &Rhsflag);
       if (Rhsflag == 'F')
          sprintf(rformat,"%% %d.%df",Rhswidth,Rhsprec);
       else
          sprintf(rformat,"%% %d.%dE",Rhswidth,Rhsprec);
       if (Rhsflag == 'D') *strchr(Rhsfmt,'D') = 'E';
       rhscrd = nrhsentries/Rhsperline; 
       if ( nrhsentries%Rhsperline != 0) rhscrd++;
       if ( Rhstype[1] == 'G' ) rhscrd+=rhscrd;
       if ( Rhstype[2] == 'X' ) rhscrd+=rhscrd;
       rhscrd*=Nrhs;
    } else rhscrd = 0;

    totcrd = 4+ptrcrd+indcrd+valcrd+rhscrd;


/*  Print header information:  */

    fprintf(out_file,"%-72s%-8s\n%14d%14d%14d%14d%14d\n",Title, Key, totcrd,
            ptrcrd, indcrd, valcrd, rhscrd);
    fprintf(out_file,"%3s%11s%14d%14d%14d\n",Type,"          ", M, N, nz);
    fprintf(out_file,"%-16s%-16s%-20s", Ptrfmt, Indfmt, Valfmt);
    if ( Nrhs != 0 ) {
/*     Print Rhsfmt on fourth line and                                    */
/*           optional fifth header line for auxillary vector information: */
       fprintf(out_file,"%-20s\n%-14s%d\n",Rhsfmt,Rhstype,Nrhs);
    } else fprintf(out_file,"\n");

    offset = 1-_SP_base;  /* if base 0 storage is declared (via macro definition), */
                          /* then storage entries are offset by 1                  */

/*  Print column pointers:   */
    for (i=0;i<N+1;i++)
    {
       entry = colptr[i]+offset;
       fprintf(out_file,pformat,entry);
       if ( (i+1)%Ptrperline == 0 ) fprintf(out_file,"\n");
    }

   if ( (N+1) % Ptrperline != 0 ) fprintf(out_file,"\n");

/*  Print row indices:       */
    for (i=0;i<nz;i++)
    {
       entry = rowind[i]+offset;
       fprintf(out_file,iformat,entry);
       if ( (i+1)%Indperline == 0 ) fprintf(out_file,"\n");
    }

   if ( nz % Indperline != 0 ) fprintf(out_file,"\n");

/*  Print values:            */

    if ( Type[0] != 'P' ) {          /* Skip if pattern only  */

    for (i=0;i<nvalentries;i++)
    {
       fprintf(out_file,vformat,val[i]);
       if ( (i+1)%Valperline == 0 ) fprintf(out_file,"\n");
    }

    if ( nvalentries % Valperline != 0 ) fprintf(out_file,"\n");

/*  If available,  print right hand sides, 
           guess vectors and exact solution vectors:  */
    acount = 1;
    linemod = 0;
    if ( Nrhs > 0 ) {
       for (i=0;i<Nrhs;i++)
       {
          for ( j=0;j<nrhsentries;j++ ) {
            fprintf(out_file,rformat,rhs[j]);
            if ( acount++%Rhsperline == linemod ) fprintf(out_file,"\n");
          }
          if ( acount%Rhsperline != linemod ) {
            fprintf(out_file,"\n");
            linemod = (acount-1)%Rhsperline;
          }
          rhs += nrhsentries;
          if ( Rhstype[1] == 'G' ) {
            for ( j=0;j<nrhsentries;j++ ) {
              fprintf(out_file,rformat,guess[j]);
              if ( acount++%Rhsperline == linemod ) fprintf(out_file,"\n");
            }
            if ( acount%Rhsperline != linemod ) {
              fprintf(out_file,"\n");
              linemod = (acount-1)%Rhsperline;
            }
            guess += nrhsentries;
          }
          if ( Rhstype[2] == 'X' ) {
            for ( j=0;j<nrhsentries;j++ ) {
              fprintf(out_file,rformat,exact[j]);
              if ( acount++%Rhsperline == linemod ) fprintf(out_file,"\n");
            }
            if ( acount%Rhsperline != linemod ) {
              fprintf(out_file,"\n");
              linemod = (acount-1)%Rhsperline;
            }
            exact += nrhsentries;
          }
       }
    }

    }

    if ( fclose(out_file) != 0){
      fprintf(stderr,"Error closing file in writeHB_mat_double().\n");
      return 0;
    } else return 1;
    
}

int readHB_mat_char(const char* filename, int colptr[], int rowind[], 
                                           char val[], char* Valfmt)
{
/****************************************************************************/
/*  This function opens and reads the specified file, interpreting its      */
/*  contents as a sparse matrix stored in the Harwell/Boeing standard       */
/*  format and creating compressed column storage scheme vectors to hold    */
/*  the index and nonzero value information.                                */
/*                                                                          */
/*    ----------                                                            */
/*    **CAVEAT**                                                            */
/*    ----------                                                            */
/*  Parsing real formats from Fortran is tricky, and this file reader       */
/*  does not claim to be foolproof.   It has been tested for cases when     */
/*  the real values are printed consistently and evenly spaced on each      */
/*  line, with Fixed (F), and Exponential (E or D) formats.                 */
/*                                                                          */
/*  **  If the input file does not adhere to the H/B format, the  **        */
/*  **             results will be unpredictable.                 **        */
/*                                                                          */
/****************************************************************************/
    FILE *in_file;
    int i,j,ind,col,offset,count,last;
    int Nrow,Ncol,Nnzero,Nentries,Nrhs;
    int Ptrcrd, Indcrd, Valcrd, Rhscrd;
    int Ptrperline, Ptrwidth, Indperline, Indwidth;
    int Valperline, Valwidth, Valprec;
    int Valflag;           /* Indicates 'E','D', or 'F' float format */
    char* ThisElement;
    char line[BUFSIZ];
    char Title[73], Key[8], Type[4], Rhstype[4];
    char Ptrfmt[17], Indfmt[17], Rhsfmt[21];

    if ( (in_file = fopen( filename, "r")) == NULL ) {
       fprintf(stderr,"Error: Cannot open file: %s\n",filename);
       return 0;
    }

    readHB_header(in_file, Title, Key, Type, &Nrow, &Ncol, &Nnzero, &Nrhs,
                  Ptrfmt, Indfmt, Valfmt, Rhsfmt,
                  &Ptrcrd, &Indcrd, &Valcrd, &Rhscrd, Rhstype);

/*  Parse the array input formats from Line 3 of HB file  */
    ParseIfmt(Ptrfmt,&Ptrperline,&Ptrwidth);
    ParseIfmt(Indfmt,&Indperline,&Indwidth);
    if ( Type[0] != 'P' ) {          /* Skip if pattern only  */
       ParseRfmt(Valfmt,&Valperline,&Valwidth,&Valprec,&Valflag);
       if (Valflag == 'D') {
          *strchr(Valfmt,'D') = 'E';
       }
    }

/*  Read column pointer array:   */

    offset = 1-_SP_base;  /* if base 0 storage is declared (via macro definition), */
                          /* then storage entries are offset by 1                  */

    ThisElement = (char *) malloc(Ptrwidth+1);
    if ( ThisElement == NULL ) IOHBTerminate("Insufficient memory for ThisElement.");
    *(ThisElement+Ptrwidth) = (char) NULL;
    count=0; 
    for (i=0;i<Ptrcrd;i++)
    {
       fgets(line, BUFSIZ, in_file);
       if ( sscanf(line,"%*s") < 0 ) 
         IOHBTerminate("iohb.c: Null (or blank) line in pointer data region of HB file.\n");
       col =  0;
       for (ind = 0;ind<Ptrperline;ind++)
       {
          if (count > Ncol) break;
          strncpy(ThisElement,line+col,Ptrwidth);
          /*ThisElement = substr(line,col,Ptrwidth);*/
          colptr[count] = atoi(ThisElement)-offset;
          count++; col += Ptrwidth;
       }
    }
    free(ThisElement);

/*  Read row index array:  */

    ThisElement = (char *) malloc(Indwidth+1);
    if ( ThisElement == NULL ) IOHBTerminate("Insufficient memory for ThisElement.");
    *(ThisElement+Indwidth) = (char) NULL;
    count = 0;
    for (i=0;i<Indcrd;i++)
    {
       fgets(line, BUFSIZ, in_file);
       if ( sscanf(line,"%*s") < 0 ) 
         IOHBTerminate("iohb.c: Null (or blank) line in index data region of HB file.\n");
       col =  0;
       for (ind = 0;ind<Indperline;ind++)
       {
          if (count == Nnzero) break;
          strncpy(ThisElement,line+col,Indwidth);
          /*ThisElement = substr(line,col,Indwidth);*/
          rowind[count] = atoi(ThisElement)-offset;
          count++; col += Indwidth;
       }
    }
    free(ThisElement);

/*  Read array of values:  AS CHARACTERS*/

    if ( Type[0] != 'P' ) {          /* Skip if pattern only  */

       if ( Type[0] == 'C' ) Nentries = 2*Nnzero;
           else Nentries = Nnzero;

    ThisElement = (char *) malloc(Valwidth+1);
    if ( ThisElement == NULL ) IOHBTerminate("Insufficient memory for ThisElement.");
    *(ThisElement+Valwidth) = (char) NULL;
    count = 0;
    for (i=0;i<Valcrd;i++)
    {
       fgets(line, BUFSIZ, in_file);
       if ( sscanf(line,"%*s") < 0 ) 
         IOHBTerminate("iohb.c: Null (or blank) line in value data region of HB file.\n");
       if (Valflag == 'D') {
          while( strchr(line,'D') ) *strchr(line,'D') = 'E';
       }
       col =  0;
       for (ind = 0;ind<Valperline;ind++)
       {
          if (count == Nentries) break;
          ThisElement = &val[count*Valwidth];
          strncpy(ThisElement,line+col,Valwidth);
          /*strncpy(ThisElement,substr(line,col,Valwidth),Valwidth);*/
          if ( Valflag != 'F' && strchr(ThisElement,'E') == NULL ) { 
             /* insert a char prefix for exp */
             last = strlen(ThisElement);
             for (j=last+1;j>=0;j--) {
                ThisElement[j] = ThisElement[j-1];
                if ( ThisElement[j] == '+' || ThisElement[j] == '-' ) {
                   ThisElement[j-1] = Valflag;                    
                   break;
                }
             }
          }
          count++; col += Valwidth;
       }
    }
    }

    return 1;
}

int readHB_newmat_char(const char* filename, int* M, int* N, int* nonzeros, int** colptr, 
                          int** rowind, char** val, char** Valfmt)
{
    FILE *in_file;
    int Nrhs;
    int Ptrcrd, Indcrd, Valcrd, Rhscrd;
    int Valperline, Valwidth, Valprec;
    int Valflag;           /* Indicates 'E','D', or 'F' float format */
    char Title[73], Key[9], Type[4], Rhstype[4];
    char Ptrfmt[17], Indfmt[17], Rhsfmt[21];

    if ((in_file = fopen( filename, "r")) == NULL) {
      fprintf(stderr,"Error: Cannot open file: %s\n",filename);
      return 0;
     }
    
    *Valfmt = (char *)malloc(21*sizeof(char));
    if ( *Valfmt == NULL ) IOHBTerminate("Insufficient memory for Valfmt.");
    readHB_header(in_file, Title, Key, Type, M, N, nonzeros, &Nrhs,
                  Ptrfmt, Indfmt, (*Valfmt), Rhsfmt,
                  &Ptrcrd, &Indcrd, &Valcrd, &Rhscrd, Rhstype);
    fclose(in_file);
    ParseRfmt(*Valfmt,&Valperline,&Valwidth,&Valprec,&Valflag);

        *colptr = (int *)malloc((*N+1)*sizeof(int));
        if ( *colptr == NULL ) IOHBTerminate("Insufficient memory for colptr.\n");
        *rowind = (int *)malloc(*nonzeros*sizeof(int));
        if ( *rowind == NULL ) IOHBTerminate("Insufficient memory for rowind.\n");
        if ( Type[0] == 'C' ) {
/*
   fprintf(stderr, "Warning: Reading complex data from HB file %s.\n",filename);
   fprintf(stderr, "         Real and imaginary parts will be interlaced in val[].\n");
*/
           /* Malloc enough space for real AND imaginary parts of val[] */
           *val = (char *)malloc(*nonzeros*Valwidth*sizeof(char)*2);
           if ( *val == NULL ) IOHBTerminate("Insufficient memory for val.\n");
        } else {
           if ( Type[0] != 'P' ) {   
             /* Malloc enough space for real array val[] */
             *val = (char *)malloc(*nonzeros*Valwidth*sizeof(char));
             if ( *val == NULL ) IOHBTerminate("Insufficient memory for val.\n");
           }
        }  /* No val[] space needed if pattern only */
        return readHB_mat_char(filename, *colptr, *rowind, *val, *Valfmt);

}

int readHB_aux_char(const char* filename, const char AuxType, char b[])
{
/****************************************************************************/
/*  This function opens and reads the specified file, placing auxilary      */
/*  vector(s) of the given type (if available) in b :                       */
/*  Return value is the number of vectors successfully read.                */
/*                                                                          */
/*                AuxType = 'F'   full right-hand-side vector(s)            */
/*                AuxType = 'G'   initial Guess vector(s)                   */
/*                AuxType = 'X'   eXact solution vector(s)                  */
/*                                                                          */
/*    ----------                                                            */
/*    **CAVEAT**                                                            */
/*    ----------                                                            */
/*  Parsing real formats from Fortran is tricky, and this file reader       */
/*  does not claim to be foolproof.   It has been tested for cases when     */
/*  the real values are printed consistently and evenly spaced on each      */
/*  line, with Fixed (F), and Exponential (E or D) formats.                 */
/*                                                                          */
/*  **  If the input file does not adhere to the H/B format, the  **        */
/*  **             results will be unpredictable.                 **        */
/*                                                                          */
/****************************************************************************/
    FILE *in_file;
    int i,j,n,maxcol,start,stride,col,last,linel,nvecs,rhsi;
    int Nrow, Ncol, Nnzero, Nentries,Nrhs;
    int Ptrcrd, Indcrd, Valcrd, Rhscrd;
    int Rhsperline, Rhswidth, Rhsprec;
    int Rhsflag;
    char Title[73], Key[9], Type[4], Rhstype[4];
    char Ptrfmt[17], Indfmt[17], Valfmt[21], Rhsfmt[21];
    char line[BUFSIZ];
    char *ThisElement;

    if ((in_file = fopen( filename, "r")) == NULL) {
      fprintf(stderr,"Error: Cannot open file: %s\n",filename);
      return 0;
     }

    readHB_header(in_file, Title, Key, Type, &Nrow, &Ncol, &Nnzero, &Nrhs,
                  Ptrfmt, Indfmt, Valfmt, Rhsfmt,
                  &Ptrcrd, &Indcrd, &Valcrd, &Rhscrd, Rhstype);

    if (Nrhs <= 0)
    {
      fprintf(stderr, "Warn: Attempt to read auxillary vector(s) when none are present.\n");
      return 0;
    }
    if (Rhstype[0] != 'F' )
    {
      fprintf(stderr,"Warn: Attempt to read auxillary vector(s) which are not stored in Full form.\n");
      fprintf(stderr,"       Rhs must be specified as full. \n");
      return 0;
    }

/* If reading complex data, allow for interleaved real and imaginary values. */ 
    if ( Type[0] == 'C' ) {
       Nentries = 2*Nrow;
     } else {
       Nentries = Nrow;
    }

    nvecs = 1;
    
    if ( Rhstype[1] == 'G' ) nvecs++;
    if ( Rhstype[2] == 'X' ) nvecs++;

    if ( AuxType == 'G' && Rhstype[1] != 'G' ) {
      fprintf(stderr, "Warn: Attempt to read auxillary Guess vector(s) when none are present.\n");
      return 0;
    }
    if ( AuxType == 'X' && Rhstype[2] != 'X' ) {
      fprintf(stderr, "Warn: Attempt to read auxillary eXact solution vector(s) when none are present.\n");
      return 0;
    }

    ParseRfmt(Rhsfmt, &Rhsperline, &Rhswidth, &Rhsprec,&Rhsflag);
    maxcol = Rhsperline*Rhswidth;

/*  Lines to skip before starting to read RHS values... */
    n = Ptrcrd + Indcrd + Valcrd;

    for (i = 0; i < n; i++)
      fgets(line, BUFSIZ, in_file);

/*  start  - number of initial aux vector entries to skip   */
/*           to reach first  vector requested               */
/*  stride - number of aux vector entries to skip between   */
/*           requested vectors                              */
    if ( AuxType == 'F' ) start = 0;
    else if ( AuxType == 'G' ) start = Nentries;
    else start = (nvecs-1)*Nentries;
    stride = (nvecs-1)*Nentries;

    fgets(line, BUFSIZ, in_file);
    linel= strchr(line,'\n')-line;
    if ( sscanf(line,"%*s") < 0 ) 
       IOHBTerminate("iohb.c: Null (or blank) line in auxillary vector data region of HB file.\n");
    col = 0;
/*  Skip to initial offset */

    for (i=0;i<start;i++) {
       col += Rhswidth;
       if ( col >= ( maxcol<linel?maxcol:linel ) ) {
           fgets(line, BUFSIZ, in_file);
           linel= strchr(line,'\n')-line;
       if ( sscanf(line,"%*s") < 0 ) 
       IOHBTerminate("iohb.c: Null (or blank) line in auxillary vector data region of HB file.\n");
           col = 0;
       }
    }

    if (Rhsflag == 'D')  {
      while( strchr(line,'D') ) *strchr(line,'D') = 'E';
    }
/*  Read a vector of desired type, then skip to next */
/*  repeating to fill Nrhs vectors                   */

  for (rhsi=0;rhsi<Nrhs;rhsi++) {

    for (i=0;i<Nentries;i++) {
       if ( col >= ( maxcol<linel?maxcol:linel ) ) {
           fgets(line, BUFSIZ, in_file);
           linel= strchr(line,'\n')-line;
       if ( sscanf(line,"%*s") < 0 ) 
       IOHBTerminate("iohb.c: Null (or blank) line in auxillary vector data region of HB file.\n");
           if (Rhsflag == 'D')  {
              while( strchr(line,'D') ) *strchr(line,'D') = 'E';
           }
           col = 0;
       }
       ThisElement = &b[i*Rhswidth]; 
       strncpy(ThisElement,line+col,Rhswidth);
          if ( Rhsflag != 'F' && strchr(ThisElement,'E') == NULL ) { 
             /* insert a char prefix for exp */
             last = strlen(ThisElement);
             for (j=last+1;j>=0;j--) {
                ThisElement[j] = ThisElement[j-1];
                if ( ThisElement[j] == '+' || ThisElement[j] == '-' ) {
                   ThisElement[j-1] = Rhsflag;                    
                   break;
                }
             }
          }
       col += Rhswidth;
    }
    b+=Nentries*Rhswidth;
 
/*  Skip any interleaved Guess/eXact vectors */

    for (i=0;i<stride;i++) {
       col += Rhswidth;
       if ( col >= ( maxcol<linel?maxcol:linel ) ) {
           fgets(line, BUFSIZ, in_file);
           linel= strchr(line,'\n')-line;
       if ( sscanf(line,"%*s") < 0 ) 
       IOHBTerminate("iohb.c: Null (or blank) line in auxillary vector data region of HB file.\n");
           col = 0;
       }
    }

  }
    

    fclose(in_file);
    return Nrhs;
}

int readHB_newaux_char(const char* filename, const char AuxType, char** b, char** Rhsfmt)
{
    FILE *in_file;
    int Ptrcrd, Indcrd, Valcrd, Rhscrd;
    int Nrow,Ncol,Nnzero,Nrhs;
    int Rhsperline, Rhswidth, Rhsprec;
    int Rhsflag;
    char Title[73], Key[9], Type[4], Rhstype[4];
    char Ptrfmt[17], Indfmt[17], Valfmt[21];

    if ((in_file = fopen( filename, "r")) == NULL) {
      fprintf(stderr,"Error: Cannot open file: %s\n",filename);
      return 0;
     }

    *Rhsfmt = (char *)malloc(21*sizeof(char));
    if ( *Rhsfmt == NULL ) IOHBTerminate("Insufficient memory for Rhsfmt.");
    readHB_header(in_file, Title, Key, Type, &Nrow, &Ncol, &Nnzero, &Nrhs,
                  Ptrfmt, Indfmt, Valfmt, (*Rhsfmt),
                  &Ptrcrd, &Indcrd, &Valcrd, &Rhscrd, Rhstype);
     fclose(in_file);
        if ( Nrhs == 0 ) {
          fprintf(stderr,"Warn: Requested read of aux vector(s) when none are present.\n");
          return 0;
        } else {
          ParseRfmt(*Rhsfmt,&Rhsperline,&Rhswidth,&Rhsprec,&Rhsflag);
          if ( Type[0] == 'C' ) {
            fprintf(stderr, "Warning: Reading complex aux vector(s) from HB file %s.",filename);
            fprintf(stderr, "         Real and imaginary parts will be interlaced in b[].");
            *b = (char *)malloc(Nrow*Nrhs*Rhswidth*sizeof(char)*2);
            if ( *b == NULL ) IOHBTerminate("Insufficient memory for rhs.\n");
            return readHB_aux_char(filename, AuxType, *b);
          } else {
            *b = (char *)malloc(Nrow*Nrhs*Rhswidth*sizeof(char));
            if ( *b == NULL ) IOHBTerminate("Insufficient memory for rhs.\n");
            return readHB_aux_char(filename, AuxType, *b);
          }
        } 
}

int writeHB_mat_char(const char* filename, int M, int N, 
                        int nz, const int colptr[], const int rowind[], 
                        const char val[], int Nrhs, const char rhs[], 
                        const char guess[], const char exact[], 
                        const char* Title, const char* Key, const char* Type, 
                        char* Ptrfmt, char* Indfmt, char* Valfmt, char* Rhsfmt,
                        const char* Rhstype)
{
/****************************************************************************/
/*  The writeHB function opens the named file and writes the specified      */
/*  matrix and optional right-hand-side(s) to that file in Harwell-Boeing   */
/*  format.                                                                 */
/*                                                                          */
/*  For a description of the Harwell Boeing standard, see:                  */
/*            Duff, et al.,  ACM TOMS Vol.15, No.1, March 1989              */
/*                                                                          */
/****************************************************************************/
    FILE *out_file;
    int i,j,acount,linemod,entry,offset;
    int totcrd, ptrcrd, indcrd, valcrd, rhscrd;
    int nvalentries, nrhsentries;
    int Ptrperline, Ptrwidth, Indperline, Indwidth;
    int Rhsperline, Rhswidth, Rhsprec;
    int Rhsflag;
    int Valperline, Valwidth, Valprec;
    int Valflag;           /* Indicates 'E','D', or 'F' float format */
    char pformat[16],iformat[16],vformat[19],rformat[19];

    if ( Type[0] == 'C' ) {
         nvalentries = 2*nz;
         nrhsentries = 2*M;
    } else {
         nvalentries = nz;
         nrhsentries = M;
    }

    if ( filename != NULL ) {
       if ( (out_file = fopen( filename, "w")) == NULL ) {
         fprintf(stderr,"Error: Cannot open file: %s\n",filename);
         return 0;
       }
    } else out_file = stdout;

    if ( Ptrfmt == NULL ) Ptrfmt = "(8I10)";
    ParseIfmt(Ptrfmt,&Ptrperline,&Ptrwidth);
    sprintf(pformat,"%%%dd",Ptrwidth);
   
    if ( Indfmt == NULL ) Indfmt =  Ptrfmt;
    ParseIfmt(Indfmt,&Indperline,&Indwidth);
    sprintf(iformat,"%%%dd",Indwidth);

    if ( Type[0] != 'P' ) {          /* Skip if pattern only  */
      if ( Valfmt == NULL ) Valfmt = "(4E20.13)";
      ParseRfmt(Valfmt,&Valperline,&Valwidth,&Valprec,&Valflag);
      sprintf(vformat,"%%%ds",Valwidth);
    }

    ptrcrd = (N+1)/Ptrperline;
    if ( (N+1)%Ptrperline != 0) ptrcrd++;

    indcrd = nz/Indperline;
    if ( nz%Indperline != 0) indcrd++;

    valcrd = nvalentries/Valperline;
    if ( nvalentries%Valperline != 0) valcrd++;

    if ( Nrhs > 0 ) {
       if ( Rhsfmt == NULL ) Rhsfmt = Valfmt;
       ParseRfmt(Rhsfmt,&Rhsperline,&Rhswidth,&Rhsprec, &Rhsflag);
       sprintf(rformat,"%%%ds",Rhswidth);
       rhscrd = nrhsentries/Rhsperline; 
       if ( nrhsentries%Rhsperline != 0) rhscrd++;
       if ( Rhstype[1] == 'G' ) rhscrd+=rhscrd;
       if ( Rhstype[2] == 'X' ) rhscrd+=rhscrd;
       rhscrd*=Nrhs;
    } else rhscrd = 0;

    totcrd = 4+ptrcrd+indcrd+valcrd+rhscrd;


/*  Print header information:  */

    fprintf(out_file,"%-72s%-8s\n%14d%14d%14d%14d%14d\n",Title, Key, totcrd,
            ptrcrd, indcrd, valcrd, rhscrd);
    fprintf(out_file,"%3s%11s%14d%14d%14d\n",Type,"          ", M, N, nz);
    fprintf(out_file,"%-16s%-16s%-20s", Ptrfmt, Indfmt, Valfmt);
    if ( Nrhs != 0 ) {
/*     Print Rhsfmt on fourth line and                                    */
/*           optional fifth header line for auxillary vector information: */
       fprintf(out_file,"%-20s\n%-14s%d\n",Rhsfmt,Rhstype,Nrhs);
    } else fprintf(out_file,"\n");

    offset = 1-_SP_base;  /* if base 0 storage is declared (via macro definition), */
                          /* then storage entries are offset by 1                  */

/*  Print column pointers:   */
    for (i=0;i<N+1;i++)
    {
       entry = colptr[i]+offset;
       fprintf(out_file,pformat,entry);
       if ( (i+1)%Ptrperline == 0 ) fprintf(out_file,"\n");
    }

   if ( (N+1) % Ptrperline != 0 ) fprintf(out_file,"\n");

/*  Print row indices:       */
    for (i=0;i<nz;i++)
    {
       entry = rowind[i]+offset;
       fprintf(out_file,iformat,entry);
       if ( (i+1)%Indperline == 0 ) fprintf(out_file,"\n");
    }

   if ( nz % Indperline != 0 ) fprintf(out_file,"\n");

/*  Print values:            */

    if ( Type[0] != 'P' ) {          /* Skip if pattern only  */
    for (i=0;i<nvalentries;i++)
    {
       fprintf(out_file,vformat,val+i*Valwidth);
       if ( (i+1)%Valperline == 0 ) fprintf(out_file,"\n");
    }

    if ( nvalentries % Valperline != 0 ) fprintf(out_file,"\n");

/*  Print right hand sides:  */
    acount = 1;
    linemod=0;
    if ( Nrhs > 0 ) {
      for (j=0;j<Nrhs;j++) {
       for (i=0;i<nrhsentries;i++)
       {
          fprintf(out_file,rformat,rhs+i*Rhswidth);
          if ( acount++%Rhsperline == linemod ) fprintf(out_file,"\n");
       }
       if ( acount%Rhsperline != linemod ) {
          fprintf(out_file,"\n");
          linemod = (acount-1)%Rhsperline;
       }
       if ( Rhstype[1] == 'G' ) {
         for (i=0;i<nrhsentries;i++)
         {
           fprintf(out_file,rformat,guess+i*Rhswidth);
           if ( acount++%Rhsperline == linemod ) fprintf(out_file,"\n");
         }
         if ( acount%Rhsperline != linemod ) {
            fprintf(out_file,"\n");
            linemod = (acount-1)%Rhsperline;
         }
       }
       if ( Rhstype[2] == 'X' ) {
         for (i=0;i<nrhsentries;i++)
         {
           fprintf(out_file,rformat,exact+i*Rhswidth);
           if ( acount++%Rhsperline == linemod ) fprintf(out_file,"\n");
         }
         if ( acount%Rhsperline != linemod ) {
            fprintf(out_file,"\n");
            linemod = (acount-1)%Rhsperline;
         }
       }
      }
    }

    }

    if ( fclose(out_file) != 0){
      fprintf(stderr,"Error closing file in writeHB_mat_char().\n");
      return 0;
    } else return 1;
    
}

int ParseIfmt(char* fmt, int* perline, int* width)
{
/*************************************************/
/*  Parse an *integer* format field to determine */
/*  width and number of elements per line.       */
/*************************************************/
    char *tmp;
    if (fmt == NULL ) {
      *perline = 0; *width = 0; return 0;
    }
    upcase(fmt);
    tmp = strchr(fmt,'(');
    tmp = substr(fmt,tmp - fmt + 1, strchr(fmt,'I') - tmp - 1);
    *perline = atoi(tmp);
    tmp = strchr(fmt,'I');
    tmp = substr(fmt,tmp - fmt + 1, strchr(fmt,')') - tmp - 1);
    return *width = atoi(tmp);
}

int ParseRfmt(char* fmt, int* perline, int* width, int* prec, int* flag)
{
/*************************************************/
/*  Parse a *real* format field to determine     */
/*  width and number of elements per line.       */
/*  Also sets flag indicating 'E' 'F' 'P' or 'D' */
/*  format.                                      */
/*************************************************/
    char* tmp;
    char* tmp2;
    char* tmp3;
    int len;

    if (fmt == NULL ) {
      *perline = 0; 
      *width = 0; 
      flag = NULL;  
      return 0;
    }

    upcase(fmt);
    if (strchr(fmt,'(') != NULL)  fmt = strchr(fmt,'(');
    if (strchr(fmt,')') != NULL)  {
       tmp2 = strchr(fmt,')');
       while ( strchr(tmp2+1,')') != NULL ) {
          tmp2 = strchr(tmp2+1,')');
       }
       *(tmp2+1) = (int) NULL;
    }
    if (strchr(fmt,'P') != NULL)  /* Remove any scaling factor, which */
    {                             /* affects output only, not input */
      if (strchr(fmt,'(') != NULL) {
        tmp = strchr(fmt,'P');
        if ( *(++tmp) == ',' ) tmp++;
        tmp3 = strchr(fmt,'(')+1;
        len = tmp-tmp3;
        tmp2 = tmp3;
        while ( *(tmp2+len) != (int) NULL ) {
           *tmp2=*(tmp2+len);
           tmp2++; 
        }
        *(strchr(fmt,')')+1) = (int) NULL;
      }
    }
    if (strchr(fmt,'E') != NULL) { 
       *flag = 'E';
    } else if (strchr(fmt,'D') != NULL) { 
       *flag = 'D';
    } else if (strchr(fmt,'F') != NULL) { 
       *flag = 'F';
    } else {
      fprintf(stderr,"Real format %s in H/B file not supported.\n",fmt);
      return 0;
    }
    tmp = strchr(fmt,'(');
    tmp = substr(fmt,tmp - fmt + 1, strchr(fmt,*flag) - tmp - 1);
    *perline = atoi(tmp);
    tmp = strchr(fmt,*flag);
    if ( strchr(fmt,'.') ) {
      *prec = atoi( substr( fmt, strchr(fmt,'.') - fmt + 1, strchr(fmt,')') - strchr(fmt,'.')-1) );
      tmp = substr(fmt,tmp - fmt + 1, strchr(fmt,'.') - tmp - 1);
    } else {
      tmp = substr(fmt,tmp - fmt + 1, strchr(fmt,')') - tmp - 1);
    }
    return *width = atoi(tmp);
}

char* substr(const char* S, const int pos, const int len)
{
    int i;
    char *SubS;
    if ( pos+len <= strlen(S)) {
    SubS = (char *)malloc(len+1);
    if ( SubS == NULL ) IOHBTerminate("Insufficient memory for SubS.");
    for (i=0;i<len;i++) SubS[i] = S[pos+i];
    SubS[len] = (char) NULL;
    } else {
      SubS = NULL;
    }
    return SubS;
}

#include<ctype.h>
void upcase(char* S)
{
/*  Convert S to uppercase     */
    int i,len;
    if ( S == NULL ) return;
    len = strlen(S);
    for (i=0;i< len;i++)
       S[i] = toupper(S[i]);
}

void IOHBTerminate(char* message) 
{
   fprintf(stderr,message);
   exit(1);
}

