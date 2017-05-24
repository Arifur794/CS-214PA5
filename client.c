#include	<sys/types.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<pthread.h>

#define PORT_NUM "52966"

//read input thread vars
pthread_t			rtid;
pthread_attr_t		reader_attr;

//print output thread vars
pthread_t			ptid;
pthread_attr_t		printer_attr;

int
connect_to_server( const char * server, const char * port )
{
	int					sd;
	struct addrinfo		addrinfo;
	struct addrinfo *	result;
	char				message[256];

	addrinfo.ai_flags = 0;
	addrinfo.ai_family = AF_INET;		// IPv4 only
	addrinfo.ai_socktype = SOCK_STREAM;	// Want TCP/IP
	addrinfo.ai_protocol = 0;		// Any protocol
	addrinfo.ai_addrlen = 0;
	addrinfo.ai_addr = NULL;
	addrinfo.ai_canonname = NULL;
	addrinfo.ai_next = NULL;
	if ( getaddrinfo( server, port, &addrinfo, &result ) != 0 )
	{
		fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed.  File %s line %d.\x1b[0m\n", server, __FILE__, __LINE__ );
		return -1;
	}
	else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
	{
		freeaddrinfo( result );
		return -1;
	}
	else
	{
		do {
			if ( errno = 0, connect( sd, result->ai_addr, result->ai_addrlen ) == -1 )
			{
				sleep( 3 );
				write( 1, message, sprintf( message, "\x1b[2;33mConnecting to server %s ...\x1b[0m\n", server ) );
			}
			else
			{
				freeaddrinfo( result );
				return sd;		// connection succeeded
			}
		} while ( errno == ECONNREFUSED );
		freeaddrinfo( result );
		return -1;
	}
}

void *
read_input( void * arg ){
	
	int			sd;
	char		request[2048];
	char		prompt[] = "Input Command >> ";
	int			len;
	
	sd = *(int *)arg;
	free( arg );					// keeping to memory management
	pthread_detach( pthread_self() );		// should not join on this thread
	
	while ( write( 1, prompt, sizeof(prompt) ), (len = read( 0, request, sizeof(request) )) > 0 )
	{
		request[len-1]= '\0';
		write( sd, request, strlen( request ) + 1 );
		sleep(2);
	}
	
	//End of reading input
	close( sd );
	return 0;
}

void *
print_output( void * arg ) {
	
	int 	sd;
	char	response[2048] = { 0 };
	
	sd = *(int *) arg;
	free( arg );
	//pthread_detach( pthread_self() );
	
	while ( read( sd, response, sizeof(response) ) > 0 )
	{
		if(strcmp(response, "\x1b[2;34mDisconnecting from the Bank\x1b[0m\n") == 0){
			break;
		}
		char *out = (char *)malloc(sizeof(response) + 3);
		memset(out, 0, sizeof(response) +3);
		strcat( out, response );
		strcat( out, "\n");
		strcat( out, "\0");
		write( 0, out, strlen(out) + 1 ) ;
		free(out);
		memset(response, 0, 2048);
	}
	
	pthread_cancel(rtid);
	close(sd);
	return 0;
	
}

int
main( int argc, char ** argv )
{
	int					sd;
	char				message[256];
	int 				*rfdptr, *pfdptr;

	if ( argc < 2 )
	{
		fprintf( stderr, "\x1b[1;31mNo host name specified.  File %s line %d.\x1b[0m\n", __FILE__, __LINE__ );
		exit( 1 );
	}
	else if ( (sd = connect_to_server( argv[1], PORT_NUM )) == -1 )
	{
		write( 1, message, sprintf( message,  "\x1b[1;31mCould not connect to server %s errno %s\x1b[0m\n", argv[1], strerror( errno ) ) );
		return 1;
	}
	else
	{
		printf( "Connected to server %s\n", argv[1] );
		//Create threads here to read commands from user and send commands to server
		if ( pthread_attr_init( &reader_attr ) != 0  || pthread_attr_init( &printer_attr ) != 0 )
		{
			printf( "pthread_attr_init() failed in file %s line %d\n", __FILE__, __LINE__ );
			return 0;
		}
		else if ( pthread_attr_setscope( &reader_attr, PTHREAD_SCOPE_SYSTEM ) != 0 || pthread_attr_setscope( &printer_attr, PTHREAD_SCOPE_SYSTEM ) != 0 )
		{
			printf( "pthread_attr_setscope() failed in file %s line %d\n", __FILE__, __LINE__ );
			return 0;
		}
		
		rfdptr = (int *)malloc( sizeof(int) );
		pfdptr = (int *)malloc( sizeof(int) );
		*rfdptr = *pfdptr = sd;
		if ( pthread_create( &rtid, &reader_attr, read_input, rfdptr ) != 0 || pthread_create( &ptid, &printer_attr, print_output, pfdptr ) != 0 )
		{
			printf( "pthread_create() failed in file %s line %d\n", __FILE__, __LINE__ );
			return 0;
		}
		
		pthread_join( ptid, NULL );
		
		printf("Session Ended.\n");
		
		free( rfdptr );
		free( pfdptr );
		pthread_attr_destroy( &reader_attr );
		pthread_attr_destroy( &printer_attr );
		
		close( sd );
		return 0;
	}
}