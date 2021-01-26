#include "userlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

ULIST *createuserlist(void){
	//initalize and return a malloc'd and empty userlist
	ULIST *list = (ULIST *)malloc(sizeof(ULIST));
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


int createaccount(ULIST *list, char newname[MAXNAMELENGTH], char newpassword[MAXNAMELENGTH]){
	//attempts to create and append a new user to the userlist, if the name matches a name already in the user list it should 
	//return 0 for failure and not create the account, otherwise it will malloc and add the user correctly
	//at the location which keeps the list sorted by alphabetical username order
	
	int count = 0;
	USERENTRY *user = list->first;
	while(count < list->length){
		int comparison = strcmp(newname, user->name);
		if(comparison == 0){
			return 0; //failure, username already taken
		}
		
		if(comparison < 0) {
			USERENTRY *new = createuserentry(newname, newpassword);
			new->next = user;
			new->prev = user->prev;
			new->ulist = list;
			if(user->prev != NULL){
				user->prev->next = new;
			} else {
				list->first = new; //appending to the front
			}
			user->prev = new;
			list->length++;
			return 1; //success, new account created
		}
		user = user->next;
		count++;
	}
	user = list->last;
	USERENTRY *new = createuserentry(newname, newpassword);
	if(list->length == 0){ //empty list
		new->ulist = list;
		list->first = new;
		list->last = new;
		list->length++;
	} else { //append to end; last alphabetically
		user->next = new;
		new->prev = user;
		list->last = new;
		list->length++;
	}
	
	return 1; //success, new name is either the last alphabetically in the list or the first in an empty list
}

USERENTRY *createuserentry(char newname[MAXNAMELENGTH], char newpassword[MAXNAMELENGTH]){
	//creates and mallocs a user entry, returning the pointer
	USERENTRY *newuser = (USERENTRY *)malloc(sizeof(USERENTRY));
	newuser->ulist = NULL;
	newuser->next = NULL;
	newuser->prev = NULL;
	strcpy( newuser->name, newname );
	strcpy( newuser->rawpass, newpassword); 
	newuser->password = encryptpass(newpassword); //encrypts the password
	
	return newuser;
}

void deleteulist(ULIST *list){
	//removes and deallocates a userlist
	USERENTRY *entry, *n; //declaring pointer variables
	assert(list);
	entry = list->first;
	while(entry){
		n = entry->next;
		deleteuser(entry);
		entry = n;
	}
	n = NULL;
	entry = NULL;
	free(list);
}/*end of DeleteImageList*/
	
	
void deleteuser(USERENTRY *entry){
	//removes a userentry from the userlist it comes from and deletes and deallocates it
	assert(entry);
	if (entry->flist != NULL) {
		deleteulist(entry->flist); 
		// checks if user has friendslist and frees it
	}
	if(entry->name){
		assert(entry->name);
		free(entry->name);
		free(entry);
	}
	free(entry);
}

	
int loginattempt(ULIST *list, char username[MAXNAMELENGTH], char pass[MAXNAMELENGTH]){
	//checks a login attempt with username and password, returning 0 if its unsuccessful (name not found OR pass incorrect or return 1 on a success

	USERENTRY *curr = list->first;
	int count = 0;
	while (count < list->length){
		int comparison = strcmp(username, curr->name);
		if(comparison == 0){
			if(checkpassword(curr, pass) == 1){
				return 1; //passwords match
			}else{
				return 0; //passwords don't match
			}
		}
		if (comparison < 0 ) {
			return 0; //current username has alphabetically passed where it could be in the list
		}
		count++;
		curr = curr->next;
	}
	return 0; //list was empty and cannot be matched
}


unsigned int encryptpass(char rawpassword[MAXNAMELENGTH]){
	//encrypts and changes the password of the given raw password
	unsigned int hash = 5381;
	int i, c;
	for(i=0; i<MAXNAMELENGTH; i++){
		if(rawpassword[i]=='\0'){
			break;
		}
		c = (int)rawpassword[i];
		hash = ((hash << 5) +hash) + c;	
	}
	
	return hash;
}
	
	
int checkpassword(USERENTRY *user, char rawpassword[MAXNAMELENGTH]){
	//checks given raw password vs the encrypted one stored in the entry returning 0 for not the same and 1 for the same
	unsigned int input_pw;
//encrypt the rawpass

	input_pw = encryptpass(rawpassword);
	if(input_pw == user->password){
		return 1;	//success passwords match	
	}else{
		return 0;	//passwords don't match
	}	 

}/*end of checkpassword*/

void LogofUsers(ULIST *list){
	//updates and stores the most recent changes in the user and pass to the text file
	//intended to be called before the server times out
	USERENTRY *entry,*n;
	FILE *file;
	int c;
	file = (fopen("bin/LogUsers.txt", "w"));
	
	if(file == NULL){
		file = fopen("bin/LogUsers.txt", "w");
		if(file == NULL){
			printf("Could not construct a LogUser\n");
		}
	}
	c = fgetc(file);
	entry = list->first;
	while(entry!=NULL){
		n = entry->next;	
		if(file){
			
			fprintf(file,"user:%s\n",entry->name);
			fprintf(file,"pass%s\n",entry->rawpass);
		}	
		entry = n;
	}
	fclose(file);
}/*end of LogofUsers*/

void ReadLog(ULIST *list){
	FILE *file;
	char buff[MAXNAMELENGTH],user[MAXNAMELENGTH], password[MAXNAMELENGTH];
	char opcode[5],c;
	file = fopen("bin/LogUsers.txt", "r");
	
	if(!file){
		printf("\nNo file to be read.\n");
	}
	if(file){
		while((c = fgetc(file)) != EOF){
			//strcat(buff,c);
			for(int i = 0; i<2; i++){
				fgets(buff, MAXNAMELENGTH, file);
				buff[strlen(buff)-1] = '\0';
			//	printf("\nbuff=%s\n", buff);
				memcpy( opcode, &buff[0], 4);
			//	printf("opcode=%s\n", opcode);
				
				if(strncmp(opcode,"ser:",4) == 0){
					memcpy(user, &buff[4], MAXNAMELENGTH);
				//	printf("printing length of user: %zu, \n",strlen(user));
				//	user[strlen(user)-1] = '\0';
				/*	for(int i=0; i<strlen(buff)-3; i++){
						user[i]=buff[4+i];
					}
				printf("user:%s\n", user);
				*/
				}
				
				if(strncmp(opcode,"pass",4) == 0){
					memcpy( password , &buff[4], MAXNAMELENGTH);	
				//	password[strlen(password)-1] = '\0';
				/*	for(int i=0; i<strlen(buff)-3; i++){
                                                password[i]=buff[4+i];
                                        }
				printf("password:%s(end)\n", password);
				*/
				}
			}
			createaccount(list, user, password);
		}
		fclose(file);
	}	
	
}/*end of ReadLog*/

int addfriend(ULIST *list, char sendinguser[MAXNAMELENGTH], char newfriend[MAXNAMELENGTH]) {
	USERENTRY *user = list->first;
	USERENTRY *user1; //sending request
	USERENTRY *user2; //recieving request
	int count = 0;
	int comparison; 
	while (count < list->length) {
		comparison = strcmp(sendinguser, user->name);
		if (comparison == 0) {
			user1 = user;
		}
		if (comparison < 0) {
			return 0; //sending user not found (shouldn't happen)
		}
		user = user->next;
		count++;
	}
	count = 0;
	user = list->first;
	while (count < list->length) {
		comparison = strcmp(newfriend, user->name);
		if (comparison < 0) {
			return 0; //friend user does not exsist 
		}
		if (comparison == 0) {
			user2 = user;
			appendfriend(user1->flist, user2);
			appendfriend(user2->flist, user1);
			
		}//adds friends togther after users found in main userlist
		user = user->next;
		count++;
	}


	return 1; //success friends added  
}

void dependfriend(USERENTRY *user1, USERENTRY *user2) {
	USERENTRY *fuser1 = user1->flist->first;
	USERENTRY *fuser2 = user2->flist->first;
	int count = 0;
	int comparison;
	while (count < user1->flist->length) {
		comparison = strcmp(fuser1->name, user2->name);
		if (comparison == 0) {
			deleteuser(fuser1);
		}
		fuser1 = fuser1->next;
		count++;
	}
	count = 0;
	while (count < user1->flist->length) {
		comparison = strcmp(fuser2->name, user1->name);
		if (comparison == 0) {
			deleteuser(fuser2);
		}
		fuser2 = fuser2->next;
		count++;
	}
}


void appendfriend(ULIST *list, USERENTRY *newfriend) {
	char fakepass[MAXNAMELENGTH] = "123456";
	USERENTRY *user = list->first;
	USERENTRY *new = createuserentry(newfriend->name, fakepass);
	if (list->length == 0) { //friends list empty 
		new->ulist = list;
		new->flist = NULL;
		list->first = new;
		list->last = new;
		list->length++;
	}
	else {
		user->next = new;
		new->prev = user;
		list->last = new;
		list->length++;
	}

}
