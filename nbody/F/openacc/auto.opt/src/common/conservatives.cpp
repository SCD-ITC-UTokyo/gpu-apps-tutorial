#include "conservatives.hpp"
#include "conservatives.h"

extern "C"
{
  void* conservatives_constructor( void ){
    conservatives* obj = new conservatives();
    return obj;
  }
  void  conservatives_destructor( void* ptr ){
    conservatives* obj = (conservatives*)ptr;
    delete obj;
  }
}
