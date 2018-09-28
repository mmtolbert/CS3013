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
#define STOP 5 // stop all threads

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
int gridToPlay;
int longestCol, longestRow; // keep track of how many columns

// added support for upper and lower bounds for true multithreading
int terminationCondition(int** currentArray, int** futureArray, int** oldArray, int lowerBounds, int upperBounds, int columns){

  // determine how many rows there are
  int rows = upperBounds - lowerBounds + 1;
	int size = rows * columns; // Will be used to compare total area to the similarities between generations
	int stillLifeCounter = 0;
	int oscillationCounter = 0;
	// Loop through the entire size of the grid and compare current to future and previous
	for(int i = lowerBounds; i <= upperBounds; i++){
		// loop invariant here is that i is less than the number of rows
		for(int j = 0; j < columns; j++){
			// loop invariant here is that j is less than the number of columns
			if(currentArray[i][j] == futureArray[i][j]){
				stillLifeCounter++;
			}
			if(oldArray[i][j] == futureArray[i][j]) {
				oscillationCounter++;
			}
		}
	}
	if(size == stillLifeCounter) { // check to see if all values from current board and future board are the same
		return 1; // termination value for still life
	}
	if(size == oscillationCounter) { // check to see if all values from the prior board and future board are the same
		return 2; // termination value for oscillation
	} else {
		return 0; // if no termination conditions, continue
	}

}


// need to update this to support taking two row values for true multithreading
int** playOneGen(int** currentArray, int** newArray, int lowerBounds, int upperBounds, int columns) {

	// Check the currentArray for growth or death conditions and apply to newArray
	for(int i = lowerBounds; i <= upperBounds; i++) {
		// loop invariant here is that i is less than the number of rows
		for(int j = 0; j < columns; j++) {
			// loop invariant here is that j is less than the number of columns
			// Inside the above loop we are at an individual point in currentArray
			int currentCellAlive = 0; // by default our current cell is dead or 0, 1 if alive
			if(currentArray[i][j] == 1) {
				currentCellAlive = 1;
			}
			int numNeighbors = 0; // Keep track of how many neighbors
			// The below loop returns a number that is equivalent to the number of cells surrounding the current cell
			// This number is then used to calculate life, death or survival for the next round
			for(int checkR = -1; checkR <= 1; checkR++) { // Check one row above, the current row, and one row below
				// the loop invariant here makes sure that we check the row above, the current row, and the row below
				for(int checkC = -1; checkC <= 1; checkC++){ // Check the values of the previous column, the current column, and the next column
					// the loop invariant here makes sure we check each column before, equal to, and after the current column of the point
					// first make sure check point is in bounds
					if((i + checkR < 0) || (i + checkR == longestRow) || (j + checkC < 0) || (j + checkC == columns) || (checkR == 0 && checkC == 0)) {
						// Loop does nothing in these conditions since they are out of bounds and thus ignored
					}
					else { // if else we are in bounds and can safely check
						if(currentArray[i + checkR][j + checkC] == 1){ // if there is a 1 in one of the neighboring spots we have a live neighbor
							numNeighbors++; // add a neighbor
						}
					}
				}
			}
			// now that we know how many alive neighbors, determine what to do
			// is the cell alive?
			if(currentCellAlive == 1){ // check to see if current cell is alive so we check for survival or death
				if(numNeighbors == 2 || numNeighbors == 3) {
					// In this case the current cell survives
					newArray[i][j] = 1; // Case of cell surviving to next generation
				} else {
					newArray[i][j] = 0; // Case of cell dying from loneliness or over-population
				}
			} else { // Cases for if cell is dead, we check if it stays as is or if it births
				if(numNeighbors == 3) {
					newArray[i][j] = 1; // Circumstance of growth when there are exactly 3 living neighbors, but cell itself is dead
				}
				else {
					newArray[i][j] = 0; // This is the default case of no growth when cell is already dead
				}
			}
		}
	}
	return newArray; // return the next generation
}

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
  }

  /************** VERY IMPORTANT GAME LOGIC ***************/

  int done = 0; // keep track of done to exit loop
  // now enter loop and wait for mail
  while(!done) {
    // wait to receive instruction to play a gen
    RecvMsg(*threadID, &localMail); // receive mail in own box
    int sentMail =0; // keep track of sending all done
    if(localMail.type == STOP) {
      struct msg *msgToSend = malloc(4 * sizeof(int));
      pthread_exit(msgToSend);
    }
    else if(localMail.type == GO) {
      // we need to cycle through our generations in an order that makes sense
      // alternate gridToPlay in order to best keep track of changing conditions that could cause termination
      if(gridToPlay == 3) {
        gridA = playOneGen(gridC, gridA, lowerBounds, upperBounds, longestCol); // Update grid A using playOneGen
        // Check for termination Conditions
        int terminates = terminationCondition(gridC, gridA, gridB, lowerBounds, upperBounds, longestCol);
        if(terminates == 1){ // 1 means steady state found
          struct msg *msgToSend = malloc(4 * sizeof(int));
          msgToSend->iSender = *threadID;
          msgToSend->type = ALLDONE;
          msgToSend->value1 = 1;
          SendMsg(0, msgToSend);
          sentMail = 1;
        }
        else if(terminates == 2) { // two means oscillation found
          struct msg *msgToSend = malloc(4 * sizeof(int));
          msgToSend->iSender = *threadID;
          msgToSend->type = ALLDONE;
          msgToSend->value1 = 2;
          SendMsg(0, msgToSend);
          sentMail = 1;
        }
      }
      else if (gridToPlay == 2) {
        gridC = playOneGen(gridB, gridC, lowerBounds, upperBounds, longestCol);
        // Check for termination Conditions
        int terminates = terminationCondition(gridB, gridC, gridA, lowerBounds, upperBounds, longestCol);
        if(terminates == 1){ // 1 means steady state found
          struct msg *msgToSend = malloc(4 * sizeof(int));
          msgToSend->iSender = *threadID;
          msgToSend->type = ALLDONE;
          msgToSend->value1 = 1;
          SendMsg(0, msgToSend);
          sentMail = 1;
        }
        else if(terminates == 2) { // two means oscillation found
          struct msg *msgToSend = malloc(4 * sizeof(int));
          msgToSend->iSender = *threadID;
          msgToSend->type = ALLDONE;
          msgToSend->value1 = 2;
          SendMsg(0, msgToSend);
          sentMail = 1;
        }
      }
      else if (gridToPlay == 1) {
        gridB = playOneGen(gridA, gridB, lowerBounds, upperBounds, longestCol);
        // Check for termination Conditions
        int terminates = terminationCondition(gridA, gridB, gridC, lowerBounds, upperBounds, longestCol);
        if(terminates == 1){ // 1 means steady state found
          struct msg *msgToSend = malloc(4 * sizeof(int));
          msgToSend->iSender = *threadID;
          msgToSend->type = ALLDONE;
          msgToSend->value1 = 1;
          SendMsg(0, msgToSend);
          sentMail = 1;
        }
        else if(terminates == 2) { // two means oscillation found
          struct msg *msgToSend = malloc(4 * sizeof(int));
          msgToSend->iSender = *threadID;
          msgToSend->type = ALLDONE;
          msgToSend->value1 = 2;
          SendMsg(0, msgToSend);
          sentMail = 1;
        }
      }
    }
    if(sentMail == 0) {
      // end of play one generation
      struct msg *msgToSend = malloc(4 * sizeof(int));
      msgToSend->iSender = *threadID;
      msgToSend->type = GENDONE;
      SendMsg(0, msgToSend); // send the message to parent
    }
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
  int **gridIntake; // fix weird problem that is caused by not dynamically allocating

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
	gridIntake = makeGrid(MAXGRID, MAXGRID); // default allocate, modify later

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
	longestRow = 0;// Keep track of longest row of input file
	longestCol = 0;// Keep track of longest column of input file
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
					gridIntake[i][j] = 0;
				}
        else { // its a 1
					gridIntake[i][j] = 1;
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
					gridIntake[i][jFinish] = 0; // fill in the rest of the row with blanks since no data from the file was specified at this particular point through the end of the row
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
  int previousMinRow = 0; // account for value1

  /********* SET INITIAL ARRAY TO PROPER SIZE *********/

  gridA = makeGrid(longestRow, longestCol);
  gridB = makeGrid(longestRow, longestCol); // allocate gridB
  gridC = makeGrid(longestRow, longestCol);

  // copy gridA into gridB because gridB is the exact size of the input
  for(int i = 0; i < longestRow; i++) {
    for(int j = 0; j < longestCol; j++) {
      gridA[i][j] = gridIntake[i][j]; // set vals = to one another
    }
  }

  // self explanatory
  if(doPrint==1) {
    // print grid
    printf("\nGeneration 0\n");
    printGrid(gridA, longestRow, longestCol);
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

  // calculate what to send each thread
  int minNums[MAXTHREAD+1];
  int maxNums[MAXTHREAD+1];
  for(int i = 1; i <= numThreads; i++) {
     minNums[i] = previousMinRow;
     maxNums[i] = previousMinRow+(rowsPerThread-1);
     if(extraRows != 0) {
       maxNums[i]++; // add remainder rows
       previousMinRow++;
       extraRows--;
     }
     previousMinRow = maxNums[i] + 1;
  }

  /*************** ASSIGN ROWS TO EACH THREAD ***************/
  int addedPrevious = 0; // added one to previous marker
  for(int i =1; i <= numThreads; i++) {
    // send a message to child
    struct msg *msgToSend = malloc(4 * sizeof(int));
    msgToSend->iSender = 0;
    msgToSend->type = RANGE;
    msgToSend->value1 = minNums[i];
    msgToSend->value2 = maxNums[i];
    SendMsg(i, msgToSend); // send the message
    // update number interval for accuracy
  }

  /*************** START GENERATIONS OF SENDING/RECEIVING ***************/

  // probs need to make this a global var
  gridToPlay = 1; // 1 for A, 2 for B, 3 for C

	for(int i = 0; i < gens; i++) { // make sure we play the correct number of addresses
    int localI = i+1; // allows us to end game but keep track of generations
    if(doPause){ // Pause each time and wait for user to hit enter if doPause is active
			printf("Please hit ENTER to continue: \n");
			getchar(); // wait for user to hit enter
		}

    /*************** SEND MESSAGES TO WORKER THREADS ***************/
    for(int i =1; i <= numThreads; i++) {
      // send a message to child
      struct msg *msgToSend = malloc(4 * sizeof(int));
      msgToSend->iSender = 0;
      msgToSend->type = GO;
      SendMsg(i, msgToSend); // send the message
    }

    /*************** RECEIVE MESSAGES FROM WORKERS ***************/

    int grandTotal = 0; // total of all sums
    // loop through and receive messages
    int numAllDone = 0; // reset every generation, need all threads to say alldone
    int terminates; // why did we terminate?
    for(int i = 1; i <= numThreads; i++) {
      //printf("top of loop\n");
      struct msg localMail; // mail to receive from box
      // try to receive messages from all children
      RecvMsg(0, &localMail);

      // Check if children return all done specifying an end condition was found
      if(localMail.type == ALLDONE) {
        numAllDone++; // add it up
        int terminates = localMail.value1; // set value of terminates
        // check if all threads say all done as the whole board needs to be oscillating or stagnant
        // the reason i chose longestRow here is because if a user specifies
        // more threads than rows of the input than the program is expecting
        // more returns than it will get, so longest row is acutal = the Number
        // of threads actually computing
        // also check numthreads because if threads < rows
        if(numAllDone == longestRow || numAllDone == numThreads) {
          // send the stop message
          /******************** Note FOR GRADERS ***********************/
          // i did not need to send a go message separately becasue if not all
          // send alldone then the program proceeds
          for(int i =1; i <= numThreads; i++) {
            // send a message to child
            struct msg *msgToSend = malloc(4 * sizeof(int));
            msgToSend->iSender = 0;
            msgToSend->type = STOP;
            SendMsg(i, msgToSend); // send the message
          }

          i == gens; // set so loop exits
          // game is done for some reason
          if(terminates == 1) {
            //FOLLOW ALL PARTS OF THE END OF FUNCTION BUT INTERNALLY BECAUSE IM TOO LAZY TO MAKE ANOTHER FUNCTION
            // steady state found condition
            if(gridToPlay == 3) {
              gridToPlay = 1; // Reset the counter for the next loop
            } else {
              gridToPlay++;
            }
            printf("\nSteady state or cell death condition found, the final generation is printed below\n");
            // print grid
            if(gridToPlay == 3) { // Checks to determine the proper generation to display
              printf("The final grid ended prematurely after %d generations\n", localI);
              printGrid(gridC, longestRow, longestCol);
            }
            else if (gridToPlay == 2) { // Checks to determine the proper generation to display
              printf("The final grid ended prematurely after %d generations\n", localI);
              printGrid(gridB, longestRow, longestCol);
            }
            else if (gridToPlay == 1) { // Checks to determine the proper generation to display
              printf("The final grid ended prematurely after %d generations\n", localI);
              printGrid(gridA, longestRow, longestCol);
            }
            for(int i =0; i < numThreads; i++) {
              (void)sem_destroy(&sendSem[i]);
              (void)sem_destroy(&receiveSem[i]);
            }
            return 0;
          }
          else if(terminates == 2){
            //FOLLOW ALL PARTS OF THE END OF FUNCTION BUT INTERNALLY BECAUSE IM TOO LAZY TO MAKE ANOTHER FUNCTION
            if(gridToPlay == 3) {
              gridToPlay = 1; // Reset the counter for the next loop
            } else {
              gridToPlay++;
            }
            // oscillation found
            printf("\nOscillation condition found, the final generation is printed below\n");
            // print grid
            if(gridToPlay == 3) { // Checks to determine the proper generation to display
              printf("The final grid ended prematurely after %d generations\n", localI);
              printGrid(gridC, longestRow, longestCol);
            }
            else if (gridToPlay == 2) { // Checks to determine the proper generation to display
              printf("The final grid ended prematurely after %d generations\n", localI);
              printGrid(gridB, longestRow, longestCol);
            }
            else if (gridToPlay == 1) { // Checks to determine the proper generation to display
              printf("The final grid ended prematurely after %d generations\n", localI);
              printGrid(gridA, longestRow, longestCol);
            }
            for(int i =0; i < numThreads; i++) {
              (void)sem_destroy(&sendSem[i]);
              (void)sem_destroy(&receiveSem[i]);
            }
            return 0;
          }
        }
      }
    }
    // no need for an else case as GENDONE is the only other case and is expected

    /************* UPDATE GRID TO PLAY AND PRINT*************/
    if(gridToPlay == 3) {
      gridToPlay = 1; // Reset the counter for the next loop
    } else {
      gridToPlay++;
    }
    if(doPrint) {
      // print grid
      if(gridToPlay == 3) { // Checks to determine the proper generation to display
  			printf("\nGeneration %d\n", localI);
  			printGrid(gridC, longestRow, longestCol);
  		}
  		else if (gridToPlay == 2) { // Checks to determine the proper generation to display
  			printf("\nGeneration %d\n", localI);
  			printGrid(gridB, longestRow, longestCol);
  		}
  		else if (gridToPlay == 1) { // Checks to determine the proper generation to display
  			printf("\nGeneration %d\n", localI);
  			printGrid(gridA, longestRow, longestCol);
  		}
    }
  }
  //end of for Loop

  // print the final grid if we havent already
  if(doPrint == 0){
    // print grid
    if(gridToPlay == 3) { // Checks to determine the proper generation to display
      printf("\nThe final grid ended naturally after %d generations is\n", gens);
      printGrid(gridC, longestRow, longestCol);
    }
    else if (gridToPlay == 2) { // Checks to determine the proper generation to display
      printf("\nThe final grid ended naturally after %d generations is\n", gens);
      printGrid(gridB, longestRow, longestCol);
    }
    else if (gridToPlay == 1) { // Checks to determine the proper generation to display
      printf("\nThe final grid ended naturally after %d generations is\n", gens);
      printGrid(gridA, longestRow, longestCol);
    }
	} else {
		printf("\nThe final grid ended naturally after %d generations is\n", gens);
	}
  //printf("not in loop yet\n");

  for(int i =0; i < numThreads; i++) {
    (void)sem_destroy(&sendSem[i]);
    (void)sem_destroy(&receiveSem[i]);
  }
  return 0;
}
