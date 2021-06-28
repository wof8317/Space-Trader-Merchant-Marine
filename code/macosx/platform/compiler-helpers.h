#ifndef __compiler_helpers_h_
#define __compiler_helpers_h_

/*
	Compiler-specific hints and helper macros.
	
	LINUX version.
 */

#ifndef __GNUC__
#error GCC specific stuff here.
#endif

#define Q_EXTERNAL_CALL
#define Q_EXTERNAL_CALL_VA

#define ASSUME( x )

#define RESTRICTED //function returns a pointer to unaliased memory (example, freshly allocated memory)
#define NOGLOBALALIAS //function only modifies memory pointed to directly by its parameters

#define RESTRICT __restrict //see the C99 requirements for "RESTRICT"
                            //**note this *must* be defined after the stdlib headers are included

#if defined( _DEBUG )
#define ASSERT( x ) assert( x )
#else
#define ASSERT( x )
#endif

#define DECL_ALIGN_PRE( n )
#define DECL_ALIGN_POST( n ) __attribute__( ( aligned( n ) ) )

#ifdef _DEBUG
#define NO_DEFAULT default: ASSERT( 0 ); //hoping for some compiler magic here
#else
#define NO_DEFAULT default: break;
#endif
  
#ifndef REF_PARAM
#define REF_PARAM( p ) p = p
#endif
  
#endif
  