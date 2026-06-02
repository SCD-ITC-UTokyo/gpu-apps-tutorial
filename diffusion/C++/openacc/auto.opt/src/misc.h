/**
 * @file misc.h
 * @brief (File brief)
 *
 * (File explanation)
 *
 * @author Takashi Shimokawabe
 * @date 2010/12/09 Created
 * @version 0.1.0
 *
 * $Id: misc.h,v 7a382b862cde 2011/02/22 13:30:31 shimokawabe $
 */

#ifndef MISC_H
#define MISC_H

#if    FP == 32
typedef float       flt;
#elif  FP == 64
typedef double      flt;
#elif  FP == 128
typedef long double flt;
#endif

void swap(flt **f, flt **fn);
void start_timer();
double get_elapsed_time();


#endif /* MISC_H */


