/*TASKS
- get functions- done
- headers reorganize alphaba- done
- add #define for constants- done
- packets reorganize-done
- packets comments for all variables-done
- struct packets names : filepacket -->  filesizepacket ;; fpackets and fsizepackets-done
- main extra variables delete
- PRINT commands - "formalize"
*/
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
      int seqno;           // The sequence no used in Stop-And-Wait Protocol
      char data[BUFSIZE];  // The next line in the text file
      long int packsize;   //size of each packet
   };

struct fsizepackets
  {
    int seqno;           // The sequence no used in Stop-And-Wait Protocol
    long int packsize;   // size of the packet of file size
  };

int main(int argc, char **argv)
{
  int sockfd, portno, n;
  int serverlen;
  struct sockaddr_in serveraddr,clientaddr;
  struct hostent *server;
  char *hostname;
  char buf[BUFSIZE];
  FILE *fp;
  char cmmd_send[20];
	char file_name[20];
	char cmd[10];
  int ack_recv=0;
  struct stat stats;
  struct fpackets packets; //
  struct fsizepackets pfsize;
  off_t file_size = 0;
  long int ack_num = 0;
  long int pfsize_ack_num = 0;
  struct timeval timeout = {0, 0};

  /* check command line arguments */

  if (argc != 3) {
     fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
     exit(0);
  }
  printf("Argument value %s\n", argv[0]);
  hostname = argv[1];
  portno = atoi(argv[2]);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		error("ERROR opening socket");
	}

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(hostname);
  if (server == NULL) {
      fprintf(stderr,"ERROR, no such host as %s\n", hostname);
      exit(0);
  }

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);
  serverlen = sizeof(serveraddr);
  // yeshu for vs while (1)
  for (;;) {

		memset(cmmd_send, 0, sizeof(cmmd_send));
		memset(cmd, 0, sizeof(cmd));
		memset(file_name, 0, sizeof(file_name));

		printf("\n Menu \n Enter any of the following commands \n 1.) get [file_name] \n 2.) put [file_name] \n 3.) delete [file_name] \n 4.) ls \n 5.) exit \n");
		scanf(" %[^\n]%*c", cmmd_send);

		//printf("----> %s\n", cmmd_send);

		sscanf(cmmd_send, "%s %s", cmd, file_name);		//parse the user input into command and filename

    // server ko send command
		if (sendto(sockfd, cmmd_send, sizeof(cmmd_send), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
			error("Client: error in command send");
		}


   /*----get function----------*/
   if((strcmp(cmd,"get") == 0) && (file_name!= "/0"))
      {
        printf("CLIENT: get function with file name --> %s\n", file_name);
        long int file_size = 0;
        int total_packets = 0, p = 1, s=1;

        timeout.tv_sec = 6;
        timeout.tv_usec = 0;
        //if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval)) < 0 ); //Set timeout option for recvfrom
        //  error("setsockopt failed.\n");

        int t = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
        printf("setsockopt = %d",t);


        memset(&pfsize, 0, sizeof(pfsize));
        //recvfrom(sockfd, &(file_size), sizeof(file_size), 0, (struct sockaddr *)&clientaddr, &clientlen);
        recvfrom(sockfd, &(pfsize), sizeof(pfsize), 0, (struct sockaddr *)&serveraddr, &serverlen);
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
        printf("Total packets %d for filesize %ld\n", total_packets, file_size);

        if (pfsize.seqno < s || pfsize.seqno > s)
        {
          s--;
          printf("Resending ack for file size %d",packets.seqno);
          sendto(sockfd, &(pfsize.seqno), sizeof(pfsize.seqno), 0, (struct sockaddr *)&serveraddr, serverlen);
        }
        else
        {
            sendto(sockfd, &(pfsize.seqno), sizeof(pfsize.seqno), 0, (struct sockaddr *)&serveraddr, serverlen);
            printf("Sending ack for file size received %d\n",pfsize.seqno);

            printf("Packet No.----> %d	Ack No.----> %d\n", pfsize.seqno, s);
        }
        // put second phase
        fp = fopen(file_name, "wb");

        while (file_size > 0)
        {
            memset(&packets, 0, sizeof(packets));
            recvfrom(sockfd, &(packets), sizeof(packets), 0, (struct sockaddr *)&serveraddr, &serverlen);
            printf("Recieved packet---> %d\n",packets.seqno);
            sendto(sockfd, &(packets.seqno), sizeof(packets.seqno), 0, (struct sockaddr *)&serveraddr, serverlen);
            printf("Send acknowledgement---> %d\n",packets.seqno);

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

    /*put function*/
   else if((strcmp(cmd,"put") == 0) && (file_name!= "/0"))
     {
       printf("CLIENT: IN Put function .\n");
       // check if file exist on sender's end
       // timeout func check
       if (access(file_name, F_OK) == 0) {
          printf("CLIENT: Access file ok \n");
          int total_packets = 0, timeout_flag = 0, packet_resent = 0; // tb removed
          int packet_resent_filesize = 0;
          long int file_size = 0;
          timeout.tv_sec = 5;
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
          sendto(sockfd, &(pfsize), sizeof(pfsize), 0, (struct sockaddr *)&serveraddr, serverlen);
          printf("Sending packet for file size with seq no %d and File size = %ld\n",pfsize.seqno, pfsize.packsize);
          recvfrom(sockfd, &(pfsize_ack_num), sizeof(pfsize_ack_num), 0, (struct sockaddr *)&serveraddr, &serverlen);
          printf("recieve ack for file size with seq no%ld\n",pfsize_ack_num);

          while (pfsize_ack_num != pfsize.seqno)
          {
              sendto(sockfd, &(pfsize), sizeof(pfsize), 0, (struct sockaddr *)&serveraddr, serverlen);
              printf("Sending packet for file size with seq no %d and File size = %ld\n",pfsize.seqno, pfsize.packsize);
              recvfrom(sockfd, &(pfsize_ack_num), sizeof(pfsize_ack_num), 0, (struct sockaddr *)&serveraddr, &serverlen);
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
              sendto(sockfd, &(packets), sizeof(packets), 0, (struct sockaddr *)&serveraddr, serverlen);
              printf("Sending packet %d\n",packets.seqno);
              recvfrom(sockfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&serveraddr, &serverlen);
              printf("recieve ack %ld\n",ack_num);

              while (ack_num != packets.seqno)
              {
                printf("Resending packet %d\n",packets.seqno);
                sendto(sockfd, &(packets), sizeof(packets), 0, (struct sockaddr *)&serveraddr, serverlen);
                printf("Sending packet %d\n",packets.seqno);
                recvfrom(sockfd, &(ack_num), sizeof(ack_num), 0, (struct sockaddr *)&serveraddr, &serverlen);
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

     /*delete function*/
     else if((strcmp(cmd,"delete")==0)&&(file_name!="/0"))
      {
		  	ack_recv = 0;
        int read_data;
		  	serverlen=sizeof(serveraddr);
        if((read_data = recvfrom(sockfd, &(ack_recv), sizeof(ack_recv), 0,  (struct sockaddr *) &serveraddr, &serverlen)) < 0)	//Recv ack from server
				error("recieve");

        if (ack_recv > 0)
				printf("Client: Deleted the file\n");
		  	else if (ack_recv < 0)
				printf("Client: Invalid file name\n");
		  	else
				printf("Client: File does not have appropriate permission\n");
      }



      /*ls function*/
        else if(strcmp(cmd,"ls")==0){
        long int numRead;
        char filename[200];
 		   	memset(filename, 0, sizeof(filename));


 		  	if ((numRead = recvfrom(sockfd, filename, sizeof(filename), 0,  (struct sockaddr *) &serveraddr, &serverlen)) < 0)
 				error("recieve");

 		  	if (filename[0] != '\0') {
 				printf("Number of bytes recieved = %ld\n", numRead);
 				printf("\nThis is the List of files and directories --> \n%s \n", filename);
 		  	}
 		  	else {
 				printf("Recieved buffer is empty\n");
 				continue;
 			}
 		}




}
}

// ADD
//sudo iptables -A INPUT -p udp -s localhost -m statistic --mode random --probability 0.1 -j DROP

// Delete
// sudo iptables -D INPUT -p udp -s localhost -m statistic --mode random --probability 0.1 -j DROP

// sudo iptables -L


// scp -i pm-ubuntu.pem googlechrome.dmg ubuntu@ec2-54-242-49-26.compute-1.amazonaws.com:~/home/ubuntu

// ls -l --block-size=M
