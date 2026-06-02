#ifndef UTIL_TIMER_H
#define UTIL_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

void* util_timer_constructor( void );
void  util_timer_destructor( void* ptr );
void  util_timer_start( void* ptr );
void  util_timer_stop( void* ptr );
void  util_timer_clear( void* ptr );
void  util_timer_get_elapsed_wall( void* ptr, double* elapsed );

#ifdef __cplusplus
}
#endif

#endif // UTIL_TIMER_H
