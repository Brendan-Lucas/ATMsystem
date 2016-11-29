#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <sys/types.h>
//#include <mqueue.h>

#define SEM_MODE (0400 | 0200 | 044)
#define SHM_MODE (SHM_R | SHM_W | 044)


union semun {
	int val;
	struct semid_ds *buf;
	ushort * array;
} arg ;


int SemaphoreWait(int semid, int iMayBlock );
int SemaphoreSignal(int semid);
void SemaphoreRemove(int semid);
int SemaphoreCreate(int iInitialValue);


int shmId;
int semID;

int uaccnum;
int upin;


//////// This is the DATA BASE //////////
typedef struct {	
  int accnum;
  int pin;
  double fundsav;

} DB;

DB* shared;///this is a shared variable of type DB


//this is the first buffer that will be transferd between ATM and DB server 
typedef struct msgbuf {
         //char[3] 	upin;
         //char[5]	uaccnum;
			int		upin;
			int      uaccnum;    
         int 		stat; // 1 if ok, 0 if wrong input
         int 		uchoice;
         double 	uwithdraw;
         double 	ufundsav;
         
} message_buf;
         
         
         
         
         
int main(){

	///////// Message Queue creation///////
	int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    message_buf sbuf,rbuf;
    size_t buf_length;
	key=1234;
	if ((msqid = msgget(key, msgflg )) < 0) {
        		perror("msgget");
        		exit(1);
    }else{
			 
		(void) fprintf(stderr,"msgget: msgget succeeded: msqid = %d\n", msqid);	
	}

	//setup message buff object. 
	sbuf.upin = 143;
	sbuf.uaccnum = 453;
	sbuf.uwithdraw = 365;	
	
	
		
	pid_t pid;
	pid_t pidd;
	
		
	semID=SemaphoreCreate(1);
	  	
	shmId = shmget(IPC_PRIVATE, 100 * sizeof(DB) , SHM_MODE);
	//depricated	 //(DB*) shmat(shmId, (DB*)0,0)) == (int*) -1)
	if((shmat(shmId, (DB*)0, 0))  ==(int*)-1){
		printf("Error allocating memory\n");	
	} 	
	shared = (DB*) shmat(shmId, (DB*)0,0);  
		
	pid=fork();
	if(pid<0){
	   printf("forking error\n"); 
	}
		///////////////////////////////////
		////////      ATM     ////////////
		/////////////////////////////////
	if(pid>0){
			
		if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){// sending account number
			perror("msgsnd");
			exit(1);	
		}  
		printf("successful message send (SenderSide)\n");				  
	}
			 
			      
		      
		///////////////////////////////////
		////////   DB server  ////////////
		/////////////////////////////////
				
	
	int msgtype=0;	

	///////////////////////////////////
	////////   DB EDITOR  ////////////
	/////////////////////////////////
	if(pid==0){
		if (msgrcv(msqid, &rbuf, sizeof(sbuf), msgtype, 0) < 0) { //what is message type, I wrote it accnum in this case  
        perror("msgrcv");
        exit(1);
    	}
    	printf("%i, and the pin number is: %i, and we want to msgType: %i ", rbuf.uaccnum, rbuf.upin, msgtype);
	
	//first child DB Editor
	  
	  
	}
   return 0;    
}
	
	


///////////////////////////////////////////////////////////////////////
////////////////// My functions////////////////////////////////
/////////////////////////////////////////////////////////////////////

/*
void atmfunc(){
	char* uaccnum1, upin1;
	int uchoice1=3;
	bool noInfo = true; 
	while(noInfo){
		printf("Enter your Account Number: \n");
		scanf(%s,uaccnum1);
		printf("Enter your PIN");
		scanf(s%,upin1);
		if(!(noInfo = (sizeOf(uaccnum1)==5*sizeof(char) && sizeof(upin1)==5*sizeof(char))))
			printf("please enter 5 digit account number and 3 digit pin");
	}
	sbuf.uaccnum=uaccnum1;
	sbuf.upin=upin1;
	
	
	if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){// sending account number
		perror("msgsnd");
		exit(1);
	}
	////////
	if (msgrcv(msqid, &rbuf, sizeof(sbuf), msgtype, 0) < 0) {//receiving 1 if account exist and 0 if it doesnt 
    	perror("msgrcv");
        exit(1);
    }
	if(rbuf.stat==1){//then the account number exist
		while(uchoice1>2){
			printf(" Choose from the following menu:\n 1- Display Funds \n 2- Withdraw Funds \n ");
			scanf(i%,uchoice1);
		}
		sbuf.uchoice=uchoise1;
		if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){// sending account number
			perror("msgsnd");
			exit(1);
		}
		if(uchoice==1){//user chose to see funds available
				if (msgrcv(msqid, &rbuf, sizeof(sbuf), ufundsav, 0) < 0) {//receiving available funds
	    			perror("msgrcv");
	        		exit(1);
	    		}
	    		printf("Account Balance: %d",rbuf.ufundsav);
	    		
		}else if(uchoice==2){//Withdraw ammount
			if (msgrcv(msqid, &rbuf, sizeof(sbuf), ufundsav, 0) < 0) {// Withdrawing funds
	    			perror("msgrcv");
	        		exit(1);
	    	}
	    	printf("New Account Balance: %d",rbuf.ufundsav);
			
		}
		
		}else if(rbuf.stat==0) {// wrong info; re-enter info
			printf("Enter your Account Number: \n");
			scanf(%i,uaccnum1);
			printf("Enter your PIN");
			scanf(i%,upin1);
			sbuf.uaccnum=uaccnum1;
			sbuf.upin=upin1;
	
		if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){// sending account number
			perror("msgsnd");
			exit(1);
		}
		
	}
	
}

void dbServer(){
	
	if (msgrcv(msqid, &rbuf, sizeof(sbuf), accnum, 0) < 0) { //what is message type, I wrote it accnum in this case  
        perror("msgrcv");
        exit(1);
    }
	
		for(i=0;i<100;i++){
			SemaphoreWait( semID, 1 ) ;
			if(rbuf.uaccnum==shared[i].accnum && rbuf.upin==shared[i].pin ){ //account nuber correct
			
				sbuf.stat=1;
				if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){
					perror("msgsnd");
					exit(1);
				}					
				if (msgrcv(msqid, &rbuf, sizeof(sbuf), stat, 0) < 0) { //what is message type, I wrote it accnum in this case  
	       			 perror("msgrcv");
	       			 exit(1);
	   			}
	   			if(rbuf.uchoice==1){//if it is equal to one then user chose to see available funds
	   				sbuf.ufundsav=shared.fundsav;
	   				if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){
						perror("msgsnd");
						exit(1);
					}
	   				
	   			}else if(rbuf.uchoice==2){//user chose to withdraw ammount 
	   				   if(rbuf.uwithdraw<=shared.fundsav){
						   shared[i]=sahred-rbuf.uwithdraw;
						   sbuf.ufundsav=shared[i];
						   if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){
								perror("msgsnd");
								exit(1);
						   }	
						   
	   			       }else{
	   			       		printf("Funds not available");
	   			       		return;
	   			       }
	   				
	   			}
				SemaphoreSignal( semID );	
				
			}else{//incorrect ack number 
				sbuf.stat=0;
				if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){
					perror("msgsnd");
					exit(1);
				}
				
			}	
			  		
		}
	
	
	
	
}

Void dbEditor(){
	
	
	
	
	
}



*/






///////////////////////////////////////////////////////////////////////
////////////////// Semaphore functions////////////////////////////////
/////////////////////////////////////////////////////////////////////


int SemaphoreWait(int semid, int iMayBlock ) {
struct sembuf
sbOperation;
sbOperation.sem_num = 0;
sbOperation.sem_op = -1;
sbOperation.sem_flg = iMayBlock;
return semop( semid, &sbOperation, 1 );
}


int SemaphoreSignal( int semid ) {struct sembuf
sbOperation;
sbOperation.sem_num = 0;
sbOperation.sem_op = +1;
sbOperation.sem_flg = 0;
return semop( semid, &sbOperation, 1 );
}


void SemaphoreRemove( int semid ) {
if(semid != -1 )
semctl( semid, 0, IPC_RMID , 0);
}


int SemaphoreCreate(int iInitialValue) {
int semid;
union semun suInitData;
int iError;
/* get a semaphore */
semid = semget( IPC_PRIVATE, 1, SEM_MODE );
/* check for errors */
if( semid == -1)
return semid;
/* now initialize the semaphore */
suInitData.val = iInitialValue;
if(semctl( semid, 0, SETVAL, suInitData) == -1 )
{ /* error occurred, so remove semaphore */

SemaphoreRemove(semid);
return -1;
}
return semid;
}
