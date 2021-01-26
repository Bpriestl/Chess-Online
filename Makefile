#Makefile for Chat

SRC = src
PROJECT = chat
CC = gcc
CFLAGS = -ansi -std=c99 -Wall -c 
GTKFLAG = `pkg-config --cflags --libs gtk+-2.0`
LFLAGS = -Wall
BIN = bin
ASSETS = assets
DOC = doc
TXT = README.txt COPYRIGHT.txt INSTALL.txt QUICKSTART.txt
#DEBUG = -DDEBUG

all: $(BIN)/$(PROJECT) $(BIN)/server

clean:
	rm -rf ./$(SRC)/*.o ./$(BIN)/$(PROJECT)
	rm -rf ./$(BIN)/server
	rm -rf $(SRC)/client $(SRC)/server
	rm -rf Chat_V1.0.tar.gz
	rm -rf Chat_V1.0_src.tar.gz

#for the bin executables should go here along with log files
test:
	./$(BIN)/chat zuma 18000

# for testing both GUI client and server in localhost
test-all:
	./$(BIN)/server 18000 &
	./$(BIN)/chat localhost 18000
	killall server

#for generating the main
$(SRC)/main.o: $(SRC)/main.c $(SRC)/messagestructs.o $(SRC)/userlist.o 
	$(CC) $(CFLAGS) $(GTKFLAG) $(SRC)/main.c -o $(SRC)/main.o -g $(DEBUG)

#generating main GUI
$(BIN)/$(PROJECT): $(SRC)/main.o $(SRC)/client.o $(SRC)/userlist.o $(SRC)/messagestructs.o $(SRC)/structures.o
	$(CC) $(LFLAGS) $(SRC)/main.o $(SRC)/client.o $(SRC)/userlist.o $(SRC)/structures.o $(SRC)/messagestructs.o -o $(BIN)/$(PROJECT) -ansi -std=c99 $(GTKFLAG) -g

$(SRC)/structures.o: $(SRC)/structures.h $(SRC)/structures.c
	$(CC) $(CFLAGS) $(SRC)/structures.c -o $(SRC)/structures.o
#---------------------- generating client and server communications ------------------------#

#generating client

$(SRC)/client.o: $(SRC)/client.c $(SRC)/messagestructs.o $(SRC)/userlist.o $(SRC)/structures.o
	$(CC) $(CFLAGS) $(SRC)/structures.o $(SRC)/client.c -o $(SRC)/client.o 


$(SRC)/userlist.o: $(SRC)/userlist.c $(SRC)/userlist.h $(SRC)/messagestructs.h
	$(CC) $(CFLAGS) $(SRC)/userlist.c -o $(SRC)/userlist.o
	
#generating message struct file
$(SRC)/messagestructs.o: $(SRC)/messagestructs.c $(SRC)/messagestructs.h $(SRC)/userlist.h $(SRC)/structures.h
	$(CC) $(CFLAGS) $(SRC)/messagestructs.c -o $(SRC)/messagestructs.o

#generating server
$(SRC)/server.o: $(SRC)/server.c $(SRC)/messagestructs.h $(SRC)/userlist.h
	$(CC) $(CFLAGS) $(SRC)/server.c -o $(SRC)/server.o $(DEBUG)

$(BIN)/server: $(SRC)/server.o $(SRC)/userlist.o $(SRC)/messagestructs.o
	$(CC) $(LFLAGS) $(SRC)/server.o $(SRC)/userlist.o $(SRC)/messagestructs.o -o $(BIN)/server
	


test-server:
	./$(BIN)/server 18000 


#------------------------- Generating Packaged Files ---------------------------------------#
user_tar:
	gtar -cvzf Chat_V1.0.tar.gz $(TXT) $(BIN)/chat $(ASSETS) $(DOC)/Chat_UserManuel.pdf

<<<<<<< HEAD
$(PROJECT): $(SRC)/main.o 
	mkdir -p ./bin
	$(CC) $(LFLAGS) $(SRC)/main.o -o $(BIN)/$(PROJECT) -ansi -std=c99 $(GTKFLAG)
=======
tar:
	gtar -cvzf Chat_V1.0_src.tar.gz $(TXT) $(ASSETS) Makefile $(BIN) $(DOC) $(SRC)		
>>>>>>> 336cd1473b376b41742299bcac2f4e1c32f10c26
