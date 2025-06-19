
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

#include <math.h>
#include <limits.h>
#include "defs.h"
#include "memory.h"
#include "particle.h"
#include "box.h"
#include "partition_grid.h"

#define DIVISOR(x) ((x <= 20) ? 1 : ((x - 20) * 50))

typedef struct _Id_Info id_info;
struct _Id_Info
{
   long id;
   long num;
};

typedef struct _Cost_Info cost_info;
struct _Cost_Info
{
   long cost;
   long num;
};

long CheckBox(long my_id, box *b, long partition_level);

void
InitPartition (long my_id)
{
   long i;

   Local[my_id].Childless_Partition = NULL;
   for (i = 0; i < MAX_LEVEL; i++) {
      Local[my_id].Parent_Partition[i] = NULL;
   }
   Local[my_id].Max_Parent_Level = -1;
}


void
PartitionIterate (long my_id, partition_function function,
		  partition_start position)
{
   box *b;
   long i;

   if (position == CHILDREN) {
      b = Local[my_id].Childless_Partition;
      while (b != NULL) {
	 (*function)(my_id, b);
	 b = b->next;
      }
   }
   else {
      if (position == TOP) {
	 for (i = 0; i <= Local[my_id].Max_Parent_Level; i++) {
	    b = Local[my_id].Parent_Partition[i];
	    while (b != NULL) {
	       (*function)(my_id, b);
	       b = b->next;
	    }
	 }
	 b = Local[my_id].Childless_Partition;
	 while (b != NULL) {
	    (*function)(my_id, b);
	    b = b->next;
	 }
      }
      else {
	 b = Local[my_id].Childless_Partition;
	 while (b != NULL) {
	    (*function)(my_id, b);
	    b = b->next;
	 }
	 for (i = Local[my_id].Max_Parent_Level; i >= 0; i--) {
	    b = Local[my_id].Parent_Partition[i];
	    while (b != NULL) {
	       (*function)(my_id, b);
	       b = b->next;
	    }
	 }
      }
   }
}


void
InsertBoxInPartition (long my_id, box *b)
{
   box *level_list;

   if (b->type == CHILDLESS) {
      b->prev = NULL;
      if (Local[my_id].Childless_Partition != NULL)
	 Local[my_id].Childless_Partition->prev = b;
      b->next = Local[my_id].Childless_Partition;
      Local[my_id].Childless_Partition = b;
   }
   else {
      level_list = Local[my_id].Parent_Partition[b->level];
      b->prev = NULL;
      if (level_list != NULL)
	 level_list->prev = b;
      b->next = level_list;
      Local[my_id].Parent_Partition[b->level] = b;
      if (b->level > Local[my_id].Max_Parent_Level) {
	 Local[my_id].Max_Parent_Level = b->level;
      }
   }
}


void
RemoveBoxFromPartition (long my_id, box *b)
{
   if (b->type == CHILDLESS) {
      if (b->prev != NULL)
	 b->prev->next = b->next;
      else
	 Local[my_id].Childless_Partition = b->next;
      if (b->next != NULL)
	 b->next->prev = b->prev;
   }
   else {
      if (b->prev != NULL)
	 b->prev->next = b->next;
      else
	 Local[my_id].Parent_Partition[b->level] = b->next;
      if (b->next != NULL)
	 b->next->prev = b->prev;
      if ((b->level == Local[my_id].Max_Parent_Level) &&
	  (Local[my_id].Parent_Partition[b->level] == NULL)) {
	 while (Local[my_id].Parent_Partition[Local[my_id].Max_Parent_Level]
		== NULL)
	    Local[my_id].Max_Parent_Level -= 1;
      }
   }
}


void
ComputeCostOfBox (box *b)
{
   long different_costs;
   long i;
   long j;
   long new_cost;
   cost_info cost_list[MAX_PARTICLES_PER_BOX];
   cost_info winner;
   long winner_index;
   long cost_index[MAX_PARTICLES_PER_BOX];

   if (b->type == PARENT)
      b->cost = ((b->num_v_list * V_LIST_COST(Expansion_Terms))
		 / DIVISOR(Expansion_Terms)) + 1;
   else {
      different_costs = 0;
      for (i = 0; i < b->num_particles; i++) {
	 new_cost = b->particles[i]->cost;
	 for (j = 0; j < different_costs; j++) {
	    if (new_cost == cost_list[j].cost)
	       break;
	 }
	 if (j == different_costs) {
	    cost_list[different_costs].cost = new_cost;
	    cost_list[different_costs].num = 1;
	    different_costs += 1;
	 }
	 else
	    cost_list[j].num += 1;
      }

      winner.cost = cost_list[0].cost;
      winner.num = 1;
      winner_index = 0;
      cost_index[0] = 0;
      for (i = 1; i < different_costs; i++) {
	 if (cost_list[i].num > cost_list[winner_index].num) {
	    winner.cost = cost_list[i].cost;
	    winner.num = 1;
	    winner_index = i;
	    cost_index[0] = i;
	 }
	 else {
	    if (cost_list[i].num == cost_list[winner_index].num) {
	       cost_index[winner.num] = i;
	       winner.num += 1;
	    }
	 }
      }
      if (winner.num != 1) {
	 for (i = 1; i < winner.num; i++)
	    winner.cost += cost_list[cost_index[i]].cost;
	 winner.cost /= winner.num;
      }
      b->cost = (winner.cost * b->num_particles) / DIVISOR(Expansion_Terms);
      if (b->cost == 0)
	 b->cost = 1;
   }
}


void
CheckPartition (long my_id)
{
   long i;
   box *b;
   long NE, NoP, CB, PB;
   long Q1, Q2, Q3, Q4;
   long PC, CC;
   real xpos, ypos;

   NE = NoP = CB = PB = Q1 = Q2 = Q3 = Q4 = PC = CC = 0;
   for (i = 0; i <= Local[my_id].Max_Parent_Level; i++) {
      b = Local[my_id].Parent_Partition[i];
      while (b != NULL) {
	 NE += CheckBox(my_id, b, i);
	 PB += 1;
	 PC += b->cost;
	 b = b->next;
      }
   }
   b = Local[my_id].Childless_Partition;
   while (b != NULL) {
      NE += CheckBox(my_id, b, -1);
      for (i = 0; i < b->num_particles; i++) {
	 xpos = b->particles[i]->pos.x;
	 ypos = b->particles[i]->pos.y;
	 if (xpos > Grid->x_center) {
	    if (ypos > Grid->y_center)
	       Q1 += 1;
	    else
	       Q4 += 1;
	 }
	 else {
	    if (ypos > Grid->y_center)
	       Q2 += 1;
	    else
	       Q3 += 1;
	 }
      }
      NoP += b->num_particles;
      CB += 1;
      CC += b->cost;
      b = b->next;
   }
}


union instance180 {struct input178{box *b;long my_id;} input178;};
union instance173 {struct input171{box *b;long my_id;} input171;};
union instance166 {struct input164{box *b;long my_id;} input164;};
union instance159 {struct input157{box *b;long my_id;} input157;};
union instance152 {struct input150{box *b;long my_id;long partition_level;} input150;};
union instance145 {struct input143{box *b;long my_id;} input143;};
union instance138 {struct input136{box *b;long my_id;} input136;};
union instance131 {struct input129{box *b;long my_id;} input129;};
union instance124 {struct input122{box *b;long my_id;} input122;};
union instance117 {struct input115{box *b;long my_id;} input115;};
union instance110 {struct input108{box *b;long my_id;} input108;};
void * function181(void *ctx179);
void *function181(void *ctx179) {
   {
      struct input178 *incontext176=&(((union instance180 *)ctx179)->input178);
      box *b=incontext176->b;
      long my_id=incontext176->my_id;
      {
         if (b->type == CHILDLESS)
            printf("ERROR : Extra CHILDLESS box in partition (B%f P%ld)\n", b->id, my_id);else
            printf("ERROR : Extra PARENT box in partition (B%f P%ld)\n", b->id, my_id);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function174(void *ctx172);
void *function174(void *ctx172) {
   {
      struct input171 *incontext169=&(((union instance173 *)ctx172)->input171);
      box *b=incontext169->b;
      long my_id=incontext169->my_id;
      {
         if (b->type == CHILDLESS)
            printf("ERROR : Extra CHILDLESS box in partition (B%f P%ld)\n", b->id, my_id);else
            printf("ERROR : Extra PARENT box in partition (B%f P%ld)\n", b->id, my_id);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function167(void *ctx165);
void *function167(void *ctx165) {
   {
      struct input164 *incontext162=&(((union instance166 *)ctx165)->input164);
      box *b=incontext162->b;
      long my_id=incontext162->my_id;
      {
         printf("ERROR : PARENT box has particles (B%f P%ld)\n", b->id, my_id);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function160(void *ctx158);
void *function160(void *ctx158) {
   {
      struct input157 *incontext155=&(((union instance159 *)ctx158)->input157);
      box *b=incontext155->b;
      long my_id=incontext155->my_id;
      {
         printf("ERROR : PARENT box has no children (B%f P%ld)\n", b->id, my_id);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function153(void *ctx151);
void *function153(void *ctx151) {
   {
      struct input150 *incontext148=&(((union instance152 *)ctx151)->input150);
      box *b=incontext148->b;
      long my_id=incontext148->my_id;
      long partition_level=incontext148->partition_level;
      {
         printf("ERROR : PARENT box in wrong partition level ");
         printf("(%ld vs %ld) (B%f P%ld)\n", b->level, partition_level, b->id, my_id);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function146(void *ctx144);
void *function146(void *ctx144) {
   {
      struct input143 *incontext141=&(((union instance145 *)ctx144)->input143);
      box *b=incontext141->b;
      long my_id=incontext141->my_id;
      {
         printf("ERROR : PARENT box in childless partition (B%f P%ld %ld)\n", b->id, my_id, b->proc);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function139(void *ctx137);
void *function139(void *ctx137) {
   {
      struct input136 *incontext134=&(((union instance138 *)ctx137)->input136);
      box *b=incontext134->b;
      long my_id=incontext134->my_id;
      {
         printf("ERROR : CHILDLESS box has more particles than expected ");
         printf("(B%f P%ld)\n", b->id, my_id);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function132(void *ctx130);
void *function132(void *ctx130) {
   {
      struct input129 *incontext127=&(((union instance131 *)ctx130)->input129);
      box *b=incontext127->b;
      long my_id=incontext127->my_id;
      {
         printf("ERROR : CHILDLESS box has fewer particles than expected ");
         printf("(B%f P%ld)\n", b->id, my_id);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function125(void *ctx123);
void *function125(void *ctx123) {
   {
      struct input122 *incontext120=&(((union instance124 *)ctx123)->input122);
      box *b=incontext120->b;
      long my_id=incontext120->my_id;
      {
         printf("ERROR : CHILDLESS box has no particles (B%f P%ld)\n", b->id, my_id);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function118(void *ctx116);
void *function118(void *ctx116) {
   {
      struct input115 *incontext113=&(((union instance117 *)ctx116)->input115);
      box *b=incontext113->b;
      long my_id=incontext113->my_id;
      {
         printf("ERROR : CHILDLESS box has children (B%f P%ld)\n", b->id, my_id);
         fflush(stdout);
      }
      return NULL;
   }
}

void * function111(void *ctx109);
void *function111(void *ctx109) {
   {
      struct input108 *incontext106=&(((union instance110 *)ctx109)->input108);
      box *b=incontext106->b;
      long my_id=incontext106->my_id;
      {
         printf("ERROR : CHILDLESS box in parent partition (B%f P%ld %ld)\n", b->id, my_id, b->proc);
         fflush(stdout);
      }
      return NULL;
   }
}

long
CheckBox (long my_id, box *b, long partition_level)
{
   long num_errors;

   num_errors = 0;
   if (b->type == CHILDLESS) {
      if (partition_level != -1) {
	 { union instance110 instance110 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance110),
                                   &function111); }
	 num_errors += 1;
      }
      if (b->num_children != 0) {
	 { union instance117 instance117 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance117),
                                   &function118); }
	 num_errors += 1;
      }
      if (b->num_particles == 0) {
	 { union instance124 instance124 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance124),
                                   &function125); }
	 num_errors += 1;
      }
      if (b->particles[b->num_particles - 1] == NULL) {
	 { union instance131 instance131 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance131),
                                   &function132); }
	 num_errors += 1;
      }
      if (b->particles[b->num_particles] != NULL) {
	 { union instance138 instance138 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance138),
                                   &function139); }
	 num_errors += 1;
      }
   }
   else {
      if (partition_level == -1) {
	 { union instance145 instance145 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance145),
                                   &function146); }
	 num_errors += 1;
      }
      else {
	 if (partition_level != b->level) {
	    { union instance152 instance152 = {
	       {
   	       b,
      	    my_id,
      	    partition_level,
   	    },
	    };
	    
	    liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance152),
                                      &function153); }
	    num_errors += 1;
	 }
      }
      if (b->num_children == 0) {
	 { union instance159 instance159 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance159),
                                   &function160); }
	 num_errors += 1;
      }
      if (b->num_particles != 0) {
	 { union instance166 instance166 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance166),
                                   &function167); }
	 num_errors += 1;
      }
   }
   if (b->parent == NULL) {
      if (b != Grid) {
	 { union instance173 instance173 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance173),
                                   &function174); }
	 num_errors += 1;
      }
   }
   else {
      if (b->parent->children[b->child_num] != b) {
	 { union instance180 instance180 = {
	    {
   	    b,
      	 my_id,
   	 },
	 };
	 
	 liblock_execute_operation(&(G_Memory->io_lock), (void *)(uintptr_t)(&instance180),
                                   &function181); }
	 num_errors += 1;
      }
   }
   return( num_errors);
}


#undef DIVISOR
