#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <pthread.h>  
#include <signal.h>   
#include <string.h>   
#include <stdarg.h>
#include <sys/types.h>
#include <sys/poll.h> 
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>   
#include <sys/stat.h>
#include <unistd.h>  
#include <errno.h>   
#include <ctype.h>
#include <sys/types.h> 

#include "define.h"

#define nMax 32
#define MAX_RETRY 5

#define MAXLEN 1024
#define TRACE 4
#define FLOW 3
#define MAX_TRACE_LINE 80 

#define myLevel 7

#define file_name "SERVER"
int _init_trace =0;
int fd_trace;
char _extension [ 20 ];
pthread_mutex_t	trace_mutex;

/*using namespace std;*/
#define MAX_LINE 16
#define LG_MAX 256

pid_t gettid(void);




#include <sys/syscall.h>

pid_t gettid(void){
pid_t tid = (pid_t) syscall (SYS_gettid);
return tid;
}


/***************************************************************************/
/*
  - probleme du concurence du fprintf dans la fonction trace // DEBUG
  - ajout d'une fonction DEBUG differente de trace
  - probleme du BACKLOG dans la fonction listen
  - Gestion des signaux LINUX
  - Procedure d'arret
  - Thread Administration
  - Gestion des exceptions/errnos
*/
/***************************************************************************/

typedef struct s_caller
{
	int fd;
	char caller_address[16];
	int  caller_port;
        long nb_messages_received;
        long nb_messages_sent;
        char statut;
}t_caller;


void GetDayYYMMDD( char *TodayDate)
{
	time_t lg;
	struct tm *date;
	struct tm date_r;
	char   jj_c[3];
	char   mm_c[3];
	char   aa_c[5];

	time(&lg);
	date = localtime_r(&lg, &date_r);

	sprintf(jj_c,"%02d",date_r.tm_mday);
	sprintf(mm_c,"%02d",date_r.tm_mon+1);
	sprintf(aa_c,"%04d",date_r.tm_year + 1900);

	memcpy(TodayDate  ,aa_c + 2 ,2);
	memcpy(TodayDate+2,mm_c,2);
	memcpy(TodayDate+4,jj_c,2);
	return;
}

void GetTimehhmmss( char *MyHour)
{
	time_t lg;
	struct tm *date;
	struct tm date_r;
	char hh_c[3];
	char mm_c[3];
	char ss_c[3];

	time(&lg);
	date = localtime_r(&lg, &date_r);
	date_r.tm_min--;
	if (date_r.tm_min == 60)
		date_r.tm_min = 0;

	sprintf(hh_c,"%02d",date_r.tm_hour);
	sprintf(mm_c,"%02d",date_r.tm_min + 1);
	sprintf(ss_c,"%02d",date_r.tm_sec);

	memcpy(MyHour,  hh_c,2);
	memcpy(MyHour+2,mm_c,2);
	memcpy(MyHour+4,ss_c,2);
	return;
}

int OpenFile ( char *_extension )
{
	char file[ 256 ];
	int fd;

	memset(file, 0, sizeof(file));
	sprintf(file, "%s.%s", file_name, _extension );
	/*
	 * The O_SYNC flag may hit the performance of the FPRINTF as on each
	 * write the date will be written to the disc and not cached whithi the
	 * kernel.
	 * We can use a counter whithin FPRINTF and whenever it will reach a
	 * threshold then we will call fsync.
	 */
	fd = open(file, O_RDWR|O_CREAT|O_APPEND, 0666);
	if (fd < 0)
		return(-1); 
	return (fd);
}

void chek_trace_date()
{
	char Date[7];

	if ( !strlen(_extension)) {
		memset(Date, 0, sizeof(Date));
		GetDayYYMMDD(Date);
		memcpy(_extension, Date, 6);
		fd_trace = OpenFile(_extension);
                // return (-1);
	}
	else
	{
		GetDayYYMMDD(Date);
		if ( memcmp(_extension, Date, 6)) {
			close(fd_trace);
			memcpy(_extension, Date, 6);
			fd_trace = OpenFile(_extension);
                        // return (-1);
		}
	}
	return;
}

void init_trace()
{
	memset(_extension, 0, sizeof(_extension));
}

void FPRINTF(char level, const char *fmt, ...)
{                                                                   
	char buffer[MAXLEN];
	char Time[7];
	char temp[LG_MAX];
	char temp2[LG_MAX];
        
	if ( !_init_trace ){
		pthread_mutex_init(&trace_mutex, NULL);
		_init_trace++;
	}
	chek_trace_date();

	memset(temp, 0, sizeof(temp));
	memset(temp2, 0, sizeof(temp2));
	if (myLevel >= level ) {
                va_list args;
                pthread_mutex_lock(&trace_mutex); 
             
           	va_start(args, fmt);
               
                vsprintf(temp, fmt, args);
                sprintf ( temp2, "<%06d> %s", gettid(), temp );
		for(int i=0;i<MAX_TRACE_LINE;i++)
			if (!isprint(temp2[i]))
				temp2[i] = 0x20;
		write( fd_trace, temp2, MAX_TRACE_LINE );
		write( fd_trace, "\n", 1 );
                va_end(args);
                pthread_mutex_unlock(&trace_mutex);
	}
}

void error(char *msg)
{
	perror(msg);
}

class PServer
{
public :
	PServer(const char *, int  , int  );
	int InitComms();
	int IfNewCall(t_caller *);
	int RunJob(void *arg);
	static void *DoPJob(void *arg);
	//int FunctionOfTheJob(int);
	//int ShutDownTheJob(int);
	char myAddress[16];
	int myPort;
	int myTimeMax;
	int nCounter;
	int sDesc;
};

class Customer
{
public :
	Customer();
	int  traite(int fd, char *caller_address, int port );
};

Customer::Customer()
{
}

int PServer::InitComms()
{
	int nb_retry,fd,on,retour,size;
	struct sockaddr_in adr_service;  
	nb_retry = 0;                                        

	do {
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) sleep(2);
		nb_retry ++;                           
	}
	while ((fd < 0) && (nb_retry < MAX_RETRY));
	if(fd < 0) {
		FPRINTF ( TRACE,"\nCan't open stream socket, errno <%d> ",
  				  errno);  
		return(ECHEC);
	}

	on=1 ;
	if((retour=setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, 
			      &on,sizeof(on)))!= 0){
		FPRINTF ( TRACE,"\nCan Not Set The Option SO_KEEPALIVE\n");
		return ( ECHEC );
	}

	on=1 ;
	if(( retour=setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			       &on,sizeof(on)))!= 0) {
		FPRINTF ( TRACE,"\nCan Not Set The Option SO_REUSEADDR\n");
		return ( ECHEC );
	}

	adr_service.sin_family       = AF_INET;
	adr_service.sin_addr.s_addr  = INADDR_ANY;
	adr_service.sin_port         = htons(myPort);

	nb_retry = 0;
	do {
		retour=bind(fd, (struct sockaddr *)&adr_service,
			    sizeof(adr_service));

		if (retour < 0) 
			sleep(2);
		nb_retry ++;
	}

	while( (retour < 0) && (nb_retry < MAX_RETRY) )
		;

	if(retour < 0) {
		FPRINTF(TRACE , "\nCan't bind local address, <%d>...", errno );
		return(ECHEC);
	}
	FPRINTF(TRACE , "\ntcp/ip server bind done ...");
	listen(fd, 500 );
	FPRINTF(TRACE , "\ntcp/ip server listning ...\n");

	sDesc = fd;
	return( SUCCES );
}

PServer::PServer(const char *Adresse, int Port, int TimeMax)
{
	memset ( myAddress, 0, sizeof ( myAddress ));
	memcpy ( myAddress, Adresse, strlen ( Adresse ));
	myPort = Port ;
	myTimeMax = TimeMax ;
	nCounter = 0;
}

int PServer::IfNewCall(t_caller  *caller)
{
	int retour;
	int long_inet;
	struct sockaddr_in adr_client;
	char ligne[134];
	int FdAccept;
	unsigned char adr[4];
	int offset;
	char calling_address [ 15 + 1 ];
	int nb_retry;
	int ERRNO;
	int Errno;


	long_inet = sizeof(struct sockaddr_in);

	FPRINTF ( TRACE,"\ntcp/ip server: waiting for a call.... ");
	nb_retry = 0;
	do {            
		FdAccept = accept(sDesc, (struct sockaddr *)&adr_client,
				  (socklen_t *)&long_inet);
		Errno = errno;
		if ( FdAccept < 0) sleep(2);
		nb_retry ++;
	} while((FdAccept < 0) && (nb_retry < MAX_RETRY) && (Errno != EINTR));

	if (FdAccept < 0 ) {
		FPRINTF(TRACE , "tcp/ip server: can't accept a call <%d><%d>"
			  ,sDesc, errno );
		return(ECHEC);
	}

	memset(adr, 0, sizeof(adr));
	FPRINTF(FLOW, "End   AcceptCall(%d)", FdAccept);
	memset(caller->caller_address, 0, sizeof (caller->caller_address));
	strcpy(caller->caller_address, (char *)inet_ntoa(adr_client.sin_addr));
	caller->caller_port = adr_client.sin_port;
	caller->fd = FdAccept;
//        caller->statut=CONNECTED;
        caller->nb_messages_received=0;
        caller->nb_messages_sent=0;
      //  Sn->UpdateSocketInfo(caller->caller_address,caller->caller_port,_CONNECTED);    
   
	return(SUCCES);
}

int PServer::RunJob(void *arg)
{
	pthread_attr_t  pattr;
	pthread_t threadId;

	pthread_attr_init(&pattr);
	pthread_attr_setscope (&pattr, PTHREAD_SCOPE_PROCESS );
	pthread_create((pthread_t *)&(threadId), NULL, DoPJob, arg );
}




int  Customer::traite(int fd, char *caller_address, int caller_port )
{
	int nLength; 
	char receivedLineMessage[MAXLEN];
	char responseLineMessage[MAXLEN]; 


	FPRINTF(TRACE,"\nIn Traitement desc = <%d> caller <%s> port <%d> \n",
		fd,caller_address , caller_port );

	memset(receivedLineMessage, 0, sizeof(receivedLineMessage));
        
	while(1) {
		nLength = recv(fd,receivedLineMessage, MAXLEN,0);
		if (( nLength <= 0 )  || ( errno == ECONNRESET ))
		{
			FPRINTF(TRACE,"\nTcp/Ip Line Disconnect.....");
			close(fd);
                        //Sn->UpdateSocketInfo( caller_address, caller_port , _DISCONNECT );
			return ( ECHEC );
		}
                //Sn->UpdateSocketInfo( caller_address, caller_port , _MESSAGE_RECEIVED );

		FPRINTF(TRACE,"\nMessage Received = <%s>",
			receivedLineMessage );
		memset(responseLineMessage, 0, sizeof(responseLineMessage));
		sprintf(responseLineMessage, "RESP_%s", receivedLineMessage );
		nLength = write(fd, responseLineMessage, 
				strlen(responseLineMessage));
               
               
                
		if ( nLength !=  strlen ( responseLineMessage )) {
			FPRINTF(TRACE,"\nWrite To Tcp/Ip Line error, errno <%d>...", errno );
                        //Sn->UpdateSocketInfo( caller_address, caller_port , _PB_WRITE_TO_SOCKET );
			return ( ECHEC );
		} 
                //Sn->UpdateSocketInfo( caller_address, caller_port , _MESSAGE_SENT );       
	}

   
	return (SUCCES);
}


void *PServer::DoPJob(void *arg)
{
	int number;
	t_caller *caller;
	char caller_address[16];
	int port;
	int fd;

	pthread_detach (pthread_self());

	caller = ((t_caller *) arg);

	fd = caller->fd;
	memset(caller_address, 0, sizeof(caller_address));
	strcpy(caller_address, caller->caller_address);
	port = caller->caller_port;
	free(arg);

	Customer C;
	C.traite ( fd, caller_address , port );
	pthread_exit(NULL);
}

int main()
{
	int retour = 0;
	int ntries = 0;
	int nDesc;
	char caller_address[16];
	PServer P("127.0.0.1", 5011, 1 );
        
        
        
	do { 
		retour = P.InitComms(); 
	} while ( retour < 0 && ntries ++ < nMax );
	// ShutDown if Not Possible 

	t_caller *caller;

        do {
		caller = (t_caller *)malloc(sizeof(t_caller));
		memset(caller->caller_address, 0,
		       sizeof(caller->caller_address));
		caller->fd = 0;
		if ((retour = P.IfNewCall( caller )) == SUCCES)
			P.RunJob ( caller );
	} while ( retour == SUCCES );
}
