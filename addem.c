/**
**  Written and Directed
**    by Matt Tolbert
**     CS3013 2018
**/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXTHREAD 10
#define RANGE 1      // one of two type values for part 1 of project
#define ALLDONE 2    // one of two type values for part 1 of project

struct msg {
  int iSender; /* send of the message (0 .. number-of-threads) */
  int type;    /* its type */
  int value1;  /* first value */
  int value2;  /* second value */
};

// Global Variable Mailbox array
struct msg mailboxArray[MAXTHREAD + 1];
pthread_t threadArray[MAXTHREAD + 1];
sem_t sendSem[MAXTHREAD+1];
sem_t receiveSem[MAXTHREAD+1];

void SendMsg(int iTo, struct msg *pMsg) {
  sem_wait(&receiveSem[iTo]);
  // set values
  mailboxArray[iTo].iSender = pMsg->iSender;
  mailboxArray[iTo].type = pMsg->type;
  mailboxArray[iTo].value1 = pMsg->value1;
  mailboxArray[iTo].value2 = pMsg->value2;
  // update sems
  sem_post(&sendSem[iTo]);
}

void RecvMsg(int iFrom, struct msg *pMsg) {
  sem_wait(&sendSem[iFrom]);
  pMsg->iSender = mailboxArray[iFrom].iSender;
  pMsg->type = mailboxArray[iFrom].type;
  pMsg->value1 = mailboxArray[iFrom].value1;
  pMsg->value2 = mailboxArray[iFrom].value2;
  sem_post(&receiveSem[iFrom]);
}

// handle workers (threads > 0)
void *worker_function(void *argument) {
  //printf("Intializing a new child thread\n");
  int *threadID = (int*)argument;
  // wait for message from parent
  struct msg localMail;
  RecvMsg(*threadID, &localMail); // receive mail in own box
  /*while(localMail.type == 0) {
    RecvMsg(*threadID, &localMail);
  }*/
  //printf("thread %d receieved msg\n", *threadID);
  // check valid message received
  int sum = 0;
  int currentVal = localMail.value1;
  int greatestVal = localMail.value2;

  for(currentVal; currentVal <= greatestVal; currentVal++) {
    sum += currentVal;
  }
  //printf("Thread %d has sum %d\n", *threadID, sum);

  // send a message to parent
  struct msg *msgToSend = malloc(4 * sizeof(int));
  msgToSend->iSender = *threadID;
  msgToSend->type = ALLDONE;
  msgToSend->value1 = sum;
  msgToSend->value2 = 0;
  //printf("im about to send my message back\n");

  int receiveVal, sendVal;
  sem_getvalue(&receiveSem[0], &receiveVal);
  sem_getvalue(&sendSem[0], &sendVal);
  //printf("before send The value of receiveSem[%d] is %d\n", 0, receiveVal);
  //printf("before send The value of sendSem[%d] is %d\n", 0, sendVal);

  SendMsg(0, msgToSend); // send the message to parent
  //printf("I sent my message to main \n");
  pthread_exit(msgToSend);
}

int main(int argc, char* argv[]) {
  int numThreads;
  int numToAdd;

  /*************** CHECK FOR VALID INPUT ***************/

  if(argc == 1 || argc > 3) {
    printf("Invalid number of arguments... \n");
    printf("The proper syntax is ./addem #threads #toAddTo \n");
    return 1; // exit
  }

  numThreads = atoi(argv[1]); // find out how many threads user wants
  // check to make sure num specified is valid
  if(numThreads > 10 || numThreads < 1) {
    printf("Please specify number of threads between 1 and 10\n");
    return 1; // exit
  }
  numToAdd = atoi(argv[2]);   // find out how much to count to

  /*************** INITIALIZE SEMAPHORES ***************/

  for(int i = 0; i <= numThreads; i++) {
    if(sem_init(&receiveSem[i], 0, 1) < 0) {
      perror("sem_init");
      exit(1);
    }
    if(sem_init(&sendSem[i], 0, 0) < 0) {
      perror("sem_init");
      exit(1);
    }
    int receiveVal, sendVal;
    sem_getvalue(&receiveSem[i], &receiveVal);
    sem_getvalue(&sendSem[i], &sendVal);
  //  printf("The value of receiveSem[%d] is %d\n", i, receiveVal);
  //  printf("The value of sendSem[%d] is %d\n", i, sendVal);
  }

  /*************** ACUTALLY IMPORTANT STUFF ***************/
  /********************************************************/

  // distribute the numToAdd Evenly if possible, add remainder to
  int numberIntervals = (numToAdd / numThreads);
  int previousNumberInterval = 1; // keep track of interval
  int remain = (numToAdd % numThreads);

  /*************** CREATE WORKER THREADS ***************/
  for(int i = 0; i < numThreads; i++) {
    int *threadID = malloc(sizeof(int));
    *threadID = i+1;
    if(pthread_create(&threadArray[i], NULL, worker_function, threadID) != 0){
        printf("There was an issue creating the child thread\n");
        break;
    }
  }

  /*************** SEND MESSAGES TO WORKER THREADS ***************/

  for(int i =0; i < numThreads; i++) {
    // send a message to child
    int localI = i + 1;
    struct msg *msgToSend = malloc(4 * sizeof(int));
    msgToSend->iSender = 0;
    msgToSend->type = RANGE;
    msgToSend->value1 = previousNumberInterval;
    msgToSend->value2 = numberIntervals * localI;
    if(i+1 == numThreads) {
      msgToSend->value2 += remain; // add remainder to last if there is one
    }
    //printf("i got to 85\n");
    //printf("msg to send val1 = %d to Thread %d\n", msgToSend->value1, localI);
    //printf("msg to send val2 = %d to Thread %d\n", msgToSend->value2, localI);
    SendMsg((localI), msgToSend); // send the message
    //printf("i sent a message to thread %d\n", localI);

    // update number interval for accuracy
    previousNumberInterval += numberIntervals;
  }
  //printf("not in loop yet\n");

  /*************** RECEIVE MESSAGES FROM WORKERS ***************/
  int grandTotal = 0; // total of all sums
  // loop through and receive messages
  for(int i = 1; i <= numThreads; i++) {
    //printf("top of loop\n");
    struct msg localMail; // mail to receive from box
    // try to receive messages from all children
    RecvMsg(0, &localMail);
    //check if the child is all done
    if(localMail.type == ALLDONE) {
        grandTotal += localMail.value1;
        //printf("Adding %d to total %d\n", localMail.value1, grandTotal);
    }
    //printf("end of loop\n");
  }
  printf("The total for 1 to %d with %d threads was %d \n", numToAdd, numThreads, grandTotal);

  for(int i =0; i < numThreads; i++) {
    (void)sem_destroy(&sendSem[i]);
    (void)sem_destroy(&receiveSem[i]);
  }
  return 0;
}
