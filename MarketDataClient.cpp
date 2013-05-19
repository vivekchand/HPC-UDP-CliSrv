#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include "exchange.h"
#include <stdio.h>
#include <iostream>
#include <map>
#include "llist.h"
#include <semaphore.h>
#include <pthread.h>

void* udp_get_function1(void *PORTA);
void* udp_get_function2(void *PORTZ);

sem_t mutex;
std::map<int,bool> processed;  // Hash Table of seqno_ & bool val True if already processed, else false   
std::map<std::string,NODE*> stockmap; // Hash Table of StockName & head node of Linked List

int cn=0; /* shared variables */ 
int PORTA;	// Stores Port 'a' val
int PORTZ;  // Stores Port 'z' val
FILE *fp;   
FILE *dbg_fp;  // File Pointer for storing Debug Messages

void err(const char *s)
{
		// Prints the error message
    perror(s);
    exit(1);
}

int main(int argc, char** argv)
{
		dbg_fp = fopen(DEBUG_LOG_FILE,"w");   // Configure DEBUG_LOG_FILE in exchange.h
		pthread_t thread1, thread2; // 2 threads

    if(argc<2 || argc>2){
      printf("Usage: %s <config_file>\n",argv[0]);
      exit(-1);
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
       printf("Failed to open %s for reading.\n",argv[1]);
       exit(0);
    }

		sem_init(&mutex, 0, 1); // Initialize the semaphore

    int a,z;
    char log_file[32],ip1[48],ip2[48];

		// scan thru the config file
    while (fscanf(fp,"%s%d%s%d%s\n", ip1,&a,ip2,&z,log_file)==5)
       fprintf(dbg_fp,"%s,%d,%s,%d,%s\n",ip1,a,ip2,z,log_file);

		// close the configFile
		fclose(fp);
		// open the logFile
		fp = fopen(log_file,"w");

		// Store the PORTA, PORTZ file just scanned from the configFile
    PORTA=a;
    PORTZ=z;
	
		int  iret1, iret2;
		// Create 2 threads, with functions udp_get_function1 & udp_get_function2
		iret1 = pthread_create(&thread1, NULL, udp_get_function1, (void*)0);
		iret2 = pthread_create(&thread2, NULL, udp_get_function2, (void*)0);

		// Join the 2 threads
		pthread_join(thread1, NULL);
		pthread_join(thread2, NULL);

		sem_destroy(&mutex); // destroy semaphore 
    return 0;
}

// Called from the Threads, every 100 UDP Packets are processed.
void output(FILE *fp)
{
    NODE *head=NULL;
		
    std::map<std::string,NODE*>::iterator it; // An Iterator to iterate thru HashTable stockmap
    for (it=stockmap.begin(); it!=stockmap.end(); ++it)
    {
      fprintf(fp,"%s\n",(it->first).c_str());
      head = it->second; // get the head pointer
      display(head,fp);  // call the display() defined in llist.h
    }
}

void* udp_get_function1(void *ptr)
{
		char hash[15];
    struct sockaddr_in serv_addr;
    int sockfd, i, slen=sizeof(serv_addr);

		// create a socket sockfd for receiving UDP packets via PORTA
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        err("socket");

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORTA);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		// Bind the socket
    if (bind(sockfd, (struct sockaddr* ) &serv_addr, sizeof(serv_addr))==-1)
      err("bind");
    else
      fprintf(dbg_fp,"Client1 : bind() successful\n");
		
    while(cn<NUM_OF_PACKETS)  // Receive till cn is less than NUM_OF_PACKETS
    {
			fprintf(dbg_fp,"\ncn:%d",cn);
				struct ExchangeA_MD my_record;
				// Receive from Server via PORTA
	      if (recvfrom(sockfd, (char*)&my_record, sizeof(struct ExchangeA_MD) , 0, (struct sockaddr*)&serv_addr, (socklen_t*)&slen)==-1)
          err("recvfrom()");
				sem_wait(&mutex);  // Acquire the Semaphore
				if(processed[my_record.seqno_]){
						fprintf(dbg_fp,"\nThread1 Skipping Already Processed %d cn=%d... \n",my_record.seqno_,cn+1);
						sem_post(&mutex); // Release the Semaphore
						continue;
				}
	  	    fprintf(dbg_fp,"\tseqno: %d\n",my_record.seqno_);
					// Make the HashName
					sprintf(hash,"%s_%c",my_record.contract_,my_record.side_);
  		    fprintf(dbg_fp,"\thash: %s\n",hash);

				// Check if Hash entry already exists
				if(stockmap[hash]){
					NODE *head;
					head = stockmap[hash];
					if(my_record.msg_==0) // Insert
					{
							fprintf(dbg_fp,"\nInsert");
							head = insert(head,my_record.level_,my_record.price_,my_record.size_);
					}
					else if(my_record.msg_==1) // Delete
					{
							fprintf(dbg_fp,"\nDelete");
							head = del(head,my_record.level_);
					}
					else  // UPDATE
					{
							fprintf(dbg_fp,"\nUpdate");
							head = modify(head,my_record.level_,my_record.price_,my_record.size_);
					}
					processed[my_record.seqno_] = true;
				}
				else{
						fprintf(dbg_fp,"\nFresh Insert");
						NODE *head;
						head = NULL;
						head = insert(head,my_record.level_,my_record.price_,my_record.size_);
						stockmap[hash] = head;
						processed[my_record.seqno_] = true;
				}
				cn++;
				if(my_record.seqno_%100 == 0){ // Log Limit Book Every 100 Messages are Processed
						fprintf(fp,"\nLimit Book @ %d\n",my_record.seqno_);
						output(fp);
						fflush(fp);
				}
				fprintf(dbg_fp,"\nThread1");
				sem_post(&mutex);  // Release the Semaphore
		}// end while
		close(sockfd);
}

void* udp_get_function2(void *ptr)
{
		char hash[15];
	  struct sockaddr_in serv_addr2;
    int sockfd2, i2, slen2=sizeof(serv_addr2);

		// create a socket sockfd for receiving UDP packets via PORTZ
    if ((sockfd2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        err("socket");

    bzero(&serv_addr2, sizeof(serv_addr2));
    serv_addr2.sin_family = AF_INET;
    serv_addr2.sin_port = htons(PORTZ);
    serv_addr2.sin_addr.s_addr = htonl(INADDR_ANY);

		// Bind the socket
    if (bind(sockfd2, (struct sockaddr* ) &serv_addr2, sizeof(serv_addr2))==-1)
      err("bind");
    else
      fprintf(dbg_fp,"Client2 : bind() successful\n");

    while(cn<NUM_OF_PACKETS) // Receive till cn is less than NUM_OF_PACKETS
    {
			fprintf(dbg_fp,"\ncn:%d",cn);
				struct ExchangeA_MD my_record;
				// Receive from Server via PORTA
	      if (recvfrom(sockfd2, (char*)&my_record, sizeof(struct ExchangeA_MD) , 0, (struct sockaddr*)&serv_addr2, (socklen_t*)&slen2)==-1)
          err("recvfrom()");
				sem_wait(&mutex);   // Acquire the Semaphore
				if(processed[my_record.seqno_]){
						fprintf(dbg_fp,"\nThread2 Skipping Already Processed %d cn=%d... \n",my_record.seqno_,cn+1);
						sem_post(&mutex); // Release the Semaphore
						continue;
				}
  	    fprintf(dbg_fp,"\tseqno: %d\n",my_record.seqno_);
				// Make the HashName
				sprintf(hash,"%s_%c",my_record.contract_,my_record.side_);
  	    fprintf(dbg_fp,"\thash: %s\n",hash);

				// Check if Hash entry already exists
				if(stockmap[hash]){
					NODE *head;
					head = stockmap[hash];
					if(my_record.msg_==0) // Insert
					{
							fprintf(dbg_fp,"\nInsert");
							head = insert(head,my_record.level_,my_record.price_,my_record.size_);
					}
					else if(my_record.msg_==1) // Delete
					{
							fprintf(dbg_fp,"\nDelete");
							head = del(head,my_record.level_);
					}
					else  // UPDATE
					{
							fprintf(dbg_fp,"\nUpdate");
							head = modify(head,my_record.level_,my_record.price_,my_record.size_);
					}
					processed[my_record.seqno_] = true;
				}
				else{
						fprintf(dbg_fp,"\nFresh Insert");
						NODE *head;
						head = NULL;
						head = insert(head,my_record.level_,my_record.price_,my_record.size_);
						stockmap[hash] = head;
						processed[my_record.seqno_] = true;
				}
				cn++;
				if(my_record.seqno_%100 == 0){  // Log Limit Book Every 100 Messages are Processed
						fprintf(fp,"\nLimit Book @ %d\n",my_record.seqno_);
						output(fp);
						fflush(fp);
				}
				
				sem_post(&mutex); // Release the Semaphore
		}// end while
		close(sockfd2);
}
