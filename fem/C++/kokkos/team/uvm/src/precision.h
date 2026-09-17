#define KINT         int

/* 浮動小数精度は cmake -DFP=32/64 で切り替える (非 Kokkos 実装と同じ方式)。
   これが無いと FP マクロが無視され、常に倍精度で計算される。 */
#if   FP ==  32
#define KREAL        float
#elif FP ==  64
#define KREAL        double 
#elif FP == 128
#define KREAL        long double
#else
#define KREAL        double 
#endif

typedef struct char_length{
	char name[64];
}CHAR_LENGTH;
typedef struct char80{
	char name[80];
}CHAR80;
