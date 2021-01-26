
#include "messagestructs.h"
#include <strings.h>
#include <assert.h>

MESSAGELIST *createmessagelist(void){
	//mallocs and returns pointer to a created empty message list 

	MESSAGELIST *list = (MESSAGELIST *)malloc(sizeof(MESSAGELIST));
	if (list == NULL)
	{
		perror("Error in allocating Memory Space!\n");
		return NULL;
	}
	list->first = NULL;
	list->last = NULL;
	list->length = 0;
	return list;
}
	
void appendmessage(MESSAGELIST *list, MENTRY *entry){
	//appends a entry to a list and increments length of list
	assert(list);
	assert(entry);
	if(list->length == 0 ){ //first entry in the list
		list->first = entry;
		list->last = entry;
		entry->prev = NULL;
		entry->next = NULL;
	} else {
		list->last->next = entry;
		entry->prev = list->last;
		list->last = entry;
		entry->next = NULL;
	}
	entry->mlist = list;
	list->length++;
}

void prependmessage(MESSAGELIST *list, MENTRY *entry){
	assert(list);
	assert(entry);
	if(list->length == 0){
		list->first = entry;
		list->last = entry;
		entry->prev = NULL;
		entry->next = NULL;
	} else {
		list->first->prev = entry;
		entry->next = list->first;
		list->first = entry;
		entry->prev = NULL;
	}
	entry->mlist = list;
	list->length++;
}

	
MENTRY *createmessage(char messagetype[2],char sendinguser[MAXNAMELENGTH], char targetuser[MAXNAMELENGTH], char newcontent[BUFFSIZE]){
	//creates and mallocs a message entry, returning the pointer
	MENTRY *entry = (MENTRY *)malloc(sizeof(MENTRY));
	if (entry == NULL)
	{
		perror("Error in allocating Memory Space!\n");
		return NULL;
	}
	strcpy(entry->opcode, messagetype);
	strcpy(entry->user, sendinguser);
	strcpy(entry->destuser, targetuser);
	strcpy(entry->content, newcontent);
	

	return entry;
}

void createandappendmessage(MESSAGELIST *list, char messagetype[2],char sendinguser[MAXNAMELENGTH], char targetuser[MAXNAMELENGTH], char newcontent[BUFFSIZE]){
	//creates and mallocs a message entry then appends to the list given
	MENTRY *entry = createmessage(messagetype,sendinguser, targetuser, newcontent);
	appendmessage(list, entry);
}

void createandprependmessage(MESSAGELIST *list, char messagetype[2],char sendinguser[MAXNAMELENGTH], char targetuser[MAXNAMELENGTH], char newcontent[BUFFSIZE]){
	//creates and mallocs a message entry then pends to the list given
	MENTRY *entry = createmessage(messagetype,sendinguser, targetuser, newcontent);
	prependmessage(list, entry);
}
	
void deleteMESSAGELIST(MESSAGELIST *list){
	//deletes and deallocates all the entries in a list and the list itself
	assert(list);
	MENTRY *entry, *temp;
	entry = list->first;

	while (entry)
	{
		temp = entry->next;
		deletemessageentry(entry);
		entry = temp;
	}
	temp = NULL;
	entry = NULL;
	list->first = NULL;
	list->last = NULL;
	free(list);
}
	
void deletemessageentry(MENTRY *entry){
	//deletes and deallocates a message entry from a list and decrements the length of the list by 1; 
	//also properly changing the surrounding linked list structure to accomodate
	assert(entry);
	assert(entry->mlist);

	if (entry->next == NULL)
	{
		entry->mlist->last = entry->prev;
	}
	else
	{
		entry->next->prev = entry->prev;
	}
	if (entry->prev == NULL)
	{
		entry->mlist->first = entry->next;
	}
	else
	{
		entry->prev->next = entry->next;
	}

	entry->mlist->length -= 1;

	entry->next = NULL;
	entry->prev = NULL;
	free(entry);
}

