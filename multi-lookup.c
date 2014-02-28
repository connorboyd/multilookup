/*
 * File: multi-lookup.c
 * Author: Connor Boyd
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2014/02/25
 * Description:
 * 	This file contains threaded lookup program
 *  
 */

/*

Some basic notes:
	Allocate one request thread for each input file
	Request threads should add hostnames to the request queue
		Maximum queue size is 50, so threads should sleep for 0-100 ms if the queue is full
		Queue needs a mutex
	Resolver thread should take a hostname from the queue, resolve the IP address, and write the results to results.txt
		At least 2 resolver threads (Match to # of cores? More?)

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>	//for usleep


#include "util.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"




void* handleFile(char* inputFileName)
{
	// printf("%s\n", inputFileName);
	char hostname[SBUFSIZE];	//Holds the individual hostname
	char firstipstr[INET6_ADDRSTRLEN]; //Holds the resolved IP address

	char someString[2048];
	char tempString[2048];

	FILE* inputFile = fopen(inputFileName, "r");
	// printf("%s\n", inputFile);

	/* Read File and Process*/
	// printf("Not in loop yet\n");
	while(fscanf(inputFile, INPUTFS, hostname) > 0)
	{
		// sprintf(tempString, "%s\n", hostname);
		// strcat(someString, tempString);
	    /* Lookup hostname and get IP string */
	    int dnsRC = dnslookup(hostname, firstipstr, sizeof(firstipstr));
	    // printf("dnsRC = %i\n", dnsRC);
	    if(dnsRC == UTIL_FAILURE)
	    {
			fprintf(stderr, "dnslookup error: %s\n", hostname);
			strncpy(firstipstr, "", sizeof(firstipstr));
	    }
	
	    /* Write to Output File */
	    // fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
	    // printf("SOMETHING ABOUT A LOOP\n");
	    printf("%s,%s\n", hostname, firstipstr);
	}
	// printf("%s\n", someString);
	/* Close Input File */
	fclose(inputFile);
}

int main(int argc, char* argv[])
{

    /* Local Vars */
    // FILE* inputfp = NULL;		//Holds the input file
    FILE* outputfp = NULL;		//Holds the output file
    

    // FILE* inputFiles[argc-1];

    // char hostname[SBUFSIZE];	//Holds the individual hostname
    // char errorstr[SBUFSIZE];	//Holds some error text
    // char firstipsmtr[INET6_ADDRSTRLEN]; //Holds the resolved IP address
    int i;	//Index variable for iterating through argv
    pthread_t requestThreads[argc-1];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);


    /* Check Arguments */
    if(argc < MINARGS)
    {
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp)
    {
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }

    /* Loop Through Input Files */
    for(i=1; i<(argc-1); i++)	//Allocate 1 thread for every iteration here
    {
    	// printf("In main loop\n");
		/* Open Input File */
		// inputFiles[i-1] = fopen(argv[i], "r");
		// if(!inputFiles[i-1])
		// {
		//     sprintf(errorstr, "Error Opening Input File: %s", argv[i]);
		//     perror(errorstr);
		//     // break;
		// }	

		int rc = pthread_create(&requestThreads[i-1], &attr, handleFile, argv[i]);
		// printf("pthread_create return code = %i\n", rc );
		
    }
    usleep(5000000);	//Sleep for 5 seconds. I dont know why, but this fixed everything. No idea why join wasn't working...

    for(int j = 1; j < argc-1; ++j)
    {
    	int rc = pthread_join( requestThreads[i-1],  NULL);		//WHY DOES THIS RETURN 3???
    	printf("Thread %i finished. rc = %i\n", j-1, rc);
    }

    /* Close Output File */
    fclose(outputfp);

    return EXIT_SUCCESS;
}


