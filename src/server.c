/***************************************************
 *   Server.c: module where it handles the server 
 *   communications with the client and stores 
 *   information from the user 
 *   
 *   Initially Created 2/25/19 By B.P
 *   Updated: 
 *   ***********************************************/

#include "messagestructs.h"
#include "userlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <assert.h>
//#include <gtk/gtk.h>



/*** global variables ****************************************************/

struct sockaddr_in
	ServerAddress;	/* server address we connect with */
ULIST *userlist;

MESSAGELIST *messageslist; //for storing everything that needs to be forwarded 


/*** global functions ****************************************************/
void interaction(int sock);
int registration(char *request);
int login(char *request);
void message(char *request);
void getuserlist(char *request);
void handleoutgoingmessage(char *SendBuf, char *RecvBuf);
void chessmove(char *request);
void friendrequest(char *request);

void ServerMainLoop(int ServSocketFD,int Timeout)/* timeout is in micro seconds */
{
    int DataSocketFD;	/* socket for a new client */
    socklen_t ClientLen;
    struct sockaddr_in
	ClientAddress;	/* client address we connect with */
    fd_set ActiveFDs;	/* socket file descriptors to select from */
    fd_set ReadFDs;	/* socket file descriptors ready to read from */
    struct timeval TimeVal;
	TimeVal.tv_sec  = Timeout / 1000000;
	TimeVal.tv_usec = Timeout % 1000000;
    int res, i;
	int shutdown = 0;
	
    FD_ZERO(&ActiveFDs);		/* set of active sockets */
    FD_SET(ServSocketFD, &ActiveFDs);	/* server socket is active */
    
    ReadLog(userlist); /*reads the userlog to update the server with the saved accounts*/
    while(!shutdown)
    {
	ReadFDs = ActiveFDs;
	/* block until input arrives on active sockets or until timeout */
	res = select(FD_SETSIZE, &ReadFDs, NULL, NULL, &TimeVal);
	if (res < 0)
	{   printf("wait for input or timeout (select) failed");
	}
	if (res == 0)	/* timeout occurred */
	{
	    //time out do nothing
	}
	else		/* some FDs have data ready to read */
	{   for(i=0; i<FD_SETSIZE; i++)
	    {   if (FD_ISSET(i, &ReadFDs))
		{   if (i == ServSocketFD)
		    {	/* connection request on server socket */
			//accepts new client
			ClientLen = sizeof(ClientAddress);
			DataSocketFD = accept(ServSocketFD,
				(struct sockaddr*)&ClientAddress, &ClientLen);
			if (DataSocketFD < 0)
			{   printf("data socket creation (accept) failed");
			}

			FD_SET(DataSocketFD, &ActiveFDs);
		    }
		    else
		    {   /* active communication with a client */
		#ifdef DEBUG
		printf("Dealing with client on FD%d...\n", i);
		#endif
			interaction(i);

			close(i);
			FD_CLR(i, &ActiveFDs);
		    }
		}
	    }
	}
    }
} /* end of ServerMainLoop */

int MakeServerSocket(		/* create a socket on this server */
	uint16_t PortNo)
{
    int ServSocketFD;
    struct sockaddr_in ServSocketName;

    /* create the socket */
    ServSocketFD = socket(PF_INET, SOCK_STREAM, 0);
    if (ServSocketFD < 0)
    {   printf("service socket creation failed");
    }
    /* bind the socket to this server */
    ServSocketName.sin_family = AF_INET;
    ServSocketName.sin_port = htons(PortNo);
    ServSocketName.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(ServSocketFD, (struct sockaddr*)&ServSocketName,
		sizeof(ServSocketName)) < 0)
    {   printf("binding the server to a socket failed");
    }
    /* start listening to this socket */
    if (listen(ServSocketFD, 5) < 0)	/* max 5 clients in backlog */
    {  printf("listening on socket failed");
    }
    return ServSocketFD;
} /* end of MakeServerSocket */


int main(int argc, char *argv[]){
	int ServSocketFD;	/* socket file descriptor for service */
    int PortNo;		/* port number */

	userlist = createuserlist();
	messageslist = createmessagelist();
	//setup port stuff
    if (argc < 2)
    {   printf("Usage: server port\n");
	exit(10);
    }
    PortNo = atoi(argv[1]);	/* get the port number */
    if (PortNo <= 2000)
    {   fprintf(stderr, "invalid port number %d, should be >2000\n", PortNo);
        exit(10);
    }
	ServSocketFD = MakeServerSocket(PortNo);
	// end setting port up
		
	ServerMainLoop(ServSocketFD, 250000);
	
	close(ServSocketFD);
	free(userlist);
    return 0;
}
/****** interaction function ******/
void interaction(int DataSocketFD)
{
	//checks the first character of the incoming message to determine what operation is requested,
	//then sends it to the proper function to be handled
    int  length, sockcheck;
    char RecvBuf[BUFFSIZE];	/* message buffer for receiving a message */
    char SendBuf[BUFFSIZE];	/* message buffer for sending a response */
	//cleanse the buffers
	memset(RecvBuf,0,sizeof(RecvBuf));
	memset(SendBuf,0,sizeof(SendBuf));
	
	
    sockcheck = read(DataSocketFD, RecvBuf, sizeof(RecvBuf)-1);
    if (sockcheck < 0) 
    {   printf("reading from data socket failed");
    }
    RecvBuf[sockcheck] = 0;
	#ifdef DEBUG
	printf("handling request: %s\n", RecvBuf);
	#endif
	SendBuf[sizeof(SendBuf)-1] = 0;
	

	if(memcmp(RecvBuf,REGISTER, 1) == 0){
		int regsuccess = registration(RecvBuf);
		if(regsuccess == 0){//failure
			strcpy(SendBuf, FAIL);
		} else {
			strcpy(SendBuf, SUCCESS);
		}
	}
	if(memcmp(RecvBuf,LOGIN, 1) == 0){
		int logsuccess = login(RecvBuf);
		if(logsuccess == 0){//failure
			strcpy(SendBuf, FAIL);
		} else {
			strcpy(SendBuf, SUCCESS);
		}
	}
	
	if(memcmp(RecvBuf,MESSAGE, 1) == 0){
		message(RecvBuf);
		strcpy(SendBuf, SUCCESS);
	}
	if(memcmp(RecvBuf, GETUSERLIST, 1) == 0) {
		getuserlist(RecvBuf);
		strcpy(SendBuf, SUCCESS);
	}
	if(memcmp(RecvBuf, PING, 1) == 0){
		handleoutgoingmessage(SendBuf, RecvBuf);
	}
	if(memcmp(RecvBuf, CHESSMOVE, 1) == 0) {
		chessmove(RecvBuf);
		strcpy(SendBuf, SUCCESS);
	}
	if(memcmp(RecvBuf, SENDFRIENDREQUEST, 1) == 0 || memcmp(RecvBuf, ACCEPTFRIENDREQUEST, 1) == 0) {
		friendrequest(RecvBuf); //send an accept friend request or a friend request to another user
		strcpy(SendBuf, SUCCESS);
	}
	
		
		
		
	
    length = strlen(SendBuf);
	#ifdef DEBUG
	printf("returning: %s\n", SendBuf);
    	#endif
	sockcheck = write(DataSocketFD, SendBuf, length);
    if (sockcheck < 0)
    {   printf("writing to data socket failed");
    }
	#ifdef DEBUG
	printf("\n\n\nthe user list is currently as follows:\n");
	#endif
	int count = 0;
	USERENTRY *curr = userlist->first;
	while(count < userlist->length){
		#ifdef DEBUG
		printf("name: %s, password: %d\n", curr->name, curr->password);
		#endif
		count++;
		curr = curr->next;
	}
	#ifdef DEBUG
	printf("\nthe message queue is currently as follows:\n\n\n");
	#endif
	count = 0;
	MENTRY *current = messageslist->first;
	while(count < messageslist->length){
		#ifdef DEBUG
		printf("sender: %s, destination: %s, content: %s,\n", current->user, current->destuser, current->content);
		#endif
		count++;
		current = current->next;
	}
	LogofUsers(userlist);		
	#ifdef DEBUG
	printf("end of list\n\n");
	#endif
} /* end of interation */


int login(char *request){
	//user is attempting to login 0 for failure 1 for success
	//form is "c-username-password"
	char *token = strtok(request, "-");
	token = strtok(NULL, "-");//called twice cause we can ignore the first letter;
	char name[MAXNAMELENGTH];
	strcpy(name,token);
	token = strtok(NULL, "-"); //collecting password
	
	
	
	printf("attempted login for username: %s, password: %s\n",name,token);
	return loginattempt(userlist, name, token);
}

int registration(char *request){
	//user is requesting a registration, adjucate and return 0 for failure 1 for success
	//form is "d-username-password"
	char *token = strtok(request, "-");
	token = strtok(NULL, "-");//called twice cause we can ignore the first letter;
	char name[MAXNAMELENGTH];
	strcpy(name,token);
	token = strtok(NULL, "-"); //collecting password
	
	printf("attempted registration for username: %s, password: %s",name,token);
		
	return createaccount(userlist, name, token);
}

void message(char *request){
	//user is attempting to send another user a message
	//form is "e-'sendingusername'-'destinationusername'-'message body'
	char *token = strtok(request, DELIMITER);
	//printf("token is %s\n",token);
	token = strtok(NULL, DELIMITER);
	char sendinguser[MAXNAMELENGTH];
	strcpy(sendinguser, token);
	//printf("token is %s\n",token);

	
	token = strtok(NULL, DELIMITER);//called three times cause we can iterate through opcode and sender
	//printf("token is %s\n",token);
	char recievinguser[MAXNAMELENGTH];
	strcpy(recievinguser, token);
	
	//printf("token is %s\n",token);
	token = strtok(NULL, DELIMITER);
	//printf("token is %s\n",token);
	char contents[BUFFSIZE];
	strcpy(contents, token);
	
	createandappendmessage(messageslist, MESSAGE, sendinguser, recievinguser, contents);
}

void chessmove(char *request){
	//user is attempting to send a chess move to another user
	//form is j-sendingusername-destinationusername-initialpiece-initialxpos-initialypos-destpiece-destxpos-destypos-special
	//everything initial piece and onwards is packaged all into content for the client to decode later
	//appends to the messageslist queue
	char *token = strtok(request, DELIMITER); //opcode
	token = strtok(request, DELIMITER); //sender
	char sendinguser[MAXNAMELENGTH];
	strcpy(sendinguser, token);
	token = strtok(NULL, DELIMITER); //target
	char targetuser[MAXNAMELENGTH];
	strcpy(targetuser, token);
	char chesscontents[BUFFSIZE];
	token = strtok(NULL, DELIMITER); //initialpiece
	strcpy(chesscontents, token);
	for(int i = 0; i < 6; i++) { //iterates through and appends the stuff to the chesscontents because the contents will have a bunch of delimiters for chess moves
		strcat(chesscontents, DELIMITER);
		token = strtok(NULL, DELIMITER); //target
		strcat(chesscontents, token);
	}
	createandappendmessage(messageslist, CHESSMOVE, sendinguser, targetuser, chesscontents);
}

void getuserlist(char *request) {
	//prepends the entire userlist for a specific user to recieve, only excluding themselves
	//expected to be in alphabetical order but not guarentees
	//form is g-requestinguser
	char *token = strtok(request, DELIMITER);
	token = strtok(NULL, DELIMITER);
	char requestinguser[MAXNAMELENGTH];
	strcpy(requestinguser, token); //get the person that requested it
	
	int count = 0;
	USERENTRY *curr = userlist->first;
	while(count < userlist->length){
		if(strcmp(requestinguser, curr->name) != 0){
			createandappendmessage(messageslist, GETUSERLIST, curr->name, requestinguser, ""); //prepends all the userlist names as messages to be sent back to the requester
		}
		count++;
		curr = curr->next;
	}
}

void friendrequest(char *request) { //works for both accepting and forwarding a request from the server perspective
	//appends a friend request to the messageslistqueue
	//request is of form opcode-user-targetuser-optionalmessage
	char *token = strtok(request, DELIMITER); // opcode
	char friendcode[2];
	strcpy(friendcode, token);
	token = strtok(NULL, DELIMITER); //sending user
	char sendinguser[MAXNAMELENGTH];
	strcpy(sendinguser, token);
	
	token = strtok(NULL, DELIMITER);//destination user
	char recievinguser[MAXNAMELENGTH];
	strcpy(recievinguser, token);
	
	char contents[BUFFSIZE];
	if(strcmp(friendcode, SENDFRIENDREQUEST) == 0){ //if its a friend request specifically theres an optional message that can be sent
		token = strtok(NULL, DELIMITER);//contents
		strcpy(contents, token); 
	} else {
		strcpy(contents, ""); //or empty
	}
	
	createandappendmessage(messageslist, friendcode, sendinguser, recievinguser, contents);
	
}

void handleoutgoingmessage(char *SendBuf, char *request){
	//check messages list for returning
	char *currentuser = strtok(request, DELIMITER);
	currentuser = strtok(NULL, DELIMITER); //skip to second to get name
	MENTRY *curr = messageslist->first;
	MENTRY *removaltarget = NULL;
	int count = 0;
	int numfound = 0; //counts up how many of the messages destined for a specific user are in the list
	#ifdef DEBUG
	printf("global user to check against is %s\n", currentuser);
	#endif
	while (count < messageslist->length){
		if((strcmp(curr->destuser, currentuser) == 0) && numfound == 0) { //matched with the first message destined for them in the list 
			numfound++;
			strcpy(SendBuf, curr->opcode);
			//potentially more deconstructing of the message to be sent here depending on the type of message, for now its just the regular message from user to user
			//message protocol is sent back as OPCODE-SENDER-CONTENT-length  --- it is assumed that the user knows their own username so it is not sent back to them
			//chessmove protocol is sent back as OPCODE-SENDER-CONTENT(many delimiters)-LENGTH
			//FRIENDREQUEST protocol is sent back as OPCODE-SENDER-LENGTH
			
			strcat(SendBuf, DELIMITER);
			strcat(SendBuf, curr->user);
			strcat(SendBuf, DELIMITER);
			if(strcmp(curr->opcode, MESSAGE) == 0 
								|| strcmp(curr->opcode, CHESSMOVE) == 0 
								|| strcmp(curr->opcode, SENDFRIENDREQUEST) == 0){ //its of type message, chessmove, or sendfriendrequest so it requires content also
				strcat(SendBuf, curr->content);
				strcat(SendBuf, DELIMITER);
			}
			
			removaltarget = curr;
			
		} else if (strcmp(curr->destuser, currentuser) == 0){ //excess matches
			numfound++;
		}
		count++;
		curr = curr->next;
	}
	
	if(removaltarget != NULL){
		deletemessageentry(removaltarget);
	}
	
	//generic return string if no matched username is just the FAIL opcode
	if(numfound == 0){
		strcpy(SendBuf, FAIL);
	} else if (numfound == 1){
		//last message in the list append that there are no extras
		strcat(SendBuf, LAST);
	} else {
		//more messages to be retrieved
		strcat(SendBuf, MORE);
	}
}



