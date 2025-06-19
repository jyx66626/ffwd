
/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include "defs.h"
#include "memory.h"

long Number_Of_Processors;
double Timestep_Dur;
real Softening_Param;
long Expansion_Terms;


real
RoundReal (real val)
{
   double shifter;
   double frac;
   long exp;
   double shifted_frac;
   double new_frac;
   double temp;
   real ret_val;

   shifter = pow((double) 10, (double) REAL_DIG - 2);
   frac = frexp((double) val, &exp);
   shifted_frac = frac * shifter;
   temp = modf(shifted_frac, &new_frac);
   new_frac /= shifter;
   ret_val = (real) ldexp(new_frac, exp);
   return( ret_val);
}


void
PrintComplexNum (complex *c)
{
   if (c->i >= (real) 0.0)
      printf("%e + %ei", c->r, c->i);
   else
      printf("%e - %ei", c->r, -c->i);
}


void
PrintVector (vector *v)
{
   printf("(%10.5f, %10.5f)", v->x, v->y);
}


union instance5 {struct input3{va_list *ap;char *format_str;} input3;};
void * function6(void *ctx4);
void *function6(void *ctx4) {
   {
      struct input3 *incontext1=&(((union instance5 *)ctx4)->input3);
      va_list *ap=incontext1->ap;
      char *format_str=incontext1->format_str;
      {
         fflush(stdout);
         vfprintf(stdout, format_str, (*ap));
         fflush(stdout);
      }
      return NULL;
   }
}

void
LockedPrint (char *format_str, ...)
{
   va_list ap;

   va_start(ap, format_str);
   { union instance5 instance5 = {
      {
         &ap,
         format_str,
      },
   };
   
   liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance5), &function6); }
   va_end(ap);
}


