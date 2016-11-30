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

int uaccnum;
int upin;
long ATMtoDBSERVER = 1;
long DBSERVERtoATM = 2;

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


void main()
{
	message_buf sbuf;
	char uaccnum1[5];
	char upin1[3];
	char* tempAccNum = "*****";
	int uchoice1;
	char noInfo;
    key_t key = ftok("Somefile.txt", 1);
    int msqid;
    int msgLength;

    msqid = msgget(key, (IPC_CREAT | 0600));
    msgLength = sizeof(sbuf)-sizeof(long);

    printf("Enter your Account Number: \n");
    scanf("%s", uaccnum1);
    printf("Enter your PIN\n");
    scanf("%s", upin1);
	
    sbuf.msgtype = 1;
    sbuf.stat = 1;
    strcpy(sbuf.uaccnum, uaccnum1);
    sbuf.uchoice = 1;
    sbuf.ufundsav = 2;
    strcpy(sbuf.upin, upin1);
    sbuf.uwithdraw = 100;

    if(msgsnd(msqid, &sbuf, msgLength, 0) == -1){// sending account number
        perror("msgsnd");
        //exit(1);
    }
    
    
}




