#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSIZE 51200
#define FLAGS 0


//wrapper for perror
void error(char *msg) {
  perror(msg);
  exit(1);
}

struct fpackets
   {
      int seqno;        // The seqno used in Stop-And-Wait
      char data[BUFSIZE];  // The next line in the text file
      long int packsize;
   };

 struct fsizepackets
   {
     int seqno;        // The seqno used in Stop-And-Wait
     long int packsize;
   };


int ls(FILE *f)
{
	struct dirent **dirent; int n = 0;

	if ((n = scandir(".", &dirent, NULL, alphasort)) < 0) {
		perror("Scanerror");
		return -1;
	}

	while (n--) {
		fprintf(f, "%s\n", dirent[n]->d_name);
		free(dirent[n]);
	}

	free(dirent);
	return 0;
}
/*
int sendto_ack(sockfd, buf, sockaddr, clientlen){
/  n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
//  printf ("%d", n);
//  if (n < 0)
//    error("ERROR in sendto");

//  return n;
}*/



int main(int argc, char **argv) {
  int sockfd;
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
	char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
	unsigned int alen;	/* length of address (for getsockname) */
	FILE* fp;
	int z;
	char message[BUFSIZE]; // check size
	char cmmd_recv[20];
	char file_name[20];
  int ack_send=0;
  int errno;
  struct fpackets packets;
  struct timeval timeout = {0, 0};
  off_t file_size = 0;
  struct stat stats;
  struct fsizepackets pfsize;
  long int pfsize_ack_num = 0;
  long int ack_num = 0;
  //check command line arguments
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  //socket: create the parent socket - yeshu
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		error("ERROR opening socket");
	}

	printf("Socket has been created: descriptor=%d\n", sockfd);

  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));


	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // yeshu - why INADDR_ANY https://stackoverflow.com/questions/16508685/understanding-inaddr-any-for-socket-programming
  serveraddr.sin_port = htons((unsigned short)portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
  	error("ERROR on binding");


	printf("Binding of the socket complete. Port number = %d\n", ntohs(serveraddr.sin_port));

	/*
	 * main loop: wait for a datagram, then echo it
	 */

	 clientlen = sizeof(clientaddr);
   for (;;) {

		//UDP File Server started
		printf("Server waiting on port to connect  %d\n", ntohs(serveraddr.sin_port));
		memset(message, 0, sizeof(message));
		memset(cmmd_recv, 0, sizeof(cmmd_recv));
		memset(file_name, 0, sizeof(file_name));

		// sizeof(message) - as per buf size
		n = recvfrom(sockfd, message, sizeof(message), 0, (struct sockaddr *)&clientaddr, &clientlen);
		if (n < 0)
			error("ERROR in recvfrom");

		printf("Server recieved command message: %s", message);
		sscanf(message, "%s %s", cmmd_recv, file_name);


    /*--------- get function-----------*/
		if((strcmp(cmmd_recv, "get") == 0) && (file_name!="\0"))
    {
      printf("SERVER: IN get function .\n");
      // check if file exist on sender's end
      // timeout func check
      if (access(file_name, F_OK) == 0) {
         printf("SERVER: Access file ok \n");
         int total_packets = 0, timeout_flag = 0, packet_resent = 0; // tb removed
         int packet_resent_filesize = 0;
         long int file_size = 0;
         timeout.tv_sec = 6;
         timeout.tv_usec = 0;

         // calculate file size to be sent to server
         if(stat(file_name, &stats) == 0)
         {
           file_size = stats.st_size;
         }
         else
         {
           printf("Unable to get file properties.\n");
           printf("Please check whether '%s' file exists.\n", file_name);
         }
         /*
         recvfrom with a Timeout Using the SO_RCVTIMEO Socket Option
         Our final example demonstrates the SO_RCVTIMEO socket option. We set this option once for a descriptor,
         specifying the timeout value, and this timeout then applies to all read operations on that descriptor.
         The nice thing about this method is that we set the option only once, compared to the previous two methods,
         which required doing something before every operation on which we wanted to place a time limit.
         But this socket option applies only to read operations, and the similar option SO_SNDTIMEO applies
         only to write operations; neither socket option can be used to set a timeout for a connect. */

         int t = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
         printf("setsockopt = %d",t); //Set timeout option for recvfrom
         // error("setsockopt failed.\n");


          // tb removed
         if ((file_size % BUFSIZE ) == 0)
         {
           total_packets = (file_size/BUFSIZE);
         }
         else
         {
           total_packets = ((file_size + (BUFSIZE -1)) / BUFSIZE);
         }
         // tb removed
         printf("total packets %d for filesize %ld\n", total_packets, file_size);

         long int s = 1;
         memset(&pfsize, 0, sizeof(pfsize));
         pfsize.seqno = s;
         pfsize.packsize = file_size;
         //sendto(sockfd, &(file_size), sizeof(file_size), 0, (struct sockaddr *)&serveraddr, serverlen);

         // sending file size to server
         sendto(sockfd, &(pfsize), sizeof(pfsize), 0, (struct sockaddr *)&clientaddr, clientlen);
         printf("Sending packet for file size with seq no %d and File size = %ld\n",pfsize.seqno, pfsize.packsize);
         recvfrom(sockfd, &(pfsize_ack_num), sizeof(pfsize_ack_num), 0, (struct sockaddr *)&clientaddr, &clientlen);
         printf("recieve ack for file size with seq no%ld\n",pfsize_ack_num);

         while (pfsize_ack_num != pfsize.seqno)
         {
             sendto(sockfd, &(pfsize), sizeof(pfsize), 0, (struct sockaddr *)&clientaddr, clientlen);
             printf("Sending packet for file size with seq no %d and File size = %ld\n",pfsize.seqno, pfsize.packsize);
             recvfrom(sockfd, &(pfsize_ack_num), sizeof(pfsize_ack_num), 0, (struct sockaddr *)&clientaddr, &clientlen);
             printf("recieve ack for file size with seq no %ld\n",pfsize_ack_num);
             packet_resent_filesize++;

             if (packet_resent_filesize > 50)
             {
                 printf("WARNING: Packet number %d has not received ack from server after resent attempt %d. Please break if you don't want to continue.\n", packets.seqno, packet_resent_filesize);
             }

         }

         // Reading the file
         fp = fopen(file_name, "rb");
         if (fp == NULL){
           perror("[ERROR] reading the file");
           exit(1);
         }

         // send packet
         // (p=1; p <= (file_size - bufsize); p++)
         long int p = 1;
         while (file_size > 0)
         {
             memset(&packets, 0, sizeof(packets));
             //ack_num = 0;
             packets.seqno = p;

             //https://www.educative.io/edpresso/c-reading-data-from-a-file-using-fread
             packets.packsize = fread(packets.data, 1, BUFSIZE, fp);
             printf("Packsize = %ld",packets.packsize);
             sendto(sockfd, &(packets), sizeof(packets), 0, (struct sockaddr *)&clientaddr, clientlen);
             printf("Sending packet %d\n",packets.seqno);
             recvfrom(sockfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&clientaddr, &clientlen);
             printf("recieve ack %ld\n",ack_num);

             while (ack_num != packets.seqno)
             {
               printf("Resending packet %d\n",packets.seqno);
               sendto(sockfd, &(packets), sizeof(packets), 0, (struct sockaddr *)&clientaddr, clientlen);
               printf("Sending packet %d\n",packets.seqno);
               recvfrom(sockfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&clientaddr, &clientlen);
               printf("recieve ack %ld\n",ack_num);
               packet_resent++;


               if (packet_resent > 50)
               {
                   printf("WARNING: Packet number %d has not received ack from server after resent attempt %d. Please break if you don't want to continue.\n", packets.seqno, packet_resent);
               }

             }

             printf("Summary - packet ----> %ld	Ack ----> %ld\n", p, ack_num);
             file_size = file_size - BUFSIZE;
             printf("file size in loop %ld\n", file_size);
             p ++;

         }

         fclose(fp);

         printf("Disable the timeout\n");
         timeout.tv_sec = 0;
         timeout.tv_usec = 0;
         int x = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
         printf("setsockopt = %d",x);
         //if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ); //Disable timeout
         //    error("setsockopt failed.\n");

         /*
          // Send file
          send_file_data(fp, sockfd, serveraddr);

          // send the message to the server
          serverlen = sizeof(serveraddr);
          n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
          if (n < 0)
            error("ERROR in sendto");

          /* print the server's reply
          n = recvfrom(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, &serverlen);
          if (n < 0)
          error("ERROR in recvfrom");
          printf("Echo from server: %s", buf);
          return 0;

          */


      }
      else {
         printf("Input file doesn't exist, provide a valid file name.\n");
         continue;
      }
    }

      /* put function
      put shpuld not create invalid files.
      */
		else if((strcmp(cmmd_recv, "put") == 0) && (file_name!="\0"))
    {
      printf("Server: Put called with file name --> %s\n", file_name);
      long int file_size = 0;
      int total_packets = 0, p = 1, s=1;

      timeout.tv_sec = 10;
      timeout.tv_usec = 0;
      //if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval)) < 0 ); //Set timeout option for recvfrom
      //  error("setsockopt failed.\n");

      int t = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
      printf("setsockopt = %d",t);


      memset(&pfsize, 0, sizeof(pfsize));
      //recvfrom(sockfd, &(file_size), sizeof(file_size), 0, (struct sockaddr *)&clientaddr, &clientlen);
      recvfrom(sockfd, &(pfsize), sizeof(pfsize), 0, (struct sockaddr *)&clientaddr, &clientlen);
      printf("recieve packet %d\n",pfsize.seqno);

      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      int x = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
      printf("setsockopt = %d",x);

      file_size = pfsize.packsize;
      printf("Recieved filesize %ld\n", file_size);
      if ((file_size % BUFSIZE ) == 0)
      {
        total_packets = (file_size/BUFSIZE);
      }
      else
      {
        total_packets = ((file_size + (BUFSIZE -1)) / BUFSIZE);
      }
      printf("total packets %d for filesize %ld\n", total_packets, file_size);

      if (pfsize.seqno < s || pfsize.seqno > s)
      {
        s--;
        printf("Resending ack for file size %d",packets.seqno);
        sendto(sockfd, &(pfsize.seqno), sizeof(pfsize.seqno), 0, (struct sockaddr *)&clientaddr, clientlen);
      }
      else
      {
          sendto(sockfd, &(pfsize.seqno), sizeof(pfsize.seqno), 0, (struct sockaddr *)&clientaddr, clientlen);
          printf("send ack for file size %d\n",pfsize.seqno);

          printf("packet ----> %d	Ack ----> %d\n", pfsize.seqno, s);
      }
      // put second phase
      fp = fopen(file_name, "wb");

      while (file_size > 0)
      {
          memset(&packets, 0, sizeof(packets));
          recvfrom(sockfd, &(packets), sizeof(packets), 0, (struct sockaddr *)&clientaddr, &clientlen);
          printf("recieve packet %d\n",packets.seqno);
          sendto(sockfd, &(packets.seqno), sizeof(packets.seqno), 0, (struct sockaddr *)&clientaddr, clientlen);
          printf("send ack %d\n",packets.seqno);

          printf("packet ----> %d	Ack ----> %d\n", packets.seqno, p);
          //printf("[RECEVING] Data: %s", packets.data);
          if (packets.seqno < p || packets.seqno > p)
          {
            p--;
            printf("Resending ack %d",packets.seqno);
          }
          else
          {
              fwrite(packets.data, 1, packets.packsize, fp);
              file_size = file_size - BUFSIZE;
          }
          printf ("file size now= %ld", file_size);
          p++;

      }
      fclose(fp);

    }
      //printf("Disable the timeout\n");
		  //timeout.tv_sec = 0;
      //timeout.tv_usec = 0;
      //int x = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
      //printf("setsockopt = %d",x);


      /*
      memset(buf,0,sizeof(buf));
  		//n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);

  		if (n < 0)
  			{error("ERROR in recvfrom");
         }


  		printf("\nFile Name Received: %s\n", file_name);


      //fp = fopen(file_name, "w");
  		//if (fp == NULL)
  		//printf("\nFile open failed!\n");

  		//char *filename = "server-file.txt";


  			fp = fopen(file_name, "wb"); //ichanged it to file_name
        printf("File has been opened /n");

  		// Receiving the data and writing it into the file.
  		while(1){
  			printf ("In loop");
  			n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*)&clientaddr, &clientlen);

  			if (strcmp(buf, "END") == 0){
  				printf ("breaking now");
  				break;
  				}
  			printf("[RECEVING] Data: %s", buf);
  			z = fwrite(buf,1,sizeof(buf),fp);
  			printf ("file write output %d", z);
  			//bzero(buf, BUFSIZE);
  		        }
  		  fclose(fp);

  	  //gethostbyaddr: determine who sent the datagram

      hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
  			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
      if (hostp == NULL)
        error("ERROR on gethostbyaddr");
      hostaddrp = inet_ntoa(clientaddr.sin_addr);
      if (hostaddrp == NULL)
        error("ERROR on inet_ntoa\n");
      printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
      printf("server received %ld/%d bytes: %s\n", strlen(buf), n, buf);
  		  /*
       * sendto: echo the input back to the client

      n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
  		printf ("%d", n);
      if(n < 0)
        error("ERROR in sendto");

      */

      /* delete function*/
      else if((strcmp(cmmd_recv, "delete") == 0) && (file_name!="\0")){
      //get request from client
      //check if the file name exists or not
     //remove the file
     //send the ack back to the client
         if(access(file_name, F_OK) == -1) {
          printf("ERROR NUMBER: %d\n",errno );
          perror("ERROR DESCRIPTION");
				 ack_send = -1;
				 sendto(sockfd, &(ack_send), sizeof(ack_send), 0, (struct sockaddr *)&clientaddr, clientlen);
		    	}
		    	else{
			  	if(access(file_name, R_OK) == -1) {  //Check if file has appropriate permission
          printf("ERROR NUMBER: %d\n",errno );
          perror("ERROR DESCRIPTION");
          ack_send = 0;
					sendto(sockfd, &(ack_send), sizeof(ack_send), 0, (struct sockaddr *)&clientaddr, clientlen);
				  }
			  	else {
					printf("Filename is %s\n", file_name);
					remove(file_name);  //delete the file
					ack_send = 1;
					sendto(sockfd, &(ack_send), sizeof(ack_send), 0, (struct sockaddr *)&clientaddr, clientlen);
				}
			}
		}

      /*ls function*/
      else if((strcmp(cmmd_recv, "ls") == 0))
      {
        char file_entry[200];
  			memset(file_entry, 0, sizeof(file_entry));

  			fp = fopen("a.log", "wb");	//Create a file with write permission

  			if (ls(fp) == -1)		//get the list of files in present directory
  				perror("ls");

  			fclose(fp);

  			fp = fopen("a.log", "rb");
  			int filesize = fread(file_entry, 1, 200, fp);

  			printf("Filesize = %d	%ld\n", filesize, strlen(file_entry));

  			if (sendto(sockfd, file_entry, filesize, 0, (struct sockaddr *)&clientaddr, clientlen)<0)  //Send the file list
  				perror("Server: send");

  			remove("a.log");  //delete the temp file
  			fclose(fp);
      }

      /*exit*/
      else if((strcmp(cmmd_recv, "exit") == 0))
      {
        close(sockfd);
        exit(0);
      }

      /*Invalid case*/
      else
      printf("Please enter a valid command from the menu\n");

		}
	  }


	//netstat -tulnp | grep 5001
