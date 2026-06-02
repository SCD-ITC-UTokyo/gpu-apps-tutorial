#include "timer.hpp"
#include "timer.h"

extern "C"
{
  void* util_timer_constructor( void ){
    util::timer* obj = new util::timer();
    return obj;
  }
  void  util_timer_destructor( void* ptr ){
    util::timer* obj = (util::timer*)ptr;
    delete obj;
  }
  void  util_timer_start( void* ptr ){
    util::timer* obj = (util::timer*)ptr;
    obj->start();
  }
  void  util_timer_stop( void* ptr ){
    util::timer* obj = (util::timer*)ptr;
    obj->stop();
  }
  void  util_timer_clear( void* ptr ){
    util::timer* obj = (util::timer*)ptr;
    obj->clear();
  }
  void  util_timer_get_elapsed_wall( void* ptr, double* elapsed ){
    util::timer* obj = (util::timer*)ptr;
    *elapsed = obj->get_elapsed_wall();
  }
}
