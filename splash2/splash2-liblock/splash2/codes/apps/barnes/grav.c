
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

/*
 * GRAV.C:
 */


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

#define global extern

#include "stdinc.h"

/*
 * HACKGRAV: evaluate grav field at a given particle.
 */

void hackgrav(bodyptr p, long ProcessId)
{
   Local[ProcessId].pskip = p;
   SETV(Local[ProcessId].pos0, Pos(p));
   Local[ProcessId].phi0 = 0.0;
   CLRV(Local[ProcessId].acc0);
   Local[ProcessId].myn2bterm = 0;
   Local[ProcessId].mynbcterm = 0;
   Local[ProcessId].skipself = FALSE;
   hackwalk(ProcessId);
   Phi(p) = Local[ProcessId].phi0;
   SETV(Acc(p), Local[ProcessId].acc0);
#ifdef QUADPOLE
   Cost(p) = Local[ProcessId].myn2bterm + NDIM * Local[ProcessId].mynbcterm;
#else
   Cost(p) = Local[ProcessId].myn2bterm + Local[ProcessId].mynbcterm;
#endif
}



/*
 * GRAVSUB: compute a single body-body or body-cell longeraction.
 */

void gravsub(register nodeptr p, long ProcessId)
{
    real drabs, phii, mor3;
    vector ai;

    if (p != Local[ProcessId].pmem) {
        SUBV(Local[ProcessId].dr, Pos(p), Local[ProcessId].pos0);
        DOTVP(Local[ProcessId].drsq, Local[ProcessId].dr, Local[ProcessId].dr);
    }

    Local[ProcessId].drsq += epssq;
    drabs = sqrt((double) Local[ProcessId].drsq);
    phii = Mass(p) / drabs;
    Local[ProcessId].phi0 -= phii;
    mor3 = phii / Local[ProcessId].drsq;
    MULVS(ai, Local[ProcessId].dr, mor3);
    ADDV(Local[ProcessId].acc0, Local[ProcessId].acc0, ai);
    if(Type(p) != BODY) {                  /* a body-cell/leaf interaction? */
       Local[ProcessId].mynbcterm++;
#ifdef QUADPOLE
       dr5inv = 1.0/(Local[ProcessId].drsq * Local[ProcessId].drsq * drabs);
       MULMV(quaddr, Quad(p), Local[ProcessId].dr);
       DOTVP(drquaddr, Local[ProcessId].dr, quaddr);
       phiquad = -0.5 * dr5inv * drquaddr;
       Local[ProcessId].phi0 += phiquad;
       phiquad = 5.0 * phiquad / Local[ProcessId].drsq;
       MULVS(ai, Local[ProcessId].dr, phiquad);
       SUBV(Local[ProcessId].acc0, Local[ProcessId].acc0, ai);
       MULVS(quaddr, quaddr, dr5inv);
       SUBV(Local[ProcessId].acc0, Local[ProcessId].acc0, quaddr);
#endif
    }
    else {                                      /* a body-body interaction  */
       Local[ProcessId].myn2bterm++;
    }
}

/*
 * HACKWALK: walk the tree opening cells too close to a given point.
 */

void hackwalk(long ProcessId)
{
    walksub(Global->G_root, Global->rsize * Global->rsize, ProcessId);
}

/*
 * WALKSUB: recursive routine to do hackwalk operation.
 */

void walksub(nodeptr n, real dsq, long ProcessId)
{
   nodeptr* nn;
   leafptr l;
   bodyptr p;
   long i;

   if (subdivp(n, dsq, ProcessId)) {
      if (Type(n) == CELL) {
	 for (nn = Subp(n); nn < Subp(n) + NSUB; nn++) {
	    if (*nn != NULL) {
	       walksub(*nn, dsq / 4.0, ProcessId);
	    }
	 }
      }
      else {
	 l = (leafptr) n;
	 for (i = 0; i < l->num_bodies; i++) {
	    p = Bodyp(l)[i];
	    if (p != Local[ProcessId].pskip) {
	       gravsub(p, ProcessId);
	    }
	    else {
	       Local[ProcessId].skipself = TRUE;
	    }
	 }
      }
   }
   else {
      gravsub(n, ProcessId);
   }
}

/*
 * SUBDIVP: decide if a node should be opened.
 * Side effects: sets  pmem,dr, and drsq.
 */

bool subdivp(register nodeptr p, real dsq, long ProcessId)
{
   SUBV(Local[ProcessId].dr, Pos(p), Local[ProcessId].pos0);
   DOTVP(Local[ProcessId].drsq, Local[ProcessId].dr, Local[ProcessId].dr);
   Local[ProcessId].pmem = p;
   return (tolsq * Local[ProcessId].drsq < dsq);
}

