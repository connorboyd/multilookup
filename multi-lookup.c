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
#include "queue.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define RESOLVER_THREAD_COUNT 10

// Global variables
queue hostnameQueue;
pthread_mutex_t queueLock;
pthread_mutex_t fileLock;
FILE* outputfp = NULL;		//Holds the output file
int requestThreadsFinished = 0;


void* requestThreadFunction(void* inputFileName)
{
	// printf("%s\n", inputFileName);
	char hostname[SBUFSIZE];	//Holds the individual hostname
	// char firstipstr[INET6_ADDRSTRLEN]; //Holds the resolved IP address
	char* payload;

	FILE* inputFile = fopen(inputFileName, "r");

	// printf("Opening %s\n", inputFileName);

	/* Read File and Process*/
	while(fscanf(inputFile, INPUTFS, hostname) > 0)
	{
	    /* Lookup hostname and get IP string */
		int queueSuccess = 0;
		while(queueSuccess == 0) 
		{
			pthread_mutex_lock(&queueLock);
			if(queue_is_full(&hostnameQueue))
			{
				pthread_mutex_unlock(&queueLock);
				usleep( rand() % 100 );	//goes somewhere. Maybe not here.
				// printf("Queue full! sleep!\n");
			}
			else
			{
				// printf("Hostname address = %p\n", hostname);
				payload = malloc(SBUFSIZE);
				payload=strncpy(payload, hostname, SBUFSIZE); 
				queue_push(&hostnameQueue, payload);
				pthread_mutex_unlock(&queueLock);
				// printf("Success!\n");
				queueSuccess = 1;
			}
		}
	}
	/* Close Input File */
	fclose(inputFile);
	return NULL;
}

void* resolverThreadFunction()
{
	char* hostname;	//Should contain the hostname
	char firstipstr[INET6_ADDRSTRLEN]; //Holds the resolved IP address

	while(!queue_is_empty(&hostnameQueue) || !requestThreadsFinished)
	{
		pthread_mutex_lock(&queueLock);
		if(!queue_is_empty(&hostnameQueue))
		{
			hostname = queue_pop(&hostnameQueue);
			// printf("Removing %s from queue\n", hostname );


			if(hostname != NULL)
			{
				pthread_mutex_unlock(&queueLock);

				int dnsRC = dnslookup(hostname, firstipstr, sizeof(firstipstr));

				if(dnsRC == UTIL_FAILURE)
			    {
					fprintf(stderr, "dnslookup error: %s\n", hostname);
					strncpy(firstipstr, "", sizeof(firstipstr));
			    }
			    pthread_mutex_lock(&fileLock);
				fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
				pthread_mutex_unlock(&fileLock);
			}
			free(hostname);
		}
		else	//Queue is empty
		{
			pthread_mutex_unlock(&queueLock);
		}
	}
}

int main(int argc, char* argv[])
{

	int numFiles = argc - 2;
    /* Local Vars */

    pthread_mutex_init(&queueLock, NULL);
    pthread_mutex_init( &fileLock, NULL);

    queue_init(&hostnameQueue, 50);
    
    pthread_t requestThreads[argc-1];
    pthread_t resolverThreads[RESOLVER_THREAD_COUNT];

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
    for(int i=1; i<(argc-1); i++)	//Allocate 1 thread for every iteration here
    {

		int rc = pthread_create(&requestThreads[i-1], &attr, requestThreadFunction, argv[i]);
		if(rc)
		{
			printf("Request thread broke\n");
		}
		// printf("pthread_create return code = %i\n", rc );
    }

    for(int i = 0; i < RESOLVER_THREAD_COUNT; ++i)
    {
    	int rc = pthread_create(&resolverThreads[i], &attr, resolverThreadFunction, NULL);
    	if(rc)
    	{
    		printf("Resolver thread broke\n");
    	}
    }

    /* Join on the request threads */
    for(int i = 0; i < numFiles; ++i)
    {
    	int rc = pthread_join( requestThreads[i],  NULL);

    	if(rc)
    	{
    		printf("Request thread broke");
    	}
    }
    requestThreadsFinished = 1;

    /* Join on the resolver threads */
    for(int i = 0; i < RESOLVER_THREAD_COUNT; ++i)
    {
    	int rc = pthread_join( resolverThreads[i], NULL);
		if(rc)
    	{
    		printf("Resolver thread broke");
    	}    
    	else
    	{
    		// printf("Resolver thread #%i successfully joined\n", i );
    	}
    }

    /* Close Output File */
    fclose(outputfp);

    /* Destroy mutexes */
    pthread_mutex_destroy(&queueLock);
	pthread_mutex_destroy( &fileLock);

    return EXIT_SUCCESS;
}


