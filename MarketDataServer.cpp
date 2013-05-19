#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include "exchange.h"
#include <sys/time.h>
#include <iostream>
void err(const char *str)
{
		// Prints the error message
    perror(str);
    exit(1);
}
 
int main(int argc, char **argv)
{
		if(argc<2 || argc>2){
			printf("Usage: %s <config_file>\n",argv[0]);
			exit(-1);
		}

		FILE *fp;
		fp = fopen(argv[1], "r");
    if (fp == NULL) {
       printf("Failed to open %s for reading.\n",argv[1]);
       exit(0);
    }

		srand(time(0));  // For generating a random value 
		int a,z;
		float p;
		char bin_file[32],log_file[32];
		char ip1[48],ip2[48];

		// scan thru the config file
    while (fscanf(fp,"%s%d%s%d%s%f%s\n", ip1,&a,ip2,&z,bin_file,&p,log_file)==5)
       printf("%s,%d,%s,%d,%s,%f,%s\n",ip1,a,ip2,z,bin_file,p,log_file);

		// Close the ConfigFile
		fclose(fp);
		// Open the logFile
		fp = fopen(log_file,"w");

		int PORTA=a;
		int PORTZ=z;

		FILE *bin_fp;
		bin_fp = fopen(bin_file,"rb");
		if(!bin_fp)
		{
			printf("\nUnable to open %s",bin_file);
			exit(-2);
		}
    struct sockaddr_in my_addr, cli_addr;
    int sockfd, i; 
    socklen_t slen=sizeof(cli_addr);

		// create a socket sockfd for sending UDP packets via PORTA 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      err("socket");
 
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORTA);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(inet_aton(ip1, &my_addr.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    struct sockaddr_in my_addr2, cli_addr2;
    int sockfd2, i2;
    socklen_t slen2=sizeof(cli_addr2);

		// create a socket sockfd for sending UDP packets via PORTZ 
    if ((sockfd2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      err("socket");

    bzero(&my_addr2, sizeof(my_addr2));
    my_addr2.sin_family = AF_INET;
    my_addr2.sin_port = htons(PORTZ);
    my_addr2.sin_addr.s_addr = htonl(INADDR_ANY);
		if(inet_aton(ip2, &my_addr2.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

		struct ExchangeA_MD my_record;
		int k;

	 	struct timespec tim, tim2;
   	tim.tv_sec = 0;
   	tim.tv_nsec = 1;

		int az=0,za=0;

		struct timeval startTime;
		struct timeval endTime;

		gettimeofday(&startTime, NULL);   // get the start time
 
		int ret=1;
    while(ret>0)
    {
      ret=fread(&my_record,sizeof(struct ExchangeA_MD),1,bin_fp); 

      k = ((rand () % 100) > (100*p)) ? 1 : 0;

			nanosleep(&tim , &tim2);
			
			if(k==0) { // With probability p first send my_record over PORTA & then over PORTZ	
				az++;
	      if (sendto(sockfd,(char*)&my_record,sizeof(struct ExchangeA_MD), 0, (struct sockaddr*)&my_addr, slen)==-1)
  	       err("sendto()");
	      if (sendto(sockfd2,(char*)&my_record,sizeof(struct ExchangeA_MD), 0, (struct sockaddr*)&my_addr2, slen)==-1)
  	       err("sendto()");
			}
			else {  // With probability 1-p first send my_record over PORTZ & then over PORTA
				za++;
	      if (sendto(sockfd2,(char*)&my_record,sizeof(struct ExchangeA_MD), 0, (struct sockaddr*)&my_addr2, slen)==-1)
  	       err("sendto()");
	      if (sendto(sockfd,(char*)&my_record,sizeof(struct ExchangeA_MD), 0, (struct sockaddr*)&my_addr, slen)==-1)
  	       err("sendto()");
			}
    }
		
		gettimeofday(&endTime, NULL);  // get the end time

		fclose(bin_fp);
    close(sockfd);
    close(sockfd2);

		double tS = startTime.tv_sec*1000000 + (startTime.tv_usec);
		double tE = endTime.tv_sec*1000000  + (endTime.tv_usec);

		fprintf(fp,"\nThe Number of Packets sent over Port %d prior to Port %d are: %d",a,z,az);
		fprintf(fp,"\nTotal time in microseconds taken by the Server Process: %.0lf",tE - tS);
		fclose(fp);
    return 0;
}
