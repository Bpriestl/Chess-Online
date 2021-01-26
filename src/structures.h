/****************************************************************/
/* EECS 22L Winter 2019 Chess Program Alpha Version		*/
/* Structures.h: [...]						*/
/* Author: [Original author]					*/
/*								*/
/* Modifications:						*/
/* 1/21/19	SL	Initial version				*/
/* 2/1/19	AL	Added value member to MOVE struct	*/
/****************************************************************/

#ifndef STRUCTURES_H
#define STRUCTURES_H

//definitions for individual piece numbering scheme for use by the structures
#define EMPTY 0
#define WHITEPAWN 1
#define WHITEROOK 2
#define WHITEBISHOP 3
#define WHITEKNIGHT 4
#define WHITEQUEEN 5
#define WHITEKING 6
#define BLACKPAWN 8
#define BLACKROOK 9
#define BLACKBISHOP 10
#define BLACKKNIGHT 11
#define BLACKQUEEN 12
#define BLACKKING 13

//definitions for team number for use by control loops
#define WHITETEAM 7
#define BLACKTEAM 14

//size of board in spaces, can be changed to a variable if we want to change the size of the board
#define BOARDXSPACES 8
#define BOARDYSPACES 8

//special definitions for specific moves
#define KINGSIDECASTLE 15
#define QUEENSIDECASTLE 16
#define LEFTPASSANT 17
#define RIGHTPASSANT 18
#define CHARGE 19
#define EITHERCASTLE 20

//definitions for gametype 
#define HUMANVHUMAN 0 
#define HUMANVAI 1 //player white AI black
#define AIVHUMAN 2 //player black AI white

/*definition for max string length, used in movetoString() */
#define SLEN 10

typedef struct MLIST MLIST;
typedef struct MOVE MOVE;

struct MLIST{
	unsigned int length;
	MOVE *first;
	MOVE *last;	
};

struct MOVE {
	MLIST *mlist;
	int initialpiece;  //the piece doing the moving
	int initialxpos; //positon of the piece doing the moving
	int initialypos; //don't change initialxpos because other functions depend on this
	int destpiece; //whatever piece WAS at the location of the move (this is for undos otherwise unessesary)
	int destxpos; //position the piece is moving to
	int destypos;
	int special; //special movement type definter such as promoting, enpassant, castling
	int value; // How good the computer judges a move to be
	MOVE *next; //linkedlist definers
	MOVE *prev;
};

MLIST *createmovelist(void);
	//initalize and return a malloc'd and empty movelist

MOVE *createmove(int initalp, int initalx, int initaly, int destp, int destx, int desty, int spec);
	//creates and returns a malloc'd move according to the definitions
	//This is a dangerous function and should ideally only be called in specific circumstances such as createandappend()
	//moves should alsost always be placed into a proper movelist to avoid memory leaks	

void appendmove(MOVE *move, MLIST *list);
	//appends a given move to a given mlist to the end
	//should ideally only be called by createandappendmove();


MOVE *createandappendmove(int initalp, int initalx, int initaly, int destp, int destx, int desty, int spec, MLIST *list);
	//mallocs and appends a movelist to the end of a movelist by calling createmove and appendmove

void deletemovefromlist(MOVE *move, MLIST *list);
	//deallocates and deletes a move from a list
	//also properly changing the surrounding linked list structure to accomodate


void deletemlist(MLIST *list);
	//deallocates a mlist and all of its moves inside


void deletemove(MOVE *move);
	//deallocates and deletes a move

MOVE *clonemove(MOVE *move, MLIST *newlist);
	//allocates a clone of a move used for creating the previous movelist since the active movelist is deleted each time
	
void clonemovelist(MLIST *oldlist, MLIST *clonelist);
	//allocates new moves into a different movelist

#endif
