#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <string.h>
//#include <mqueue.h>

#define SEM_MODE (0400 | 0200 | 044)
#define SHM_MODE (SHM_R | SHM_W | 044)


union semun {
	int val;
	struct semid_ds *buf;
	ushort * array;
} arg ;

void atmfunc(int msqid);
void dbServer(void);
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
  char   accnum[5];
  char   pin[3];
  double fundsav;

} DB;

DB* shared;///this is a shared variable of type DB


//this is the first buffer that will be transferd between ATM and DB server
typedef struct msgbuf {
    char    upin[3];
    char    uaccnum[5];
    int     stat; // 1 if ok, 0 if wrong input
    int     uchoice;
    double  ufundsav;
    double  uwithdraw;
} message_buf;



void dbEditor();

int main(){
    //Test variables:
    char* ACCOUNT= "00342";
    char* PIN = "123";
	///////// Message Queue creation///////
	int msqid;
	int msgtype;
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




	pid_t pid;
	pid_t pidd;


	semID=SemaphoreCreate(1);

	shmId = shmget(IPC_PRIVATE, 100 * sizeof(DB) , SHM_MODE);

	if((shmat(shmId, (DB*)0, 0))  ==(int*)-1){
		printf("Error allocating memory\n");
	}
	shared = (DB*) shmat(shmId, (DB*)0,0);
    
    for(int i=0;i<100;i++){
    ////////////////////////////////////
    ///// Modified!!!!/////////////////
    //////////////////////////////////
        strcpy(shared[i].accnum,{0,0,0,0,0,});
        
    }
	
	pid=fork();



	if(pid<0){
		printf("forking error\n");
	}
	else if(pid>0){
		//atmfunc(msqid);
		///////////////////////////////////
		////////      ATM     ////////////
		/////////////////////////////////

        
	}

	pidd=fork();

	if(pidd==0){
		//while(true){
			//dbServer();
		//}
		///////////////////////////////////
		////////   DB server  ////////////
		/////////////////////////////////
		//The second child " DB server"
		if (msgrcv(msqid, &rbuf, sizeof(sbuf), msgtype, 0) < 0) { //what is message type, I wrote it accnum in this case
            perror("msgrcv");
            exit(1);
        }
        if (rbuf.stat == 0){
            if(rbuf.uaccnum == ACCOUNT && rbuf.upin == PIN){
                printf("message recieved and account and pin match\n");
                sbuf.stat = 1;
            } else sbuf.stat = 0;
            printf("sending message\n");
            if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){// sending account number
                perror("msgsnd");
                exit(1);
            }
        }else if (rbuf.stat == 1){
            if (rbuf.uchoice == 1){
                printf("now showing accont number pretend you see it cause it worked\n");
            } else if (rbuf.uchoice ==2) {
                printf("now displaying the other thing like changing your bank account\nYou requested %f\n", rbuf.uwithdraw);
            }
            else printf("something wierd happenned printing u choice: %i", rbuf.uchoice);
        }
        else printf("something wierd happenned printing stat: %i", rbuf.stat);
	}//end DB server proccess specific



	if(pid==0){

		///////////////////////////////////
		////////   DB EDITOR  ////////////
		/////////////////////////////////
		//first child DB Editor


	}
    return 0;
}


////////////////////////////////////////////////////////////////
////////////////// My functions////////////////////////////////
//////////////////////////////////////////////////////////////


void atmfunc(int msqid){
    message_buf sbuf, rbuf;
	char* uaccnum1, upin1;
	char* tempAccNum = "*****";
	int uchoice1, msgtype;
	char noInfo;
	for(int attemptCount = 3; attemptCount>0; attemptCount--){
        noInfo = 0x01;
        while(noInfo){
            printf("Enter your Account Number: \n");
            scanf("%s",uaccnum1);
            printf("Enter your PIN");
            scanf("%s",upin1);
            if(noInfo = (sizeOf(uaccnum1)==5*sizeof(char) && sizeof(upin1)==5*sizeof(char)))
                printf("please enter 5 digit account number and 3 digit pin");
        }
        if(tempAccNum != uaccnum1) {
            attemptCount=3;
            tempAccNum = uaccnum1;
        }
        strcpy(sbuf.uaccnum, uaccnum1);
        strcpy(sbuf.upin, upin1);

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
            attemptCount = 4; //sets to 4 so that
            uchoice1=3;
            while (uchoice1>2 || uchoice1<0){
                printf(" Choose from the following menu:\n 1- Display Funds \n 2- Withdraw Funds \n ");
                scanf("i%", uchoice1);
            }
            sbuf.uchoice=uchoice1;

            if(uchoice==2){//Withdraw ammount
                printf("please enter the amount you would like to withdraw: \n");
                scanf("f%", sbuf.uwithdraw);
            }

            if (msgsnd(msqid, &rbuf, sizeof(sbuf), IPC_NOWAIT) < 0) {// Withdrawing funds
                perror("msgsnd");
                exit(1);
            }

            if (msgrcv(msqid, &rbuf, sizeof(sbuf), msgtype, 0) < 0) {//receiving available funds
                perror("msgrcv");
                exit(1);
            }

            if(uchoice1==1){//user chose to see funds available
	    		printf("Account Balance: %d\n",rbuf.ufundsav);
            }else if(uchoice1==2){
                if(rbuf.stat==3){
                    printf("not enough funds\n");
                    stat=1;
                }else {
                    printf("Enough funds\nNew Account Balance: %d",rbuf.ufundsav);
                }
            }
        }
    }
}

void dbServer(){
//TODO: set stat to 3 if not enough funds in server
	if (msgrcv(msqid, &rbuf, sizeof(sbuf), accnum, 0) < 0) { //what is message type, I wrote it accnum in this case
        perror("msgrcv");
        exit(1);
    }

    for(i=0;i<100;i++){
			SemaphoreWait( semID, 1 ) ;//do we actually have to wait ??
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
                              /////////////////////////////
                             // MODIFICATIONS HERE ///////
                            /////////////////////////////
	   			       		sbuf.stat=3;
	   			       		if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){
								perror("msgsnd");
								exit(1);

                       }

	   			}
				SemaphoreSignal( semID );// do we actually have tosignal ?

			}else{//incorrect ack number
				sbuf.stat=0;
				if(msgsnd(msqid, &sbuf, sizeof(sbuf), IPC_NOWAIT) < 0 ){
					perror("msgsnd");
					exit(1);
				}

			}
        }


    }




}




/////////////////////////////
// MODIFICATIONS HERE !!////
///////////////////////////
void dbEditor(){


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
