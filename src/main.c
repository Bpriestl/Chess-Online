#include "client.h"
#include "userlist.h"
#include "messagestructs.h"
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
#include <gtk/gtk.h>
#include <ctype.h>

#define MAXIMUM_MESSAGE_LENGTH 600

#define CURRENT_TIMEZONE_OFFSET (-7) //It's daylight saving time

/* Database Hierarchy is included in the doc folder
This is a well-designed database that supports multiple message types and group chat!*/

#define MESSAGE_TYPE_TEXT 1
#define MESSAGE_TYPE_IMAGE 2
#define MESSAGE_TYPE_EMOJI 3
#define MESSAGE_TYPE_FRIEND_REQUEST 4
#define MESSAGE_TYPE_CHESS_REQUEST 5
#define MESSAGE_TYPE_CHESS_MOVE 6
#define MESSAGE_STATUS_SUCCESSFUL_TO_SEND 0
#define MESSAGE_STATUS_FAILED_TO_SEND 1

/* List of chats, with a container referring to a single chat */
typedef struct DB_Toplevel_struct DB_Toplevel;
/* Me_Container: Pointer to currently selected chat(?)
   container_length: Number of chats in the list
   First_Container: Pointer to first chat in list
   Last_Container: Pointer to last chat in list */

/* Struct for a chat */
typedef struct DB_Container_struct DB_Container;
/* is_group_chat: Determines whether or not the chat is a group chat
   member_number: Number of members in the chat
   First_Contact: Pointer to first contact in list of members
   Last_Contact: Pointer to last contact in list of members
   DB_Toplevel: Pointer to the Toplevel struct this chat is a part of
   DB_Message_Toplevel: Pointer to the list of messages in the chat
   Previous_Container: Pointer to the previous chat in the DB_Toplevel list
   Next_Container: Pointer to the next chat in the DB_Toplevel list */

/* Struct for a member and his or her information */
typedef struct DB_Contact_Card_struct DB_Contact_Card;
/* uid: Identifying ID
   nickname: Username; screen or displayed name
   email: Email
   online_status: Indicates whether the user is online or not
   grouping: Used if this user is placed into a group such as a blacklist
   Container_Toplevel: Pointer to the chat which the user is a part of
   Previous_Contact_Card: Pointer to the previous contact in the list that is part of the Container
   Next_Contact_Card: Pointer to the next contact in the list that is part of the Container */

/* Struct for a list of the messages in a given chat */
typedef struct DB_Message_Toplevel_struct DB_Message_Toplevel;
/* Container: Pointer to the chat that contains the messages in this list
   message_length: The number of messages in the chat
   First_Message: Pointer to the first message in the list
   Last_Message: Pointer to the last message in the list */

/* Struct for a message */
typedef struct DB_Message_Store_struct DB_Message_Store;
/* Message_Toplevel: Pointer to the list containing this message
   message_type: Identifies the message as a type, an image, an emoji, a friend request, an invitation to play chess, or a chess move
   base64_content: The content of the message
   from_uid: The sender of the message
   status: Successfully sent or failed to send 
   Previous_Message_Store: Pointer to the previous message in the DV_Message_Toplevel list
   Next_Message_Store: Pointer to the next message in the DV_Message_Toplevel list
   unix_time_stamp: Time at which the message was sent(?) */

/* Struct definition for DB_Toplevel */
struct DB_Toplevel_struct {
    DB_Container* Me_Container;
    int container_length;
    DB_Container* First_Container;
    DB_Container* Last_Container;
};

/* Struct definition for DB_Container */
struct DB_Container_struct {
    short int is_group_chat;
    int member_number;
    int has_unread_messages;
    DB_Contact_Card* First_Contact;
    DB_Contact_Card* Last_Contact;
    DB_Toplevel* DB_Toplevel;
    DB_Message_Toplevel* DB_Message_Toplevel;
    DB_Container* Previous_Container;
    DB_Container* Next_Container;
};

/* Struct definition for DB_Contact_Card */
struct DB_Contact_Card_struct {
    char uid[30];
    char nickname[60];
    char email[60];
    int online_status;
    /* Grouping allows user to sort different contacts to different groups, ex. Mason to Blacklist (represented by 1) */
    int grouping; //Perhaps change it to a string?
    DB_Container* Container_Toplevel;
    DB_Contact_Card* Previous_Contact_Card;
    DB_Contact_Card* Next_Contact_Card;
};

/* Struct definition for DB_Message_Toplevel */
struct DB_Message_Toplevel_struct {
    DB_Container* Container;
    int message_length;
    DB_Message_Store* First_Message;
    DB_Message_Store* Last_Message;
};

/* Struct definition for DB_Message_Store */
struct DB_Message_Store_struct {
    DB_Message_Toplevel* Message_Toplevel;
    int message_type;
    char base64_content[10000];
    char from_uid[30];
    int status;
    DB_Message_Store* Previous_Message_Store;
    DB_Message_Store* Next_Message_Store;
    unsigned long unix_time_stamp;
};

/* Database ends */

GdkColor g_widget_background_color;

GtkBuilder *builder_login;
GtkBuilder *builder_chat;
GtkBuilder *builder_profile;
GtkBuilder *builder_settings;
GtkBuilder *builder_confirmation;
GtkBuilder *builder_add_contact;
GtkBuilder *builder_accept_contact;
GtkBuilder *builder_request_accepted;
GtkBuilder *builder_save_log_browser;
GtkBuilder *builder_emoji_chooser;

GtkWidget *window_widget;
GtkWidget *chat_window_widget;
GtkWidget *profile_window_widget;
GtkWidget *settings_window_widget;
GtkWidget *confirmation_window_widget;
GtkWidget *add_contact_window_widget;
GtkWidget *accept_contact_window_widget;
GtkWidget *request_accepted_window_widget;
GtkWidget *save_log_browser_dialog_widget;
GtkWidget *emoji_chooser_window_widget;

GtkWidget *g_label_welcome;
GtkWidget *g_entry_username;
GtkWidget *g_entry_password;
GtkWidget *g_label_register;
GtkWidget *g_label_login;
GtkWidget *g_eventbox_register;
GtkWidget *g_eventbox_login;
GtkWidget *g_eventbox_settings;
GtkWidget *g_label_settings;

//For widgets on the left panel
GtkWidget *g_image_left_panel_avatar;
GtkWidget *g_eventbox_left_panel_image_avatar;
GtkWidget *g_label_left_panel_1;
GtkWidget *g_eventbox_label_left_panel_1;
GtkWidget *g_label_left_panel_2;
GtkWidget *g_eventbox_label_left_panel_2;
GtkWidget *g_label_add;
GtkWidget *g_eventbox_add;
GtkWidget *g_treeview_left_panel_friends;

//For special objects
GtkListStore *g_liststore_friends;
GtkTextBuffer *g_textbuffer_messages;
GtkAdjustment *g_adjustment_textview;
GtkTextIter g_textIter_messages; //This cannot be a pointer. Use its address in GTK APIs.
GtkTreeIter g_treeIter_frends;
GtkTreeSelection *g_treeselection_friends;
enum{
    COL_indicator,
    COL_username,
    COL_online_status,
    NUM_cols
};


//For widgets on the right panel
GtkWidget *g_eventbox_label_sender;
GtkWidget *g_label_sender;
GtkWidget *g_textview_right_panel;
GtkWidget *g_label_right_panel_send;
GtkWidget *g_eventbox_label_right_panel_send;
GtkWidget *g_entry_message;
GtkWidget *g_image_remove;
GtkWidget *g_image_chess;
GtkWidget *g_eventbox_image_remove;
GtkWidget *g_eventbox_image_chess;
GtkWidget *g_image_save_log;
GtkWidget *g_eventbox_image_save_log;
GtkWidget *g_eventbox_image_emoji_chooser;


GtkWidget *g_label_uid;
GtkWidget *g_label_profile_slogan;
GtkWidget *g_image_avatar;
GtkWidget *g_label_dismiss;
GtkWidget *g_label_delete_account;
GtkWidget *g_hseparator_1;
GtkWidget *g_eventbox_dismiss;
GtkWidget *g_eventbox_delete_account;

GtkWidget *g_entry_hostname;
GtkWidget *g_entry_port;
GtkWidget *g_label_apply_settings;
GtkWidget *g_eventbox_apply_settings;

GtkWidget *g_label_uid_to_remove;
GtkWidget *g_eventbox_ok_to_remove;
GtkWidget *g_label_ok_to_remove;
GtkWidget *g_eventbox_cancel_removing;
GtkWidget *g_label_cancel_removing;

GtkWidget *g_label_add_contact_title;
GtkWidget *g_entry_add_contact;
GtkWidget *g_eventbox_label_add_contact;
GtkWidget *g_label_add_contact;
GtkWidget *g_eventbox_label_cancel_add_contact;
GtkWidget *g_label_cancel_add_contact;
GtkWidget *g_entry_attached_messages;

GtkWidget *g_label_request_title;
GtkWidget *g_label_requesting_from_uid;
GtkWidget *g_label_request_attached_message;
GtkWidget *g_eventbox_label_accept_request;
GtkWidget *g_eventbox_label_decline_request;
GtkWidget *g_label_accept_request;
GtkWidget *g_label_decline_request;

GtkWidget *g_label_request_accepted_uid;
GtkWidget *g_label_request_accepted_title;
GtkWidget *g_eventbox_label_request_accepted_ok;
GtkWidget *g_label_request_accepted_ok;

GtkWidget *g_button_cancel_saving;
GtkWidget *g_button_save;

GtkWidget *g_eventbox_image_emoji_1;
GtkWidget *g_eventbox_image_emoji_2;
GtkWidget *g_eventbox_image_emoji_3;
GtkWidget *g_eventbox_image_emoji_4;
GtkWidget *g_eventbox_image_emoji_5;
GtkWidget *g_eventbox_image_emoji_6;
GtkWidget *g_eventbox_label_emoji_cancel;
GtkWidget *g_label_emoji_cancel;
GtkWidget *g_label_emoji_chooser_title;

GtkWindow *login_window;
GtkWindow *chat_window;
GtkWindow *profile_window;
GtkWindow *settings_window;
GtkWindow *confirmation_window;
GtkWindow *add_contact_window;
GtkWindow *accept_contact_window;
GtkWindow *request_accepted_window;
GtkDialog *save_log_browser_dialog;
GtkWindow *emoji_chooser_window;

typedef struct Thread_Arguments_struct{
    //Put stuff that needs to be shared with the thread here
} Thread_Arguments;

struct sockaddr_in ServerAddress;    /* server address we connect with */
struct hostent *Server;
char server_hostname[60];
int server_port_number;

int set_server_info();

void show_profile_window();
void show_settings_window();
void show_confirmation_window();
void show_add_contact_window();
void show_chat_window();
void show_accept_contact_window(Message_struct *Friend_Request_Instance);
void show_request_accepted_window(Message_struct *Request_Accepted_Instance);
void show_save_log_browser_dialog();
void show_emoji_chooser_window();

void show_notification_dialog(GtkWindow *parent, gint statuscode);

char passwd[30];// This string stores password for logging in and registration

DB_Toplevel *db_toplevel_instance;
DB_Container *db_container_me_instance;
DB_Contact_Card *db_contact_card_me_instance;

/*Database support functions*/

DB_Toplevel *db_toplevel_new(){
    DB_Toplevel *new_db_toplevel = malloc(sizeof(DB_Toplevel));
    new_db_toplevel->Me_Container = NULL;
    new_db_toplevel->container_length = 0;
    new_db_toplevel->First_Container = NULL;
    new_db_toplevel->Last_Container = NULL;
    return new_db_toplevel;
}

DB_Container *db_container_new(DB_Toplevel *new_db_toplevel){
    assert(new_db_toplevel);
    DB_Container *new_db_container = malloc(sizeof(DB_Container));
    new_db_container->is_group_chat = 0;
    new_db_container->member_number = 0;
    new_db_container->has_unread_messages = 0;
    new_db_container->First_Contact = NULL;
    new_db_container->Last_Contact = NULL;
    new_db_container->DB_Toplevel = new_db_toplevel;
    new_db_container->DB_Message_Toplevel = NULL;
    new_db_container->Previous_Container = NULL;
    new_db_container->Next_Container = NULL;
    if(new_db_toplevel->container_length == 0){
        assert(new_db_toplevel->First_Container == NULL);
        assert(new_db_toplevel->Last_Container == NULL);
        new_db_toplevel->First_Container = new_db_container;
        new_db_toplevel->Last_Container = new_db_container;
        new_db_toplevel->container_length ++;
    }
    else{
        assert(new_db_toplevel->First_Container);
        assert(new_db_toplevel->Last_Container);
        new_db_toplevel->Last_Container->Next_Container = new_db_container;
        new_db_container->Previous_Container = new_db_toplevel->Last_Container;
        new_db_toplevel->Last_Container = new_db_container;
        new_db_toplevel->container_length ++;
    }
    return new_db_container;
}

DB_Contact_Card *db_contact_card_new(DB_Container *new_db_container, char *uid){
    assert(new_db_container);
    assert(uid);
    DB_Contact_Card *new_db_contact_card = malloc(sizeof(DB_Contact_Card));
    strcpy(new_db_contact_card->uid, uid);
    strcpy(new_db_contact_card->nickname, uid);
    strcpy(new_db_contact_card->email, "");
    new_db_contact_card->online_status = 1;
    new_db_contact_card->grouping = 0; //Perhaps change it to a string?
    new_db_contact_card->Container_Toplevel = new_db_container;
    new_db_contact_card->Previous_Contact_Card = NULL;
    new_db_contact_card->Next_Contact_Card = NULL;
    if(new_db_container->member_number == 0){
        assert(new_db_container->First_Contact == NULL);
        assert(new_db_container->Last_Contact == NULL);
        new_db_container->First_Contact = new_db_contact_card;
        new_db_container->Last_Contact = new_db_contact_card;
        new_db_container->member_number ++;
    }
    else{
        assert(new_db_container->First_Contact);
        assert(new_db_container->Last_Contact);
        new_db_container->Last_Contact->Next_Contact_Card = new_db_contact_card;
        new_db_contact_card->Previous_Contact_Card = new_db_container->Last_Contact;
        new_db_container->Last_Contact = new_db_contact_card;
        new_db_container->member_number ++;
    }
    return new_db_contact_card;
}

DB_Message_Toplevel *db_message_toplevel_new(DB_Container *new_db_container){
    assert(new_db_container);
    DB_Message_Toplevel *new_db_message_toplevel = malloc(sizeof(DB_Message_Toplevel));
    new_db_message_toplevel->Container = new_db_container;
    new_db_container->DB_Message_Toplevel = new_db_message_toplevel;
    new_db_message_toplevel->message_length = 0;
    new_db_message_toplevel->First_Message = NULL;
    new_db_message_toplevel->Last_Message = NULL;
    return new_db_message_toplevel;
}

DB_Message_Store *db_message_store_new(DB_Message_Toplevel *new_db_message_toplevel, 
                                        int new_message_type, char *new_message_base64_content, 
                                        char *new_from_uid, int new_message_status, 
                                        unsigned long new_message_unix_time_stamp){
    assert(new_db_message_toplevel);
    assert(new_message_base64_content);
    assert(new_from_uid);
    //Do not assert new_message_status as 0 is meaningful here
    DB_Message_Store *new_db_message_store = malloc(sizeof(DB_Message_Store));
    new_db_message_store->Message_Toplevel = new_db_message_toplevel;
    new_db_message_store->message_type = new_message_type;
    strcpy(new_db_message_store->base64_content, new_message_base64_content);
    strcpy(new_db_message_store->from_uid, new_from_uid);
    new_db_message_store->status = new_message_status;
    new_db_message_store->Previous_Message_Store = NULL;
    new_db_message_store->Next_Message_Store = NULL;
    new_db_message_store->unix_time_stamp = new_message_unix_time_stamp;
    if(new_db_message_toplevel->message_length == 0){
        assert(new_db_message_toplevel->First_Message == NULL);
        assert(new_db_message_toplevel->Last_Message == NULL);
        new_db_message_toplevel->First_Message = new_db_message_store;
        new_db_message_toplevel->Last_Message = new_db_message_store;
        new_db_message_toplevel->message_length ++;
    }
    else{
        assert(new_db_message_toplevel->First_Message);
        assert(new_db_message_toplevel->Last_Message);
        new_db_message_toplevel->Last_Message->Next_Message_Store = new_db_message_store;
        new_db_message_store->Previous_Message_Store = new_db_message_toplevel->Last_Message;
        new_db_message_toplevel->Last_Message = new_db_message_store;
        new_db_message_toplevel->message_length ++;
    }
    return new_db_message_store;
}

void db_message_store_purge(DB_Message_Store *db_message_store_to_purge){
    assert(db_message_store_to_purge);
    free(db_message_store_to_purge->base64_content);
    free(db_message_store_to_purge->from_uid);
    DB_Message_Store *Historical_Previous = db_message_store_to_purge->Previous_Message_Store;
    DB_Message_Store *Historical_Next = db_message_store_to_purge->Next_Message_Store;
    if(db_message_store_to_purge->Message_Toplevel->First_Message == db_message_store_to_purge){
        db_message_store_to_purge->Message_Toplevel->First_Message = Historical_Next;
    }
    if(db_message_store_to_purge->Message_Toplevel->Last_Message == db_message_store_to_purge){
        db_message_store_to_purge->Message_Toplevel->Last_Message = Historical_Previous;
    }
    if(db_message_store_to_purge->Previous_Message_Store)
        db_message_store_to_purge->Previous_Message_Store->Next_Message_Store = Historical_Next;
    if(db_message_store_to_purge->Next_Message_Store)
        db_message_store_to_purge->Next_Message_Store->Previous_Message_Store = Historical_Previous;
    db_message_store_to_purge->Message_Toplevel->message_length --;
    free(db_message_store_to_purge);
}

void db_message_toplevel_purge(DB_Message_Toplevel *db_message_toplevel_to_purge){
    assert(db_message_toplevel_to_purge);
    DB_Message_Store *pending_operating_db_message_store, *current_operating_db_message_store;
    if(db_message_toplevel_to_purge->First_Message){
        current_operating_db_message_store = db_message_toplevel_to_purge->First_Message;
        while(!current_operating_db_message_store){
            pending_operating_db_message_store = current_operating_db_message_store->Next_Message_Store;
            db_message_store_purge(current_operating_db_message_store);
            current_operating_db_message_store = pending_operating_db_message_store;
        }
    }
    free(db_message_toplevel_to_purge);
}

void db_contact_card_purge(DB_Contact_Card *db_contact_card_to_purge){
    assert(db_contact_card_to_purge);
    free(db_contact_card_to_purge->uid);
    free(db_contact_card_to_purge->nickname);
    free(db_contact_card_to_purge->email);
    DB_Contact_Card *Historical_Previous = db_contact_card_to_purge->Previous_Contact_Card;
    DB_Contact_Card *Historical_Next = db_contact_card_to_purge->Next_Contact_Card;
    if(db_contact_card_to_purge->Container_Toplevel->First_Contact == db_contact_card_to_purge){
        db_contact_card_to_purge->Container_Toplevel->First_Contact = Historical_Next;
    }
    if(db_contact_card_to_purge->Container_Toplevel->Last_Contact == db_contact_card_to_purge){
        db_contact_card_to_purge->Container_Toplevel->Last_Contact = Historical_Previous;
    }
    if(db_contact_card_to_purge->Previous_Contact_Card)
        db_contact_card_to_purge->Previous_Contact_Card->Next_Contact_Card = Historical_Next;
    if(db_contact_card_to_purge->Next_Contact_Card)
        db_contact_card_to_purge->Next_Contact_Card->Previous_Contact_Card = Historical_Previous;
    db_contact_card_to_purge->Container_Toplevel->member_number --;
    free(db_contact_card_to_purge);
}

void db_container_purge(DB_Container *db_container_to_purge){
    assert(db_container_to_purge);
    DB_Contact_Card *pending_operating_db_contact_card, *current_operating_db_contact_card;
    if(db_container_to_purge->First_Contact){
        current_operating_db_contact_card = db_container_to_purge->First_Contact;
        while(!current_operating_db_contact_card){
            pending_operating_db_contact_card = current_operating_db_contact_card->Next_Contact_Card;
            db_contact_card_purge(current_operating_db_contact_card);
            current_operating_db_contact_card = pending_operating_db_contact_card;
        }
    }
    if(db_container_to_purge->DB_Message_Toplevel){
        db_message_toplevel_purge(db_container_to_purge->DB_Message_Toplevel);
    }
    DB_Container *Historical_Previous = db_container_to_purge->Previous_Container;
    DB_Container *Historical_Next = db_container_to_purge->Next_Container;
    if(db_container_to_purge->DB_Toplevel->First_Container == db_container_to_purge){
        db_container_to_purge->DB_Toplevel->First_Container = Historical_Next;
    }
    if(db_container_to_purge->DB_Toplevel->Last_Container == db_container_to_purge){
        db_container_to_purge->DB_Toplevel->Last_Container = Historical_Previous;
    }
    if(db_container_to_purge->Previous_Container)
        db_container_to_purge->Previous_Container->Next_Container = Historical_Next;
    if(db_container_to_purge->Next_Container)
        db_container_to_purge->Next_Container->Previous_Container = Historical_Previous;
    db_container_to_purge->DB_Toplevel->container_length --;
    free(db_container_to_purge);
}

void db_toplevel_purge(DB_Toplevel *db_toplevel_to_purge){
    assert(db_toplevel_to_purge);
    #ifdef DEBUG
        fprintf(stderr, "I'm making this phishing pseudo function\n");
        fprintf(stderr, "to just ask you:\n");
        fprintf(stderr, "Do you have any idea about why you are calling this function\n");
        fprintf(stderr, "Have you ever thought about the severe consequence of deleting a top-level db\n");
        fprintf(stderr, "It might crash the entire GUI or result in unpredictable behaviors\n");
        fprintf(stderr, "I won't delete this db_toplevel thing for you this time\n");
        fprintf(stderr, "Please do something more meaningful such as making a pizza\n");
    #endif
}

/*Database related function ends*/

int is_digits_only(const char *stringtocheck){
    while (*stringtocheck) {
        if (isdigit(*stringtocheck++) == 0) return 0;
    }

    return 1;
}

int set_server_info(){
    strcpy(server_hostname, gtk_entry_get_text(GTK_ENTRY(g_entry_hostname)));
    if(is_digits_only(gtk_entry_get_text(GTK_ENTRY(g_entry_port))) == 0){
        show_notification_dialog(login_window, 8);
    }
    else{
        server_port_number = atoi(gtk_entry_get_text(GTK_ENTRY(g_entry_port)));
        Server = gethostbyname(server_hostname);
        if (Server == NULL){
            show_notification_dialog(login_window, 10);
        }
        else{
            if (server_port_number <= 2000 || server_port_number >65535){
                show_notification_dialog(login_window, 9);
            }
            else{
                #ifdef DEBUG
                fprintf(stderr, "Info: Setting server address to %s:%d\n",server_hostname, server_port_number);
                #endif
                ServerAddress.sin_family = AF_INET;
                ServerAddress.sin_port = htons(server_port_number);
                ServerAddress.sin_addr = *(struct in_addr*)Server->h_addr_list[0];
                
                show_notification_dialog(login_window, 7);
            }
        }
    }
    return 0;
}

static gboolean set_send_label_availability(int label_availability){
    char *markup_label_right_panel_send;
    if(label_availability){
        markup_label_right_panel_send = g_strconcat("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Condensed Bold 10\">", 
                                                    "Send", "</span>", NULL);
    }
    else{
        markup_label_right_panel_send = g_strconcat("<span foreground=\"#C0C0C0\" font_desc=\" DejaVu Sans Condensed Bold 10\">", 
                                                    "Send", "</span>", NULL);
    }
    gtk_label_set_markup(GTK_LABEL(g_label_right_panel_send), markup_label_right_panel_send);
    g_free(markup_label_right_panel_send);
    return TRUE;
}

gboolean append_emoji_to_textbuffer(char *emoji_text){//WORKING
    char emoji_path[100];
    if(strcmp(emoji_text,"[smile]") == 0){
        strcpy(emoji_path, "./assets/emoji/smile.png");
    }
    else if(strcmp(emoji_text,"[smirk]") == 0){
        strcpy(emoji_path, "./assets/emoji/smirk.png");
    }
    else if(strcmp(emoji_text,"[facepalm]") == 0){
        strcpy(emoji_path, "./assets/emoji/facepalm.png");
    }
    else if(strcmp(emoji_text,"[kiss]") == 0){
        strcpy(emoji_path, "./assets/emoji/kiss.png");
    }
    else if(strcmp(emoji_text,"[poop]") == 0){
        strcpy(emoji_path, "./assets/emoji/poop.png");
    }
    else if(strcmp(emoji_text,"[thinking]") == 0){
        strcpy(emoji_path, "./assets/emoji/thinking.png");
    }
    else{
        return FALSE;
    }
    GdkPixbuf *g_pixbuf_emoji;

    g_pixbuf_emoji = gdk_pixbuf_new_from_file(emoji_path, NULL);

    GtkTextIter g_textIter_ending;
    GtkTextIter g_textIter_beginning;
    gtk_text_buffer_get_end_iter(g_textbuffer_messages, &g_textIter_ending);
    gtk_text_buffer_get_start_iter(g_textbuffer_messages, &g_textIter_beginning);
    gtk_text_buffer_insert_pixbuf(g_textbuffer_messages,
                                    &g_textIter_ending,
                                    g_pixbuf_emoji);
    gtk_text_buffer_insert(g_textbuffer_messages, &g_textIter_ending, "\n", -1);
    return TRUE;
}

void fetch_contact_list_from_server(){
    //This function save the contact book from the server to the database
    DB_Container *db_container_new_instance;

    //TODO-Client: Currently contact list is hardcoded. 
    //This has to be linked with client API such as Talk2Server.
    //Below shows the example of how to create a new contact.
    //Forgetting to create DB_Message_Toplevel would cause unpredictable behavior!
    db_container_new_instance = db_container_new(db_toplevel_instance);
    db_contact_card_new(db_container_new_instance, "Bot1");
    db_message_toplevel_new(db_container_new_instance);

    db_container_new_instance = db_container_new(db_toplevel_instance);
    db_contact_card_new(db_container_new_instance, "Bot2");
    db_message_toplevel_new(db_container_new_instance);
    //Suppose there's a new unread message for testing purpose
    db_container_new_instance->has_unread_messages = 1;

    db_container_new_instance = db_container_new(db_toplevel_instance);
    db_contact_card_new(db_container_new_instance, "Bot3");
    db_message_toplevel_new(db_container_new_instance);

}

void refresh_treeview_from_db(int tick_tock){
    DB_Container *db_container_operating;
    char online_status_text[12];
    char indicator_text[12];

    gtk_list_store_clear(GTK_LIST_STORE(g_liststore_friends));
    
    db_container_operating = db_toplevel_instance->First_Container;
    while(db_container_operating){
        if(db_container_operating->is_group_chat == 0){
            if(db_container_operating->First_Contact->online_status == 1){
                strcpy(online_status_text, "Online");
            }
            else if(db_container_operating->First_Contact->online_status == 0){
                strcpy(online_status_text, "Offline");
            }
            else if(db_container_operating->First_Contact->online_status == 2){
                strcpy(online_status_text, "Busy");
            }

            if(db_container_operating->has_unread_messages && !tick_tock){
                strcpy(indicator_text, " •••");
            }
            else{
                strcpy(indicator_text, "      ");
            }

            gtk_list_store_append(g_liststore_friends, &g_treeIter_frends);
            gtk_list_store_set (g_liststore_friends, &g_treeIter_frends,
                                COL_indicator, indicator_text,
                                COL_username, db_container_operating->First_Contact->nickname,
                                COL_online_status, online_status_text,
                                -1);
        }
        db_container_operating = db_container_operating->Next_Container;
    }
}

DB_Container *get_ungrouped_db_container_with_uid(char *uid_to_search){
    //Returns NULL if no result was found
    DB_Container *db_container_operating;
    db_container_operating = db_toplevel_instance->First_Container;
    while(db_container_operating){
        if(db_container_operating->is_group_chat == 0 
            && strcmp(db_container_operating->First_Contact->uid, uid_to_search) == 0){
            break;
        }
        db_container_operating = db_container_operating->Next_Container;
    }
    return db_container_operating;
}

DB_Container *get_ungrouped_db_container_with_nickname(char *nickname_to_search){
    //Returns NULL if no result was found
    DB_Container *db_container_operating;
    db_container_operating = db_toplevel_instance->First_Container;
    while(db_container_operating){
        if(db_container_operating->is_group_chat == 0
            && strcmp(db_container_operating->First_Contact->nickname, nickname_to_search) == 0){
            break;
        }
        db_container_operating = db_container_operating->Next_Container;
    }
    return db_container_operating;
}

void refresh_textbuffer_from_db_with_nickname(char *nickname_to_display){
    DB_Container *db_container_with_nickname = get_ungrouped_db_container_with_nickname(nickname_to_display);
    GtkTextIter g_textIter_ending;
    GtkTextIter g_textIter_beginning;
    gtk_text_buffer_get_end_iter(g_textbuffer_messages, &g_textIter_ending);
    gtk_text_buffer_get_start_iter(g_textbuffer_messages, &g_textIter_beginning);
    gtk_text_buffer_delete (g_textbuffer_messages,
                            &g_textIter_beginning,
                            &g_textIter_ending);
    
    if(db_container_with_nickname == NULL){
        #ifdef DEBUG
        fprintf(stderr, "CRITICAL: Unable to display messages from %s\n", nickname_to_display);
        #endif
        return;
    }

    char message_status_text[20];
    char time_stamp_text[20];
    char new_line_text[40];
    gchar *textbuffer_message_to_append;
    time_t time_stamp;

    DB_Message_Store *db_message_store_operating;
    db_message_store_operating = db_container_with_nickname->DB_Message_Toplevel->First_Message;

    if(!db_message_store_operating){
        textbuffer_message_to_append = g_strconcat("You have added ", 
                                                    nickname_to_display,
                                                    " as your friend.\nStart chatting!", NULL);
        gtk_text_buffer_get_end_iter(g_textbuffer_messages, &g_textIter_ending);
        gtk_text_buffer_insert(g_textbuffer_messages,
                                &g_textIter_ending,
                                textbuffer_message_to_append,
                                -1);
    }
    while(db_message_store_operating){
        if(db_message_store_operating->message_type == MESSAGE_TYPE_TEXT){
            strcpy(message_status_text, "");
            if(db_message_store_operating->status == MESSAGE_STATUS_FAILED_TO_SEND){
                strcpy(message_status_text, "[ ! ] ");
            }
            time_stamp = db_message_store_operating->unix_time_stamp;
            struct tm *timestamp_text_struct = gmtime(&time_stamp);
            if(timestamp_text_struct->tm_hour + CURRENT_TIMEZONE_OFFSET < 0){
                timestamp_text_struct->tm_hour = timestamp_text_struct->tm_hour + 24;
            }
            snprintf(time_stamp_text, sizeof(time_stamp_text), "[%2d:%02d:%02d] ", 
                        timestamp_text_struct->tm_hour + CURRENT_TIMEZONE_OFFSET, 
                        timestamp_text_struct->tm_min, 
                        timestamp_text_struct->tm_sec);
            if(!db_message_store_operating->Previous_Message_Store){
                strcpy(new_line_text, "--Beginning of the chat--\n");
            }
            else{
                strcpy(new_line_text, "\n");
            }
            textbuffer_message_to_append = g_strconcat(new_line_text, message_status_text, time_stamp_text,
                                                        db_message_store_operating->from_uid, ": \n", 
                                                        db_message_store_operating->base64_content, NULL);
            
            gtk_text_buffer_get_end_iter(g_textbuffer_messages, &g_textIter_ending);
            gtk_text_buffer_insert(g_textbuffer_messages,
                                   &g_textIter_ending,
                                   textbuffer_message_to_append,
                                   -1);
        }
        else if(db_message_store_operating->message_type == MESSAGE_TYPE_EMOJI){
            strcpy(message_status_text, "");
            if(db_message_store_operating->status == MESSAGE_STATUS_FAILED_TO_SEND){
                strcpy(message_status_text, "[ ! ] ");
            }
            time_stamp = db_message_store_operating->unix_time_stamp;
            struct tm *timestamp_text_struct = gmtime(&time_stamp);
            if(timestamp_text_struct->tm_hour + CURRENT_TIMEZONE_OFFSET < 0){
                timestamp_text_struct->tm_hour = timestamp_text_struct->tm_hour + 24;
            }
            snprintf(time_stamp_text, sizeof(time_stamp_text), "[%2d:%02d:%02d] ", 
                        timestamp_text_struct->tm_hour + CURRENT_TIMEZONE_OFFSET, 
                        timestamp_text_struct->tm_min, 
                        timestamp_text_struct->tm_sec);
            if(!db_message_store_operating->Previous_Message_Store){
                strcpy(new_line_text, "--Beginning of the chat--\n");
            }
            else{
                strcpy(new_line_text, "\n");
            }
            textbuffer_message_to_append = g_strconcat(new_line_text, message_status_text, time_stamp_text,
                                                        db_message_store_operating->from_uid, ": \n", NULL);
            
            gtk_text_buffer_get_end_iter(g_textbuffer_messages, &g_textIter_ending);
            gtk_text_buffer_insert(g_textbuffer_messages,
                                   &g_textIter_ending,
                                   textbuffer_message_to_append,
                                   -1);
            //WORKING
            append_emoji_to_textbuffer(db_message_store_operating->base64_content);
        }
        
        db_message_store_operating = db_message_store_operating->Next_Message_Store;
    }

    //gtk_widget_queue_draw(g_textview_right_panel);
    gtk_adjustment_set_value(g_adjustment_textview, gtk_adjustment_get_upper(g_adjustment_textview));

}

//Done: The most important thing is to get the iter or the cursor from GtkTreeview

void *chat_daemon_thread(void *args){
    Thread_Arguments *thread_arguments_instance = (Thread_Arguments *)args;

    static int tick_tock = 0;
    char *incoming_message_from_uid;
    //char *incoming_message_to_uid;
    char *incoming_message_raw_content;
    int incoming_message_type;
    int incoming_message_status;
    char label_sender_text[30];
    char *decoded_message_content;

    //struct timespec tim, tim2;
    //tim.tv_sec  = 0;
    //tim.tv_nsec = 300000000L;//That's 0.3 sec

    gboolean if_no_new_messages = TRUE;
    decoded_message_content = NULL;
    if(!thread_arguments_instance){
        #ifdef DEBUG
        fprintf(stderr, "Info[Thread]: No argument passed to the thread\n");
        #endif
    }
    while(TRUE){
        while(TRUE){
            if(if_no_new_messages){
                //We don't want the client to ask the server too frequently
                //nanosleep(&tim , &tim2);
                sleep(1);

                tick_tock = 1- tick_tock;
                gdk_threads_enter();
                refresh_treeview_from_db(tick_tock);
                gdk_threads_leave();
            }

            #ifdef DEBUG
            fprintf(stderr, "Info[Thread]: Sending PING message to server...\n");
            #endif
            struct message ping_message;
            strcpy(ping_message.opcode, PING); 
            strcpy(ping_message.user, db_contact_card_me_instance->uid);

            struct message ping_recieved = Talk2Server(ping_message, ServerAddress);
            if(strcmp(ping_recieved.opcode, FAIL) != 0) { //messages are in the queue, ping until its flushed
                do {
                    #ifdef DEBUG
                    fprintf(stderr, "Info[Thread]: Received an incoming message with opcode %c\n", ping_recieved.opcode[0]);
                    #endif

                    if_no_new_messages = FALSE;
                    incoming_message_from_uid = ping_recieved.user;
                    //incoming_message_to_uid = ping_recieved.destuser;
                    incoming_message_raw_content = ping_recieved.content;

                    switch(ping_recieved.opcode[0]){
                        case 'j': //CHESSMOVE
                            //TODO-Chess
                            break;

                        case 'k': //SENDFRIENDREQUEST
                            tick_tock = 0;
                            //Get GDK thread lock to solve "[xcb] Unknown request in queue while dequeuing" error
                            gdk_threads_enter();
                            show_accept_contact_window(&ping_recieved);
                            gdk_threads_leave();
                            break;

                        case 'l': //ACCEPTFRIENDREQUEST
                            tick_tock = 0;
                            gdk_threads_enter();

                            DB_Container *db_container_new_instance;

                            db_container_new_instance = db_container_new(db_toplevel_instance);
                            db_contact_card_new(db_container_new_instance, incoming_message_from_uid);
                            db_message_toplevel_new(db_container_new_instance);
                            //Create a little badge to notify the user
                            db_container_new_instance->has_unread_messages = 1;
                            refresh_treeview_from_db(0);

                            show_request_accepted_window(&ping_recieved);

                            gdk_threads_leave();
                            break;

                        case 'e': //MESSAGE
                            //We assume the message has been successgully delivered. This can be changed with the support of client API
                            incoming_message_status = MESSAGE_STATUS_SUCCESSFUL_TO_SEND;
                            if(incoming_message_raw_content[0] == 't'){
                                incoming_message_type = MESSAGE_TYPE_TEXT;
                                decoded_message_content = &incoming_message_raw_content[2];
                            }
                            else if(incoming_message_raw_content[0] == 'e'){
                                incoming_message_type = MESSAGE_TYPE_EMOJI;
                                decoded_message_content = &incoming_message_raw_content[2];
                            }
                            //Handling incoming messages
                            
                            //Get GDK thread lock to prevent unexpected behavior
                            gdk_threads_enter();
                            DB_Container *db_container_with_sender = get_ungrouped_db_container_with_nickname(incoming_message_from_uid);
                            
                            if(!db_container_with_sender){
                                //In case the contact does not exist in DB
                                #ifdef DEBUG
                                fprintf(stderr, "Info[Thread]: Creating DB_Container for %s\n", incoming_message_from_uid);
                                #endif
                                DB_Container *db_container_new_instance;

                                //Forgetting to create DB_Message_Toplevel would cause unpredictable behavior!
                                db_container_new_instance = db_container_new(db_toplevel_instance);
                                db_contact_card_new(db_container_new_instance, incoming_message_from_uid);
                                db_message_toplevel_new(db_container_new_instance);
                                //Create a little badge to notify the user
                                db_container_new_instance->has_unread_messages = 1;
                                refresh_treeview_from_db(0);
                                db_container_with_sender = get_ungrouped_db_container_with_nickname(incoming_message_from_uid);
                                
                            }

                            db_message_store_new(db_container_with_sender->DB_Message_Toplevel, 
                                                    incoming_message_type,
                                                    decoded_message_content,
                                                    incoming_message_from_uid,
                                                    incoming_message_status,
                                                    (unsigned long)time(NULL));
                            
                            strcpy(label_sender_text, gtk_label_get_text(GTK_LABEL(g_label_sender)));
                            if(!strcmp(label_sender_text, incoming_message_from_uid)){
                                refresh_textbuffer_from_db_with_nickname(label_sender_text);
                                gtk_adjustment_set_value(g_adjustment_textview, gtk_adjustment_get_upper(g_adjustment_textview));
                            }
                            else{
                                tick_tock = 0;
                                //Done: Notify user that there's a new message from a non-selected contact
                                db_container_with_sender->has_unread_messages = 1;
                                refresh_treeview_from_db(0);
                            }
                            //Release GDK thread lock
                            gdk_threads_leave();
                            break;

                    }
                    //Do not change if_no_new_messages to FALSE right now

                }while (strcmp(ping_recieved.remainingmessages, MORE) == 0); 
                
                
            } else { //no messages in the queue for this ping
                #ifdef DEBUG
                fprintf(stderr, "Info[Thread]: No incoming message. Thread will keep asking...\n");
                #endif
                if_no_new_messages = TRUE;
            }
        }
        
    }
}

//TODO: Add a variable int has_been_read in DB_Container_struct. 1 for normal and 0 for highlighting

static gboolean on_entry_message_changed(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    //This function checks if the message is valid to send and set the sensitivity of the button accordingly

    char entry_message_text[MAXIMUM_MESSAGE_LENGTH];
    char label_sender_text[30];
    strcpy(entry_message_text, gtk_entry_get_text(GTK_ENTRY(g_entry_message)));
    strcpy(label_sender_text, gtk_label_get_text(GTK_LABEL(g_label_sender)));

    if((!strcmp(entry_message_text,"")) || (!strcmp(label_sender_text,""))){
        #ifdef DEBUG
        fprintf(stderr, "Info: Graying out Send label...\n");
        #endif
        gtk_entry_set_text(GTK_ENTRY(g_entry_message), "");
        set_send_label_availability(0);
    }

    else{
        //In this case the message is OK to send
        set_send_label_availability(1);
    }
    return TRUE;
}

static gboolean on_image_chess_button_press_event(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    #ifdef DEBUG
    fprintf(stderr, "Info: Chess icon clicked\n");
    #endif
    
    //TODO: Chess integration definitely needs more work to make it work along with client API
    system("bin/chess");

    return TRUE;
}

static gboolean on_image_remove_button_press_event(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    #ifdef DEBUG
    fprintf(stderr, "Info: Remove contact icon clicked\n");
    #endif

    char label_sender_text[30];
    strcpy(label_sender_text, gtk_label_get_text(GTK_LABEL(g_label_sender)));
    if(!strcmp(label_sender_text, "")){
        show_notification_dialog(chat_window, 12);
        return TRUE;
    }
    show_confirmation_window();
    return TRUE;
}

static gboolean on_eventbox_apply_settings_button_press_event(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    #ifdef DEBUG
    fprintf(stderr, "Info: Apply Settings clicked\n");
    #endif
    set_server_info();
    gtk_widget_destroy(settings_window_widget);

    return TRUE;
}

static gboolean on_eventbox_image_left_panel_avatar_button_press_event(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    #ifdef DEBUG
    fprintf(stderr, "Info: User avatar or nickname clicked\n");
    #endif
    show_profile_window();
    return TRUE;
}

static gboolean on_eventbox_add_button_press_event(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    #ifdef DEBUG
    fprintf(stderr, "Info: Add contact clicked\n");
    #endif
    show_add_contact_window();
    return TRUE;
}

static gboolean on_button_send_clicked(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    //Function for sending messages
    char entry_message_text[MAXIMUM_MESSAGE_LENGTH];
    char label_sender_text[30];
    strcpy(entry_message_text, gtk_entry_get_text(GTK_ENTRY(g_entry_message)));
    strcpy(label_sender_text, gtk_label_get_text(GTK_LABEL(g_label_sender)));
    if((!strcmp(entry_message_text,"")) || (!strcmp(label_sender_text,""))){
        #ifdef DEBUG
        fprintf(stderr, "Info: Nothing to send\n");
        #endif
        set_send_label_availability(0);
        return TRUE;
    }
    else{
        char *encoded_message = g_strconcat("t~", entry_message_text, NULL);
        //Put the message into a message struct and pass it to the client API HERE
        struct message p2p_message;
        strcpy(p2p_message.opcode, MESSAGE); 
        strcpy(p2p_message.user, db_contact_card_me_instance->uid);
        strcpy(p2p_message.destuser, label_sender_text);
        strcpy(p2p_message.content, encoded_message);
        #ifdef DEBUG
        fprintf(stderr, "Info: From: %s(%s) To %s\n", db_contact_card_me_instance->nickname,
                db_contact_card_me_instance->uid, label_sender_text);
        fprintf(stderr, "Info: Content: %s\n", encoded_message);
        fprintf(stderr, "Info: Message has been packed and ready to send\n");
        fprintf(stderr, "Info: Calling Talk2Server API...\n\n");
        #endif

        struct message recieved = Talk2Server(p2p_message, ServerAddress);
        int new_message_status;
        if(strcmp(recieved.opcode, SUCCESS) == 0){
            #ifdef DEBUG
            fprintf(stderr, "Info: Message has been successfully sent\n");
            #endif
            new_message_status = MESSAGE_STATUS_SUCCESSFUL_TO_SEND;
        }
        else {
            //In case the message is failed to send
            //Unfortunately GTK2 does not support markup in textbuffers
            //Done: create a db_message_store to save messages

            new_message_status = MESSAGE_STATUS_FAILED_TO_SEND;
            show_notification_dialog(chat_window, 11);
        }
        //Append the message to db
        DB_Container *db_container_with_sender = get_ungrouped_db_container_with_nickname(label_sender_text);
        #ifdef DEBUG
        if(!db_container_with_sender){
            fprintf(stderr, "CRITICAL: Unable to get DB_Container with nickname %s\n", label_sender_text);
        }
        #endif
        db_message_store_new(db_container_with_sender->DB_Message_Toplevel, MESSAGE_TYPE_TEXT,//WORKING
                                entry_message_text,
                                db_contact_card_me_instance->uid,
                                new_message_status,
                                (unsigned long)time(NULL));
    }
    refresh_textbuffer_from_db_with_nickname(label_sender_text);
    gtk_adjustment_set_value(g_adjustment_textview, gtk_adjustment_get_upper(g_adjustment_textview));
    gtk_entry_set_text(GTK_ENTRY(g_entry_message), "");
    set_send_label_availability(0);
    return TRUE;
}

static void on_treeview_left_panel_row_activated(GtkTreeView *treeview, 
                                GtkTreePath *path, 
                                GtkTreeViewColumn *column,
                                gpointer userdata){
    gchar *g_selected_nickname;

    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);

    if (gtk_tree_model_get_iter(model, &iter, path)){
        char *markup_label_sender;
        gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, 1, &g_selected_nickname, -1);
        #ifdef DEBUG
        fprintf(stderr, "Contact with nickname %s has been selected\n", g_selected_nickname);
        #endif
        markup_label_sender = g_strconcat("<span foreground=\"#808080\" font_desc=\" DejaVu Sans Extralight 20\">", 
                                            g_selected_nickname, "</span>", NULL);
        gtk_label_set_markup(GTK_LABEL(g_label_sender), markup_label_sender);
        
        //Clear message entry when switching contacts
        gtk_entry_set_text(GTK_ENTRY(g_entry_message), "");

        //Clear the badge for the selected user
        DB_Container *current_db_container_instance;
        current_db_container_instance = get_ungrouped_db_container_with_nickname(g_selected_nickname);
        current_db_container_instance->has_unread_messages = 0;
        refresh_treeview_from_db(0);
        
        //Here is the part where Textbuffer refreshes its content
        refresh_textbuffer_from_db_with_nickname(g_selected_nickname);
        gtk_adjustment_set_value(g_adjustment_textview, gtk_adjustment_get_upper(g_adjustment_textview));
   }
}

static gboolean on_eventbox_register_button_press_event(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_label_set_text(GTK_LABEL(g_label_welcome), "Register");
    strcpy(db_contact_card_me_instance->uid, gtk_entry_get_text(GTK_ENTRY(g_entry_username)));
    strcpy(passwd, gtk_entry_get_text(GTK_ENTRY(g_entry_password)));
    #ifdef DEBUG
    fprintf(stderr, "Info: get username %s\n",db_contact_card_me_instance->uid);
    fprintf(stderr, "Info: get password %s\n",passwd);
    #endif
    if(strlen(db_contact_card_me_instance->uid) < 2){
        show_notification_dialog(login_window, 6);
    }
    else if(is_digits_only(passwd) == 1){
        show_notification_dialog(login_window, 5);
    }
    else if(strlen(passwd) < 7){
        show_notification_dialog(login_window, 4);
    }
    else {
        //This is the part where the GUI send the registration message via client API
        struct message registration;
        strcpy(registration.opcode, REGISTER); 
        strcpy(registration.user, db_contact_card_me_instance->uid);
        strcpy(registration.content, passwd);
        #ifdef DEBUG
        fprintf(stderr, "Info: Registration information has been packed and is ready to send\n");
        fprintf(stderr, "Info: Calling Talk2Server API...\n\n");
        #endif
        struct message recieved = Talk2Server(registration, ServerAddress);
        if(strcmp(recieved.opcode, SUCCESS) == 0){
            show_notification_dialog(login_window, 2);
        } else {
            show_notification_dialog(login_window, 3);
        }
    }

    return TRUE;
}

static gboolean on_eventbox_settings_button_press_event(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    show_settings_window();
    return TRUE;
}

static gboolean on_eventbox_login_button_press_event(__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){

    gtk_label_set_text(GTK_LABEL(g_label_welcome), "Logging in");
    strcpy(db_contact_card_me_instance->uid, gtk_entry_get_text(GTK_ENTRY(g_entry_username)));
    strcpy(passwd, gtk_entry_get_text(GTK_ENTRY(g_entry_password)));
    #ifdef DEBUG
    fprintf(stderr, "Info: get username %s\n", db_contact_card_me_instance->uid);
    fprintf(stderr, "Info: get password %s\n", passwd);
    #endif

    //This is the part where the GUI send the login message via client API
    struct message login;
    strcpy(login.opcode, LOGIN); 
    strcpy(login.user, db_contact_card_me_instance->uid);
    strcpy(login.content, passwd);

    #ifdef DEBUG
    fprintf(stderr, "Info: Login information has been packed and is ready to send\n");
    fprintf(stderr, "Info: Calling Talk2Server API...\n\n");
    #endif
    struct message recieved = Talk2Server(login, ServerAddress);
    fprintf(stderr, "DEBUG %s %d...\n", recieved.opcode, strcmp(recieved.opcode, SUCCESS));
    if(strcmp(recieved.opcode, SUCCESS)){
        show_notification_dialog(login_window, 1);
    }
    else{
        #ifdef DEBUG
        fprintf(stderr, "Info: logging in...\n");
        #endif
        gtk_main_quit();
        gtk_window_set_destroy_with_parent(login_window, TRUE);
        gtk_widget_destroy(window_widget);
        
        /*TODO: Right now the uid will be copied as nickname when a DB_Contact_Card instance is created.
        What we might implement in the future is to allow user to have their customized nickname
        This would be stored to db_contact_card->Nickname with strcpy.
        Thus, in the future, the nickname would be fetched *from the server* instead of copying from uid.
        */
        strcpy(db_contact_card_me_instance->nickname,db_contact_card_me_instance->uid);
        show_chat_window();
    }
    
    return TRUE;
}

static gboolean on_eventbox_ok_to_remove_button_press_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    char label_sender_text[30];
    strcpy(label_sender_text, gtk_label_get_text(GTK_LABEL(g_label_sender)));

    //TODO-Client: Send a message to server via client API
    //to tell the server to remove contact
    //Contact to remove: "label_sender_text" 
    //Who am I: "db_contact_card_me_instance->uid"

    DB_Container *db_container_with_sender = get_ungrouped_db_container_with_nickname(label_sender_text);
    if(!db_container_with_sender){
        #ifdef DEBUG
        fprintf(stderr, "CRITICAL: Unable to find db_container with uid %s to remove\n", label_sender_text);
        #endif
    }
    db_container_purge(db_container_with_sender);

    gtk_label_set_markup(GTK_LABEL(g_label_sender), "");
    gtk_entry_set_text(GTK_ENTRY(g_entry_message), "");
    set_send_label_availability(0);
    refresh_treeview_from_db(0);
    GtkTextIter g_textIter_ending;
    GtkTextIter g_textIter_beginning;
    char *default_textbuffer_text;
    gtk_text_buffer_get_end_iter(g_textbuffer_messages, &g_textIter_ending);
    gtk_text_buffer_get_start_iter(g_textbuffer_messages, &g_textIter_beginning);
    gtk_text_buffer_delete (g_textbuffer_messages,
                            &g_textIter_beginning,
                            &g_textIter_ending);
    default_textbuffer_text = g_strconcat("Start chatting by double-tapping a contact on the left panel.", NULL);
    gtk_text_buffer_get_end_iter(g_textbuffer_messages, &g_textIter_ending);
    gtk_text_buffer_insert(g_textbuffer_messages,
                                &g_textIter_ending,
                                default_textbuffer_text,
                                -1);
    gtk_widget_destroy(confirmation_window_widget);
    return TRUE;
}

static gboolean on_eventbox_image_save_log_button_press_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    show_save_log_browser_dialog();
    return TRUE;
}

static gboolean on_button_save_clicked (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    char *log_history_file_path;
    gchar *g_textbuffer_text;

    log_history_file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_log_browser_dialog_widget));

    if(strcmp(log_history_file_path, "")){
        #ifdef DEBUG
        fprintf(stderr, "Saving chat history to %s...\n", log_history_file_path);
        #endif

        GtkTextIter g_textIter_beginning;
        GtkTextIter g_textIter_ending;
        gtk_text_buffer_get_start_iter(g_textbuffer_messages, &g_textIter_beginning);
        gtk_text_buffer_get_end_iter(g_textbuffer_messages, &g_textIter_ending);
        g_textbuffer_text = gtk_text_buffer_get_text(g_textbuffer_messages, &g_textIter_beginning, &g_textIter_ending, FALSE); 

        g_file_set_contents(log_history_file_path, g_textbuffer_text, -1, NULL);

        g_free(g_textbuffer_text);
        gtk_widget_destroy(save_log_browser_dialog_widget);
        show_notification_dialog(chat_window, 16);
    }
    else{
        gtk_widget_destroy(save_log_browser_dialog_widget);
        show_notification_dialog(chat_window, 17);
    }
    return TRUE;
}

static gboolean on_eventbox_label_accept_request_button_press_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    char requesting_from_uid[30];
    strcpy(requesting_from_uid, gtk_label_get_text(GTK_LABEL(g_label_requesting_from_uid)));
    #ifdef DEBUG
    fprintf(stderr, "Info: Got %s...\n", requesting_from_uid);
    #endif

    gtk_widget_destroy(accept_contact_window_widget);

    struct message accept_friend_request_message;
    strcpy(accept_friend_request_message.opcode, ACCEPTFRIENDREQUEST);
    strcpy(accept_friend_request_message.user, db_contact_card_me_instance->uid);
    strcpy(accept_friend_request_message.destuser, requesting_from_uid);
    strcpy(accept_friend_request_message.content, "");
    #ifdef DEBUG
    fprintf(stderr, "Info: From: %s(%s) To %s\n", db_contact_card_me_instance->nickname,
            accept_friend_request_message.user, accept_friend_request_message.destuser);
    fprintf(stderr, "Info: \"Accept friend request\" message has been packed and ready to send\n");
    fprintf(stderr, "Info: Calling Talk2Server API...\n\n");
    #endif

    struct message recieved = Talk2Server(accept_friend_request_message, ServerAddress);

    if(strcmp(recieved.opcode, SUCCESS) == 0){
        #ifdef DEBUG
        fprintf(stderr, "Info: Request for accepting friend has been successfully sent\n");
        #endif

        DB_Container *db_container_new_instance;
        //Forgetting to create DB_Message_Toplevel would cause unpredictable behavior!
        db_container_new_instance = db_container_new(db_toplevel_instance);
        db_contact_card_new(db_container_new_instance, accept_friend_request_message.destuser);
        db_message_toplevel_new(db_container_new_instance);
        //Create a cute badge for the newly added user
        db_container_new_instance->has_unread_messages = 1;

        refresh_treeview_from_db(0);

        gtk_widget_destroy(add_contact_window_widget);
    }
    else{
        show_notification_dialog(GTK_WINDOW(add_contact_window), 0);
    }
    return TRUE;
}

static gboolean on_eventbox_image_emoji_button_press_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    char *emoji_text = data;
    char label_sender_text[30];
    strcpy(label_sender_text, gtk_label_get_text(GTK_LABEL(g_label_sender)));
    gtk_widget_destroy(emoji_chooser_window_widget);
    //WORKING
    if(!strcmp(label_sender_text, "")){
        show_notification_dialog(chat_window, 18);
        return TRUE;
    }
    char *encoded_message = g_strconcat("e~", emoji_text, NULL);
    //Put the message into a message struct and pass it to the client API HERE
    struct message p2p_message;
    strcpy(p2p_message.opcode, MESSAGE); 
    strcpy(p2p_message.user, db_contact_card_me_instance->uid);
    strcpy(p2p_message.destuser, label_sender_text);
    strcpy(p2p_message.content, encoded_message);
    #ifdef DEBUG
    fprintf(stderr, "Info: From: %s(%s) To %s\n", db_contact_card_me_instance->nickname,
            db_contact_card_me_instance->uid, label_sender_text);
    fprintf(stderr, "Info: Content: %s\n", encoded_message);
    fprintf(stderr, "Info: Emoji message has been packed and ready to send\n");
    fprintf(stderr, "Info: Calling Talk2Server API...\n\n");
    #endif

    struct message recieved = Talk2Server(p2p_message, ServerAddress);
    int new_message_status;
    if(strcmp(recieved.opcode, SUCCESS) == 0){
        #ifdef DEBUG
        fprintf(stderr, "Info: Emoji has been successfully sent\n");
        #endif
        new_message_status = MESSAGE_STATUS_SUCCESSFUL_TO_SEND;
    }
    else {
        //In case the message is failed to send

        new_message_status = MESSAGE_STATUS_FAILED_TO_SEND;
        show_notification_dialog(chat_window, 11);
    }
    //Append the emoji message to db
    DB_Container *db_container_with_sender = get_ungrouped_db_container_with_nickname(label_sender_text);
    #ifdef DEBUG
    if(!db_container_with_sender){
        fprintf(stderr, "CRITICAL: Unable to get DB_Container with nickname %s\n", label_sender_text);
    }
    #endif
    db_message_store_new(db_container_with_sender->DB_Message_Toplevel, MESSAGE_TYPE_EMOJI,
                        emoji_text,
                        db_contact_card_me_instance->uid,
                        new_message_status,
                        (unsigned long)time(NULL));
    refresh_textbuffer_from_db_with_nickname(label_sender_text);
    gtk_adjustment_set_value(g_adjustment_textview, gtk_adjustment_get_upper(g_adjustment_textview));
    return TRUE;
}

static gboolean on_eventbox_label_add_contact_button_press_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    char entry_add_contact_text[30];
    char entry_attached_messages_text[80];
    strcpy(entry_attached_messages_text, gtk_entry_get_text(GTK_ENTRY(g_entry_attached_messages)));
    strcpy(entry_add_contact_text, gtk_entry_get_text(GTK_ENTRY(g_entry_add_contact)));
    //TODO: entry_add_contact_text cannot be blank

    struct message add_contact_message;
    strcpy(add_contact_message.opcode, SENDFRIENDREQUEST);
    strcpy(add_contact_message.user, db_contact_card_me_instance->uid);
    strcpy(add_contact_message.destuser, entry_add_contact_text);
    strcpy(add_contact_message.content, entry_attached_messages_text);
    #ifdef DEBUG
    fprintf(stderr, "Info: From: %s(%s) To %s\n", db_contact_card_me_instance->nickname,
            db_contact_card_me_instance->uid, entry_add_contact_text);
    fprintf(stderr, "Info: Content: %s\n", entry_attached_messages_text);
    fprintf(stderr, "Info: Add contact request message has been packed and ready to send\n");
    fprintf(stderr, "Info: Calling Talk2Server API...\n\n");
    #endif

    struct message recieved = Talk2Server(add_contact_message, ServerAddress);

    if(strcmp(recieved.opcode, SUCCESS) == 0){
        #ifdef DEBUG
        fprintf(stderr, "Info: Request has been successfully sent\n");
        #endif
        show_notification_dialog(GTK_WINDOW(chat_window), 13);

        gtk_widget_destroy(add_contact_window_widget);
    }
    else{
        show_notification_dialog(GTK_WINDOW(add_contact_window), 14);
    }

    return TRUE;
}

static gboolean on_eventbox_image_emoji_chooser_button_press_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    show_emoji_chooser_window();
    return TRUE;
}

static gboolean on_linkbutton_delete_account_pressed (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    //TODO: prompt a comfirmation dialog
    //Then tell the server to remove this user
    //Then exit the program
    show_notification_dialog(GTK_WINDOW(add_contact_window), 15);
    gtk_main_quit();
    exit(0);
    return TRUE;
}

static gboolean on_login_window_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_main_quit();
    return TRUE;
}

static gboolean on_request_accepted_window_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_destroy(request_accepted_window_widget);
    return TRUE;
}

static gboolean on_emoji_chooser_window_widget_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_destroy(emoji_chooser_window_widget);
    return TRUE;
}

static gboolean on_save_log_browser_dialog_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_destroy(save_log_browser_dialog_widget);
    return TRUE;
}

static gboolean on_confirmation_window_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_destroy(confirmation_window_widget);
    return TRUE;
}

static gboolean on_add_contact_window_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_destroy(add_contact_window_widget);
    return TRUE;
}

static gboolean on_profile_window_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_destroy(profile_window_widget);
    return TRUE;
}

static gboolean on_settings_window_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_destroy(settings_window_widget);
    return TRUE;
}

static gboolean on_accept_contact_window_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_destroy(accept_contact_window_widget);
    return TRUE;
}

static gboolean on_chat_window_destroy_event (__attribute__((unused))GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_main_quit();
    return TRUE;
}

void show_settings_window(){
    char *markup_label_apply_settings = g_strconcat("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Condensed Bold 10\">", 
                                                    "Apply", "</span>", NULL);
    builder_settings = gtk_builder_new();
    if (!gtk_builder_add_from_file (builder_settings, "./assets/settings_aqua.xml", NULL)){
        fprintf(stderr, "Error adding settings xml file\n");
    }

    settings_window_widget = GTK_WIDGET(gtk_builder_get_object(builder_settings, "settings_window"));
    if (settings_window_widget == NULL){
        fprintf(stderr, "Unable to file object with id \"settings_window\" \n");
    }

    settings_window = GTK_WINDOW(gtk_builder_get_object(builder_settings, "settings_window"));

    g_entry_hostname = GTK_WIDGET(gtk_builder_get_object(builder_settings, "entry_hostname"));
    g_entry_port = GTK_WIDGET(gtk_builder_get_object(builder_settings, "entry_port"));
    g_label_apply_settings = GTK_WIDGET(gtk_builder_get_object(builder_settings, "lable_apply_settings"));
    g_eventbox_apply_settings = GTK_WIDGET(gtk_builder_get_object(builder_settings, "eventbox_apply_settings"));
    
    gtk_widget_modify_bg(settings_window_widget, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_apply_settings, GTK_STATE_NORMAL, &g_widget_background_color);

    g_signal_connect(settings_window, "destroy", G_CALLBACK(on_settings_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_apply_settings, "button-press-event", G_CALLBACK(on_eventbox_apply_settings_button_press_event),NULL);
    //Hit Enter or Ctrl+Enter to apply settings
    g_signal_connect(g_eventbox_apply_settings, "composited-changed", G_CALLBACK(on_eventbox_apply_settings_button_press_event),NULL);

    //BUG: This is the part where pango does not do its job well. Still figuring out why...
    gtk_label_set_markup(GTK_LABEL(g_label_apply_settings), markup_label_apply_settings);

    g_object_unref(builder_settings);
    g_free(markup_label_apply_settings);

    char port_number_string[6];
    sprintf(port_number_string, "%d", server_port_number);
    gtk_entry_set_text(GTK_ENTRY(g_entry_hostname), server_hostname);
    gtk_entry_set_text(GTK_ENTRY(g_entry_port), port_number_string);

    gtk_widget_show_all(settings_window_widget);
}

void show_notification_dialog(GtkWindow *parent, gint statuscode){
    GtkWidget *dialog;
    GtkWidget *dialog_label;
    GtkWidget *dialog_icon;
    GtkWidget *dialog_hbox;
    char dialog_label_text[80];
    char dialog_title[60];
    switch(statuscode){
        case 0:
            strcpy(dialog_label_text,"We are experiencing some connectivity issue. Try again later.");
            strcpy(dialog_title,"Communication Failed");
            break;
        case 1:
            strcpy(dialog_label_text,"We are unable to sign you in due to incorrect username or password.");
            strcpy(dialog_title,"Login Failed");
            break;
        case 2:
            strcpy(dialog_label_text,"We have successfully created your account.");
            strcpy(dialog_title,"Registration Succeeded");
            break;
        case 3:
            strcpy(dialog_label_text,"The username has already been taken. Try something different.");
            strcpy(dialog_title,"Registration Failed");
            break;
        case 4:
            strcpy(dialog_label_text,"Your password is too short. Try something longer.");
            strcpy(dialog_title,"Registration Failed");
            break;
        case 5:
            strcpy(dialog_label_text,"The complexity of the password does not meet the requirements.");
            strcpy(dialog_title,"Registration Failed");
            break;
        case 6:
            strcpy(dialog_label_text,"Your username is too short. Try something longer.");
            strcpy(dialog_title,"Registration Failed");
            break;
        case 7:
            strcpy(dialog_label_text,"Server settings has been successfully applied.");
            strcpy(dialog_title,"Settings Applied");
            break;
        case 8:
            strcpy(dialog_label_text,"The port number has to be a number.");
            strcpy(dialog_title,"Failed to Apply Settings");
            break;
        case 9:
            strcpy(dialog_label_text,"The port number must be between 2000 and 65535.");
            strcpy(dialog_title,"Failed to Apply Settings");
            break;
        case 10:
            strcpy(dialog_label_text,"We could not find server with specified hostname.");
            strcpy(dialog_title,"Failed to Apply Settings");
            break;
        case 11:
            strcpy(dialog_label_text,"We are unable to deliver your message. Try again later.");
            strcpy(dialog_title,"Failed to Send Message");
            break;
        case 12:
            strcpy(dialog_label_text,"Please double-click a contact to remove.");
            strcpy(dialog_title,"Unable to Remove Contact");
            break;
        case 13:
            strcpy(dialog_label_text,"We have successfully sent your request for adding contact.");
            strcpy(dialog_title,"Add Contact Request Sent");
            break;
        case 14:
            strcpy(dialog_label_text,"We are unable to deliver your request for adding contact.");
            strcpy(dialog_title,"Unable to Send Your Request");
            break;
        case 15:
            strcpy(dialog_label_text,"Your have been signed out. Chat will quit soon.");
            strcpy(dialog_title,"Account Deleted");
            break;
        case 16:
            strcpy(dialog_label_text,"Your current chat history has been successfully saved.");
            strcpy(dialog_title,"Chat History Saved");
            break;
        case 17:
            strcpy(dialog_label_text,"Your chat history has not been saved.");
            strcpy(dialog_title,"Chat History");
            break;
        case 18:
            strcpy(dialog_label_text,"Please select a contact from the left panel.");
            strcpy(dialog_title,"Unable to Send Emoji");
            break;
        
    }
    if(statuscode == 2){
        strcpy(db_contact_card_me_instance->nickname, db_contact_card_me_instance->uid);
        show_profile_window();
        gtk_label_set_text(GTK_LABEL(g_label_welcome), "Welcome");
    }
    else{
        dialog = gtk_dialog_new_with_buttons(dialog_title,parent,GTK_DIALOG_DESTROY_WITH_PARENT,GTK_STOCK_OK,GTK_RESPONSE_OK,NULL);
        gtk_dialog_set_has_separator(GTK_DIALOG(dialog),FALSE);
        dialog_label = gtk_label_new(dialog_label_text);

        dialog_icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,GTK_ICON_SIZE_DIALOG);

        dialog_hbox = gtk_hbox_new(FALSE,5);
        gtk_container_set_border_width(GTK_CONTAINER(dialog_hbox),60);
        gtk_box_pack_start_defaults(GTK_BOX(dialog_hbox),dialog_icon);
        gtk_box_pack_start_defaults(GTK_BOX(dialog_hbox),dialog_label);
        gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),dialog_hbox);

        gtk_widget_modify_bg(dialog, GTK_STATE_NORMAL, &g_widget_background_color);

        gtk_widget_show_all(dialog);

        gtk_dialog_run(GTK_DIALOG(dialog));

        if(g_label_welcome && (!g_treeview_left_panel_friends)){
            gtk_label_set_text(GTK_LABEL(g_label_welcome), "Welcome");
        }
        gtk_widget_destroy(dialog);
    }
}

void show_confirmation_window(){
    char label_sender_text[30];
    strcpy(label_sender_text, gtk_label_get_text(GTK_LABEL(g_label_sender)));
    char *markup_label_uid_to_remove = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Extralight 20\">", label_sender_text, "</span>", NULL);
    char *markup_label_ok_to_remove = g_strconcat ("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Bold 10\">", "Affirmative", "</span>", NULL);
    char *markup_label_cancel_removing = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Bold 10\">", 
                                                     "Wait don't!", "</span>", NULL);
    
    builder_confirmation = gtk_builder_new();

    if (!gtk_builder_add_from_file (builder_confirmation, "./assets/confirmation_aqua.xml", NULL)){
        fprintf(stderr, "Error adding confirmation xml file\n");
    }

    confirmation_window_widget = GTK_WIDGET(gtk_builder_get_object(builder_confirmation, "confirmation_window"));
    if (confirmation_window_widget == NULL){
        fprintf(stderr, "Unable to file object with id \"confirmation_window\" \n");
    }

    confirmation_window = GTK_WINDOW(gtk_builder_get_object(builder_confirmation, "confirmation_window"));

    g_label_uid_to_remove = GTK_WIDGET(gtk_builder_get_object(builder_confirmation, "label_uid_to_remove"));
    g_eventbox_ok_to_remove = GTK_WIDGET(gtk_builder_get_object(builder_confirmation, "eventbox_ok_to_remove"));
    g_label_ok_to_remove = GTK_WIDGET(gtk_builder_get_object(builder_confirmation, "label_ok_to_remove"));
    g_eventbox_cancel_removing = GTK_WIDGET(gtk_builder_get_object(builder_confirmation, "eventbox_cancel_removing"));
    g_label_cancel_removing = GTK_WIDGET(gtk_builder_get_object(builder_confirmation, "label_cancel_removing"));

    gtk_widget_modify_bg(confirmation_window_widget, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_ok_to_remove, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_cancel_removing, GTK_STATE_NORMAL, &g_widget_background_color);

    g_signal_connect(confirmation_window, "destroy", G_CALLBACK(on_confirmation_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_cancel_removing, "button-press-event", G_CALLBACK(on_confirmation_window_destroy_event),NULL);
    //Keyboard shortcut is supported!
    //Hit Enter or Space to remove contact
    //Hit Ctrl+C to cancel
    g_signal_connect(g_eventbox_cancel_removing, "composited-changed",G_CALLBACK(on_confirmation_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_ok_to_remove, "composited-changed", G_CALLBACK(on_eventbox_ok_to_remove_button_press_event),NULL);
    g_signal_connect(g_eventbox_ok_to_remove, "button-press-event", G_CALLBACK(on_eventbox_ok_to_remove_button_press_event),NULL);

    g_object_unref(builder_confirmation);

    gtk_label_set_markup(GTK_LABEL(g_label_uid_to_remove), markup_label_uid_to_remove);
    gtk_label_set_markup(GTK_LABEL(g_label_ok_to_remove), markup_label_ok_to_remove);
    gtk_label_set_markup(GTK_LABEL(g_label_cancel_removing), markup_label_cancel_removing);

    g_free(markup_label_uid_to_remove);
    g_free(markup_label_ok_to_remove);
    g_free(markup_label_cancel_removing);

    gtk_widget_show_all(confirmation_window_widget);
}

void show_profile_window(){
    char *markup_label_dismiss = g_strconcat ("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Bold 10\">", "OK", "</span>", NULL);
    char *markup_label_delete_account = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans ExtraLight 8\">", 
                                                     "Delete my Account", "</span>", NULL);
    
    builder_profile = gtk_builder_new();

    if (!gtk_builder_add_from_file (builder_profile, "./assets/profile_aqua.xml", NULL)){
        fprintf(stderr, "Error adding profile xml file\n");
    }

    profile_window_widget = GTK_WIDGET(gtk_builder_get_object(builder_profile, "profile_window"));
    if (profile_window_widget == NULL){
        fprintf(stderr, "Unable to file object with id \"profile_window\" \n");
    }

    profile_window = GTK_WINDOW(gtk_builder_get_object(builder_profile, "profile_window"));

    g_label_uid = GTK_WIDGET(gtk_builder_get_object(builder_profile, "label_uid"));
    g_label_profile_slogan = GTK_WIDGET(gtk_builder_get_object(builder_profile, "label_profile_slogan"));
    g_image_avatar = GTK_WIDGET(gtk_builder_get_object(builder_profile, "image_avatar"));
    g_label_dismiss = GTK_WIDGET(gtk_builder_get_object(builder_profile, "label_dismiss"));
    g_label_delete_account = GTK_WIDGET(gtk_builder_get_object(builder_profile, "label_delete_account"));
    g_eventbox_dismiss = GTK_WIDGET(gtk_builder_get_object(builder_profile, "eventbox_dismiss"));
    g_eventbox_delete_account = GTK_WIDGET(gtk_builder_get_object(builder_profile, "eventbox_delete_account"));
    g_hseparator_1 = GTK_WIDGET(gtk_builder_get_object(builder_profile, "hseparator_1"));

    gtk_widget_modify_bg(profile_window_widget, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_dismiss, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_delete_account, GTK_STATE_NORMAL, &g_widget_background_color);

    g_signal_connect(profile_window, "destroy", G_CALLBACK(on_profile_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_dismiss, "button-press-event", G_CALLBACK(on_profile_window_destroy_event),NULL);
    //Keyboard shortcut is supported!
    //Hit Enter or Space to dismiss the window
    //Hit Ctrl+D or Del to delete account
    g_signal_connect(g_eventbox_dismiss, "composited-changed",G_CALLBACK(on_profile_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_delete_account, "composited-changed", G_CALLBACK(on_linkbutton_delete_account_pressed),NULL);
    g_signal_connect(g_eventbox_delete_account, "button-press-event", G_CALLBACK(on_linkbutton_delete_account_pressed),NULL);

    g_object_unref(builder_profile);
    
    gtk_label_set_markup(GTK_LABEL(g_label_dismiss), markup_label_dismiss);
    gtk_label_set_markup(GTK_LABEL(g_label_delete_account), markup_label_delete_account);
    gtk_label_set_text(GTK_LABEL(g_label_uid),db_contact_card_me_instance->nickname);

    g_free(markup_label_delete_account);
    g_free(markup_label_dismiss);

    if(!g_entry_message){
        gtk_label_set_text(GTK_LABEL(g_label_profile_slogan), "Your profile has been created!");
        gtk_widget_destroy(g_label_delete_account);
        gtk_widget_destroy(g_eventbox_delete_account);
        gtk_widget_destroy(g_hseparator_1);
    }
    gtk_widget_show_all(profile_window_widget);
}

void show_emoji_chooser_window(){
    char *markup_g_label_emoji_chooser_title = g_strconcat("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Condensed Extralight 20\">", 
                                                    "Emoji Gallery", "</span>", NULL);
    char *markup_g_label_emoji_cancel = g_strconcat("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Condensed Bold 10\">", 
                                                    "Go Back", "</span>", NULL);
    
    builder_emoji_chooser = gtk_builder_new();
    if (!gtk_builder_add_from_file (builder_emoji_chooser, "./assets/emoji_chooser_aqua.xml", NULL)){
        fprintf(stderr, "Error adding save emoji chooser xml file\n");
    }
    
    emoji_chooser_window_widget = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "emoji_chooser_window"));
    emoji_chooser_window = GTK_WINDOW(gtk_builder_get_object(builder_emoji_chooser, "emoji_chooser_window"));

    g_eventbox_image_emoji_1 = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "eventbox_image_emoji_1"));
    g_eventbox_image_emoji_2 = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "eventbox_image_emoji_2"));
    g_eventbox_image_emoji_3 = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "eventbox_image_emoji_3"));
    g_eventbox_image_emoji_4 = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "eventbox_image_emoji_4"));
    g_eventbox_image_emoji_5 = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "eventbox_image_emoji_5"));
    g_eventbox_image_emoji_6 = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "eventbox_image_emoji_6"));
    g_eventbox_label_emoji_cancel = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "eventbox_label_emoji_cancel"));
    g_label_emoji_cancel = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "label_emoji_cancel"));
    g_label_emoji_chooser_title = GTK_WIDGET(gtk_builder_get_object(builder_emoji_chooser, "label_emoji_chooser_title"));

    gtk_widget_modify_bg(emoji_chooser_window_widget, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_emoji_1, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_emoji_2, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_emoji_3, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_emoji_4, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_emoji_5, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_emoji_6, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_emoji_cancel, GTK_STATE_NORMAL, &g_widget_background_color);

    g_signal_connect(emoji_chooser_window_widget, "destroy", G_CALLBACK(on_emoji_chooser_window_widget_destroy_event),NULL);
    g_signal_connect(g_eventbox_label_emoji_cancel, "button-press-event", G_CALLBACK(on_emoji_chooser_window_widget_destroy_event),NULL);
    g_signal_connect(g_eventbox_image_emoji_1, "button-press-event", G_CALLBACK(on_eventbox_image_emoji_button_press_event), "[smile]");
    g_signal_connect(g_eventbox_image_emoji_2, "button-press-event", G_CALLBACK(on_eventbox_image_emoji_button_press_event), "[smirk]");
    g_signal_connect(g_eventbox_image_emoji_3, "button-press-event", G_CALLBACK(on_eventbox_image_emoji_button_press_event), "[facepalm]");
    g_signal_connect(g_eventbox_image_emoji_4, "button-press-event", G_CALLBACK(on_eventbox_image_emoji_button_press_event), "[kiss]");
    g_signal_connect(g_eventbox_image_emoji_5, "button-press-event", G_CALLBACK(on_eventbox_image_emoji_button_press_event), "[poop]");
    g_signal_connect(g_eventbox_image_emoji_6, "button-press-event", G_CALLBACK(on_eventbox_image_emoji_button_press_event), "[thinking]");
    
    gtk_label_set_markup(GTK_LABEL(g_label_emoji_chooser_title), markup_g_label_emoji_chooser_title);
    gtk_label_set_markup(GTK_LABEL(g_label_emoji_cancel), markup_g_label_emoji_cancel);
    g_free(markup_g_label_emoji_chooser_title);
    g_free(markup_g_label_emoji_cancel);

    g_object_unref(builder_emoji_chooser);
    gtk_widget_show_all(emoji_chooser_window_widget);
}

void show_save_log_browser_dialog(){
    builder_save_log_browser = gtk_builder_new();
    if (!gtk_builder_add_from_file (builder_save_log_browser, "./assets/save_chat_log_dialog.xml", NULL)){
    fprintf(stderr, "Error adding save chat history xml file\n");
    }
    
    save_log_browser_dialog_widget = GTK_WIDGET(gtk_builder_get_object(builder_save_log_browser, "save_log_browser_dialog"));
    save_log_browser_dialog = GTK_DIALOG(gtk_builder_get_object(builder_save_log_browser, "save_log_browser_dialog"));

    g_button_cancel_saving = GTK_WIDGET(gtk_builder_get_object(builder_save_log_browser, "button_cancel_saving"));
    g_button_save = GTK_WIDGET(gtk_builder_get_object(builder_save_log_browser, "button_save"));

    gtk_widget_modify_bg(save_log_browser_dialog_widget, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_button_cancel_saving, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_button_save, GTK_STATE_NORMAL, &g_widget_background_color);

    g_signal_connect(save_log_browser_dialog_widget, "destroy", G_CALLBACK(on_save_log_browser_dialog_destroy_event),NULL);
    g_signal_connect(g_button_cancel_saving, "clicked", G_CALLBACK(on_save_log_browser_dialog_destroy_event),NULL);
    g_signal_connect(g_button_save, "clicked", G_CALLBACK(on_button_save_clicked),NULL);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save_log_browser_dialog_widget), 
                                        g_strconcat("My Chat History with ", 
                                                    gtk_label_get_text(GTK_LABEL(g_label_sender)),
                                                    ".txt", NULL));

    g_object_unref(builder_save_log_browser);
    gtk_widget_show_all(save_log_browser_dialog_widget);
}

void show_request_accepted_window(Message_struct *Request_Accepted_Instance){
    char *markup_label_request_accepted_uid = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Extralight 20\">", Request_Accepted_Instance->user, "</span>", NULL);
    char *markup_label_request_accepted_title = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Regular 10\">", "has accepted your request", "</span>", NULL);
    char *markup_label_request_accepted_ok = g_strconcat ("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Bold 10\">", "Let's Chat Now!", "</span>", NULL);

    builder_request_accepted = gtk_builder_new();

    if (!gtk_builder_add_from_file (builder_request_accepted, "./assets/request_accepted_aqua.xml", NULL)){
        fprintf(stderr, "Error adding request accepted xml file\n");
    }

    request_accepted_window_widget = GTK_WIDGET(gtk_builder_get_object(builder_request_accepted, "request_accepted_window"));

    request_accepted_window = GTK_WINDOW(gtk_builder_get_object(builder_request_accepted, "request_accepted_window"));

    g_label_request_accepted_uid = GTK_WIDGET(gtk_builder_get_object(builder_request_accepted, "label_request_accepted_uid"));
    g_label_request_accepted_title = GTK_WIDGET(gtk_builder_get_object(builder_request_accepted, "label_request_accepted_title"));
    g_eventbox_label_request_accepted_ok = GTK_WIDGET(gtk_builder_get_object(builder_request_accepted, "eventbox_label_request_accepted_ok"));
    g_label_request_accepted_ok = GTK_WIDGET(gtk_builder_get_object(builder_request_accepted, "label_request_accepted_ok"));

    gtk_widget_modify_bg(request_accepted_window_widget, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_request_accepted_ok, GTK_STATE_NORMAL, &g_widget_background_color);

    g_signal_connect(request_accepted_window, "destroy", G_CALLBACK(on_request_accepted_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_label_request_accepted_ok, "button-press-event", G_CALLBACK(on_request_accepted_window_destroy_event),NULL);
    //Keyboard shortcut support
    g_signal_connect(g_eventbox_label_request_accepted_ok, "composited-changed",G_CALLBACK(on_accept_contact_window_destroy_event),NULL);

    g_object_unref(builder_request_accepted);

    gtk_label_set_markup(GTK_LABEL(g_label_request_accepted_uid), markup_label_request_accepted_uid);
    gtk_label_set_markup(GTK_LABEL(g_label_request_accepted_title), markup_label_request_accepted_title);
    gtk_label_set_markup(GTK_LABEL(g_label_request_accepted_ok), markup_label_request_accepted_ok);

    g_free(markup_label_request_accepted_uid);
    g_free(markup_label_request_accepted_title);
    g_free(markup_label_request_accepted_ok);

    //Create a cute badge for the newly added user
    DB_Container *current_db_container_instance;
    current_db_container_instance = get_ungrouped_db_container_with_nickname(Request_Accepted_Instance->user);
    current_db_container_instance->has_unread_messages = 1;
    refresh_treeview_from_db(0);

    gtk_widget_show_all(request_accepted_window_widget);
}

void show_accept_contact_window(Message_struct *Friend_Request_Instance){
    char *markup_label_request_title = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Regular 10\">", "would like to add you as friend", "</span>", NULL);
    char *markup_label_requesting_from_uid = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Extralight 20\">", Friend_Request_Instance->user, "</span>", NULL);
    char *markup_label_request_attached_message = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Regular 10\">", Friend_Request_Instance->content, "</span>", NULL);
    char *markup_label_accept_request = g_strconcat ("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Bold 10\">", "Accept Unwillingly", "</span>", NULL);
    char *markup_label_decline_request = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Bold 10\">", 
                                                     "Decline Gracefully", "</span>", NULL);
    
    builder_accept_contact = gtk_builder_new();

    if (!gtk_builder_add_from_file (builder_accept_contact, "./assets/accept_contact_request_aqua.xml", NULL)){
        fprintf(stderr, "Error adding accept contact xml file\n");
    }

    accept_contact_window_widget = GTK_WIDGET(gtk_builder_get_object(builder_accept_contact, "accept_contact_window"));
    if (accept_contact_window_widget == NULL){
        fprintf(stderr, "Unable to file object with id \"accept_contact_window\" \n");
    }

    accept_contact_window = GTK_WINDOW(gtk_builder_get_object(builder_accept_contact, "accept_contact_window"));

    g_label_request_title = GTK_WIDGET(gtk_builder_get_object(builder_accept_contact, "label_request_title"));
    g_label_requesting_from_uid = GTK_WIDGET(gtk_builder_get_object(builder_accept_contact, "label_requesting_from_uid"));
    g_label_request_attached_message = GTK_WIDGET(gtk_builder_get_object(builder_accept_contact, "label_request_attached_message"));
    g_eventbox_label_accept_request = GTK_WIDGET(gtk_builder_get_object(builder_accept_contact, "eventbox_label_accept_request"));
    g_eventbox_label_decline_request = GTK_WIDGET(gtk_builder_get_object(builder_accept_contact, "eventbox_label_decline_request"));
    g_label_accept_request = GTK_WIDGET(gtk_builder_get_object(builder_accept_contact, "label_accept_request"));
    g_label_decline_request = GTK_WIDGET(gtk_builder_get_object(builder_accept_contact, "label_decline_request"));

    gtk_widget_modify_bg(accept_contact_window_widget, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_accept_request, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_decline_request, GTK_STATE_NORMAL, &g_widget_background_color);

    g_signal_connect(accept_contact_window, "destroy", G_CALLBACK(on_accept_contact_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_label_decline_request, "button-press-event", G_CALLBACK(on_accept_contact_window_destroy_event),NULL);
    //Keyboard shortcut is supported!
    //Hit Enter or Space to remove contact
    //Hit Ctrl+C to cancel
    g_signal_connect(g_eventbox_label_decline_request, "composited-changed",G_CALLBACK(on_accept_contact_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_label_accept_request, "composited-changed", G_CALLBACK(on_eventbox_label_accept_request_button_press_event),NULL);//BUG
    g_signal_connect(g_eventbox_label_accept_request, "button-press-event", G_CALLBACK(on_eventbox_label_accept_request_button_press_event),NULL);//BUG

    g_object_unref(builder_accept_contact);

    gtk_label_set_markup(GTK_LABEL(g_label_request_title), markup_label_request_title);
    gtk_label_set_markup(GTK_LABEL(g_label_requesting_from_uid), markup_label_requesting_from_uid);
    gtk_label_set_markup(GTK_LABEL(g_label_request_attached_message), markup_label_request_attached_message);
    gtk_label_set_markup(GTK_LABEL(g_label_accept_request), markup_label_accept_request);
    gtk_label_set_markup(GTK_LABEL(g_label_decline_request), markup_label_decline_request);

    g_free(markup_label_request_title);
    g_free(markup_label_requesting_from_uid);
    g_free(markup_label_request_attached_message);
    g_free(markup_label_accept_request);
    g_free(markup_label_decline_request);

    gtk_widget_show_all(accept_contact_window_widget);
}

void show_add_contact_window(){
    char *markup_label_add_contact_title = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Extralight 25\">", "Add Contact", "</span>", NULL);
    char *markup_label_add_contact = g_strconcat ("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Bold 10\">", "Add this guy!", "</span>", NULL);
    char *markup_cancel_add_contact = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans Bold 10\">", 
                                                     "Cancel", "</span>", NULL);
    
    builder_add_contact = gtk_builder_new();

    if (!gtk_builder_add_from_file (builder_add_contact, "./assets/add_contact_aqua.xml", NULL)){
        fprintf(stderr, "Error adding add_contact xml file\n");
    }

    add_contact_window_widget = GTK_WIDGET(gtk_builder_get_object(builder_add_contact, "add_contact_window"));
    if (add_contact_window_widget == NULL){
        fprintf(stderr, "Unable to file object with id \"add_contact_window\" \n");
    }

    add_contact_window = GTK_WINDOW(gtk_builder_get_object(builder_add_contact, "add_contact_window"));
    
    g_label_add_contact_title = GTK_WIDGET(gtk_builder_get_object(builder_add_contact, "label_add_contact_title"));
    g_entry_add_contact = GTK_WIDGET(gtk_builder_get_object(builder_add_contact, "entry_add_contact"));
    g_eventbox_label_add_contact = GTK_WIDGET(gtk_builder_get_object(builder_add_contact, "eventbox_label_add_contact"));
    g_label_add_contact = GTK_WIDGET(gtk_builder_get_object(builder_add_contact, "label_add_contact"));
    g_eventbox_label_cancel_add_contact = GTK_WIDGET(gtk_builder_get_object(builder_add_contact, "eventbox_label_cancel_add_contact"));
    g_label_cancel_add_contact = GTK_WIDGET(gtk_builder_get_object(builder_add_contact, "label_cancel_add_contact"));
    g_entry_attached_messages = GTK_WIDGET(gtk_builder_get_object(builder_add_contact, "entry_attached_messages"));

    gtk_widget_modify_bg(add_contact_window_widget, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_add_contact, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_cancel_add_contact, GTK_STATE_NORMAL, &g_widget_background_color);

    g_signal_connect(add_contact_window, "destroy", G_CALLBACK(on_add_contact_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_label_cancel_add_contact, "button-press-event", G_CALLBACK(on_add_contact_window_destroy_event),NULL);
    //Keyboard shortcut is supported!
    //Hit Enter to confirm
    //Hit Ctrl+C to cancel
    g_signal_connect(g_eventbox_label_cancel_add_contact, "composited-changed",G_CALLBACK(on_add_contact_window_destroy_event),NULL);
    g_signal_connect(g_eventbox_label_add_contact, "composited-changed", G_CALLBACK(on_eventbox_label_add_contact_button_press_event),NULL);
    g_signal_connect(g_eventbox_label_add_contact, "button-press-event", G_CALLBACK(on_eventbox_label_add_contact_button_press_event),NULL);

    g_object_unref(builder_add_contact);

    gtk_label_set_markup(GTK_LABEL(g_label_add_contact_title), markup_label_add_contact_title);
    gtk_label_set_markup(GTK_LABEL(g_label_add_contact), markup_label_add_contact);
    gtk_label_set_markup(GTK_LABEL(g_label_cancel_add_contact), markup_cancel_add_contact);

    g_free(markup_label_add_contact_title);
    g_free(markup_label_add_contact);
    g_free(markup_cancel_add_contact);

    gtk_widget_show_all(add_contact_window_widget);
}

void show_chat_window(){
    builder_chat = gtk_builder_new();
    char *markup_label_add = g_strconcat("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Condensed Bold 10\">", "Add contact", "</span>", NULL);
    char *markup_label_left_panel_1 = g_strconcat("<span foreground=\"#A0A0A0\" font_desc=\" DejaVu Sans Extralight 20\">", 
                                                    db_contact_card_me_instance->nickname, "</span>", NULL);
    char *markup_label_left_panel_2 = g_strconcat("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Bold 8\">", "Online", "</span>", NULL);
    char *markup_label_sender = g_strconcat("<span foreground=\"#808080\" font_desc=\" DejaVu Sans Extralight 20\">", "", "</span>", NULL);
    
    Thread_Arguments thread_arguments_instance;
    GError *g_error_thread = NULL;

    if (!gtk_builder_add_from_file (builder_chat, "./assets/chat_aqua.xml", NULL)){
        fprintf(stderr, "Error adding xml file\n");
    }

    chat_window_widget = GTK_WIDGET(gtk_builder_get_object(builder_chat, "chat_window"));
    if (chat_window_widget == NULL){
        fprintf(stderr, "Unable to file object with id \"chat_window\" \n");
    }

    chat_window = GTK_WINDOW(gtk_builder_get_object(builder_chat, "chat_window"));
    //For widgets on the left panel
    g_image_left_panel_avatar = GTK_WIDGET(gtk_builder_get_object(builder_chat, "image_left_panel_avatar"));
    g_eventbox_left_panel_image_avatar = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_left_panel_image_avatar"));
    g_label_left_panel_1 = GTK_WIDGET(gtk_builder_get_object(builder_chat, "label_left_panel_1"));
    g_eventbox_label_left_panel_1 = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_label_left_panel_1"));
    g_label_left_panel_2 = GTK_WIDGET(gtk_builder_get_object(builder_chat, "label_left_panel_2"));
    g_eventbox_label_left_panel_2 = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_label_left_panel_2"));
    g_label_add = GTK_WIDGET(gtk_builder_get_object(builder_chat, "label_add"));
    g_eventbox_add = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_add"));
    g_treeview_left_panel_friends = GTK_WIDGET(gtk_builder_get_object(builder_chat, "treeview_left_panel_friends"));

    //For widgets on the right panel
    g_eventbox_label_sender = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_label_sender"));
    g_label_sender = GTK_WIDGET(gtk_builder_get_object(builder_chat, "label_sender"));
    g_textview_right_panel = GTK_WIDGET(gtk_builder_get_object(builder_chat, "textview_right_panel"));

    g_adjustment_textview = GTK_ADJUSTMENT(gtk_builder_get_object(builder_chat, "adjustment_textview"));

    g_label_right_panel_send = GTK_WIDGET(gtk_builder_get_object(builder_chat, "label_right_panel_send"));
    g_eventbox_label_right_panel_send = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_label_right_panel_send"));
    g_entry_message = GTK_WIDGET(gtk_builder_get_object(builder_chat, "entry_message"));
    g_image_remove = GTK_WIDGET(gtk_builder_get_object(builder_chat, "image_remove"));
    g_image_chess = GTK_WIDGET(gtk_builder_get_object(builder_chat, "image_chess"));
    g_eventbox_image_remove = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_image_remove"));
    g_eventbox_image_chess = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_image_chess"));
    g_image_save_log = GTK_WIDGET(gtk_builder_get_object(builder_chat, "image_save_log"));
    g_eventbox_image_save_log = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_image_save_log"));
    g_eventbox_image_emoji_chooser = GTK_WIDGET(gtk_builder_get_object(builder_chat, "eventbox_image_emoji_chooser"));

    g_liststore_friends = GTK_LIST_STORE(gtk_builder_get_object(builder_chat, "liststore_friends"));
    g_textbuffer_messages = GTK_TEXT_BUFFER(gtk_builder_get_object(builder_chat, "textbuffer_messages"));

    //Labels and images are No_Window widgets. Thus do not require setting bg
    gtk_widget_modify_bg(chat_window_widget, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_emoji_chooser, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_left_panel_image_avatar, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_left_panel_1, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_left_panel_2, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_add, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_treeview_left_panel_friends, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_sender, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_textview_right_panel, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_label_right_panel_send, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_remove, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_chess, GTK_STATE_NORMAL, &g_widget_background_color);
    gtk_widget_modify_bg(g_eventbox_image_save_log, GTK_STATE_NORMAL, &g_widget_background_color);

    gtk_label_set_markup(GTK_LABEL(g_label_add), markup_label_add);
    gtk_label_set_markup(GTK_LABEL(g_label_left_panel_1), markup_label_left_panel_1);
    gtk_label_set_markup(GTK_LABEL(g_label_left_panel_2), markup_label_left_panel_2);
    gtk_label_set_markup(GTK_LABEL(g_label_sender), markup_label_sender);
    set_send_label_availability(0);
    
    g_signal_connect(chat_window, "destroy",G_CALLBACK(on_chat_window_destroy_event),NULL);

    g_signal_connect(g_eventbox_left_panel_image_avatar, "button-press-event", G_CALLBACK(on_eventbox_image_left_panel_avatar_button_press_event),NULL);
    g_signal_connect(g_eventbox_label_left_panel_1, "button-press-event", G_CALLBACK(on_eventbox_image_left_panel_avatar_button_press_event),NULL);
    //Perhaps the next release would support changing online status
    g_signal_connect(g_eventbox_label_left_panel_2, "button-press-event", G_CALLBACK(on_eventbox_image_left_panel_avatar_button_press_event),NULL);
    g_signal_connect(g_eventbox_add, "button-press-event", G_CALLBACK(on_eventbox_add_button_press_event),NULL);

    g_signal_connect(g_entry_message, "changed", G_CALLBACK(on_entry_message_changed),NULL);

    g_signal_connect(g_eventbox_image_emoji_chooser, "button-press-event", G_CALLBACK(on_eventbox_image_emoji_chooser_button_press_event),NULL);
    g_signal_connect(g_eventbox_image_chess, "button-press-event", G_CALLBACK(on_image_chess_button_press_event),NULL);
    g_signal_connect(g_eventbox_label_right_panel_send, "button-press-event", G_CALLBACK(on_button_send_clicked),NULL);
    
    //Keyboard Shortcut support is finally here!
    //Hit Enter or Ctrl+Enter to send a message
    g_signal_connect(g_eventbox_label_right_panel_send, "composited-changed", G_CALLBACK(on_button_send_clicked),NULL);

    g_signal_connect(g_treeview_left_panel_friends, "row-activated", G_CALLBACK(on_treeview_left_panel_row_activated),NULL);
    g_signal_connect(g_eventbox_image_remove, "button-release-event", G_CALLBACK(on_image_remove_button_press_event),NULL);

    g_signal_connect(g_eventbox_image_save_log, "button-release-event", G_CALLBACK(on_eventbox_image_save_log_button_press_event),NULL);

    g_object_unref(builder_chat);
    gtk_widget_show_all(chat_window_widget);

    g_treeselection_friends = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_treeview_left_panel_friends));
    //Exactly one contact can be selected
    gtk_tree_selection_set_mode(g_treeselection_friends, GTK_SELECTION_BROWSE);
    //g_signal_connect(g_treeselection_friends, "changed", G_CALLBACK(on_treeselection_friends_changed),NULL);
    
    fetch_contact_list_from_server();
    refresh_treeview_from_db(0);

    g_free(markup_label_add);
    g_free(markup_label_left_panel_1);
    g_free(markup_label_left_panel_2);
    g_free(markup_label_sender);

    #if __GNUC__<= 4
    if(!g_thread_create(chat_daemon_thread, &thread_arguments_instance, FALSE, &g_error_thread)){
        #ifdef DEBUG
        fprintf(stderr, "CRITICAL: Unable to create thread: %s\n", g_error_thread->message);
        #endif
    }

    #else
    if(!g_thread_try_new("Fetch_Messages_Thread", chat_daemon_thread, &thread_arguments_instance, &g_error_thread)){
        #ifdef DEBUG
        fprintf(stderr, "CRITICAL: Unable to create thread: %s\n", g_error_thread->message);
        #endif
    }
    #endif

    gtk_main();//This is the main loop for the chat window
}

void show_login_window(){
        //This is the part where the top-level db sould be created and "Me" DB_Contact_Card to be filled
        db_toplevel_instance = db_toplevel_new();
        db_container_me_instance = db_container_new(db_toplevel_instance);
        db_toplevel_instance->container_length = 0;
        db_toplevel_instance->First_Container = NULL;
        db_toplevel_instance->Last_Container = NULL;
        db_contact_card_me_instance = db_contact_card_new(db_container_me_instance, "");

        
        gdk_color_parse("#F5F6F7", &g_widget_background_color);//This is default in Ubuntu GTK3 theme

        builder_login = gtk_builder_new();
        //Define the style of labels here
        //#55B3E9 is one of the theme colors in Ubuntu 18.10
        char *markup_label_login = g_strconcat ("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans Bold 10\">", "Login", "</span>", NULL);
        char *markup_label_register = g_strconcat ("<span foreground=\"#55B3E9\" font_desc=\" DejaVu Sans ExtraLight 10\">", "Register", "</span>", NULL);
        char *markup_label_settings = g_strconcat ("<span foreground=\"#606060\" font_desc=\" DejaVu Sans ExtraLight 8\">", "Settings", "</span>", NULL);
        
        int builder_return;
        builder_return = gtk_builder_add_from_file (builder_login, "./assets/login_aqua.xml",NULL);
        if (!builder_return){
            fprintf(stderr, "Error adding xml file. code: %d\n", builder_return);
        }

        window_widget = GTK_WIDGET(gtk_builder_get_object(builder_login, "login_window"));
        if (window_widget == NULL)
        {
            fprintf(stderr, "Unable to file object with id \"login_window\" \n");
        }
        login_window = GTK_WINDOW(gtk_builder_get_object(builder_login, "login_window"));
        g_label_welcome = GTK_WIDGET(gtk_builder_get_object(builder_login, "label_welcome"));
        g_entry_username = GTK_WIDGET(gtk_builder_get_object(builder_login, "entry_username"));
        g_entry_password = GTK_WIDGET(gtk_builder_get_object(builder_login, "entry_password"));

        g_eventbox_login = GTK_WIDGET(gtk_builder_get_object(builder_login, "eventbox_login"));
        g_eventbox_register = GTK_WIDGET(gtk_builder_get_object(builder_login, "eventbox_register"));
        g_label_register = GTK_WIDGET(gtk_builder_get_object(builder_login, "label_register"));
        g_label_login = GTK_WIDGET(gtk_builder_get_object(builder_login, "label_login"));
        g_eventbox_settings = GTK_WIDGET(gtk_builder_get_object(builder_login, "eventbox_settings"));
        g_label_settings = GTK_WIDGET(gtk_builder_get_object(builder_login, "label_settings"));
        
        //Apply label styles with pango
        gtk_label_set_markup(GTK_LABEL(g_label_login), markup_label_login);
        gtk_label_set_markup(GTK_LABEL(g_label_register), markup_label_register);
        gtk_label_set_markup(GTK_LABEL(g_label_settings), markup_label_settings);
        gtk_builder_connect_signals(builder_login, NULL);
        
        gtk_widget_modify_bg(window_widget, GTK_STATE_NORMAL, &g_widget_background_color);
        gtk_widget_modify_bg(g_eventbox_register, GTK_STATE_NORMAL, &g_widget_background_color);
        gtk_widget_modify_bg(g_eventbox_login, GTK_STATE_NORMAL, &g_widget_background_color);
        gtk_widget_modify_bg(g_eventbox_settings, GTK_STATE_NORMAL, &g_widget_background_color);

        //Alright. This time I try to connect callbacks manually. Sounds good huh
        g_signal_connect(window_widget, "destroy", G_CALLBACK(on_login_window_destroy_event),NULL);

        g_signal_connect(g_eventbox_login, "button-press-event", G_CALLBACK(on_eventbox_login_button_press_event),NULL);
        g_signal_connect(g_eventbox_register, "button-press-event", G_CALLBACK(on_eventbox_register_button_press_event),NULL);
        g_signal_connect(g_eventbox_settings, "button-press-event", G_CALLBACK(on_eventbox_settings_button_press_event),NULL);
        
        //Keyboard Shortcut support is finally here!
        //Hit Enter or Ctrl+Enter to login
        //Hit Ctrl+R to register
        //His Ctrl+S to change settings
        g_signal_connect(g_eventbox_login, "composited-changed", G_CALLBACK(on_eventbox_login_button_press_event),NULL);
        g_signal_connect(g_eventbox_register, "composited-changed", G_CALLBACK(on_eventbox_register_button_press_event),NULL);
        g_signal_connect(g_eventbox_settings, "composited-changed", G_CALLBACK(on_eventbox_settings_button_press_event),NULL);

        g_object_unref(builder_login);
        g_free(markup_label_login);
        g_free(markup_label_register);
        g_free(markup_label_settings);
        gtk_widget_show_all(window_widget);

        Server = gethostbyname(server_hostname);

        if (Server == NULL){
            show_notification_dialog(login_window, 10);
        }
        else{
            if (server_port_number <= 2000 || server_port_number >65535){
                show_notification_dialog(login_window, 9);
            }
            else{
                #ifdef DEBUG
                fprintf(stderr, "Info: Setting server to %s:%d\n",server_hostname, server_port_number);
                #endif
                ServerAddress.sin_family = AF_INET;
                ServerAddress.sin_port = htons(server_port_number);
                ServerAddress.sin_addr = *(struct in_addr*)Server->h_addr_list[0];
            }
        }

        gtk_main();//This is the main loop for the login window
}

int main(int argc, char *argv[]) {
	/* Set defaults */
	strcpy(server_hostname, "localhost");
	server_port_number = 18000;

    /* Process command line arguments */
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			fprintf(stderr, "Usage: chat [hostname (optional)] [port number (optional)]\n"); // Print usage information
		}
		else if (!isdigit(argv[i])) {
			strcpy(server_hostname, argv[i]);
		}
		else {
			server_port_number = atoi(argv[i]);
		}
	}

	if (!strcmp(server_hostname, "localhost") && server_port_number == 18000) {
		fprintf(stderr, "Using default hostname localhost and port number 18000...\n");
	}
	else {
		fprintf(stderr, "Using hostname %s and port number %d...\n", server_hostname, server_port_number);
	}
    
    // Initializing threads and GTK
    #if __GNUC__ <= 4 //Since EECS servers are using legacy GCC 4.2 something
    g_thread_init(NULL);
    #endif
    gdk_threads_init();
    gtk_init(&argc, &argv);

    show_login_window();
}

/* EOF */
