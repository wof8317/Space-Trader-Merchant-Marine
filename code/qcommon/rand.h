#define MT_RAND_N 624
typedef struct randState_t
{
	unsigned long mt[MT_RAND_N];
	int mti;
} randState_t;

void Rand_Initialize( randState_t *rs, unsigned long s );
unsigned long Rand_NextUInt32( randState_t *rs );
int Rand_NextInt32InRange( randState_t *rs, int low, int high );
