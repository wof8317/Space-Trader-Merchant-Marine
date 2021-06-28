/* 
	The following code is based on the implementatino of MT19937 coded
	by Takuji Nishimura and Makoto Matsumoto. This is NOT their original
	mt19937ar.c - IT HAS BEEN MODIFIED to fit into Space Trader's codebase.
	
	Nevertheless the original header and license is reproduced below:

	===========================================================================

		A C-program for MT19937, with initialization improved 2002/1/26.
		Coded by Takuji Nishimura and Makoto Matsumoto.

		Before using, initialize the state by using init_genrand(seed)  
		or init_by_array(init_key, key_length).

		Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
		All rights reserved.                          

		Redistribution and use in source and binary forms, with or without
		modification, are permitted provided that the following conditions
		are met:

		 1. Redistributions of source code must retain the above copyright
			notice, this list of conditions and the following disclaimer.

		 2. Redistributions in binary form must reproduce the above copyright
			notice, this list of conditions and the following disclaimer in the
			documentation and/or other materials provided with the distribution.

		 3. The names of its contributors may not be used to endorse or promote 
			products derived from this software without specific prior written 
			permission.

		THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
		"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
		LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
		A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
		CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
		EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
		PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
		PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
		LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
		NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
		SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


		Any feedback is very welcome.
		http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
		email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#include "rand.h"

/* Period parameters */  
#define N MT_RAND_N
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

/* initializes mt[N] with a seed */
/* void init_genrand( randState_t *rs, unsigned long s ) */
void Rand_Initialize( randState_t *rs, unsigned long s )
{
	rs->mti = N + 1;

    rs->mt[0]= s & 0xffffffffUL;
    for( rs->mti = 1; rs->mti < N; rs->mti++ )
	{
        rs->mt[rs->mti] = 
	    (1812433253UL * (rs->mt[rs->mti - 1] ^ (rs->mt[rs->mti - 1] >> 30)) + rs->mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        rs->mt[rs->mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* generates a random number on [0,0xffffffff]-interval */
/* unsigned long genrand_int32( randState_t *rs ) */
unsigned long Rand_NextUInt32( randState_t *rs )
{
    unsigned long y;
    static const unsigned long mag01[2] = { 0x0UL, MATRIX_A };
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if( rs->mti >= N )
	{
		/* generate N words at one time */
        int kk;

        if( rs->mti == N + 1 )
		{
			/* if init_genrand() has not been called, */
			/* a default initial seed is used */
            Rand_Initialize( rs, 5489UL );
		}

        for( kk = 0; kk < N - M; kk++ )
		{
            y = (rs->mt[kk] & UPPER_MASK) | (rs->mt[kk + 1] & LOWER_MASK);
            rs->mt[kk] = rs->mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for( ; kk < N - 1; kk++ )
		{
            y = (rs->mt[kk] & UPPER_MASK) | (rs->mt[kk + 1] & LOWER_MASK);
            rs->mt[kk] = rs->mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (rs->mt[N - 1] & UPPER_MASK) | (rs->mt[0] & LOWER_MASK);
        rs->mt[N - 1] = rs->mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        rs->mti = 0;
    }
  
    y = rs->mt[rs->mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

int Rand_NextInt32InRange( randState_t *rs, int low, int high )
{
	int rv = Rand_NextUInt32( rs );

	if( rv < 0 ) rv = -rv;

	return low + rv % (high - low);
}