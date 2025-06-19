
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


#include <pthread.h>
#include <liblock.h>
#include <liblock-splash2.h>
//;
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <liblock.h>
#include <liblock-splash2.h>
//;
#include <stdint.h>
#include <malloc.h>
extern pthread_t PThreadTable[];

#include "math.h"
#include "mdvar.h"
#include "parameters.h"
#include "mddata.h"
#include "split.h"
#include "global.h"

  union instance54 {struct input52{double *SUM;double S;long dir;} input52;};
  void * function55(void *ctx53);
  void *function55(void *ctx53) {
      {
          struct input52 *incontext50=&(((union instance54 *)ctx53)->input52);
          double *SUM=incontext50->SUM;
          double S=incontext50->S;
          long dir=incontext50->dir;
          {
              SUM[dir] += S;
          }
          return NULL;
      }
  }
  
  /* this routine computes kinetic energy in each of the three spatial
     dimensions, and puts the computed values in the SUM array */
void KINETI(double *SUM, double HMAS, double OMAS, long ProcID)
{
    long dir, mol;
    double S;

    /* loop over the three directions */
    for (dir = XDIR; dir <= ZDIR; dir++) {
        S=0.0;
        /* loop over the molecules */
        for (mol = StartMol[ProcID]; mol < StartMol[ProcID+1]; mol++) {
            double *tempptr = VAR[mol].F[VEL][dir];
            S += ( tempptr[H1] * tempptr[H1] +
                  tempptr[H2] * tempptr[H2] ) * HMAS
                      + (tempptr[O] * tempptr[O]) * OMAS;
        }
        { union instance54 instance54 = {
            {
                SUM,
                S,
                dir,
            },
        };
        
        liblock_execute_operation(&(gl->KinetiSumLock), (void *)(uintptr_t)(&instance54),
                                  &function55); }
    } /* for */
} /* end of subroutine KINETI */

