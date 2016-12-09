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

void *atmfunc(void *msqid);
void *dbServer(void *msqid);
void *dbEditor(void *zero);
void getInput(char *question, char *returnString, int inputLength);
char* getFeild(char* line, int n, char c);

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

	if(pthread_create(&threads[2], NULL, dbEditor, (void *)0)){
			printf("ERROR: pthread 2 supposed to return 0\n");
			exit(1);
	}

	//pthread_join(threads[0], NULL);
	//pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
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

	        if(uchoice1==2){//Withdraw amount
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
		rbuf.contents.uchoice=0;
		sbuf.contents.stat=0;
    DB shared[100];
    FILE* io = fopen("database.txt", "r+");
    char line[1024];
    int sharedSize=0;
    //fill shared data array.
    while(fgets(line, 1024, io)){
        strcpy(shared[sharedSize].accnum, getFeild(line,  1, ','));
        strcpy(shared[sharedSize].pin, getFeild(line, 2, ','));
        shared[sharedSize++].fundsav = atof(getFeild(line, 3, ','));
    }
		if (msgrcv(msqid, &rbuf, msgLength, fromATM, 0) < 0) { //what is message type, I wrote it accnum in this case
				perror("msgrcv");
				exit(1);
		}
		printf("Server: message recieved \n");
		for(i=0; i<sharedSize; i++){
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
		            }else if(rbuf.contents.uchoice==2){//user chose to withdraw amount
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
		printf("Server: exiting\n");
};

void *dbEditor(void *zero){
    char newpin[4];
    double amount;
    char *amountStr;
    char decision=0x00;
    char newaccnum[6];
		bool found = false;
    char temp=0x00;
		char line[1024];
    int accnumLine=0;
		char lines[100][50];
    FILE* io;
		char *initialArr[4] = {"00001,107,3443.22,\n", "00011,323,10089.97,\n", "00117,259,112.00,\n", NULL};
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
		io =  fopen("database.txt", "r+");

    decision=0x00;
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
				while(fgets(line, 1024, io)){
						strcpy(lines[lncount],line);
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
                    printf("What would you like to change the balance to?\n");
                    scanf("%c", &temp);
                    scanf("%s", amountStr);

                    strcpy(newpin, getFeild(lines[accnumLine], 2, ','));

                    strcpy(lines[accnumLine], newaccnum);
                    strcat(lines[accnumLine],",");
                    strcat(lines[accnumLine], newpin);
                    strcat(lines[accnumLine],",");
                    strcat(lines[accnumLine], amountStr);
                    strcat(lines[accnumLine],",\n");
                    //REWRITE
                    io = fopen("database.txt", "w+");
                    for (int i=0; i<lncount; i++){
                        fputs(lines[i], io);
                    }
                    fclose(io);
                }else{
                    printf("What would you like to change the PIN to?\n");
                    scanf("%s", newpin);

                    amountStr = getFeild(lines[accnumLine], 3, ',');
                    printf("amountstr= %s\n", amountStr);
                    strcpy(lines[accnumLine], newaccnum);
                    strcat(lines[accnumLine],",");
                    strcat(lines[accnumLine], newpin);
                    strcat(lines[accnumLine],",");
                    strcat(lines[accnumLine], amountStr);
                    strcat(lines[accnumLine],",\n");
                    printf("%s\n", lines[accnumLine]);
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
            printf("adding account number to database\n");
            printf("Please enter the pin for the account (3 characters please):\n");
            scanf("%c", &temp);
            for (int i=0; i<3; i++) scanf("%c", &newpin[i]);
            printf("Please enter the new account balance\n");
            //scanf("%s", amountStr);
            amount=90.9;
            printf("%f\n", amount);
            amountStr = "90.90";

            strcpy(lines[accnumLine],  newaccnum);
            strcat(lines[accnumLine],",");
            strcat(lines[accnumLine], newpin);
            strcat(lines[accnumLine],",");
            strcat(lines[accnumLine],  amountStr);
            strcat(lines[accnumLine],",\n");
            printf("%s\n", lines[accnumLine]);
            printf("Printing new line to file\n");
            io=fopen("database.txt", "w+");
            for (int i=0; i<lncount; i++){
                fputs(lines[i], io);
            }
            fclose(io);
            printf("exiting last part of edit\n");
        }
		}
    printf("exiting database editor\n");
    // int z =10425;
    //  while(z=10425){ no need for a while true or it will be stuck forever
    //we need to put a defult case ==> every empty slot in the array of 100 elemnt has an accnum of 00000
    /*for(int i=0;i<100;i++){
       if(strcmp(shared[i].accnum, "00000")==0){ //since it returns zero if they are equivalent
            printf("Enter New Account Number: \n");
            scanf("%s",newaccnum);
            strcpy(shared[i].accnum, newaccnum);
            printf("Enter new pin\n");
            scanf("%s", newpin);
            strcpy(shared[i].pin, newpin);
            printf("enter the amount of the deposite/withdraw\n");
            scanf("%lf", &amount);
            shared[i].fundsav = shared[i].fundsav - amount;
           	// shared+=sizeof(DB);
        }else{
            printf("NO MORE SPACE FOR A NEW COSTUMER");
        }
    }*/
   // }
}

void getInput(char *question, char *returnString, int inputLength){
		printf("%s *you may only use %d charavters:\n", question, inputLength);
		//scanf("%s", returnString);
		printf("returning\n");
};

char* getFeild(char* line, int n, char c){
		char* tok;
		for (tok = strtok(line, &c); tok && *tok; tok = strtok(NULL, ("%c\n",&c))){
				if (!--n)
						return tok;
		}
		return NULL;
}

//void rewrite()
