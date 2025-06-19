
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
#include "mdvar.h"
#include "parameters.h"
#include "mddata.h"
#include "split.h"
#include "global.h"

union instance33 {struct input31{struct link *curr_ptr;long Z_INDEX;long Y_INDEX;long X_INDEX;} input31;};
union instance26 {struct input24{struct link *curr_ptr;struct link *last_ptr;long k;long j;long i;} input24;};
void * function34(void *ctx32);
void *function34(void *ctx32) {
    {
        struct input31 *incontext29=&(((union instance33 *)ctx32)->input31);
        struct link *temp_ptr;
        struct link *curr_ptr=incontext29->curr_ptr;
        long Z_INDEX=incontext29->Z_INDEX;
        long Y_INDEX=incontext29->Y_INDEX;
        long X_INDEX=incontext29->X_INDEX;
        {
            temp_ptr = BOX[X_INDEX][Y_INDEX][Z_INDEX].list;
            BOX[X_INDEX][Y_INDEX][Z_INDEX].list = curr_ptr;
            curr_ptr->next_mol = temp_ptr;
        }
        return NULL;
    }
}

void * function27(void *ctx25);
void *function27(void *ctx25) {
    {
        struct input24 *incontext22=&(((union instance26 *)ctx25)->input24);
        struct link *curr_ptr=incontext22->curr_ptr;
        struct link *last_ptr=incontext22->last_ptr;
        long k=incontext22->k;
        long j=incontext22->j;
        long i=incontext22->i;
        {
            if (last_ptr != NULL)
                last_ptr->next_mol = curr_ptr->next_mol;else
                BOX[i][j][k].list = curr_ptr->next_mol;
        }
        return NULL;
    }
}

void BNDRY(long ProcID)     /* this routine puts the molecules back inside the box if they are out */
{
    long i, j, k, dir;
    long X_INDEX, Y_INDEX, Z_INDEX;
    struct link *curr_ptr, *last_ptr, *next_ptr, *temp_ptr;
    struct list_of_boxes *curr_box;
    double *extra_p;

    /* for each box */
    curr_box = my_boxes[ProcID];
    while (curr_box) {
        i = curr_box->coord[XDIR];  /* X coordinate of box */
        j = curr_box->coord[YDIR];  /* Y coordinate of box */
        k = curr_box->coord[ZDIR];  /* Z coordinate of box */

        last_ptr = NULL;
        curr_ptr = BOX[i][j][k].list;

        /* Go through molecules in current box */

        while (curr_ptr) {
            next_ptr = curr_ptr->next_mol;

            /* for each direction */

            for ( dir = XDIR; dir <= ZDIR; dir++ ) {
                extra_p = curr_ptr->mol.F[DISP][dir];

                /* if the oxygen atom is out of the box */
                if (extra_p[O] > BOXL ) {

                    /* move all three atoms back in the box */
                    extra_p[H1] -= BOXL;
                    extra_p[O]  -= BOXL;
                    extra_p[H2] -= BOXL;
                }
                else if (extra_p[O] < 0.00) {
                    extra_p[H1] += BOXL;
                    extra_p[O]  += BOXL;
                    extra_p[H2] += BOXL;
                }
            } /* for dir */

            /* If O atom moves out of current box, put it in correct box */
            X_INDEX = (long) (curr_ptr->mol.F[DISP][XDIR][O] / BOX_LENGTH);
            Y_INDEX = (long) (curr_ptr->mol.F[DISP][YDIR][O] / BOX_LENGTH);
            Z_INDEX = (long) (curr_ptr->mol.F[DISP][ZDIR][O] / BOX_LENGTH);

            if ((X_INDEX != i) ||
                (Y_INDEX != j) ||
                (Z_INDEX != k)) {

                /* Remove link from BOX[i][j][k] */

                { union instance26 instance26 = {
                    {
                        curr_ptr,
                        last_ptr,
                        k,
                        j,
                        i,
                    },
                };
                
                liblock_execute_operation(&(BOX[i][j][k].boxlock),
                                          (void *)(uintptr_t)(&instance26), &function27); }

                /* Add link to BOX[X_INDEX][Y_INDEX][Z_INDEX] */

                { union instance33 instance33 = {
                    {
                        curr_ptr,
                        Z_INDEX,
                        Y_INDEX,
                        X_INDEX,
                    },
                };
                
                liblock_execute_operation(&(BOX[X_INDEX][Y_INDEX][Z_INDEX].boxlock),
                                          (void *)(uintptr_t)(&instance33), &function34); }

            }
            else last_ptr = curr_ptr;
            curr_ptr = next_ptr;   /* Go to next molecule in current box */
        } /* while curr_ptr */
        curr_box = curr_box->next_box;
    } /* for curr_box */

} /* end of subroutine BNDRY */
