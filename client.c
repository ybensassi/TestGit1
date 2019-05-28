
/* 
 * tcpclient.c - A simple TCP client
 * usage: tcpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h> 
#include <sys/types.h> 
#include <sys/poll.h>  
#include <arpa/inet.h> 
#include <stdarg.h>

#define BUFSIZE 1024
#define portno 5011

#define NMAX   99000
#define MAXLEN 1024
#define TRACE  4
#define FLOW   3
#define myLevel 7

void
FPRINTF(char level, char *fmt, ...) {
        char buffer[MAXLEN];
 
        if (myLevel >= level ) {
                va_list args;
                va_start(args, fmt);
                vsnprintf(buffer, MAXLEN, fmt, args);
                fprintf(stderr, "\n(%d) %s", pthread_self(), buffer);
                fflush(stderr);
                va_end(args);
        }
}

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    pthread_exit(NULL);

}

static void *traite( void *arg)
{
    int sockfd, number, n,size;
    long nCounter=1 ;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char hostname[20];
    char buf[BUFSIZE];
    struct hostent               *hostStructName;
    struct sockaddr_in           adr_serveur;
    struct servent               *serveur;
    int      nb_retry;   
    int result;                                                              
    struct pollfd pollfds;                                                   
    char comm[13];
    char sTid[5];
    char sN[7];

    pthread_detach ( pthread_self() );
    number = *((int *) arg);
    free(arg);

    /* check command line arguments */
    memset ( hostname, 0, sizeof (  hostname ));
    strcpy ( hostname, "youssef-Aspire-5735" );

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        FPRINTF(TRACE,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }


  adr_serveur.sin_family = AF_INET;
  adr_serveur.sin_addr.s_addr  = inet_addr("127.0.0.1"); 
  adr_serveur.sin_port   = htons(portno);
  size = sizeof(adr_serveur);                                               
  nb_retry = 0;                                                             
    if (connect( sockfd ,(struct sockaddr *) &adr_serveur, size) < 0 )
      error("ERROR connecting");


    do {
    sprintf (buf, "MESSAGE%04dC%06d", number, nCounter );
    
    n = write(sockfd, buf, strlen(buf));
    if (n < 0) 
      error("ERROR writing to socket");

       bzero(buf, BUFSIZE);

       pollfds.fd = sockfd;                                                  
       pollfds.events = POLLIN;                                            
       result = poll (&pollfds, 1, 10000);                              
       switch (result)                                                     
       {                                                                   
        case 0:                                                            
               FPRINTF ( TRACE,"\nTimeOut Detected");
               nCounter = NMAX+1;
               break;
        default:                                                           
                  n = read(sockfd, buf, BUFSIZE);
                  if (n < 0) 
                  error("ERROR reading from socket");
                  memset (  comm , 0, sizeof ( comm ));
                  memset (  sTid , 0, sizeof ( sTid ));
                  memset (  sN , 0, sizeof ( sN ));

                  memcpy ( comm, buf , 12 ); 
                  memcpy ( sTid , buf + 12 , 4 );
                  memcpy ( sN , buf + 17 , 6 );
               
                   
                  if (
                       ( memcmp ( comm , "RESP_MESSAGE" , 12 ) != 0 ) ||
                       ( atoi(sTid) != number ) ||
                       ( atol ( sN ) != nCounter )
                     )
                     {
                          FPRINTF(TRACE,"\nERR RESPONSE ");
                          nCounter = NMAX+1;
                      }
                  else
                          FPRINTF ( TRACE,"<%s><%s><%s>", comm, sTid, sN ); 
                  
                  break;                                                      
       }                                                                   
       usleep(100000);
    }
    while ( nCounter++ < NMAX );

    close(sockfd);
    pthread_exit( NULL );
    return 0;
}



main()
{

   int i,*Number;
   pthread_t threadId;
   for ( i = 1; i <= 200 ; i++ )
   {
      Number = malloc( sizeof ( int  *));
      *Number = i;
     pthread_attr_t  pattr;                                                       
   pthread_t threadId;
   pthread_attr_init(&pattr);                                                   
                                                                                
       pthread_attr_setscope (&pattr, PTHREAD_SCOPE_PROCESS );
      pthread_create((pthread_t *)&(threadId), NULL, traite, Number ); 
   }
   sleep(10000);
}

