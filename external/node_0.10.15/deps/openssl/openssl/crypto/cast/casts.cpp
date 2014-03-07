//
// gettsc.inl
//
// gives access to the Pentium's (secret) cycle counter
//
// This software was written by Leonard Janke (janke@unixg.ubc.ca)
// in 1996-7 and is entered, by him, into the public domain.

#if defined(__WATCOMC__)
void GetTSC(unsigned long&);
#pragma aux GetTSC = 0x0f 0x31 "mov [edi], eax" parm [edi] modify [edx eax];
#elif defined(__GNUC__)
inline
void GetTSC(unsigned long& tsc)
{
  asm volatile(".byte 15, 49\n\t"
	       : "=eax" (tsc)
	       :
	       : "%edx", "%eax");
}
#elif defined(_MSC_VER)
inline
void GetTSC(unsigned long& tsc)
{
  unsigned long a;
  __asm _emit 0fh
  __asm _emit 31h
  __asm mov a, eax;
  tsc=a;
}
#endif      

#include <stdio.h>
#include <stdlib.h>
#include <openssl/cast.h>

void main(int argc,char *argv[])
	{
	CAST_KEY key;
	unsigned long s1,s2,e1,e2;
	unsigned long data[2];
	int i,j;
	static unsigned char d[16]={0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};

	CAST_set_key(&key, 16,d);

	for (j=0; j<6; j++)
		{
		for (i=0; i<1000; i++) /**/
			{
			CAST_encrypt(&data[0],&key);
			GetTSC(s1);
			CAST_encrypt(&data[0],&key);
			CAST_encrypt(&data[0],&key);
			CAST_encrypt(&data[0],&key);
			GetTSC(e1);
			GetTSC(s2);
			CAST_encrypt(&data[0],&key);
			CAST_encrypt(&data[0],&key);
			CAST_encrypt(&data[0],&key);
			CAST_encrypt(&data[0],&key);
			GetTSC(e2);
			CAST_encrypt(&data[0],&key);
			}

		printf("cast %d %d (%d)\n",
			e1-s1,e2-s2,((e2-s2)-(e1-s1)));
		}
	}

