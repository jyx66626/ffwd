
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

/* this version guarentees that all blocks are zeroed, and it expects
   that all returned blocks are zero */


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


#include "matrix.h"

#define ALIGN 8
#define MAXFAST 16
#define MAXFASTBL (1<<(MAXFAST-1))

#define SIZE(block) (block[-1]) /* in all blocks */
#define HOME(block) (block[-2]) /* in all blocks */
#define NEXTFREE(block) (*((long **) block)) /* in free blocks */

struct MemPool {
	liblock_lock_t memoryLock;
	long *volatile*freeBlock;
	long tally, touched, maxm;
	} *mem_pool;

long mallocP = 1, machineP = 1;

extern struct GlobalMemory *Global;


void MallocInit(long P)
{
  long p;
  
  mallocP = P;

  mem_pool = (struct MemPool *)
    valloc((mallocP+1)*sizeof(struct MemPool));;
  mem_pool++;
  /****** access to mem_pool[-1] is valid ******/

  for (p = -1; p<mallocP; p++)
    InitOneFreeList(p);
}


void InitOneFreeList(long p)
{
  long j;

  liblock_lock_init(TYPE_NOINFO, ARG_NOINFO, &(mem_pool[p].memoryLock), NULL);;
  if (p > 0) {
    mem_pool[p].freeBlock = (long **)
      valloc((MAXFAST+1)*sizeof(long *));;
    MigrateMem(mem_pool[p].freeBlock, (MAXFAST+1)*sizeof(long *), p);
  }
  else {
    mem_pool[p].freeBlock = (long **)
      valloc((MAXFAST+1)*sizeof(long *));;
    MigrateMem(mem_pool[p].freeBlock, (MAXFAST+1)*sizeof(long *),
	       DISTRIBUTED);
  }
  for (j=0; j<=MAXFAST; j++)
    mem_pool[p].freeBlock[j] = NULL;
  mem_pool[p].tally = mem_pool[p].maxm = mem_pool[p].touched = 0;
}


void MallocStats()
{
  long i;

  printf("Malloc max: ");
  for (i=0; i<mallocP; i++)
    if (mem_pool[i].touched > 0)
      printf("%ld* ", mem_pool[i].maxm);
    else
      printf("%ld ", mem_pool[i].maxm);
  printf("\n");

}


/* returns first bucket where 2^bucket >= size */

long FindBucket(long size)
{
  long bucket;

  if (size > MAXFASTBL)
    bucket = MAXFAST;
  else {
    bucket = 1;
    while (1<<bucket < size)
      bucket++;
  }

  return((bucket));
}


union instance19 {struct input17{long home;long block_size;} input17;};
union instance12 {struct input10{long *result;long alloc_size;long home;} input10;};
union instance5 {struct input3{long bucket;long home;} input3;};
void * function20(void *ctx18);
void *function20(void *ctx18) {
  {
    struct input17 *incontext15=&(((union instance19 *)ctx18)->input17);
    long *freespace;
    long home=incontext15->home;
    long block_size=incontext15->block_size;
    {
      freespace = (long *)valloc(block_size + 2 * sizeof(long));
      ;
      MigrateMem(freespace, block_size + 2 * sizeof(long), home);
      mem_pool[home].touched++;
    }
    return (void *)(uintptr_t)freespace;
  }
}

void * function13(void *ctx11);
void *function13(void *ctx11) {
  {
    struct input10 *incontext8=&(((union instance12 *)ctx11)->input10);
    long *d;
    long *prev;
    long leftover;
    long block_size;
    long *result=incontext8->result;
    long alloc_size=incontext8->alloc_size;
    long home=incontext8->home;
    result = result;
    {
      prev = NULL;
      d = mem_pool[home].freeBlock[MAXFAST];
      while (d) {
        block_size = SIZE(d);
        if (block_size >= alloc_size) {
          leftover = block_size - alloc_size - 2 * sizeof(long);
          result = d + (leftover / sizeof(long)) + 2;
          SIZE(result) = alloc_size;
          HOME(result) = home;
          if (leftover > MAXFASTBL) {
            SIZE(d) = leftover;
          }else {
            if (prev)
              NEXTFREE(prev) = NEXTFREE(d);else
              mem_pool[home].freeBlock[MAXFAST] = NEXTFREE(d);
            if (leftover > 0) {
              SIZE(d) = leftover;
              MyFreeNow(d);
            }
          }
          break;
        }
        prev = d;
        d = NEXTFREE(d);
      }
    }
    return (void *)(uintptr_t)result;
  }
}

void * function6(void *ctx4);
void *function6(void *ctx4) {
  {
    struct input3 *incontext1=&(((union instance5 *)ctx4)->input3);
    long *result;
    long bucket=incontext1->bucket;
    long home=incontext1->home;
    {
      result = mem_pool[home].freeBlock[bucket];
      if (result)
        mem_pool[home].freeBlock[bucket] = NEXTFREE(result);
    }
    return (void *)(uintptr_t)result;
  }
}

/* size is in bytes */

char *MyMalloc(long size, long home)
{
  long i, bucket, leftover, alloc_size, block_size;
  long *d, *result, *prev, *freespace;

  if (size < ALIGN)
    size = ALIGN;

  if (home == DISTRIBUTED)
    home = -1;

  bucket = FindBucket(size);

  if (bucket < MAXFAST)
    alloc_size = 1<<bucket;
  else
    alloc_size = ((size+ALIGN-1)/ALIGN)*ALIGN;

  result = NULL;

  if (bucket < MAXFAST) {
    if (mem_pool[home].freeBlock[bucket]) {
      { union instance5 instance5 = {
        {
          bucket,
          home,
        },
      };
      
      result =(long *)(uintptr_t)(liblock_execute_operation(&(mem_pool[home].memoryLock), (void *)(uintptr_t)(&instance5),
                                &function6));
      }
    }
  }

  if (!result) {
    { union instance12 instance12 = {
      {
        result,
        alloc_size,
        home,
      },
    };
    
    result =(long *)(uintptr_t)(liblock_execute_operation(&(mem_pool[home].memoryLock), (void *)(uintptr_t)(&instance12),
                              &function13));
    }

  }

  if (result) {
    NEXTFREE(result) = 0;  /* in case it was set earlier */
  }
  else {
    /* grab a big block, free it, then retry request */
    block_size = max(alloc_size, 4*(1<<MAXFAST));
    { union instance19 instance19 = {
      {
        home,
        block_size,
      },
    };
    
    freespace =(long *)(uintptr_t)(liblock_execute_operation(&(Global->memLock), (void *)(uintptr_t)(&instance19), &function20));
    }
    freespace+=2;
    SIZE(freespace) = block_size;
    HOME(freespace) = home;
    for (i=0; i<block_size/sizeof(long); i++)
      freespace[i] = 0;
    if (block_size == alloc_size)
      result = freespace;
    else {
      mem_pool[home].tally += SIZE(freespace);
      if (mem_pool[home].tally > mem_pool[home].maxm) {
	mem_pool[home].maxm = mem_pool[home].tally;
      }
      MyFree(freespace);
      result = (long *) MyMalloc(alloc_size, home);
    }
  }

  mem_pool[home].tally += SIZE(result);
  if (mem_pool[home].tally > mem_pool[home].maxm) {
    mem_pool[home].maxm = mem_pool[home].tally;
  }

  if (SIZE(result) < size)
    printf("*** Bad size from malloc %ld, %ld\n", size, SIZE(result));

  return(((char *) result));

}


void MigrateMem(long *start, long length, long home)
{
/*  unsigned long *finish;
  unsigned long currpage, endpage;
  long i;
  long j;*/

/* POSSIBLE ENHANCEMENT:  Here is where one might distribute the memory
   pages across physically distributed memories as desired.

   One way to do this is as follows:

   finish = (unsigned long *) (((char *) start) + length);

   currpage = (unsigned long) start;
   endpage = (unsigned long) finish;
   if ((home == DISTRIBUTED) || (home < 0) || (home >= mallocP)) {
     j = 0;
     while (currpage < endpage) {

       Place all addresses x such that
         (currpage <= x < currpage + PAGE_SIZE) on node j

       j++;
       currpage += PAGE_SIZE;
     }
   } else {
     Place all addresses x such that
       (currpage <= x < endpage) on node home
   }  
*/
}
    

void * function27(void *ctx25);
void *function27(void *ctx25) {
  {
    long *block=(long *)(uintptr_t)ctx25;
    {
      MyFreeNow(block);
    }
    return NULL;
  }
}

void MyFree(long *block)
{
  long home;

  home = HOME(block);
  {
  
  liblock_execute_operation(&(mem_pool[home].memoryLock), (void *)(uintptr_t)(block),
                            &function27); }
}


void MyFreeNow(long *block)
{
  long bucket, size, home;

  size = SIZE(block);
  home = HOME(block);

  if (size <= 0) {
    printf("Bad size %ld\n", size);
    exit(-1);
  }
  if (home < -1 || home >= mallocP) {
    printf("Bad home %ld\n", home);
    exit(-1);
  }

  if (size > MAXFASTBL)
    bucket = MAXFAST;
  else {
    bucket = FindBucket(size);
    if (size < 1<<bucket)
      bucket--;
  }

  /* throw away tiny blocks */
  if (bucket == 0) {
    
      return;
    }

  NEXTFREE(block) = (long *) mem_pool[home].freeBlock[bucket];
  mem_pool[home].freeBlock[bucket] = block;
  mem_pool[home].tally -= size;

}

