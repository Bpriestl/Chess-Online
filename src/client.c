<<<<<<< HEAD
/* EECS 22L Winter 2019 Chat Program Alpha Version
   client.c: Handles client (user app) and allows it to connect to server
   Author: Buonkuang Priestley
   
   Modifications:
   2/25/19	BP	Initial Version */

=======
/*********************************************************************
 *  Client.c module: this contains the code necessary to run a client
 *  and have it connect to the server
 *
 *  Initial Creation: 2/25/19 by B.P
 *  Updated:
 *  *******************************************************************/
#include "messagestructs.h"
#include "userlist.h"
#include "structures.h"
>>>>>>> 336cd1473b376b41742299bcac2f4e1c32f10c26
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
<<<<<<< HEAD
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg){
	perror(msg);
	exit(0);
}

int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	char buffer[256];
	if (argc < 3) {
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
    }
    portno = atoi(argv[2]);

	while (1) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			error("ERROR opening socket");
		}
    	server = gethostbyname(argv[1]);
		
		if (server == NULL) {
     		fprintf(stderr,"ERROR, no such host\n");
			exit(0);
		}

		bzero((char *) &serv_addr, sizeof(serv_addr)); // default socket stuff
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
		serv_addr.sin_port = htons(portno);

		if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
			error("ERROR connecting");
		}
		printf("Please enter the message: ");
		bzero(buffer, 256);
		fgets(buffer, 255, stdin); // fgets is used to read the message from stdin.
		n = write(sockfd, buffer, strlen(buffer)); // writes into the socket
		if (n < 0) {
			error("ERROR writing to socket");
		}
		bzero(buffer,256);
		n = read(sockfd,buffer,255); //reads back the socket
		if (n < 0) {
			error("ERROR reading from socket");
		}
		printf("%s\n",buffer); //once it reads it it prints out that msg
		close(sockfd);
	}	
	return 0;
}	

/* EOF */
=======
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <assert.h> 


struct message Talk2Server(struct message outgoing, struct sockaddr_in ServerAddress) //add a struct for the message type later
{
			/*INITIALIZE VARIABLES */
    int sockcheck;
    int SocketFD;
	char RecvBuf[BUFFSIZE];	/* message buffer for receiving a message */
	char messagestring[BUFFSIZE + 100];
	memset(RecvBuf,0,sizeof(RecvBuf));
	memset(messagestring,0,sizeof(messagestring));
		/* END INITIALIZE VARIABLES  */
		
		
			/* ENCODING THE MESSAGE                                 */
	strcpy(messagestring, outgoing.opcode); //append opcode first since its always needed
	strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
	strncat(messagestring, outgoing.user, sizeof(messagestring)-1-strlen(messagestring)); //then cancatenate who is sending the message, since its always needed
	
	if(strcmp(outgoing.opcode, REGISTER) == 0 || strcmp(outgoing.opcode, LOGIN) == 0){ //login or registration only requires username and password
		//has form OPCODE-USERNAME-PASSWORD for either request
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, outgoing.content, sizeof(messagestring)-1-strlen(messagestring)); //then cancatenate password
	}
	if(strcmp(outgoing.opcode, MESSAGE) == 0) {
		//message being sent, is of form opcode-username-targetusername-messagecontent
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, outgoing.destuser, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, outgoing.content, sizeof(messagestring)-1-strlen(messagestring));
	}
	if(strcmp(outgoing.opcode, SENDFRIENDREQUEST) == 0) { //friend request encoding
		//friend request being sent, is of form opcode-username-targetusername-optionalmessage
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, outgoing.destuser, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, outgoing.content, sizeof(messagestring)-1-strlen(messagestring));
	}
	if (strcmp(outgoing.opcode, ACCEPTFRIENDREQUEST) == 0) {
		//accepting friend request is of form opcode-username-targetusername
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, outgoing.destuser, sizeof(messagestring)-1-strlen(messagestring));
	}
	
	#ifdef DEBUG
	fprintf(stderr, "sending message: %s to server\n", messagestring);
	#endif
	
	//if opcode is of PING, GETUSERLIST, or GETFRIENDLIST just send opcode + username
		/* END ENCODING THE MESSAGE */
	
	
	
			/* TCP PROTOCOL FOR SENDING THAT MESSAGE AND RECIEVING NEW ONE */
    SocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (SocketFD < 0)
    {   printf("socket creation failed");
    }
    if (connect(SocketFD, (struct sockaddr*)&ServerAddress,
		sizeof(struct sockaddr_in)) < 0)
    {   printf("connecting to server failed");
    }
    sockcheck = write(SocketFD, messagestring, strlen(messagestring));
    if (sockcheck < 0)
    {   printf("writing to socket failed");
    }
    sockcheck = read(SocketFD, RecvBuf, sizeof(RecvBuf)-1);
    if (sockcheck < 0) 
    {   printf("reading from socket failed");
    }
    //RecvBuf[sockcheck] = 0;
    close(SocketFD);
	#ifdef DEBUG
	printf("recieved: %s \n", RecvBuf);
	#endif
		/* END TCP PROTOCOL                              */
	
	
	
	
			/* PACKAGE RECIEVED MESSAGE INTO A STRUCT FOR RETURNING TO CALLER         */
		
			/*of form OPCODE(j)-(sendingusername-destinationusername)-intitialpiece-initialxpos,initialypos-destpiece-destxpos,destypos-special
 * 	decode opcode,user,destuser,content,chessmove*/
/*
	struct message mess;
	char medium[3];
	if(strcmp(outgoing.opcode, REQUESTCHESS)){
		strcpy(messagestring, CHESSMOVE); //Request chess opcode
	
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, mess.user, sizeof(messagestring)-1-strlen(messagestring));
	
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		strncat(messagestring, mess.destuser, sizeof(messagestring)-1-strlen(messagestring));

		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		sprintf(medium, "%d", mess.chessmove->initialpiece);
		strncat(messagestring, medium, sizeof(messagestring)-1-strlen(messagestring));
	
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		sprintf(medium, "%d", mess.chessmove->initialxpos);
		strncat(messagestring, medium, sizeof(messagestring)-1-strlen(messagestring));		
		
		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		sprintf(medium, "%d", mess.chessmove->initialypos);
		strncat(messagestring, medium, sizeof(messagestring)-1-strlen(messagestring));

		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		sprintf(medium, "%d", mess.chessmove->destpiece);
		strncat(messagestring, medium, sizeof(messagestring)-1-strlen(messagestring));

		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		sprintf(medium, "%d", mess.chessmove->destxpos);
		strncat(messagestring, medium, sizeof(messagestring)-1-strlen(messagestring));

		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		sprintf(medium, "%d", mess.chessmove->destypos);
		strncat(messagestring, medium, sizeof(messagestring)-1-strlen(messagestring));

		strncat(messagestring, DELIMITER, sizeof(messagestring)-1-strlen(messagestring));
		sprintf(medium, "%d", mess.chessmove->special);
		strncat(messagestring, medium, sizeof(messagestring)-1-strlen(messagestring));
} 
*/
	// should always have atleast opcode 
	struct message incoming; 
	char *token = strtok(RecvBuf, DELIMITER);
	strcpy(incoming.opcode, token); //just this for FAIL and SUCCESS
	
	//then extras for the other types
	if(strcmp(incoming.opcode, MESSAGE) == 0){
		//of form OPCODE-user-content-length
		token = strtok(NULL, DELIMITER);
		strcpy(incoming.user, token); //extract username
		token = strtok(NULL, DELIMITER);
		strcpy(incoming.content, token); //extract content
		token = strtok(NULL, DELIMITER);
		strcpy(incoming.remainingmessages, token); //extract remaining messages
	}
	if(strcmp(incoming.opcode, GETUSERLIST) == 0 || strcmp(incoming.opcode, GETFRIENDLIST) == 0 || strcmp(incoming.opcode, ACCEPTFRIENDREQUEST) == 0){
		//of form OPCODE-user-length
		token = strtok(NULL, DELIMITER);
		strcpy(incoming.user, token); //extract username
		token = strtok(NULL, DELIMITER);
		strcpy(incoming.remainingmessages, token); //extract remaining messages
	}
	if(strcmp(incoming.opcode, SENDFRIENDREQUEST) == 0){
		//of form OPCODE-user-content-length
		token = strtok(NULL, DELIMITER);
		strcpy(incoming.user, token); //extract username
		token = strtok(NULL, DELIMITER);
		strcpy(incoming.content, token); //extract message
		token = strtok(NULL, DELIMITER);
		strcpy(incoming.remainingmessages, token); //extract remaming messages
	}
	
	
	//transfering moves to the message struct
	if(strcmp(incoming.opcode, CHESSMOVE) == 0){
	//of form OPCODE(j)-(sendingusername-destinationusername)-intitialpiece-initialxpos,initialypos-destpiece-destxpos,destypos-special
		
	//how tokens work "text-secondtext-thirdtext"
	//it takes the first string before the deliminter and saves it to token
	//second time it runs it takes the secondtext
		token = strtok(NULL, DELIMITER); //username
		strcpy(incoming.user, token);
		
		token = strtok(NULL, DELIMITER); //initialpeice
		int intp = atoi(token);  	
		
		token = strtok(NULL, DELIMITER); //initialxpos
                int intx = atoi(token);
                
		token = strtok(NULL, DELIMITER); //initialypos
                int inty = atoi(token);
		
		token = strtok(NULL, DELIMITER); //destpiece
                int destp = atoi(token);
                
		token = strtok(NULL, DELIMITER); //destxpos
                int destx = atoi(token);
		
		token = strtok(NULL, DELIMITER); //destypos
                int desty = atoi(token);
                
		token = strtok(NULL, DELIMITER); //special
                int spec = atoi(token);
		createmovelist();	
		incoming.chessmove = createmove(intp,intx,inty,destp,destx,desty,spec);
		
		token = strtok(NULL, DELIMITER);
		strcpy(incoming.remainingmessages, token);
	}
	return incoming;
} /* end of Talk2Server */
>>>>>>> 336cd1473b376b41742299bcac2f4e1c32f10c26
