#define KINT         int

#if   FP ==  32
#define KREAL        float
#elif FP ==  64
#define KREAL        double 
#elif FP == 128
#define KREAL        long double
#endif

typedef struct char_length{
	char name[64];
}CHAR_LENGTH;
typedef struct char80{
	char name[80];
}CHAR80;
