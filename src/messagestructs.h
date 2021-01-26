#ifndef MESSAGESTRUCTS
#define MESSAGESTRUCTS

#include "userlist.h"
#include "structures.h"


//In effect these set each of our server client interaction protocols
//to be the first letter of the sent message for easier decoding
#define FAIL "a" //server response for request failed
#define SUCCESS "b" //server response for request successful
#define LOGIN "c" //login attempt
#define REGISTER "d" //registration attempt
#define MESSAGE "e" //user to user message
#define PING "f" //user pings server for the next item in the waiting queue for that user
#define GETUSERLIST "g" //prepends all the usernames to the message queue with the requesting user as the target
#define GETFRIENDLIST "h" //prepends all the usernames to the message queue with the requesting user as the target
#define REQUESTCHESS "i" //appends a move request to the message queue with dest user as the target
#define CHESSMOVE "j"
#define SENDFRIENDREQUEST "k"
#define ACCEPTFRIENDREQUEST "l"
#define BUFFSIZE 256
#define DELIMITER "-"
#define LAST "0" //indicates this message being recieved is the last one
#define MORE "1" //indicates more messages are stored in the server buffer and need to be retrieved

typedef struct MESSAGELIST MESSAGELIST;
typedef struct MENTRY MENTRY;

//message for use by outside components
typedef struct message{
	unsigned int length;
	char opcode[2]; //FAIL/SUCCESS/LOGIN etc
	char user[MAXNAMELENGTH]; //from the perspective of the main.c this is the person who wishes to send the message, or who it was sent from
	char destuser[MAXNAMELENGTH]; //from the perspective of the main.c this is the person who the message is destined for, 
									//if it is an incoming message struct, this is the current client for example, but for a sending message its the target
	char remainingmessages[2]; //LAST if the currently recieved message is the last one, MORE if there are more messages waiting in the server buffer
							//%warning not guarenteed to correspond with the exact remaining number%
	char content[BUFFSIZE]; //main content of the message sent or recieved, either password or some messsage, or a chess move etc 
	MOVE *chessmove;
} Message_struct;

/****** quick rundown of what will be defined in the struct message for the completed protocols ****/
//the following are for incoming messages:

//if OPCODE is FAIL, OR SUCCESS, none of the rest of the message struct will have anything meaningful in in
//if OPCODE is MESSAGE, "user" will have the person who sent the message, 
		//remaining messages will have a value (LAST for no more messages or MORE for more messages in queue), and content will have a string

//if OPCODE is GETUSERLIST or GETFRIENDLIST "user" will be another username entry for one of the people in the list
		//remaining messages will have a value (LAST or MORE)
		
//if OPCODE is REQUESTCHESS *** not sure yet it might have either a port num for the port offered to peer to peer or just a username


/****the following are what is nessesary for an outgoing message of each typedef  *****/
//to send a LOGIN, user must be the username, opcode must be LOGIN, and content is for the password, will return either a SUCCESS or FAIL opcode in the message struct
//to send a REGISTER user must be the username, opcode must be REGISTER, and content is the password, will return either a SUCCESS or FAIL opcode in the message struct
//to send a MESSAGE user must be the username, destuser must be the person the message is meant for, opcode must be MESSAGE, and content is the message body,
//to send a GETUSERLIST or GETFRIENDLIST, opcode must be one of them, and user must be the current user requesting the list
//to send a REQUESTCHESS, opcode must be REQUESTCHESS, user must be current user, destuser must be the target user of the request.
//to send a PING, opcode must be PING and user must be current user
		//will return the next thing in the queue for that user recipient
//to send a SENDFRIENDREQUEST opcode must be SENDFRIENDREQUEST user must be current user, destuser must be target, content optional
//to send an ACCEPTFRIENDREQUEST opcode must be ACCEPTFRIENDREQUEST user must be current user, destuser must be target
		
		
//message linked list to be used by server
struct MESSAGELIST{
	unsigned int length;
	MENTRY *first;
	MENTRY *last;
};

struct MENTRY{
	MESSAGELIST *mlist;
	MENTRY *next; //linkedlist definers
	MENTRY *prev;
	
	char opcode[2]; //FAIL/SUCCESS/LOGIN etc
	char user[MAXNAMELENGTH]; //from the perspective of the main.c this is the person who wishes to send the message, or who it was sent from
	char destuser[MAXNAMELENGTH]; //from the perspective of the main.c this is the person who the message is destined for, 
									//if it is an incoming message struct, this is the current client for example, but for a sending message its the target
	char content[BUFFSIZE]; //main content of the message sent or recieved, either password or some messsage, or a chess move etc 
};

MESSAGELIST *createmessagelist(void);
	//mallocs and returns pointer to a created message list 

void appendmessage(MESSAGELIST *list, MENTRY *entry);
	//appends a entry to a list and increments length of list
	
MENTRY *createmessage(char messagetype[2],char sendinguser[MAXNAMELENGTH], char targetuser[MAXNAMELENGTH], char newcontent[BUFFSIZE]);
	//creates and mallocs a message entry, returning the pointer

void createandappendmessage(MESSAGELIST *list, char messagetype[2],char sendinguser[MAXNAMELENGTH], char targetuser[MAXNAMELENGTH], char newcontent[BUFFSIZE]);
	//creates and mallocs a message entry then appends to the list given

void createandprependmessage(MESSAGELIST *list, char messagetype[2],char sendinguser[MAXNAMELENGTH], char targetuser[MAXNAMELENGTH], char newcontent[BUFFSIZE]);
	//creates and mallocs a message entry then prepends to the list given
	
	
void deletemessagelist(MESSAGELIST *list);
	//deletes and deallocates all the entries in a list and the list itself
	
void deletemessageentry(MENTRY *entry);
	//deletes and deallocates a message entry from a list and decrements the length of the list by 1; 



#endif
 
