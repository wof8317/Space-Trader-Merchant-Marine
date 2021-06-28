#ifndef __compiler_helpers_h_
#define __compiler_helpers_h_

/*
	Compiler-specific hints and helper macros.
	
	WINDOWS version.
*/

#if !defined( _MSC_VER ) || _MSC_VER < 1400
#error Microsoft Visual C++ 2005 or later required for the Windows build.
#endif

#define Q_EXTERNAL_CALL __stdcall
#define Q_EXTERNAL_CALL_VA __cdecl

#define ASSUME( x ) __assume( x )

#define RESTRICTED __declspec( restrict ) //function returns a pointer to unaliased memory (example, freshly allocated memory)
#define NOGLOBALALIAS __declspec( noalias ) //function only modifies memory pointed to directly by its parameters

#define RESTRICT __restrict //see the C99 requirements for "RESTRICT"
							//**note this *must* be defined after the stdlib headers are included

#if defined( _DEBUG )
#define ASSERT( x ) (assert( x ), ASSUME( x ))
#else
#define ASSERT( x ) __assume( x )
#endif

#define DECL_ALIGN_PRE( n ) __declspec( align( n ) )
#define DECL_ALIGN_POST( n )

#ifdef _DEBUG
#define NO_DEFAULT default: ASSERT( 0 ); //hoping for some compiler magic here
#else
#define NO_DEFAULT default: __assume( 0 ); //hoping for some compiler magic here
#endif

#ifndef REF_PARAM
#define REF_PARAM( p ) p
#endif

#endif