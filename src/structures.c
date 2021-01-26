#include "structures.h"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

MLIST *createmovelist(void)
{
	//initalize and return a malloc'd and empty movelist
	MLIST *list = (MLIST *)malloc(sizeof(MLIST));
	if (list == NULL)
	{
		perror("Error in allocating Memory Space!\n");
		return NULL;
	}
	list->first = NULL;
	list->last = NULL;
	list->length = 0;
	return list;
} /*end of createmovelist*/

MOVE *createmove(int initialp, int initialx, int initialy, int destp, int destx, int desty, int spec)
{
	//creates and returns a malloc'd move according to the definitions
	//This is a dangerous function and should ideally only be called in specific circumstances such as createandappend()
	//moves should almost always be placed into a proper movelist to avoid memory leaks
	MOVE *move = (MOVE *)malloc(sizeof(MOVE));
	if (move == NULL)
	{
		perror("Error in allocating Memory Space!\n");
		return NULL;
	}
	move->mlist = NULL;
	move->next = NULL;
	move->prev = NULL;
	move->initialpiece = initialp;
	move->initialxpos = initialx;
	move->initialypos = initialy;
	move->destpiece = destp;
	move->destxpos = destx;
	move->destypos = desty;
	move->special = spec;

	return move;
} /*end of createmove*/

void appendmove(MOVE *move, MLIST *list)
{
	//appends a given move to a given mlist to the end
	//should ideally only be called by createandappendmove();
	assert(list);
	assert(move);
	if (list->last)
	{
		move->mlist = list;
		move->next = NULL;
		move->prev = list->last;
		list->last->next = move;
		list->last = move;
	}
	else //appending to empty list
	{
		move->mlist = list;
		move->next = NULL;
		move->prev = NULL;
		list->first = move;
		list->last = move;
	}
	list->length++;
} /*end of appendmove*/

MOVE *createandappendmove(int initialp, int initialx, int initialy, int destp, int destx, int desty, int spec, MLIST *list)
{
	//mallocs and appends a move to the end of a movelist by calling createmove and appendmove
	assert(list);
	MOVE *move = NULL;

	move = createmove(initialp, initialx, initialy, destp, destx, desty, spec);
	appendmove(move, list);

	return move;
} /*end of createandappendmove*/

void deletemlist(MLIST *list)
{
	//deallocates a mlist and all of its moves inside
	assert(list);
	MOVE *move, *temp;
	move = list->first;

	while (move)
	{
		temp = move->next;
		deletemove(move);
		move = temp;
	}
	temp = NULL;
	move = NULL;
	list->first = NULL;
	list->last = NULL;
	free(list);
} /*end of deletemlist*/

void deletemove(MOVE *move)
{
	//deallocates and deletes a move
	assert(move);

	move->next = NULL;
	move->prev = NULL;
	free(move);
} /*end of deletemove*/

void deletemovefromlist(MOVE *move, MLIST *list)
{
	//deallocates and deletes a move from a list
	//also properly changing the surrounding linked list structure to accomodate
	if (!move)
	{
		printf("Quiting...");
		exit(0);
	}
	assert(list);

	if (move->next == NULL)
	{
		list->last = move->prev;
	}
	else
	{
		move->next->prev = move->prev;
	}
	if (move->prev == NULL)
	{
		list->first = move->next;
	}
	else
	{
		move->prev->next = move->next;
	}

	deletemove(move);
	list->length -= 1;
}

MOVE *clonemove(MOVE *move, MLIST *newlist)
{
	//allocates a clone of a move and appends to new list used for creating the previous movelist since the active movelist is deleted each time
	MOVE *newmove = createandappendmove(move->initialpiece, move->initialxpos, move->initialypos, move->destpiece, move->destxpos, move->destypos, move->special, newlist);
	newmove->value = move->value;
	return newmove;
}

void clonemovelist(MLIST *oldlist, MLIST *clonelist)
{
	//allocates new moves into a different movelist
	int counter = 0;
	MOVE *move = oldlist->first;
	while (counter < oldlist->length)
	{
		clonemove(move, clonelist);
		move = move->next;
		counter++;
	}
}

/*EOF*/
