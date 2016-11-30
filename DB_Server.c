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
	message_buf rbuf;
    key_t key = ftok("Somefile.txt", 1);
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
    printf("%s\n", rbuf.upin);


}






