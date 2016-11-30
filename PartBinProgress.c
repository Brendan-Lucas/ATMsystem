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

void atmfunc(void);
void DB_Server(void);

//db edditor declared
int SemaphoreWait(int semid, int iMayBlock );
int SemaphoreSignal(int semid);
void SemaphoreRemove(int semid);
int SemaphoreCreate(int iInitialValue);


int shmId;
int semID;

int uaccnum;
int upin;
long ATMtoDBSERVER = 1;
long DBSERVERtoATM = 2;

//////// This is the DATA BASE //////////
typedef struct {
  int accnum;
  int pin;
  double fundsav;

} DB;

DB* shared;///this is a shared variable of type DB


//this is the first buffer that will be transferd between ATM and DB server
typedef struct msgbuf {
    long    msgtype;
    char    upin[3];
    int     stat; // 1 if ok, 0 if wrong input
    char    uaccnum[5];
    int     uchoice;
    double  ufundsav;
    double  uwithdraw;
} message_buf;



void main(int argc, char **argv){

    //Test variables:
    char* ACCOUNT= "00342";
    char* PIN = "123";

	///////// Message Queue creation///////
	pid_t pid;
	pid_t pidd;

	/*semID=SemaphoreCreate(1);

	shmId = shmget(IPC_PRIVATE, 100 * sizeof(DB) , SHM_MODE);

	if((shmat(shmId, (DB*)0, 0)) == (int*)-1){
		printf("Error allocating memory\n");
	}
	shared = (DB*) shmat(shmId, (DB*)0,0);
*/
	pid=fork();

    if(pid==0){
        // Child (ATM)
        DB_Server();
    }else{
        pidd = fork();
        if(pidd == 0){
            // Child (DB_Server)
            atmfunc();
        }else{
            // Parent
            // DB_Editor
            sleep(100);
        }
    }

}


///////////////////////////////////////////////////////////////
////////////////// My functions////////////////////////////////
///////////////////////////////////////////////////////////////


void atmfunc(void){
    message_buf sbuf;
	char uaccnum1[5];
	char upin1[3];
	char* tempAccNum = "*****";
	int uchoice1;
	char noInfo;
    key_t key = (key_t) 1234;
    int msqid;
    int msgLength;

    msgLength = sizeof(sbuf)-sizeof(long);
    msqid = msgget(key, (IPC_CREAT | 0600));

    printf("Enter your Account Number: \n");
    scanf("%s", uaccnum1[0]);
    printf("Enter your PIN\n");
    scanf("%s", upin1[0]);

    sbuf.msgtype = 1;
    sbuf.stat = 1;
    strcpy(sbuf.uaccnum, uaccnum1);
    sbuf.uchoice = 1;
    sbuf.ufundsav = 2;
    strcpy(sbuf.upin, upin1);
    sbuf.uwithdraw = 100;

    if(msgsnd(msqid, &sbuf, msgLength, 0) < 0 ){// sending account number
        perror("msgsnd");
        //exit(1);
    }

    /*
	for(int attemptCount = 3; attemptCount>0; attemptCount--){

        noInfo = 0x01;

        while(noInfo){
            printf("Enter your Account Number: \n");
            fgets(uaccnum1, sizeof(uaccnum1), stdin);
            printf("Enter your PIN\n");
            fgets(upin1, sizeof(upin1), stdin);
            if(noInfo = (sizeof(uaccnum1)==5*sizeof(char) && sizeof(upin1)==3*sizeof(char)))
                printf("please enter 5 digit account number and 3 digit pin");
        }

        if(tempAccNum != uaccnum1) {
            attemptCount=3;
            tempAccNum = uaccnum1;
        }

        strcpy(sbuf.uaccnum, uaccnum1);
        strcpy(sbuf.upin, upin1);
        sbuf.msgtype = 1;

        if(msgsnd(msqid, &sbuf, msgLength, 0) < 0 ){// sending account number
            perror("msgsnd");
            exit(1);
        }
        ////////
        /*if (msgrcv(msqid, &rbuf, sizeof(sbuf)-sizeof(long), DBSERVERtoATM, 0) < 0) {//receiving 1 if account exist and 0 if it doesnt
            perror("msgrcv");
            exit(1);
        }
        printf("%i", rbuf.stat);
        */
        /*if(rbuf.stat==1){//then the account number exist
            attemptCount = 4; //sets to 4 so that
            uchoice1=3;
            while (uchoice1>2 || uchoice1<0){
                printf(" Choose from the following menu:\n 1- Display Funds \n 2- Withdraw Funds \n ");
                scanf("i%", uchoice1);
            }
            sbuf.uchoice=uchoice1;
            if(uchoice1==2){//Withdraw ammount
                printf("please enter the amount you would like to withdraw: \n");
                scanf("f%", sbuf.uwithdraw);
            }

            if (msgsnd(msqid, &sbuf, sizeof(sbuf)-sizeof(long), IPC_NOWAIT) < 0) {// Withdrawing funds
                perror("msgsnd");
                exit(1);
            }

            if (msgrcv(msqid, &rbuf, sizeof(sbuf)-sizeof(long), DBSERVERtoATM, 0) < 0) {//receiving available funds
                perror("msgrcv");
                exit(1);
            }

            if(uchoice1==1){//user chose to see funds available
	    		printf("Account Balance: %d\n",rbuf.ufundsav);
            }else if(uchoice1==2){
                if(rbuf.stat==3){
                    printf("not enough funds\n");
                    rbuf.stat=1;
                }else {
                    printf("Enough funds\nNew Account Balance: %d",rbuf.ufundsav);
                }
            }
        }*/
    //}


}

// ================================================================================= //
// DB_Server
void DB_Server()
{

    message_buf rbuf;
    key_t key = (key_t) 1234;
    int msqid;
    int msgLength;

    msqid = msgget(key, IPC_CREAT);
    msgLength = (sizeof(rbuf)-sizeof(long));

    if (msgrcv(msqid, &rbuf, msgLength, 1, 0) == -1) { //what is message type, I wrote it accnum in this case
        perror("msgrcv");
        //exit(1);
    }

    printf("recieved message in DBserver\n");
    printf("%s\n", rbuf.uaccnum);


    /*if (rbuf.stat == 0){
        if(rbuf.uaccnum == ACCOUNT && rbuf.upin == PIN){
            printf("message recieved and account and pin match\n");
            sbuf.stat = 1;
        } else sbuf.stat = 0;
        printf("sending message\n");
        sbuf.msgtype = DBSERVERtoATM;
        if(msgsnd(msqid, &sbuf, sizeof(sbuf)-sizeof(long), IPC_NOWAIT) < 0 ){// sending account number
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
    else printf("something wierd happenned printing stat: %i", rbuf.stat);*/
}

/*
void dbServer(){
//TODO: set stat to 3 if not enough funds in server
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
