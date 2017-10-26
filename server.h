#ifndef FIRST_H_INCLUDED
#define FIRST_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>


struct account{

    float balance;
    int IN_SESSION;
    pthread_mutex_t acc_lock;
    char name[100];

};

/*struct bank {
    struct account accounts[20];
    pthread_mutex_t bank_lock;
    int num; //number of accounts    
};

struct clientConnection {

	pthread_t thread;
	struct account acc;
	int socket_d;		

};*/


void printBank();
void client_service();
void acc_init();
void setAccnum();
void closeAllAccs();
//struct bank* bank_initialize(struct bank*);
int open_account(char*);
int start_account(char*);
void credit_account(float, char*);
int debit_account(float, char*);
void acc_mutex_init(char*);
void acc_mutex_destroy(char*);
void balance_account(char*);

#endif // FIRST_H_INCLUDED
