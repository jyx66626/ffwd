
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


#include <stdio.h>

#include "parameters.h"
#include "mdvar.h"
#include "water.h"
#include "wwpot.h"
#include "cnst.h"
#include "mddata.h"
#include "fileio.h"
#include "split.h"
#include "global.h"

union instance19 {struct input17{double XVIR;double AVGT;double XTT;double POTRF;double POTR;double POTA;double TEN;long i;} input17;};
void * function20(void *ctx18);
void *function20(void *ctx18) {
    {
        struct input17 *incontext15=&(((union instance19 *)ctx18)->input17);
        double XVIR=incontext15->XVIR;
        double AVGT=incontext15->AVGT;
        double XTT=incontext15->XTT;
        double POTRF=incontext15->POTRF;
        double POTR=incontext15->POTR;
        double POTA=incontext15->POTA;
        double TEN=incontext15->TEN;
        long i=incontext15->i;
        {
            fprintf(six, "     %5ld %14.5lf %12.5lf %12.5lf %12.5lf \n", i, TEN, POTA, POTR, POTRF);
            fprintf(six, " %16.3lf %16.5lf %16.5lf\n", XTT, AVGT, XVIR);
            fflush(six);
        }
        return NULL;
    }
}

/************************************************************************/

double MDMAIN(long NSTEP, long NPRINT, long NSAVE, long NORD1, long ProcID)
{
    double TVIR = 0.0;
    double TTMV = 0.0;
    double TKIN = 0.0;
    double XTT;
    long i,j,k;
    double POTA,POTR,POTRF;
    double XVIR,AVGT,TEN;
    struct list_of_boxes *new_box, *curr_box;

    for (i=start_end[ProcID]->box[XDIR][FIRST]; i<=start_end[ProcID]->box[XDIR][LAST]; i++) {
        for (j=start_end[ProcID]->box[YDIR][FIRST]; j<=start_end[ProcID]->box[YDIR][LAST]; j++) {
            for (k=start_end[ProcID]->box[ZDIR][FIRST]; k<=start_end[ProcID]->box[ZDIR][LAST]; k++) {
                new_box = (box_list *) valloc(sizeof(box_list));;
                new_box->coord[XDIR] = i;
                new_box->coord[YDIR] = j;
                new_box->coord[ZDIR] = k;
                new_box->next_box = NULL;
                curr_box = my_boxes[ProcID];
                if (curr_box == NULL)
                    my_boxes[ProcID] = new_box;
                else {
                    while (curr_box->next_box != NULL)
                        curr_box = curr_box->next_box;
                    curr_box->next_box = new_box;
                } /* else */
            }
        }
    }

    /* calculate initial value for acceleration */

    INTRAF(&gl->VIR,ProcID);

    {
	pthread_barrier_wait(&(gl->start));
};

    INTERF(ACC,&gl->VIR,ProcID);

    {
	pthread_barrier_wait(&(gl->start));
};

    /* MOLECULAR DYNAMICS LOOP */

    for (i=1;i <= NSTEP; i++) {
        TTMV=TTMV+1.00;

        /* POSSIBLE ENHANCEMENT:  Here's where one start measurements to avoid
           cold-start effects.  Recommended to do this at the beginning of the
           second timestep; i.e. if (i == 2).
           */

        /* initialize various shared sums */
        if (ProcID == 0) {
            long dir;
            if (i >= 2) {
                {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(gl->trackstart) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
            }
            gl->VIR = 0.0;
            gl->POTA = 0.0;
            gl->POTR = 0.0;
            gl->POTRF = 0.0;
            for (dir = XDIR; dir <= ZDIR; dir++)
                gl->SUM[dir] = 0.0;
        }

        if ((ProcID == 0) && (i >= 2)) {
            {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(gl->intrastart) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
        }

        {
	pthread_barrier_wait(&(gl->start));
};

        PREDIC(TLC,NORD1,ProcID);
        INTRAF(&gl->VIR,ProcID);

        {
	pthread_barrier_wait(&(gl->start));
};

        if ((ProcID == 0) && (i >= 2)) {
            {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(gl->intraend) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
            gl->intratime += gl->intraend - gl->intrastart;
        }

        if ((ProcID == 0) && (i >= 2)) {
            {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(gl->interstart) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
        }

        INTERF(FORCES,&gl->VIR,ProcID);

        if ((ProcID == 0) && (i >= 2)) {
            {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(gl->interend) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
            gl->intertime += gl->interend - gl->interstart;
        }

        CORREC(PCC,NORD1,ProcID);

        BNDRY(ProcID);

        KINETI(gl->SUM,HMAS,OMAS,ProcID);

        {
	pthread_barrier_wait(&(gl->start));
};

        if ((ProcID == 0) && (i >= 2)) {
            {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(gl->intraend) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
            gl->intratime += gl->intraend - gl->interend;
        }

        TKIN=TKIN+gl->SUM[0]+gl->SUM[1]+gl->SUM[2];
        TVIR=TVIR-gl->VIR;

        /* CHECK if  PRINTING AND/OR SAVING IS TO BE DONE */

        if ( ((i % NPRINT) == 0) || ((NSAVE > 0) && ((i % NSAVE) == 0))) {

            /* if so, call poteng to compute potential energy.  Note
               that we are attributing all the time in poteng to intermolecular
               computation although some of it is intramolecular (see poteng.C) */

            if ((ProcID == 0) && (i >= 2)) {
                {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(gl->interstart) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
            }

            POTENG(&gl->POTA,&gl->POTR,&gl->POTRF,ProcID);

            {
	pthread_barrier_wait(&(gl->start));
};

            if ((ProcID == 0) && (i >= 2)) {
                {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(gl->interend) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
                gl->intertime += gl->interend - gl->interstart;
            }

            POTA=gl->POTA*FPOT;
            POTR=gl->POTR*FPOT;
            POTRF=gl->POTRF*FPOT;
            XVIR=TVIR*FPOT*0.50/TTMV;
            AVGT=TKIN*FKIN*TEMP*2.00/(3.00*TTMV);
            TEN=(gl->SUM[0]+gl->SUM[1]+gl->SUM[2])*FKIN;
            XTT=POTA+POTR+POTRF+TEN;

            /* if it is time to print output as well ... */
            if ((i % NPRINT) == 0 && ProcID == 0) {
                { union instance19 instance19 = {
                    {
                        XVIR,
                        AVGT,
                        XTT,
                        POTRF,
                        POTR,
                        POTA,
                        TEN,
                        i,
                    },
                };
                
                liblock_execute_operation(&(gl->IOLock), (void *)(uintptr_t)(&instance19),
                                          &function20); }
            }

        }

        {
	pthread_barrier_wait(&(gl->start));
};

        if ((ProcID == 0) && (i >= 2)) {
            {
	struct timeval	FullTime;

	gettimeofday(&FullTime, NULL);
	(gl->trackend) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
};
            gl->tracktime += gl->trackend - gl->trackstart;
        }

    } /* for i */

    return((XTT));

} /* mdmain.c */
