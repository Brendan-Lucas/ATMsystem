#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

//////// DATA BASE Entrys //////////
typedef struct data_base_entry{
  char   accnum[6];
  double fundsav;
	char   pin[4];

} DB;

//message truct for sending between atm and  DBserver.
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

void *atmfunc(void *msqid);
void *dbServer(void *msqid);
void *dbEditor(void *zero);
void getInput(char *question, char *returnString, int inputLength);
char* getFeild(char* line, int n, char c);
void getFeild2(char* line, int n, char* retrunStr);
void initBuff(my_msgbuf* buf, int msgtypeNum);
void makeLine(char* line, char* arg1, char* arg2, char* arg3);

//Globals
//constants
#define NUMoTHREADs 3

pthread_t threads[NUMoTHREADs];
///this is a shared variable of type DB//TODO: get rid of this one;
///////////////______MAIN_____////////////////////////
int main(){
	///////// Message Queue creation///////
	int msqid;
	int msgtype;
  int msgflg = 0666 | IPC_CREAT;
  key_t key;
  my_msgbuf sbuf,rbuf;
  size_t buf_length;
	key=1234;
	if ((msqid = msgget(key, msgflg)) < 0) {
        		perror("msgget");
        		exit(1);
    }else{
				(void)fprintf(stderr,"msgget: msgget succeeded: msqid = %i\n", msqid);
	}
  //IT'S PTHREAD TIME:::
  if(pthread_create(&threads[0], NULL, dbEditor, (void *)0)){
			printf("ERROR: pthread 2 supposed to return 0\n");
			exit(1);
	}
  pthread_join(threads[0], NULL);

	int rc=0;
  if(rc=pthread_create(&threads[1], NULL, atmfunc, (void *)(long)msqid)){
			printf("ERROR: pthread 0 supposed to return 0\n");
			exit(1);
	}else{
			printf("pthreadsuccessful return code = %i\n",rc);
	}
while(true){
    if(pthread_create(&threads[2], NULL, dbServer, (void *)(long)msqid)){
        printf("ERROR: pthread 1 supposed to return 0\n");
        exit(1);
    }else {
       printf("New server\n");
    }
    pthread_join(threads[2], NULL);
}

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
  std::string tempstr;
  char tempAccNum[6];
	int uchoice1;
	char upin1[4];
	int msqid = (int)(long)msq;
	printf("Message que id as recieved is: %i\n", msqid);
	int toServer = 1;
	int fromServer = 2;
  initBuff(&sbuf, toServer);
  initBuff(&rbuf, fromServer);
	int msgLength = sizeof(rbuf.contents);

	for(int attemptCount = 3; attemptCount>0; attemptCount--){
	    getInput((char*)"ATM: Enter your Account Number:", uaccnum1, 5);
	    getInput((char*)"ATM: Enter your pin number:", upin1, 3);
	    if(strcmp(tempAccNum, uaccnum1)!=0) {
					attemptCount=3;
	        strcpy(tempAccNum, uaccnum1);
	    }
	    strcpy(sbuf.contents.uaccnum, uaccnum1);
	    strcpy(sbuf.contents.upin, upin1);
      if(attemptCount==1)sbuf.contents.stat=1;
      printf("ATM: Sending Message\n");
	    if(msgsnd(msqid, &sbuf, msgLength, IPC_NOWAIT) < 0 ){// sending account number
	        perror("ATM: msgsnd\n");
	        exit(1);
	    }
      printf("ATM: Message sent\n");
	    ////////
	    if (msgrcv(msqid, &rbuf, msgLength, fromServer, 0) < 0) {//receiving 1 if account exist and 0 if it doesnt
	        perror("ATM: msgrcv\n");
	        exit(1);
	    }
      printf("ATM: Message recieved\n");
	    if(rbuf.contents.stat==1){//then the account number exist
	        attemptCount = 4; //sets to 4 so that the loop will start again with attemptCount == 3;
	        uchoice1=3;
	        while (uchoice1>2 || uchoice1<0){
	            printf("ATM: Choose from the following menu:\n 1- Display Funds \n 2- Withdraw Funds\n ");
	            scanf("%i", &uchoice1);
	        }
	        sbuf.contents.uchoice=uchoice1;

	        if(uchoice1==2){//Withdraw amount
	            printf("ATM: Please enter the amount you would like to withdraw: \n");
	            scanf("%lf", &sbuf.contents.uwithdraw);
	        }

	        if (msgsnd(msqid, &sbuf, msgLength, IPC_NOWAIT) < 0) {// Withdrawing funds
	            perror("ATM: msgsnd");
	            exit(1);
	        }

	        if (msgrcv(msqid, &rbuf, msgLength, fromServer, 0) < 0) {//receiving available funds
	            perror("ATM: msgrcv");
	            exit(1);
	        }

	        if(uchoice1==1){//user chose to see funds available
	  					printf("ATM: Account Balance: %lf\n", rbuf.contents.ufundsav);
	        }else if(uchoice1==2){
	            if(rbuf.contents.stat==3){
	                printf("ATM: not enough funds\n");
	            }else {
	                printf("ATM: Enough funds\nNew Account Balance: %lf\n", rbuf.contents.ufundsav);
	            }
          }
      }
    }
};

void *dbServer(void *msq){
    my_msgbuf rbuf, sbuf;
    int toATM = 2;
    int fromATM = 1;
    char lines[100][50];
    bool found = false;
    int msqid = (int)(long)msq;
    int msgLength = sizeof(rbuf.contents);
    DB shared[100];
    FILE* io;
    char line[1024];
    int sharedSize=0;
    printf("Server: entered server\n");
    while (rbuf.contents.stat!=2){
      found=false;
      initBuff(&rbuf, fromATM);
      initBuff(&sbuf, toATM);

      if (msgrcv(msqid, &rbuf, msgLength, fromATM, 0) < 0) { //what is message type, I wrote it accnum in this case
          perror("msgrcv");
          exit(1);
      }
      printf("Server: message recieved \n");
      //fill shared data
      io = fopen("database.txt", "r");
      for(sharedSize=0; fgets(line, 1024, io); sharedSize++){
          strcpy(lines[sharedSize], line);
          getFeild2(line,  1, shared[sharedSize].accnum);
          getFeild2(line,  2, shared[sharedSize].pin);
          shared[sharedSize].fundsav = atof(getFeild(line, 3, ','));
      }
      fclose(io);

      printf("Server: finished initializing shared\n");
      for(int i=0; i<sharedSize && !found; i++){
          printf("Server: account number is %s, while account number #%d is %s \n", rbuf.contents.uaccnum, i, shared[i].accnum);
          if(strcmp(rbuf.contents.uaccnum, shared[i].accnum)==0){
              found=true;
              printf("Server: first if reached in accnumber= %s..\n", shared[i].accnum);
              printf("	the pin we have for this account is: %s, and expected pin is: %s\n", rbuf.contents.upin, shared[i].pin);
              if(strcmp(rbuf.contents.upin, shared[i].pin)==0){ //pin nuber correct
                  printf("Server: seccond if reached stat is one\nServer: sending message\n");
                  sbuf.contents.stat=1;
                  if(msgsnd(msqid, &sbuf, msgLength, IPC_NOWAIT) < 0 ){
                      perror("Server: msgsnd");
                      exit(1);
                  }
                  //User makes choice about weather they want to deposit or withdraw funds and how much they want to withdraw;
                  if (msgrcv(msqid, &rbuf, msgLength, fromATM, 0) < 0) { //what is message type, I wrote it accnum in this case
                       perror("Server:msgrcv");
                       exit(1);
                  }
                  printf("Server: Choice message recieved\n");
                  if(rbuf.contents.uchoice==1){//if it is equal to one then user chose to see available funds
                      sbuf.contents.ufundsav=shared[i].fundsav;
                      printf("Server: sending back available funds\n");
                  }else if(rbuf.contents.uchoice==2){//user chose to withdraw amount
                      if(rbuf.contents.uwithdraw<=shared[i].fundsav){
                          shared[i].fundsav = shared[i].fundsav - rbuf.contents.uwithdraw;
                          sbuf.contents.ufundsav = shared[i].fundsav;
                          printf("Server: returning successful withdrawral funds left\n..rewriting Database...\n");
                          stringstream ss;
                          ss << (double)shared[i].fundsav;
                          makeLine(lines[i], shared[i].accnum, shared[i].pin, strdup(ss.str().c_str()));
                          printf("Server: %s\n", lines[i]);
                          //REWRITE
                          io = fopen("database.txt", "w+");
                          for (int j=0; j<sharedSize; j++){
                              fputs(lines[j], io);
                          }
                          fclose(io);
                      }else{
                            sbuf.contents.stat=4;
                            printf("Server: Returning not enough fundsav\n");
                      }
                  }
              }else{//incorrect pin number
                if(rbuf.contents.stat==1){
                    printf("Server: 3rd Attempt, Locking Account Num %s\n",rbuf.contents.uaccnum);
                    sbuf.contents.stat=5;
                    lines[i][0]='x';
                    //REWRITE
                    io = fopen("database.txt", "w+");
                    for (int j=0; j<sharedSize; j++){
                        fputs(lines[j], io);
                    }
                    fclose(io);
                }else sbuf.contents.stat=0;
              }
          }
      }
      if (!found){
          sbuf.contents.stat=3;
          printf("Server: returning Invalid Account number\n");
      }

      if(msgsnd(msqid, &sbuf, msgLength, IPC_NOWAIT) < 0 ){
          perror("msgsnd");
          exit(1);
      }
    }//ENDWHILE

		printf("Server: exiting\n");
};

void *dbEditor(void *zero){
    char newpin[4];
    double amount;
    char amountStr[50];
    char decision=0x00;
    char newaccnum[6];
		bool found = false;
    char temp=0x00;
		char line[1024];
    int accnumLine=0;
		char lines[100][50];
    FILE* io;
		char *initialArr[4] = {(char*)"00001,107,3443.22,\n", (char*)"00011,323,10089.97,\n", (char*)"00117,259,112.00,\n", (char*)NULL};
		printf("enteredDBEditor\n");
		printf("would you like to set the database back to default? [Y|n]\n");
		scanf("%c", &decision);
    if (decision == 'y' || decision == 'Y'){
	      io =  fopen("database.txt", "w+");
				for (int i=0; i<3; i++) {
						printf("in for %i\n", i);
						fputs(initialArr[i], io);
				}
        fclose(io);
				printf("reset file\n");
		}
		printf("oppenned File data\n");
		printf("Make changes to database?? [Y|n]\n");
    scanf("%c", &temp);
    scanf("%c", &decision);
		if (decision == 'y' || decision == 'Y'){
				printf("enter an account number, if it exiests you will be asked to delete or edit it\n");
				printf("if it does not, it will be added and you will be asked to edit its specifics\n");
				scanf("%s", newaccnum);
				int lncount=0;
				found = false;
        io =  fopen("database.txt", "r+");
				while(fgets(line, 1024, io)){
						strcpy(lines[lncount], line);
						if (strcmp(getFeild(line, 1, ','), newaccnum)==0){
								found = true;
								accnumLine = lncount;
						}
						lncount++;
				}
        fclose(io);
        for(int i=0; i<lncount; i++)printf("line at %i = %s\n", i, lines[i]);
				if(found){
						printf("Account number found would you like to delete or edit?[d|e]\n");
            scanf("%c", &temp);
            scanf("%c", &decision);
            if(decision == 'd' || decision == 'D'){
                //REWRITE
                io = fopen("database.txt", "w+");
                for (int i=0; i<lncount; i++){
                    if(i!=accnumLine)fputs(lines[i], io);
                }
                fclose(io);
            }else {
                printf("would you like to change the account ballance or the pin? [b/p]\n");
                scanf("%c", &temp);
                scanf("%c", &decision);
                if(decision == 'b' || decision == 'B'){
                    getInput((char*)"What would you like to change the balance to?\n", amountStr, 49);
                    strcpy(newpin, getFeild(lines[accnumLine], 2, ','));
                    makeLine(lines[accnumLine], newaccnum, newpin, amountStr);
                    printf("line: %s\n", lines[accnumLine]);
                    //REWRITE
                    io = fopen("database.txt", "w+");
                    for (int i=0; i<lncount; i++){
                        fputs(lines[i], io);
                    }
                    fclose(io);
                }else{
                    getInput((char *)"What would you like to change the PIN to?", newpin, 3);
                    strcpy(amountStr, getFeild(lines[accnumLine], 3, ','));
                    makeLine(lines[accnumLine], newaccnum, newpin, amountStr);
                    //REWRITE
                    io = fopen("database.txt", "w+");
                    for (int i=0; i<lncount; i++){
                        fputs(lines[i], io);
                    }
                    fclose(io);
                }
            }
				}else {
            accnumLine=lncount;
            lncount++;
            printf("Account not found,\n");
            printf("..Adding account number to database\n");
            getInput((char *)"Please enter the pin for the account (3 characters please)", newpin, 3);
            getInput((char *)"Please enter the new account balance", amountStr, 49);
            makeLine(lines[accnumLine], newaccnum, newpin, amountStr);
            printf("Printing new line to file\n");
            io=fopen("database.txt", "w+");
            for (int i=0; i<lncount; i++){
                fputs(lines[i], io);
            }
            fclose(io);
        }
		}
    printf("Exiting database editor\n");
}

void getInput(char *question, char *returnString, int inputLength){
    std::string tempstr;
    printf("%s *you may only use %d charavters:\n", question, inputLength);
    cin >> tempstr;
    strcpy(returnString, strdup(tempstr.substr(0,inputLength).c_str()));
};

char* getFeild(char* line, int n, char c){
		char* tok;
		for (tok = strtok(line, &c); tok && *tok; tok = strtok(NULL, ("%c\n",&c))){
				if (!--n)
						return tok;
		}
		return NULL;
}

void getFeild2(char* inLine, int n, char* returnStr){
  char* tok;
  char* line = strdup(inLine);
  for(tok = strtok(line, ","); tok && *tok && n; tok = strtok(NULL, ("%c\n", ","))){
      if (!--n)
          strcpy(returnStr, tok);
  }
  return;
}

void initBuff(my_msgbuf* buf, int msgtypeNum){
    buf->msgtype=msgtypeNum;
    strcpy(buf->contents.upin, (char*)"upi");
    buf->contents.uchoice=0;
    strcpy(buf->contents.uaccnum, (char*)"*****");
    buf->contents.stat=0;
    buf->contents.ufundsav=0.0;
    buf->contents.uwithdraw=0.0;
}

void makeLine(char* line, char* arg1, char* arg2, char* arg3){
    strcpy(line, arg1);
    strcat(line,",");
    strcat(line, arg2);
    strcat(line, ",");
    strcat(line, arg3);
    strcat(line, ",\n");
}
//void rewrite()
