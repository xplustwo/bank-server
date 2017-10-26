#ifndef FIRST_H_INCLUDED
#define FIRST_H_INCLUDED

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

int response_output(int);
int command_input(int);

#endif // FIRST_H_INCLUDED
