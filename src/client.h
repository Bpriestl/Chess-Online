<<<<<<< HEAD
/* EECS 22L Winter 2019 Chat Program Alpha Version
   client.c: Declares functions used in communication with server
   Author: Andrew Lin
   
   Modifications:
   3/3/19	AL	Initial Version */

#ifndef CLIENT_H
#define CLIENT_H

/* Send authentication info (username, password) to the server
   username is the username being sent to the server
   password is the password being sent to the server (consider encrypting?)
   mode is 0 for register, 1 for login (may consider changing these to constants later)
   Returns a string in the form MODE_USERNAME_username_PASSWORD_password */
void sendAuth(char username[AUTHLEN], char password[AUTHLEN], char mode);

/* Send message to the server
   recipient is the user receiving the message
   sender is the user sending the message
   message contains the message being sent
   Returns a string in the form of MESSAGE_RECIPIENT_recipient_SENDER_sender */
void sendMsg(char recipient[AUTHLEN], char sender [AUTHLEN], char message[MSGLEN]);

/* Receive message from the server and perform the appropriate operation by calling a function */
void receiveMsg();

#endif
/* EOF */
=======
/*********************************************************************
 *  Header file for Client.c module: this contains the code necessary to run a client
 *  and have it connect to the server
 *
 *  Initial Creation: 2/25/19 by B.P
 *  Updated:
 *  *******************************************************************/
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


struct message Talk2Server(struct message outgoing, struct sockaddr_in ServerAddress);
>>>>>>> 336cd1473b376b41742299bcac2f4e1c32f10c26
