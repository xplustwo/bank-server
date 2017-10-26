#include "server.h"
#define FILESIZE (sizeof(struct account)*(20))

/**
 * PA5 - Multithreaded Banking System
 * @Authors - Aditya Geria, Tanya Balaraju
 * @Description - uses mmap, multiprocessing and multithreading
 * To create a banking simulation
 */

struct account* accounts;
int mfd; //mapped file descriptor - copy mainly
int csd; //client service descriptor - copy mainly
int accnum; //num accounts

pthread_mutex_t bank_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Signal Handler for coordinating a SIGINT shutdown between
 * the client and the server
 */
void signal_handler(int signo) {

	switch(signo) {

		case SIGINT:
			munmap(accounts, FILESIZE);
			
			write(csd,"SIGINT",strlen("SIGINT"));
			//communicate to client to exit
			exit(0);
			break;
		case SIGSEGV:
			munmap(accounts, FILESIZE);
			
			printf("Seg fault blocked uncle\n");
	                if((write(csd,"SIGINT",strlen("SIGINT"))) == -1) {
				printf("Write call failed on line: %d\n", __LINE__);
				exit(0);
			}
			close(csd);
			close(mfd);
			exit(0);
			break;
		/*default:
			printf("Signal recieved: %d\n", signo);
			break*/;
	}

}

/*
 * Coordinates the number of accounts in the bank
 * This is because we need to load the accounts from the
 * memory mapped file, and we need to keep track of accounts
 * between executions
 */
void setAccnum(){

    	int i;
    	int j = 0;
    	for(i = 0; i<20; i++){
        	if(isalpha(accounts[i].name[0])) j++;
    	}

    	accnum = j;
	//accnum++;
    	//printf("Set accnum to %d\n",j);
    	return;

}


/*
 * Initializes all accounts in the bank to empty/default values
 */
void acc_init(){

	int i;
    	for(i = 0; i < 20; i++){

        	memset(accounts[i].name,0,sizeof(accounts[i].name));
        	accounts[i].balance = 0;
       		accounts[i].IN_SESSION = 0;
    	}

    	return;

}

/**
 * @Description: Formulates the name from a given start/open command
 * @param command - command sent to server
 * @param token - command without name (ex. "open", "start")
 * @param ret - char* for string that will be the name
 * @param len3 - length of returned string
 */
char* getName(char* command, char* token, char* ret, int len3){

	int len2 = strlen(token);
    	int i;

	for(i = 0; i < len3-1; i++){
        //printf("%c ",command[i+len2+1]);
        	if(command[i+len2+1] == '\n'){
            		i++;
           	 	break;
       		} 
        	ret[i] = command[i+len2+1];
        //printf("ret[i] is %c\n",ret[i]);

    	}
    	ret[i-1] = '\0';
    	//printf("ret is %s\n",ret);
    	return ret;

}

int main(int argc, char** argv) {

	printf("Starting server...\n");

	struct addrinfo *result, request;
	struct sockaddr_in senderAddr;
	struct sigaction sa;

	//ssize_t nread;
	int on = 1;
	int sd, fd, clientsd;
	int ic = sizeof(senderAddr);
	//int len;
	int fileExists = 0;
	int rval;

	char* BANKNAME = "bank";
	//char* message;

	//set up getaddrinfo
	request.ai_flags = AI_PASSIVE;
	request.ai_family = AF_INET;
	request.ai_socktype = SOCK_STREAM;
	request.ai_protocol = 0;
	request.ai_addr = NULL;
	request.ai_addrlen = 0;
	request.ai_canonname = NULL;
	request.ai_next = NULL;

	if(getaddrinfo(NULL, "11226", &request, &result) != 0) {
		printf("getaddrinfo failed on line: %d\n", __LINE__);
		exit(0);
	}

	if((sd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1) {
		printf("Socket call failed on line: %d\n", __LINE__);
		exit(0);
	}

	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
		printf("setsockopt failed on line: %d\n", __LINE__);
		exit(0);
	}

	if(bind(sd, result->ai_addr, result->ai_addrlen) != 0) {
		printf("bind failed on line: %d\n", __LINE__);
		exit(0);
	}

	if(listen(sd, 50) != 0) {
		printf("listen failed on line: %d\n", __LINE__);
		exit(0);
	}

	//the memory mapped file must be made by the Makefile command
	//use make cleanmmmap to clear the bank file
	//use make bank to make an empty bank file
	if((fd = open(BANKNAME, O_RDWR , (mode_t)0600)) != -1) {
		printf("Could not create bank file to memory map to (Might already exist). Line: %d\n", __LINE__);
		//setAccnum();
		fileExists = 1;
	}
	/*else if((fd = open(BANKNAME, O_RDWR)) == -1) {
		printf("Could not find bank file to memory map to. Line: %d\n", __LINE__);
		exit(0);
	}*/
	printf("fd = %d on line %d\n", fd, __LINE__);
	mfd = fd; //set copy

	if((rval = lseek(fd, FILESIZE-1, SEEK_SET)) == -1) {
		printf("lseek failed on line: %d\n", __LINE__);
		exit(0);
	}

	if((rval = write(fd, "", 1)) == -1) {
		printf("Error writing last byte of file. Line: %d\n", __LINE__);
		exit(0);
	}

	/*if((rval = lseek(fd, 0, 0)) == -1) {
		printf("lseek call 2 failed on line: %d\n", __LINE__);
		exit(0);
	}*/

	if((accounts = (struct account*)mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		printf("Could not memory map the file to bank file. Line: %d\n", __LINE__);
		exit(0);
	}


	printf("Successfully memory mapped the file! fd = %d\n", fd);

	//bank = bank_initialize(bank);
	//accnum = 0;

	//setting up sigaction
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &(signal_handler);
	sigaction(SIGINT, &sa, NULL);

	//create a print thread
	pthread_t print;
	if((pthread_create(&print, 0, (void*)(&printBank), NULL)) != 0) {
		printf("Child process failed to create thread to print bank on line: %d\n", __LINE__);
		exit(0);
	}
	
	//check if theres a memory map, then either initialize the account, or set 
	//the  number of accounts based on how  many are found
	if(fileExists == 0){
                acc_init();
        }
        else{
                printf("Got to line: %d\n", __LINE__);
                setAccnum();
        }

	//pthread_join(print, NULL);
	printf("main - thread created\n");

	//pthread_t client_service_thread;

	printf("In parent process - session acceptor - PID: %d\n", getpid());
	while(1) {
		clientsd = accept(sd, (struct sockaddr*)&senderAddr, &ic);
		if(clientsd == -1) {
			continue;
		}
		csd = clientsd;
		printf("clientsd: %d, csd: %d\n", clientsd, csd);

		if(fork()!= 0){
			//int* status ;
			printf("In parent process - session acceptor - PID: %d\n", getpid());
			write(clientsd, "Welcome to the bank!\n", strlen("Welcome to the bank!\n"));
			//wait(0);
		}
		else {
			printf("In child process - client service - PID: %d\n", getpid());
			client_service();
		}
	}

	//wait for the thread to finish
	pthread_join(print, NULL);
	printf("Unmapping the file\n");
	if(munmap(accounts, FILESIZE) != 0) {
		printf("Failed to unmap on line: %d\n", __LINE__);
		close(csd);
		close(mfd);
		exit(0);
	}
	
	return 0;
}

/*
 * Look up the index of an account in the bank, given its name
 * Returns -1 if account not found, otherwise returns index
 */
int bank_lookup(char* name) {

	printf("Name to be searched: %s | on line %d\n", name, __LINE__);
	//printf("Number of accounts: %d\n",accnum);
	int i;
	for(i = 0; i < accnum; i++) {
		if(strcmp(accounts[i].name, name) == 0){
            	//printf("Found account at index %d\n",i);
			return i;
		}
	}
	return -1;
}

/*void closeAllAccs() {

	int i;
	printf("Accnum: %d in closeAllAccs()\n", accnum);
	for(i = 0; i < accnum; i++) {
		if((pthread_mutex_trylock(&(bank_lock))) != 0) {
			printf("Trylock failed on line; %d\n", __LINE__);
			exit(0);
		}
		printf("Turning off session at index: %d\n", i);
		accounts[i].IN_SESSION = 0;
		printf("Session off\n");
		if((pthread_mutex_unlock(&(bank_lock))) != 0) {
			printf("Unlock failed on line: %d\n", __LINE__);
			exit(0);
		}
	}

}*/

/*
 * Print bank thread function
 * Prints the bank every 20 seconds
 * Places a mutex on the bank before printing, then prints it,
 * then unlocks the mutex when finished
 */
void printBank( ) {

	printf("Starting printBank thread\n");
	//struct bank* b = (struct bank*) tempbank;	
	pthread_detach(pthread_self());	
	int i = 0, rval;
	
	//need to set mutexattrs for shared processes memory
	pthread_mutexattr_t bankmutexattr;

	if((rval = pthread_mutexattr_init(&bankmutexattr)) != 0) {
		printf("mutex attr init failed on line: %d\n", __LINE__);
		exit(0);
	}
	if((rval = pthread_mutexattr_setpshared(&bankmutexattr, PTHREAD_PROCESS_SHARED)) != 0) {
		printf("setpshared failed on line: %d\n", __LINE__);
		exit(0);
	}
	if((rval =  pthread_mutex_init(&(bank_lock), &bankmutexattr)) != 0) {
		printf("Failed to initialize bank mutex lock on line: %d\n", __LINE__);
		exit(0);
	}
	while(1) {
		setAccnum(); //update number of accounts

		//printf("Accnum is: %d\n", accnum);

		printf("Bank Accounts\n-------------\n");
		if((rval = pthread_mutex_lock(&(bank_lock))) != 0) {
			printf("failed to lock bank\n");
			char perr[] = "There was an issue in the server. Sorry!\n";
			write(csd,perr,strlen(perr));
        		//signal_handler(SIGINT);
			exit(0);
		}

		for(i = 0; i < accnum; i++) {
			printf("Account name: %s\n", accounts[i].name);
			printf("Account balance: %.2f\n", accounts[i].balance);
			if(accounts[i].IN_SESSION == 1) {
				printf("In session...\n\n");
			}
			else
				printf("Inactive\n\n");
		}
		//printf("Print Complete\n");
		if((rval = pthread_mutex_unlock(&(bank_lock))) != 0) {
			printf("Mutex unlock failed on line: %d\n", __LINE__);
			char perr[] = "There was an issue in the server. Sorry!\n";
			write(csd,perr,strlen(perr));
            		//signal_handler(SIGINT);
			exit(0);
		}
		printf("-------------\nPrint complete\n");
		sleep(20);
		continue;


	}

}

/*struct bank* bank_initialize(struct bank* bank) {

	int i =  0;
	int fd = mfd;
	printf("Initializing bank... with fd = %d\n", fd);

	for(i = 0; i < 20; i++) {
		if(((accounts[i]) = (struct account*) mmap(0, (sizeof(struct account)), PROT_READ | PROT_WRITE, MAP_SHARED, fd, lseek(fd, SEEK_SET, 0))) == MAP_FAILED) {
			printf("Memory map on accounts failed, line; %d\n", __LINE__);
			exit(0);
		}

		//printf("Sucecssfully mapped to file on line: %d\n", __LINE__);


		//printf("Entered the loop on line: %d\n", __LINE__);
		printf("mmap on line %d worked!\n", __LINE__);
		//accounts[i].name = "John Smith";

		//accounts[i].balance = 50;
		//printf("Named %s #%d\n", bank->accounts[i].name, i+1);
	}
	return bank;
}*/

/*
 * initializes a string to be null
 */
void str_init(char* str){

    	int i;
    	for(i = 0; i < strlen(str); i++){
        	str[i] = (char)0;
    	}

    	return;
}


/*
 * Client service process function
 * The child process from main calls this function
 * Reads in an initial input from the server, then continues to look for
 * Commands such as open/start, then finish, credit, debit, balance and exit
 * It is assumed that the client will be responsible and "log out" 
 * (exit or finish) before quitting their session.
 */
void client_service() {

	int cd = csd;
	//int cd = *(int*)clientsd;
	//int fd = mfd;
	float amt;
	int len = -1;
	int rval;

	
	char command[2048];
	memset(command,0,sizeof(command));
	printf("Accepting inputs... \n");
	while(len <= 0) {
		//printf("Accepting inputs...\n");
		len = read(cd, command, 120);
		if(len > 0) break;
		//sleep(2);
	}
	command[strlen(command)] = '\0';
	printf("Command: %s\n", command);

	int len1 = strlen(command);
    	char accname[100];
	//int finished = 0;

	//checks for the command to be one of the opening commands
	//the first command must be either open or start
	while(1) {
        	do{
                    	char token[150];
                	sscanf(command,"%s ",token);
	                //printf("Token is %s\n",token);
			
			printf("Command is: %s\n", command);	
			
			 /*
			 * Open command execution
		 	 * First, get the name to open
			 * Then pass it to open_account
			 * Error checks for already existing account		 
			 */
            		if(strcmp(token, "open") == 0) {
                        	printf("opening an account...\n");

                        	int L = len1-strlen(token);
                        	printf("Length of name is %d\n",L);

                        	char person[L]; //holds the name of the person
                        	getName(command,token,person,L);

                        	printf("Account name: %s\n",person);
                        	strcpy(accname, person);
                        	
				printf("Copied token: %s into accname: %s\n", person, accname);
                        	if((rval = open_account(accname)) != 0) {
                        		printf("open_account went wrong on line %d\n", __LINE__);
                                	//exit(0);
                        	}else{
				
                        		if((write(csd, "In session now.\n", strlen("In session now.\n"))) == -1) {
                            			printf("Write call on line: %d failed\n", __LINE__);
                            			exit(0);
                        		}
					break;
				}
            		}

			/*
                         * Start command execution
                         * First, get the name to open
                         * Then pass it to start_account
			 * Error checks for in-session and non-existant account
                         */
            		else if(strcmp(token, "start") == 0) {
                		printf("starting an account...\n");
                		//token = strtok(NULL, " ");
				len1 = strlen(command);
                		int L = len1-strlen(token);
				printf("L1: %d, len1: %d\n", L, len1);
                        	char person[L];
                       		getName(command,token,person,L);
                		printf("account name: %s\n", person);
                		strcpy(accname, person);
                		printf("copied %s to accname: %s\n", person, accname);
                		if((rval = start_account(person)) == 1) {
                    			printf("Duplicate Session for %s. Please wait and try again later.\n", accname);
                    			//continue;
					//return;
                		}
				else if(rval == -1) {
					printf("Account not found\n");
					//continue;
					//return;
				}
                        	else {
					if((write(csd, "In session now.\n", strlen("In session now.\n"))) == -1) {
                                        	printf("Write call on line: %d failed\n", __LINE__);
                                        	exit(0);
					}
					break;
                                }
                		//break;

            		}

			/*
                         * Exit command execution
                         * In case the client exits as soon as it starts
                         */
            		else if(strcmp(token,"exit") == 0){
                    		printf("exiting the client\n");
                    		char sigint[] = "SIGINT";
				if((write(csd, sigint, strlen(sigint))) == -1) {
					printf("Write failed on line: %d\n",__LINE__);
					exit(0);
				}
            		}
			//might fire unexpectedly - ignore if so
           		else {
                		printf("unrecognized opening command found. Try 'open' or 'start' followed by the account name\n");
                		char unrec[] = "Unrecognized opening command found. Try 'open' or 'start' followed by the account name.\n";
                        	if(write(csd,unrec,strlen(unrec)) == -1) {
					printf("Write call failed on line: %d\n", __LINE__);
					exit(0);
				}
                        	
                		//sleep(1);
                		//return;
            		}
			//memset(command, 0, sizeof(command));
               		if(read(csd,command,120) == -1) {
				printf("Read failed on line: %d\n", __LINE__);
				exit(0);
			}
        	}while(1);
		
        	acc_mutex_init(accname);
        	//printf("Successfully initialized mutex for %s | %s\n", accname, token);
            	memset(command,0,sizeof(command));
        	
		do {
                	printf("Waiting for new command (credit, debit, balance, finish, exit)\n");
                	//read(csd,command,120);
                	len = -1;
                	while(len <= 0) {
                        	//printf("Accepting inputs...\n");
                        	len = read(cd, command, 120);
                        	if(len > 0) break;
                            	//sleep(1);
                	}
                    	command[strlen(command)] = '\0';

                    	//printf("Read: %s\n", command);

                    	char tok[200];
                    	sscanf(command,"%s %f\n",tok,&amt);
                    	//printf("Token is %s\n",tok);

                    	//printf("Awaiting more inputs...\n");
                        
			/*
                         * Credit command execution
                         * Get the amounnt, and then send it to the 
			 * credit_account function
			 * No error checks needed
                         */
			if(strcmp(tok, "credit") == 0) {
                                printf("credit...\n");
                             	printf("Amount is $%.2f\n",amt);
                                printf("Crediting account: %s, $%.2f\n", accname, amt);
                                credit_account(amt, accname);
                                char cr[] = "Credited amount";
                                if((write(csd, cr, strlen(cr))) == -1) {
                                  	printf("Write call failed on line: %d\n", __LINE__);
                                    	exit(0);
                               	}
				else{
                                    	printf("Sent info to client.\n");
                                }
                                continue;
                        }
		
			/*
                         * Debit command execution
                         * Get the value, and then send to debit_account
			 * Error checks for invalid values - cant have people in debt
                         */
               	        else if(strcmp(tok, "debit") == 0) {
                                printf("debit....\n");
                                
                                printf("Deduction amount: %s, $%.2f\n", accname, amt);
                                
				if(debit_account(amt, accname) == 0) {
                                	char db[] = "Deducted amount.";
                                	if((write(csd, db, strlen(db))) == -1) {
                                                printf("Write call on line: %d failed\n", __LINE__);
                                               	exit(0);
                                	}
					else{
                                    		printf("Sent info to client.\n");
                                	}

                                	continue;
				}
				else {
					char dbfail[] = "Could not debit account\n";
					if((write(csd, dbfail, strlen(dbfail))) == -1) {
                                        	printf("Write call on line: %d failed\n", __LINE__);
                                                exit(0);
                                        }	
					continue;
				}	
                        }

			/*
                         * Balance command execution
                         * Returns the balance
			 * No error checks needed
                         */
                        else if(strcmp(tok, "balance") == 0) {
                                printf("balance for %s ...\n", accname);
                                char b[] = "Getting current balance...";
                                if((write(csd, b, strlen(b))) == -1) {
                                        printf("Write call on line: %d failed\n", __LINE__);
                                        exit(0);
                                }
                                balance_account(accname);

                                continue;
                        }

			/*
                         * Finish command execution
                         * turn off session, but dont kill execution
                         */
                        else if(strcmp(tok, "finish") == 0) {
                                printf("ending your session..\n");
                                write(csd,"Ending your session!\n",strlen("Ending your session!\n"));
                                int k = bank_lookup(accname);
                                accounts[k].IN_SESSION = 0;
				//finished = 1;
                                break;
                        }
			
			/*
                         * Exit command execution
                         * Exits communication with client, and turns off the session
                         */
                        else if(strcmp(tok, "exit") == 0) {
				//close(csd);
                                printf("exiting the client\n");
				char sigint[] = "SIGINT";
                                //message to client is covered in client
                                int k = bank_lookup(accname);
                                accounts[k].IN_SESSION = 0;
                                if((write(csd, sigint, strlen(sigint))) == -1) {
					printf("Write call failed on line: %d\n", __LINE__);
					exit(0);
				}
				close(csd);
                        }

                        else {
                                char nrec[] = "Command not recognized.\nIf you entered 'open' or 'start', you need to finish your session first.\n";
                                write(csd,nrec,strlen(nrec));
                                printf("command not recognized...\n");
                                sleep(1);
				continue;
                        }
                        //memset(command,"",sizeof(command));
			return;

        	} while(1);
		memset(command,0,sizeof(command));
		read(csd,command,sizeof(command));
		//when the loop is done, destroy the mutex
        	acc_mutex_destroy(accname);
       	 	continue;
 	}

}


//initializes a mutex lock (but does not lock) for an account
void acc_mutex_init(char* name) {

	int index;
	int rval;

	if((index = bank_lookup(name)) == -1) {
		printf("Could not initialize mutex for account that doesnt exist. Line: %d\n", __LINE__);
		exit(0);
	}
	else {
		pthread_mutexattr_t accmutexattr;
		if((rval = pthread_mutexattr_init(&accmutexattr)) != 0) {
			printf("mutex attr init failed on line: %d for index: %d\n", __LINE__, index);
			exit(0);
		}
		if((rval = pthread_mutexattr_setpshared(&accmutexattr, PTHREAD_PROCESS_SHARED)) != 0) {
			printf("mutex setpshared failed on line: %d for index: %d\n", __LINE__, index);
			exit(0);
		}
		if((rval = pthread_mutex_init(&(accounts[index].acc_lock), &accmutexattr)) != 0) {
			printf("Failed to initialize mutex on line: %d, for index: %d\n", __LINE__, index);
			exit(0);
		}
	}
	return;

}

//destroys the same mutex lock created by acc_mutex_init()
void acc_mutex_destroy(char* name) {

	int index, rval;
	index = bank_lookup(name);

	if(index == -1) {
		printf("Account not found in bank. Line: %d\n", __LINE__);
		return;
	}
	if((rval = pthread_mutex_destroy(&(accounts[index].acc_lock))) != 0) {
		printf("Could not destroy mutex. Line: %d\n", __LINE__);
		exit(0);
	}
	return;
}

/***
  * @Description: opens an account in the bank
  * @param name: name to be opened
  * First, check if the capacity is full. Then, check if the account already exists
  * Return -1 for either case
  * Then, lock the bank_lock, create a new account at the last [accnum] index
  * then unlock the bank, and return a successful value
  **/
int open_account(char name[]) {

	int rval;
	if(accnum == 20) {
		char out[]  = "Sorry, out of capacity! \n";
		if((rval = write(csd, out, strlen(out)) == -1)) {
			printf("Write call to client failed on line: %d\n", __LINE__);
			close(csd);
			close(mfd);
			exit(0);
		}
	}
	if((rval = pthread_mutex_lock(&(bank_lock))) != 0) {
		printf("Couldnt lock the bank on line: %d\n", __LINE__);
		exit(0);
	}
	//int fd = mfd;
	printf("name to add: %s\n", name);
	if((rval = bank_lookup(name)) == -1) {

		printf("bank num: %d\n", accnum);
		strcpy(accounts[accnum].name, name);
		printf("Assigned name\n");
		accounts[accnum].balance = 60;
		printf("Assigned balance\n");
		accounts[accnum].IN_SESSION = 1;
		(accnum)++;
		if((rval = pthread_mutex_unlock(&(bank_lock))) != 0) {
			printf("Couldnt unlock the lock on line: %d\n", __LINE__);
			exit(0);
		}
		printf("Account created!\n");

		return 0;
	}

	else {
		char exists[] = "Account already exists in system. Please exit and start a new session.\n";
		if((write(csd, exists, strlen(exists))) == -1) {
			printf("Write call failed on line: %d\n", __LINE__);
			exit(0);
		}
		if((rval = pthread_mutex_unlock(&(bank_lock))) != 0) {
			printf("Couldnt unlock the lock on line: %d\n", __LINE__);
			exit(0);
		}

		return -1;
	}

	return -2;
}


/***
  * @Description: starts a session for an account in the bank
  * @param name: name to be opened
  * First, check if the account already exists, or if its in session
  * Return the respective error
  * Then, lock the bank_lock, starts a new account session 
  * then unlock the bank, and return a successful value
  **/
int start_account(char* name) {

	int rval;

	if((rval = pthread_mutex_trylock(&(bank_lock))) != 0) {
		printf("Could not acquire a lock on line: %d\n", __LINE__);
		exit(0);
	}

	printf("name to start session: %s\n", name);
	if((rval = bank_lookup(name)) == -1) {
		char find[] = "Could not find account in bank. If you would like to open this account, use 'open'\n";
		if((write(csd, find, strlen(find))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }
		if((rval = pthread_mutex_unlock(&(bank_lock))) != 0) {
			printf("Could not unlock the bank mutex on line: %d\n", __LINE__);
			close(csd);
			close(mfd);
			exit(0);
		}

		return -1;
	}
	else {
		if(accounts[rval].IN_SESSION == 0) {
			accounts[rval].IN_SESSION = 1;
			char begin[] = "Beginning session.\n";
			if((write(csd, begin, strlen(begin))) == -1) {
                        	printf("Write call failed on line: %d\n", __LINE__);
                	        exit(0);
        	        }
			if((rval = pthread_mutex_unlock(&(bank_lock))) !=	0) {
                       		printf("Could not unlock the bank mutex	on line: %d\n",	__LINE__);
                        	close(csd);
                        	close(mfd);
                        	exit(0);
                	}

			return 0;
		}
		else {
			char running[] = "Already a session running.\n";
			if((write(csd, running, strlen(running))) == -1) {
                	        printf("Write call failed on line: %d\n", __LINE__);
        	                exit(0);
	                }
			if((rval = pthread_mutex_unlock(&(bank_lock))) !=	0) {
                       		 printf("Could not unlock the bank mutex	on line: %d\n",	__LINE__);
                        	close(csd);
                        	close(mfd);
                        	exit(0);
                	}

			return 1;
		}
	}

	return -1;
}


/***
  * @Description: Returns the balance for the account
  * @param name: name to be opened
  * First, check if the capacity is full. Then, check if the account already exists
  * Return -1 for either case
  * Then, lock the bank_lock, create a new account at the last [accnum] index
  * then unlock the bank, and return a successful value
  **/
void balance_account(char name[]) {

	int rval;
	int index;

    	//printf("name is %s\n",name);

	if((index = bank_lookup(name)) == -1) {
		char find[] = "Account name not found in bank. Try 'open' or 'start' or check your spelling\n";
		if((write(csd, find, strlen(find))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }

		return;
	}
	if((rval = pthread_mutex_trylock(&(accounts[index].acc_lock))) != 0) {
		printf("Lock on account failed on line: %d\n", __LINE__);
		return;
	}

	char balance[10];
	float x = (accounts[index].balance);
	sprintf(balance, "%.2f", x);

	if((write(csd, balance, strlen(balance))) == -1) {
		printf("Write call failed on line: %d\n", __LINE__);
                exit(0);
        }


	if((rval = pthread_mutex_unlock(&(accounts[index].acc_lock))) != 0) {
		printf("Failed to unlock lock on line: %d\n", __LINE__);
		return;
	}

	return;
}

void credit_account(float amt, char name[]) {

	int index, rval;
	index = bank_lookup(name);
	if(amt <= 0) {
		char inv[] = "invalid amount. Must be >= 0\n";
		if((write(csd, inv, strlen(inv))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }
		return;

	}
	if(index == -1) {
		char exists[] = "Account not found in the bank.\n";
		if((write(csd, exists, strlen(exists))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }

		return;
	}
	else if(accounts[index].IN_SESSION == 0) {
		char session[] = "Account is not in session. How did you even get here?\n";
		if((write(csd, session, strlen(session))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }

		return;
	}
	else {
		if((rval = pthread_mutex_trylock(&(accounts[index].acc_lock))) != 0) {
			printf("Lock on account failed on line: %d\n", __LINE__);
			exit(0);
		}

		accounts[index].balance+= amt;

		char balance[10];
        	float x = (accounts[index].balance);
		sprintf(balance, "%.2f", x);
		printf("Credit added for account [%s]. New Balance: [$%.2f]\n", name, accounts[index].balance);

		if((write(csd, balance, strlen(balance))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }

		if((rval = pthread_mutex_unlock(&(accounts[index].acc_lock))) != 0) {
			printf("Unlocking account lock failed on line: %d\n", __LINE__);
			exit(0);
		}

		printf("Unlocked account.\n");

	}
	return;
}

int debit_account(float amt, char name[]) {

	int index, rval;
	index = bank_lookup(name);

	if(amt <= 0) {
		char inv[] = "Invalid amount, must be > 0.\n";
		if((write(csd, inv, strlen(inv))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }
		return -1;
	}
	if(index == -1) {
		char exists[] = "Could not find name in the bank.\n";
		if((write(csd, exists, strlen(exists))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }

		return -1;
	}
	else if(accounts[index].IN_SESSION == 0) {
		char exists[] = "Account currently not in session. How did you get here?\n";
		if((write(csd, exists, strlen(exists))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }
		return -1;
	}
	else {
		if((rval = pthread_mutex_trylock(&(accounts[index].acc_lock))) != 0) {
			printf("Locking account lock failed on line: %d\n", __LINE__);
			exit(0);
		}

		int y = accounts[index].balance- amt;
		if( y < 0) {
			char withdraw[] = "Not allowed to withdraw more than you have\n";
			if((write(csd, withdraw, strlen(withdraw))) == -1) {
				printf("Write failed on line: %d\n", __LINE__);
				exit(0);
			}
			if((rval - pthread_mutex_unlock(&(accounts[index].acc_lock))) != 0) {
                        	printf("Unlocking the account failed on line: %d\n", __LINE__);
                       		exit(0);
         	        }
			return -1;
		}
		accounts[index].balance -= amt;	
		printf("Deducted $%.2f from [%s]. New balance: $[%.2f]\n", amt, name, accounts[index].balance);

		char balance[10];
        	float x = (accounts[index].balance);
        	sprintf(balance, "%.2f", x);

		if((write(csd, balance, strlen(balance))) == -1) {
                        printf("Write call failed on line: %d\n", __LINE__);
                        exit(0);
                }

		if((rval - pthread_mutex_unlock(&(accounts[index].acc_lock))) != 0) {
			printf("Unlocking the account failed on line: %d\n", __LINE__);
			exit(0);
		}

		printf("Unlocked account.\n");
	}

	return 0;
}


