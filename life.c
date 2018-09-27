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
#define GO 3
#define GENDONE 4 // generation done
#define MAXGRID 40 // max grid size

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
int **gridA,**gridB,**gridC; // declare generations so they can be seen

// make grid function
int** makeGrid(int rows, int columns) {
	int **grid; // Array of pointers to rows
	unsigned int i; // Loop counter

	grid = (int **) malloc(rows * sizeof(int *));
	if (!grid) { // Unable to allocate the array
		return (int **) NULL;
	}
	for (i = 0; i < rows; i++) {
		grid[i] = malloc(columns * sizeof (int));
		if (!grid[i]) {
			return (int **) NULL; // Unable to allocate
		}
	}
	return grid;
}

// set grid to all 0's
int** resetGrid(int** array, int rows, int columns) {
	for(int i = 0; i < rows; i++) {
		for(int j = 0; j < columns; j++) {
			array[i][j] = 0; // replace each index with a blank space
		}
	}
	return array;
}

// print out the grid to console
void printGrid(int** array, int rows, int columns) {

	printf("\n"); // Add a line of spacing for clarity

	// loop through each row and column and print the array out in a readable context for the user
	for(int i = 0; i < rows; i++) {
		for(int j = 0; j < columns; j++) {
			printf("%d ", array[i][j]);
			if(j == columns - 1) { // check if columns are about to update printing and add a line
				printf("\n"); // put a new line in for readability
			}
		}
	}
}

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

  int lowerBounds, upperBounds;
  // first case where we are getting our range
  if(localMail.type == RANGE) {
    lowerBounds = localMail.value1;
    upperBounds = localMail.value2;
    //printf("intiating thread %d\n", *threadID);
  }

  /************** VERY IMPORTANT GAME LOGIC ***************/

  int done = 0; // keep track of done to exit loop
  // now enter loop and wait for mail
  while(!done) {
    RecvMsg(*threadID, &localMail); // receive mail in own box
    if(localMail.type == GO) {
      // thread 0 wants us to play our rows
      //printf("parent wants me to play @%d\n", *threadID);
      // send a message to parent
      struct msg *msgToSend = malloc(4 * sizeof(int));
      msgToSend->iSender = *threadID;
      msgToSend->type = GENDONE;
      SendMsg(0, msgToSend); // send the message to parent
    }
    //check for end conditions...
    done = 1;
  }
  struct msg *msgToSend = malloc(4 * sizeof(int));
  pthread_exit(msgToSend);
}

int main(int argc, char* argv[]) {

  char *inputFileName; // Name of file containing initial grid
	FILE *input; // Stream descriptor for file containing initial grid
	int gens; // Number of generations to produce
	int doPrint; // 1 if user wants to print each generation, 0 if not
	int doPause; // 1 if user wants to pause after each generation, 0 if not
  int numThreads;

  /*************** CHECK FOR VALID INPUT ***************/

  if ((argc < 4) || (argc > 6)) {
		// If not, tell the user what to enter.
		printf("Usage:\n");
		printf("  ./life threads filename generations print input\n");
		return EXIT_FAILURE;
	}

  numThreads = atoi(argv[1]); // find out how many threads user wants
  // check to make sure num specified is valid
  if(numThreads > 10 || numThreads < 1) {
    printf("Please specify number of threads between 1 and 10\n");
    return 1; // exit
  }
  inputFileName = argv[2];
  gens = atoi(argv[3]);

  // open file
	input = fopen(inputFileName, "r");
	if (!input) {
		printf("Unable to open input file: %s\n", inputFileName);
		return EXIT_FAILURE;
	}

	unsigned int i; // Loop counter

	// First allocate the array of pointers to rows
	gridA = makeGrid(MAXGRID, MAXGRID); // default allocate, modify later

  // make sure we have the proper number of argumemts before we try to access them in the array
	if(argc == 6) { // if there are seven we have a specification for both doPrint and doPause
		if(*argv[4] == 'y') { // check if user wants to print each generation
			doPrint = 1;
		} else {
			doPrint = 0; // if not do not print
		}
		if(*argv[5] == 'y' && doPrint == 1) { // check if user wants to pause after each generation and make sure they also have printing enabled
			doPause = 1;
		}
		else if(*argv[5] == 'y' && doPrint == 0) { // check if the want to pause, but not print which is not allowed
			doPause = 0;
			printf("It is not allowed to pause without also enabling printing\n");
			return EXIT_FAILURE;
		} else { // default case for doPause since it was specified but not = 'y' it must be off
			doPause = 0;
		}
	}
	else if(argc == 5) { // user only specified a value for do print since we have 6 arguments
		if(*argv[4] == 'y') { // check if user wants to print each generation
			doPrint = 1;
		} else {
			doPrint = 0;
		}
		doPause = 0; // doPause was not specified so it defaults to off
	}
	else {
		doPrint = 0; // Neither value were specified by user so both default to off
		doPause = 0;
	}

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
  }

  /*************** ACUTALLY IMPORTANT STUFF ***************/
  /********************************************************/

  // put input onto gridA so that it can be measured and then put into cetner of gridb
	int longestRow = 0;// Keep track of longest row of input file
	int longestCol = 0;// Keep track of longest column of input file
	// Loop through entire grid and fill in input file without adjusting to center, after
	for(int i = 0; i < MAXGRID; i++) {
		// loop invariant here is that i is less than the number of rows
		for(int j = 0; j < MAXGRID; j++) {
			// loop invariant here is that j is less than the number of columns
			int currentCol = 0; // keep track of length of columns in current row
			int c; // current character for fgetc
			c = fgetc(input); // read character by character from file
      // check if c is a space because if it we need to get the next character
      if(c == 32) {
        c = fgetc(input);
      }
      // in ASCII 48 = 0, 49 = 1
			if(c == 48 || c == 49) { // the only other character it will find is '\n' which is a new line
				if(c == 48){
					gridA[i][j] = 0;
				}
        else { // its a 1
					gridA[i][j] = 1;
				}
				if(j == 0) {
					longestRow++;
				}
				if(j + 1 == MAXGRID) { // Make sure that we have a big enough grid
					printf("There are more columns in the file than in MAXGRID size! \n");
					return EXIT_FAILURE;
				}
			}
      else {
				for(int jFinish = j; jFinish < MAXGRID; jFinish++) {
					gridA[i][jFinish] = 0; // fill in the rest of the row with blanks since no data from the file was specified at this particular point through the end of the row
				}
				// update the longestCol counter if necessary
				currentCol = j;
				if(currentCol > longestCol){
					longestCol = currentCol;
				}
        break;
			}

		}
	}

  /********* DETERMINE HOW MANY ROWS PER THREAD *********/

  int rowsPerThread = longestRow / numThreads;
  int extraRows = (longestRow % numThreads); // add remainder to last thread
  int previousMinRow = 1; // account for value1

  gridB = makeGrid(longestRow, longestCol); // allocate gridB

  // copy gridA into gridB because gridB is the exact size of the input
  for(int i = 0; i < longestRow; i++) {
    for(int j = 0; j < longestCol; j++) {
      gridB[i][j] = gridA[i][j]; // set vals = to one another
    }
  }

  /********* SET INITIAL ARRAY TO PROPER SIZE *********/

  free(gridA); // free and reallocate to proper size
  gridA = makeGrid(longestRow, longestCol);
  // now set it to a blank grid
  gridA = resetGrid(gridA, longestRow, longestCol);

  // self explanatory
  if(doPrint==1) {
    // print grid
    printf("\nGeneration 0\n");
    printf("longestRow = %d", longestRow);
    printf("longestCol = %d", longestCol);
    printGrid(gridB, longestRow, longestCol);
  }

  /*************** CREATE WORKER THREADS ***************/
  for(int i = 1; i <= numThreads; i++) {
    int *threadID = malloc(sizeof(int));
    *threadID = i;
    if(pthread_create(&threadArray[i], NULL, worker_function, threadID) != 0){
        printf("There was an issue creating the child thread\n");
        break;
    }
  }

  /*************** ASSIGN ROWS TO EACH THREAD ***************/

  for(int i =1; i <= numThreads; i++) {
    // send a message to child
    struct msg *msgToSend = malloc(4 * sizeof(int));
    msgToSend->iSender = 0;
    msgToSend->type = RANGE;
    msgToSend->value1 = previousMinRow;
    msgToSend->value2 = rowsPerThread*i;
    if(i+1 >= numThreads) {
      msgToSend->value2 += extraRows; // add remainder to last if there is one
    }
    SendMsg(i, msgToSend); // send the message
    // update number interval for accuracy
    previousMinRow += rowsPerThread;
  }

  /*************** START GENERATIONS OF SENDING/RECEIVING ***************/

  // probs need to make this a global var
  int gridToPlay = 2; // 1 for A, 2 for B, 3 for C

	for(int i = 0; i < gens; i++) { // make sure we play the correct number of addresses

    /*************** SEND MESSAGES TO WORKER THREADS ***************/
    for(int i =1; i <= numThreads; i++) {
      // send a message to child
      struct msg *msgToSend = malloc(4 * sizeof(int));
      msgToSend->iSender = 0;
      msgToSend->type = GO;
      msgToSend->value1 = 0;
      msgToSend->value2 = 0;
      if(i+1 >= numThreads) {
        msgToSend->value2 += 0; // add remainder to last if there is one
      }
      SendMsg(i, msgToSend); // send the message
      // update number interval for accuracy
      previousMinRow += rowsPerThread;
    }
    int receivedMsgs = 0;
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
        // game is done for some reason
        // do something to exit
      }
      else if(localMail.type == GENDONE) {
        //generation is done for thread, do nothing allow loop to proceed
        receivedMsgs++;
      }
    }
    if(receivedMsgs==numThreads){
      return 0; // proves execution works for message passing now get into the good stuff
    }
    if(doPrint) {
      // print grid
      printf("\nGeneration %d\n", i);
      printGrid(gridB, longestRow, longestCol);
    }
  }
  //printf("not in loop yet\n");

  for(int i =0; i < numThreads; i++) {
    (void)sem_destroy(&sendSem[i]);
    (void)sem_destroy(&receiveSem[i]);
  }
  return 0;
}
