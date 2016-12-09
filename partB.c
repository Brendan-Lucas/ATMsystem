#include <stdlib.h>
#include <stdio.h>
//#include <stdin.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
//#include <mqueue.h>

#define SEM_MODE (0400 | 0200 | 044)
#define SHM_MODE (SHM_R | SHM_W | 044)


union semun {
	int val;
	struct semid_ds *buf;
	short * array;
} arg ;

void *atmfunc(void *msqid);
void *dbServer(void *msqid);
void *dbEditor(void);
void getInput(char *question, char *returnString, int inputLength);
//db edditor declared
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
  char   accnum[6];
  double fundsav;
	char   pin[4];

} DB;

DB* shared;///this is a shared variable of type DB


//this is the first buffer that will be transferd between ATM and DB server
typedef struct my_msgbuf {
    long    msgtype;
    struct msgContents{
				char    upin[4];
		    int     uchoice;
		    char    uaccnum[6];
		    int     stat; // 1 if ok, 0 if wrong input
		    double  ufundsav;
		    double  uwithdraw;
		}contents;
}my_msgbuf;

#define NUMoTHREADs 3

///////////////______MAIN_____////////////////////////
int main(){

  pthread_t threads[NUMoTHREADs];
  //Test variables:
  char* ACCOUNT= "00342";
  char* PIN = "123";
	///////// Message Queue creation///////
	int msqid;
	int msgtype;
  int msgflg = 0666 | IPC_CREAT;
  key_t key;
  my_msgbuf sbuf,rbuf;
  size_t buf_length;
	key=1234;
	if ((msqid =(int)msgget(key, msgflg)) < 0) {
        		perror("msgget");
        		exit(1);
    }else{
				(void)fprintf(stderr,"msgget: msgget succeeded: msqid = %i\n", msqid);
	}

	semID=SemaphoreCreate(1);
	shmId = shmget(IPC_PRIVATE, 100 * sizeof(DB) , SHM_MODE);
	if((shmat(shmId, (DB*)0, 0))  ==(int*)-1){
		printf("Error allocating memory\n");
	}
	shared = (DB*) shmat(shmId, (DB*)0,0);

  //Initialize shared memory
	strcpy(shared[0].accnum, "00123");
	strcpy(shared[0].pin, "001");
	shared[0].fundsav = 123.7;

  for(int i=3;i<100;i++){
      strcpy(shared[i].accnum, "00000");
  }
	int rc=0;
  if(rc=pthread_create(&threads[0], NULL, atmfunc, (void *)(long)msqid)){
			printf("ERROR: pthread 0 supposed to return 0\n");
			exit(1);
	}else{
			printf("pthreadsuccessful return code = %i\n",rc);
	}

  if(pthread_create(&threads[1], NULL, dbServer, (void *)(long)msqid)){
      printf("ERROR: pthread 1 supposed to return 0\n");
      exit(1);
	}

	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	printf("exiting main\n");
  pthread_exit(NULL);
  return 0;
};

//***************Functions*************************//

void *atmfunc(void *msq){
	printf("reached beginning of atmfunc\n");
	my_msgbuf sbuf, rbuf;
	char uaccnum1[6];
	int uchoice1;
	char upin1[4];
	int msqid = (int)(long)msq;
	char *tempAccNum;
	printf("Message que id as recieved is: %i\n", msqid);
	int toServer = 1;
	sbuf.msgtype = toServer;
	int fromServer = 2;
	rbuf.msgtype=fromServer;
	int msgLength = sizeof(rbuf.contents);

	for(int attemptCount = 3; attemptCount>0; attemptCount--){
	    //getInput("Enter your Account Number:", uaccnum1, 5);
			strcpy(uaccnum1, "00123");
			printf("%s\n", uaccnum1);
	    //getInput("Enter your PIN: ", upin1, 3);
			strcpy(upin1, "001");
			printf("%s\n", upin1);

	    if(strcmp(tempAccNum, uaccnum1)!=0) {
					printf("entered if\n");
					attemptCount=3;
	        strcpy(tempAccNum, uaccnum1);
	    }
	    strcpy(sbuf.contents.uaccnum, uaccnum1);
	    strcpy(sbuf.contents.upin, upin1);
			printf("Sending Message\n");

	    if(msgsnd(msqid, &sbuf, msgLength, IPC_NOWAIT) < 0 ){// sending account number
	        perror("msgsnd\n");
	        exit(1);
	    }
	    ////////
	    if (msgrcv(msqid, &rbuf, msgLength, fromServer, 0) < 0) {//receiving 1 if account exist and 0 if it doesnt
	        perror("msgrcv\n");
	        exit(1);
	    }
	    if(rbuf.contents.stat==1){//then the account number exist
	        attemptCount = 4; //sets to 4 so that the loop will start again with attemptCount == 3;
	        uchoice1=3;
	        while (uchoice1>2 || uchoice1<0){
	            printf("Choose from the following menu:\n 1- Display Funds \n 2- Withdraw Funds \n ");
	            scanf("%i", &uchoice1);
	        }
	        sbuf.contents.uchoice=uchoice1;

	        if(uchoice1==2){//Withdraw ammount
	            printf("please enter the amount you would like to withdraw: \n");
	            scanf("%lf", &sbuf.contents.uwithdraw);
	        }

	        if (msgsnd(msqid, &rbuf, msgLength, IPC_NOWAIT) < 0) {// Withdrawing funds
	            perror("msgsnd");
	            exit(1);
	        }

	        if (msgrcv(msqid, &rbuf, msgLength, fromServer, 0) < 0) {//receiving available funds
	            perror("msgrcv");
	            exit(1);
	        }

	        if(uchoice1==1){//user chose to see funds available
	  					printf("Account Balance: %lf\n", rbuf.contents.ufundsav);
	        }else if(uchoice1==2){
	            if(rbuf.contents.stat==3){
	                printf("not enough funds\n");
	            }else {
	                printf("Enough funds\nNew Account Balance: %lf\n", rbuf.contents.ufundsav);
	            }
          }
      }
    }
};

void *dbServer(void *msq){
    my_msgbuf rbuf, sbuf;
    int toATM = 2;
		sbuf.msgtype = toATM;
    int fromATM = 1;
		rbuf.msgtype = fromATM;
    int msqid = (int)(long)msq;
    int msgLength = sizeof(rbuf.contents);
		int i=0;
		//TODO: set stat to 3 if not enough funds in server
		if (msgrcv(msqid, &rbuf, msgLength, fromATM, 0) < 0) { //what is message type, I wrote it accnum in this case
				perror("msgrcv");
				exit(1);
		}
		printf("Server: message recieved \n");
		for(i=0; i<100; i++){
				printf("Server: account number is %s, while account number #%d is %s \n", rbuf.contents.uaccnum, i, shared[i].accnum);
        if(strcmp(rbuf.contents.uaccnum, shared[i].accnum)==0){
						printf("Server: first if reached in accnumber= %s..\n", shared[i].accnum);
						printf("	the pin we have for this account is: %s, and expected pin is: %s\n", rbuf.contents.upin, shared[i].pin);
						if(strcmp(rbuf.contents.upin, shared[i].pin)==0){ //account nuber correct
								printf("Server: seccond if reached stat is one\nServer: sending message\n");
		            sbuf.contents.stat=1;
		            if(msgsnd(msqid, &sbuf, msgLength, IPC_NOWAIT) < 0 ){
		                perror("msgsnd");
		                exit(1);
		            }

		            if (msgrcv(msqid, &rbuf, msgLength, fromATM, 0) < 0) { //what is message type, I wrote it accnum in this case
		                 perror("msgrcv");
		                 exit(1);
		            }
								printf("Server: Choice message recieved\n");
		            if(rbuf.contents.uchoice==1){//if it is equal to one then user chose to see available funds
		                sbuf.contents.ufundsav=shared[i].fundsav;
										printf("Server: sending back available funds\n");
		                if(msgsnd(msqid, &sbuf, msgLength, IPC_NOWAIT) < 0 ){
		                    perror("msgsnd");
		                    exit(1);
		                }
		            }else if(rbuf.contents.uchoice==2){//user chose to withdraw ammount
		            		if(rbuf.contents.uwithdraw<=shared[i].fundsav){
		                		shared[i].fundsav = shared[i].fundsav - rbuf.contents.uwithdraw;
		                		sbuf.contents.ufundsav = shared[i].fundsav;
												printf("Server: returning successful withdrawral funds left\n");
		                		if(msgsnd(msqid, &sbuf, msgLength, IPC_NOWAIT) < 0 ){
		                    		perror("msgsnd");
		                      	exit(1);
		              			}
		                }else{
		                  		sbuf.contents.stat=4;
													printf("Server: Returning not enough fundsav\n");
		                      if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){
	                            perror("msgsnd");
	                            exit(1);
		                      }
		              	}
		            }
		        }else{//incorrect pin number
		            sbuf.contents.stat=0;
								printf("Server: returning Invalid Pin number\n");
		            if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){
		                perror("msgsnd");
		                exit(1);
		            }
        		}
						i=101;
				}
    }
		if (i==100){
				sbuf.contents.stat=3;
				printf("Server: returning Invalid Account number\n");
				if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){
						perror("msgsnd");
						exit(1);
				}
		}
};
/*

void *dbEditor(){


    char newpin[3];
    double amount;
    char newaccnum[5];
    char empty[5]={ 0, 0, 0, 0, 0 };
    printf("%s",empty);
    // int z =10425;
    //  while(z=10425){ no need for a while true or it will be stuck forever
    //we need to put a defult case ==> every empty slot in the array of 100 elemnt has an accnum of 00000
        for(int i=0;i<100;i++){
            if(!strcmp(shared[i].accnum, empty)){ //since it returns zero if they are equivalent

                printf("Enter New Account Number: \n");
                scanf("%s",newaccnum);
                strcpy(shared[i].accnum,newaccnum);
                printf("Enter new pin");
                scanf("%i",newpin);
                strcpy(shared[i].pin,newpin);
                printf("enter the amount of the deposite/withdraw");
                scanf("%f",amount);
                shared[i].fundsav= shared[i].fundsav - amount;

                //send update to dbserver "HOW !??"
               // shared+=sizeof(DB);
            }else{

                printf("NO MORE SPACE FOR A NEW COSTUMER");

            }
        }
   // }


}
*/

void getInput(char *question, char *returnString, int inputLength){
		printf("%s *you may only use %d charavters:\n", question, inputLength);
		//scanf("%s", returnString);
		printf("returning\n");
};







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
