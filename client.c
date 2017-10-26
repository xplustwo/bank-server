
#include "client.h"
//adapter = server
//cd = client

int csd;

/*void signal_handler(int signo) {
	switch(signo) {
		char f[] = "finish";
		
		case SIGINT:	
			if((write(csd, f, strlen(f))) == -1) {
				printf("Write call failed on line: %d\n", __LINE__);
				close(csd);
				exit(0);
			}
			close(csd);	
			exit(0);
	}

}*/

/** @Description: function used by command_input thread. Gets input from user and sends it to the server.
 *  @param sfd: socket descriptor used to communicate with server
 **/

int command_input(int sfd){

    //printf("Started command_input\n");

    char buffer[120];
    //memset(buffer,0,sizeof(buffer));
    int length = 0;
    
    //memset(buffer,0,sizeof(buffer));
    while(1) {
        printf("Enter a command... \n");
        length = read(0, buffer, 120);
        //printf("You entered %s\n",buffer);
        if(strcmp(buffer,"exit\n") == 0){
            char exitmsg[] = "Exiting program...\n";
            write(1,exitmsg,strlen(exitmsg));
            //pthread_detach(pthread_self());
            //exit(0);
        }
        //printf("Got here.\n");
        //exit(0);

        if(length == -1) {
            printf("Read call failed\n");
            exit(0);
        }

        buffer[length+1] = '\0';
            //write(1, buf, length);
            //write to server



        if(write(sfd, buffer, length) == -1) {
            printf("write failed\n");
            exit(0);
        }else{
            if(strcmp(buffer,"exit") == 0){ //"exit" will be sent to server, and server will shut down the client's process and send back "SIGINT" to cause client to end.
                printf("Exiting program...");
		pthread_detach(pthread_self());
                //exit(0);
            }

            printf("Command sent to server.\n");
        }
            sleep(2);
    }

    printf("command_input ended.\n");
    //pthread_detach(pthread_self());

    return 0;

}


/** @Description: function used by response_output thread. Gets output from server and sends it to the user
 *  @param sfd: socket descriptor used to communicate with server
 **/

int response_output(int sfd){

    //printf("Started response_output\n");

    //pthread_detach(pthread_self());
    int length;
    char buffer[120];
    //memset(buffer,0,sizeof(buffer));

    while(1){
	
        length = read(sfd, buffer, 50);
        //printf("buffer: %s\n", buffer);
	if(length == -1) {
            printf("read from server failed\n");
	    close(sfd);
	    pthread_detach(pthread_self());
            exit(0);
        }
	else if(strcmp(buffer,"SIGINTSIGINT") == 0){ //Defensive coding: inconsistent server response must also cause client to exit.
                char sigint[] = "The bank is closed. Thank you for banking with us!\n";
                write(1,sigint,strlen(sigint));
                close(sfd);
		pthread_detach(pthread_self());
                exit(0);
        }
  	else if(strcmp(buffer, "SIGINT") == 0) { //Server has ended, or is forcing the client to end.
                char sigint[] = "The bank is closed. Thank you for banking with us!\n";
                write(1,sigint,strlen(sigint));
                close(sfd);
		pthread_detach(pthread_self());
                exit(0);
	}
        else { //Normal server output
		buffer[length+1] = '\0';
        	if(length > 0){
           		 printf("BANK: %s\n", buffer);
        	}
	}
        //write(1, buffer, length);
        //printf("\n");
        memset(buffer,0,sizeof(buffer));
    }

    printf("response_output ended.\n");
    return 0;

}

int main(int argc, char ** argv) {

    struct addrinfo request;
    struct addrinfo* result, *temp;
    
    int sd; //socket desc
    size_t sfd;
    char buf[120];

    memset(&request, 0, sizeof(struct addrinfo));
    memset(&buf, 0, 120);
    request.ai_family = AF_INET;
    request.ai_flags = 0;
    request.ai_socktype = SOCK_STREAM;
    request.ai_protocol = 0;

    sd = getaddrinfo(argv[1], "11226", &request, &result);
    if(sd != 0) {
        printf("getaddrinfo failed :( \n");
        exit(0);
    }

    for(temp = result; temp != NULL; temp = temp->ai_next) {
        sfd = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
        if(sfd == -1) //socket failed
            continue;
        if(connect(sfd, temp->ai_addr, temp->ai_addrlen) != -1) {
	    //printf("It worked!\n");
            break; //successful connection
        }
        //close(sfd);
    }

    if(temp == NULL) {
        printf("Could not connect - temp is NULL\n");
        exit(0);

    }

    freeaddrinfo(result);
    csd = sfd; //make a socket descriptor clone

/*    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = &(signal_handler);
    sigaction(SIGINT, &sa, NULL);
*/

    pthread_t p_in, p_out;

    if(pthread_create(&p_out,0,(void*)(*response_output),(void *)sfd) == -1){
        printf("Couldn't pthread_create for p_out\n");
        exit(0);
    }
    pthread_detach(p_out);
    if(pthread_create(&p_in,0,(void*)(*command_input),(void *)sfd) == -1){
        printf("Couldn't pthread_create for p_in\n");
        exit(0);
    }
    pthread_detach(p_out);

    pthread_join(p_in,NULL);
    pthread_join(p_out,NULL);

    close(sfd);
    return 0;
}




