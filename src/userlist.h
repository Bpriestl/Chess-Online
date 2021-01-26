#ifndef USERLIST
#define USERLIST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXNAMELENGTH 30 //used as a limit for how large a username or password can be

typedef struct ULIST ULIST;
typedef struct USERENTRY USERENTRY;

struct ULIST{
	unsigned int length;
	USERENTRY *first;
	USERENTRY *last;	
};

struct USERENTRY {
	ULIST *ulist;
	ULIST *flist;
	USERENTRY *next; //linkedlist definers
	USERENTRY *prev;
	char name[MAXNAMELENGTH];
	unsigned int password; //stores password in hased form
	char rawpass[MAXNAMELENGTH];
};


ULIST *createuserlist(void);
	//initalize and return a malloc'd and empty userlist

int createaccount(ULIST *list, char newname[MAXNAMELENGTH], char newpassword[MAXNAMELENGTH]);	
	//attempts to create and append a new user to the userlist, if the name matches a name already in the user list it should 
	//return 0 for failure and not create the account, otherwise it will malloc and add the user correctly
	//at the location which keeps the list sorted by alphabetical username order

USERENTRY *createuserentry(char newname[MAXNAMELENGTH], char newpassword[MAXNAMELENGTH]);
	//creates and mallocs a user entry, returning the pointer
	
void deleteulist(ULIST *list);
	//removes and deallocates a userlist
	
	
void deleteuser(USERENTRY *entry);
	//removes a userentry from the userlist it comes from and deletes and deallocates it
	
int loginattempt(ULIST *list, char username[MAXNAMELENGTH], char pass[MAXNAMELENGTH]);
	//checks a login attempt with username and password, returning NULL if its unsuccessful or the USERENTRY if the login is successful
	

unsigned int encryptpass(char rawpassword[MAXNAMELENGTH]);
	//encrypts and changes the password of the given raw password
	
int checkpassword(USERENTRY *user, char rawpassword[MAXNAMELENGTH]);
	//checks given raw password vs the encrypted one stored in the entry returning 0 for not the same and 1 for the same

void LogofUsers(ULIST *list);
	//stores the registration information in a file so when the server starts up again it can retain the information

void ReadLog(ULIST *list);
	//reads the text file containing the usernames and passwords and stores it back into the listp

int addfriend(ULIST *list, char sendinguser[MAXNAMELENGTH], char newfriend[MAXNAMELENGTH]);
	//adds new friend for both users if request is accepted 
	
void dependfriend(USERENTRY *user1, USERENTRY *user2);
	//deletes friend for both users if one deletes the other

void appendfriend(ULIST *list, USERENTRY *newfriend);
	//add friend to friends list 
#endif
