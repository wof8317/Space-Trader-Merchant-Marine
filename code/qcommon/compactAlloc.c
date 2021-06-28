#include "q_shared.h"
#include "qcommon.h"

/*
	Start up and shut down the compacting memory allocator.
*/
void Mem_CaInit( void *pMem, size_t size );
void Mem_CaKill( void );

typedef int MemHandle_t;

/*
	Allocates memory on the compacting heap. Calling this invalidates all existing
	heap pointers (handles are OK).
*/
MemHandle_t Mem_CaAlloc( size_t cb );
MemHandle_t Mem_CaCalloc( size_t cb );

/*
	Release a block on the compacting heap.
*/
void Mem_CaFree( MemHandle_t h );

/*
	Gets a pointer to the memory referenced by a MemHandle_t. This pointer is valid
	until the next call to Mem_CaAlloc, Mem_CaCalloc, or Mem_CaFree, or Mem_CaCompact.
*/
void* Mem_CaGetPtr( MemHandle_t h );

/*
	Mem_CaCompact

	Compacts memory. The compacting process will stop either
	when it's done all it can or when it has grown the free
	segment to stopSize. Setting stopSize to zero will cause
	the compactor to do a full pass.

	The function returns the new size of the free segment.
*/
size_t Mem_CaCompact( size_t stopSize );

/*
	All allocations will be created and kept on MEM_ALIGN boundaries.
*/
#define MEM_ALIGN					4

#define MEM_MIN_ALLOC_SIZE			16

/*
	If an allocation for nBytes can't be made at the end of memory,
	the memory manager will do a collection and attempt to free up to
	nBytes * MEM_QUICK_COLLECT_FACTOR.
*/
#define MEM_QUICK_COLLECT_FACTOR	4

/*
	The maximum allocation size is MEM_MAX_SIZE. The max size is
	set by reserving MEM_MAX_SIZE_LOG2 bits in the memHead_t struct.

	At least two bits must remain free so the max value of
	MEM_MAX_SIZE_LOG2 is 30. There shouldn't be any reason to change
	this unless you want to tighten up the error checking somewhere.
*/
#define MEM_MAX_SIZE_LOG2			30
#define MEM_MAX_SIZE				(((1 << MEM_MAX_SIZE_LOG2) - 1) * MEM_MIN_ALLOC_SIZE)

#if MEM_MAX_SIZE_LOG2 < 10 || MEM_MAX_SIZE_LOG2 > 30
#error MEM_MAX_SIZE_LOG2 is out of range
#endif

/*
	One of these precedes each allocation in the heap.

	Given memHead_t *pCurr, the next one *pNext is located at
		
		pNext = Align( pCurr + sizeof( memHead_t ) * 2 + GetSize( pCurr ), MEM_ALIGN ) - sizeof( memHead_t ).

	Breaking it down:
		1.	Start at pCurr.
		2.	Advance sizeof( memHead_t ) to the start of the user memory block.
		3.	Advance pCurr->size bytes to the end of the user memory block.
		4.	Note that another memHead_t comes before the next alignment.
		5.	Snap to the next alignment.
		6.	Move back to the memHead_t for that alignment.
*/
typedef struct memHead_t
{
	int		handle_val;

	uint	size	: MEM_MAX_SIZE_LOG2;
	uint	used	: 1;
	//uint	locked	: 1;
} memHead_t;

#define GetHeader( p ) ((memHead_t*)(p) - 1)
#define GetNextHeader( h ) GetHeader( AlignP( (byte*)(h) + sizeof( memHead_t ) * 2 + (h)->size, MEM_ALIGN ) )

#define GetUserData( h ) ((byte*)((memHead_t*)(h) + 1))

#define GetSize( h ) ((h)->size * MEM_MIN_ALLOC_SIZE)
#define GetMapEntry( h ) ((MemPtr*)md.pEnd + (h)->handle_val)

typedef byte *MemPtr;

/*
	Memory is divided into three sections:

		-	User memory.
		-	The free segment.
		-	The map segment.

	User memory consists of an end-to-end list of memory blocks that are (or have been)
	in use by the calling code. Each memory block is aligned to a MEM_ALIGN byte boundary.
	Each block is preceeded by a memHead_t record who's pointer can be derived from the
	block's pointer p via GetHeader( p ). The first block is located at pBase. The last
	block ends before pFree (which points to the next aligned location).

	The free segment begins at pFree and ends at pLimit. When allocating, the code will first
	try to pull from this spot.

	The map segment starts at pLimit and ends at pEnd. The map is an array of MemPtr. The
	array is indexed by the handle values (h) as follows: p = ((MemPtr*)pEnd)[h]. The handle
	values will always be negative as they index back from the end of the map array. The map
	array grows into the free segment as needed (and is occasionally collapsed). When a handle
	is freed its map entry is set to NULL.

	The allocCount maintains the number of active allocations. If the length of the map segment
	is greater than the number of allocations it means the map segment has become fragmented and
	Alloc will do a quick linear search to find a free handle. Note that handle values are always
	taken towards the pEnd side of the map. As values near the pLimit side get NULL'd out the
	compactor will reclaim the space into the free segment.
*/
static struct
{
	byte		*pBase;
	byte		*pLimit;
	byte		*pFree;
	byte		*pEnd;

	size_t		allocCount;
} md;

#define AlignP( p, a ) (byte*)Align( (size_t)(p), a )

static void Mem_Error( const char *msg, ... )
{
	va_list vargs;
	char buff[2048];

	va_start( vargs, msg );
	vsnprintf( buff, sizeof( buff ) - 1, msg, vargs );
	buff[sizeof( buff ) - 1] = 0;
	va_end( vargs );

	Com_Error( ERR_FATAL, msg, "%s", buff );
}

void Mem_CaInit( void *pMem, size_t size )
{
	if( size < MEM_ALIGN + MEM_ALIGN + sizeof( memHead_t ) )
		/*
			Minimum memory size must be enough to guarantee alignment (MEM_ALIGN)
			plus enough to allocate at least once (again, MEM_ALIGN) and the
			tracking record for the allocation (the size of a memHead_t).
		*/
		Mem_Error( "Initial memory block is too small" );

	md.pBase = AlignP( (byte*)pMem + sizeof( memHead_t ), MEM_ALIGN );
	md.pEnd = md.pBase + size;
	
	md.pFree = md.pBase;
	md.pLimit = md.pEnd;
}

void Mem_CaKill( void )
{
	Com_Memset( &md, 0, sizeof( md ) );
}

size_t Mem_CaCompact( size_t stopSize )
{
	/*
		Try clearing out old map records.
		
		Note: this chunk of the allocator *is* prone to fragmentation, but
		since it's fixed size it will never result in "lost" memory.

		Map records are cleaned up by trimming away any that fall on the
		"data-side" portion of the memory block.
	*/
	if( (MemPtr*)md.pEnd - (MemPtr*)md.pLimit > md.allocCount )
	{
		MemPtr *pCheck;
		for( pCheck = (MemPtr*)md.pLimit; pCheck < (MemPtr*)md.pEnd && !*pCheck; pCheck++ )
			//*pCheck points to nothing, reclaim it into the free segment
			md.pLimit += sizeof( MemPtr ); 
	}

	if( stopSize )
	{
		if( md.pLimit - md.pFree >= stopSize )
			//already done
			return md.pLimit - md.pFree;

		/*
			Do fast things to try to get this much memory freed up.
		*/

		if( md.pLimit - md.pFree >= stopSize )
			//we've freed enough
			return md.pLimit - md.pFree;
	}
	
	/*
		Do a funn collection. Fully compact the heap.
	*/

	{
		memHead_t *pS, *pD;
		
		//find the first free spot
		for( pD = GetHeader( md.pBase ); GetUserData( pD ) < md.pFree; pD = GetNextHeader( pD ) )
			if( !pD->used )
				break;

		//scan across memory, moving blocks back from the scanning pointer (pS) to the destination pointer (pD)
		for( pS = pD; GetUserData( pS ) < md.pFree; pS = GetNextHeader( pS ) )
		{
			if( pS->used )
			{
				//move it back to pD, advance pD
				MemPtr *mS = GetMapEntry( pS );
				MemPtr *mD = GetMapEntry( pD );

				*mD = *mS;
				*mS = NULL;

				memcpy( GetUserData( pD ), GetUserData( pS ), GetSize( pS ) );
				*pD = *pS;

				pD = GetNextHeader( pD );
			}
		}

		//the free block now begins where the destination pointer ended
		md.pFree = GetUserData( pD );
	}

	//we've freed everything we possibly could
	return md.pLimit - md.pFree;
}

void* Mem_CaGetPtr( MemHandle_t h )
{
	if( !h )
		return NULL;

	return ((MemPtr*)md.pEnd)[h];
}

MemHandle_t Mem_CaAlloc( size_t cb )
{
	//find a free handle
	int h;

	if( (MemPtr*)md.pEnd - (MemPtr*)md.pLimit > md.allocCount )
	{
		//a free h already exists
		for( h = -1; h >= (MemPtr*)md.pLimit - (MemPtr*)md.pEnd; h-- )
			if( !((MemPtr*)md.pEnd)[h] )
				//found a free slot - use it
				break;
	}
	else
	{
		//need to make room for a new slot - set h = 0 as a flag
		h = 0;
	}

	if( md.pLimit - md.pFree - (h ? 0 : sizeof( MemPtr )) < cb )
		//not enough memory left, must compact
		if( Mem_CaCompact( cb * MEM_QUICK_COLLECT_FACTOR ) < cb )
			//compacting didn't free enough, out of memory
			return 0;

	if( !h )
	{
		//alloc the new slot
		md.pLimit -= sizeof( MemPtr );
		h = (MemPtr*)md.pLimit - (MemPtr*)md.pEnd;
	}

	{
		//do the actual allocation

		void *m = md.pFree;
		memHead_t *pM = GetHeader( m );

		pM->handle_val = h;
		pM->size = cb;
		pM->used = 1;

		md.pFree = GetUserData( GetNextHeader( pM ) );
		md.allocCount++;
	}

	return h;
}

MemHandle_t Mem_CaCalloc( size_t cb )
{
	MemHandle_t h = Mem_CaAlloc( cb );

	if( !h )
		return 0;

	memset( Mem_CaGetPtr( h ), 0, cb );

	return h;
}

void Mem_CaFree( MemHandle_t h )
{
	if( h )
	{
		MemPtr *pp = (MemPtr*)md.pEnd + h;

		GetHeader( *pp )->used = 0;

		*pp = NULL;
		md.allocCount--;
	}
}