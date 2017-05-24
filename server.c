#include	<sys/types.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<pthread.h>
#include	"bank.h"

#define PORT_NUM "52966"
	


//A global Bank for all to see
static Bank		daBank;

void flush(char* str){
	for(int i = 0; i< strlen(str)+1; i++)
		str[i] = '\0';
}

int
claim_port( const char * port )
{
	struct addrinfo		addrinfo;
	struct addrinfo *	result;
	int			sd;
	char		message[256];
	int			on = 1;

	addrinfo.ai_flags = AI_PASSIVE;			// for bind()
	addrinfo.ai_family = AF_INET;			// IPv4 only
	addrinfo.ai_socktype = SOCK_STREAM;		// Want TCP/IP
	addrinfo.ai_protocol = 0;				// Any protocol
	addrinfo.ai_addrlen = 0;
	addrinfo.ai_addr = NULL;
	addrinfo.ai_canonname = NULL;
	addrinfo.ai_next = NULL;
	if ( getaddrinfo( 0, port, &addrinfo, &result ) != 0 )		// want port 3000
	{
		fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed errno is %s.  File %s line %d.\x1b[0m\n", port, strerror( errno ), __FILE__, __LINE__ );
		return -1;
	}
	else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
	{
		write( 1, message, sprintf( message, "socket() failed.  File %s line %d.\n", __FILE__, __LINE__ ) );
		freeaddrinfo( result );
		return -1;
	}
	else if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) == -1 )
	{
		write( 1, message, sprintf( message, "setsockopt() failed.  File %s line %d.\n", __FILE__, __LINE__ ) );
		freeaddrinfo( result );
		close( sd );
		return -1;
	}
	else if ( bind( sd, result->ai_addr, result->ai_addrlen ) == -1 )
	{
		freeaddrinfo( result );
		close( sd );
		write( 1, message, sprintf( message, "\x1b[2;33mBinding to port %s ...\x1b[0m\n", port ) );
		return -1;
	}
	else
	{
		write( 1, message, sprintf( message,  "\x1b[1;32mSUCCESS : Bind to port %s\x1b[0m\n", port ) );
		freeaddrinfo( result );		
		return sd;			// bind() succeeded;
	}
}

void *
client_session_thread( void * arg )
{
	int			sd;
	char		request[2048];
	char		open[] = "open";
	char		*command;
	char		*data;
	
	int 		loc = -1; 
	bool		starting = FALSE; //Are we starting?
	bool		disconnect = FALSE;
	Accnt		*account;
	float		money = 0;
				
	
	
	sd = *(int *)arg;
	free( arg );					// keeping to memory management covenant
	pthread_detach( pthread_self() );		// Don't join on this thread
	
	printf("Connection Accepted from SD: %d\n", sd);
	
	//Read input from the client
	while ( read( sd, request, sizeof(request) ) > 0 && !disconnect)
	{
		
		//request is what we got from the client
		//printf( "server receives input:  %s from SD: %d \n", request, sd );
		if(strcmp(request, "\0") == 0) {
			flush(request);
			strcpy(request, "\x1b[2;31mNo input command provided.\x1b[0m\n");
		} else {		
			command = strtok(request, " ");
			data = strtok(NULL, "\0");
			if((strcmp(command, open) == 0) && !starting){
				//printf("Create\n");
				if(addAccount(&daBank, data) != 0){
					flush(request);
					strcpy(request, "\x1b[2;31mCould not add account to bank (name is over 100 characters or name already exists in system)\x1b[0m\n");
				}
				else{
					flush(request);
					strcpy(request, "\x1b[2;32mAccount successfully created\x1b[0m\n");
				}
			}
			else if((strcmp(command, "start") == 0) && !starting){
				//printf("start\n");
				if( (loc = findAccount(&daBank, data)) < 0){
					flush(request);
					strcpy(request, "\x1b[2;31mCould not find the input account name in the bank system\x1b[0m\n");
				}

				else{
					account = &daBank.accounts[loc];
					
					//try to access a locked(maybe) mutex
					while( pthread_mutex_trylock(&account->lock)!= 0 ){
						flush(request);
						sprintf(request, "\x1b[2;33mAccount Locked.\x1b[0m Trying again.\n");
						write( sd, request, strlen(request) + 1 );
						sleep(2);
					}
									
					account->inSession = TRUE;
					starting = TRUE;
					
					flush(request);
					strcpy(request, "\x1b[2;32mYou are now starting: \x1b[0m");
					strcat(request, account->name);
					strcat(request, "\n");
				}
			}
			
			else if((strcmp(command, "credit") == 0) && starting){
				money = atof(data);
				loc = creditMoney(account, money);
				flush(request);
				sprintf(request, "%s has \x1b[2;33m$%4.2f \x1b[0m in the bank\n", account->name, account->currentBalance);
			}
			
			else if((strcmp(command, "debit") == 0) && starting){
				money = atof(data);
				loc = debitMoney(account, money);
				flush(request);
				sprintf(request, "%s has \x1b[2;33m$%4.2f \x1b[0m in the bank\n", account->name, account->currentBalance);
			}
			
			else if((strcmp(command, "balance") == 0) && starting){
				flush(request);
				sprintf(request, "%s has \x1b[2;33m$%4.2f \x1b[0m in the bank\n", account->name, account->currentBalance);
			}
			
			else if((strcmp(command, "finish") == 0 )&& starting){
				flush(request);
				sprintf(request, "%s is no longer in service", account->name);
				starting = FALSE;
				account->inSession = FALSE;
				pthread_mutex_unlock(&account->lock);
				account = NULL;
			}
			
			else if((strcmp(command, "exit") == 0 )){
				flush(request);
				sprintf(request, "\x1b[2;34mDisconnecting from the Bank\x1b[0m\n");
				if(account && account->inSession){
					account->inSession = FALSE;
					starting = FALSE;
					pthread_mutex_unlock(&account->lock);
					account = NULL;
				}
				disconnect = TRUE; //should quit on next iteration
			}		
			
			else{
				flush(request);
				sprintf(request, "\x1b[2;31m Invalid input sent to the server. \x1b[0m\n");
			}
		}
		//Write back to the client
		//printf("%s\n", request);
		write( sd, request, strlen(request) + 1 );
	}
	printf("Disconnecting from %d\n", sd);
	//End of communications 
	close( sd );
	return 0;
}

void * 
printBank(void *arg ){
	
	pthread_detach(pthread_self());
	
	while(1){
		sleep(20);
		printf("\x1b[2;36mBank Status:\x1b[0m\n");
		printAccounts(&daBank);
		printf("\n");
	}
	return 0;
}


int
main( int argc, char ** argv )
{
	int					sd;
	char				message[256];
	pthread_t			tid;
	pthread_attr_t		kernel_attr;
	socklen_t			ic;
	int					fd;
	struct sockaddr_in	senderAddr;
	int *				fdptr;
	
	if( buildDaBank(&daBank) != 0){
		printf("DESTROYED: The Avengers were unable to save this bank.\n");
		return 0;
	}

	//This was an if in Russel's code
	else if ( pthread_attr_init( &kernel_attr ) != 0 )
	{
		printf( "pthread_attr_init() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	else if ( pthread_attr_setscope( &kernel_attr, PTHREAD_SCOPE_SYSTEM ) != 0 )
	{
		printf( "pthread_attr_setscope() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	else if ( (sd = claim_port( PORT_NUM )) == -1 )
	{
		write( 1, message, sprintf( message,  "\x1b[1;31mCould not bind to port %s errno %s\x1b[0m\n", PORT_NUM, strerror( errno ) ) );
		return 1;
	}
	else if ( listen( sd, 20 ) == -1 ) //lowered this on Russell's instruction
	{
		printf( "listen() failed in file %s line %d\n", __FILE__, __LINE__ );
		close( sd );
		return 0;
	}
	else
	{
		//Create print out all accounts thread
		if ( pthread_create( &tid, &kernel_attr, printBank, NULL ) != 0 )
			{
				printf( "pthread_create() failed in file %s line %d\n", __FILE__, __LINE__ );
				return 0;
			}
			
		ic = sizeof(senderAddr);
		while ( (fd = accept( sd, (struct sockaddr *)&senderAddr, &ic )) != -1 )
		{
			fdptr = (int *)malloc( sizeof(int) );
			*fdptr = fd;					// pointers are not the same size as ints any more.
			if ( pthread_create( &tid, &kernel_attr, client_session_thread, fdptr ) != 0 )
			{
				printf( "pthread_create() failed in file %s line %d\n", __FILE__, __LINE__ );
				return 0;
			}
			else
			{
				continue;
			}
		}
		close( sd );
		return 0;
	}
}