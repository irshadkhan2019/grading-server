/* run client using: ./client localhost <server_port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>       
#include <sys/stat.h>       
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h> 


void error(char *msg) {
  perror(msg);
  exit(0);
}

float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

int main(int argc, char *argv[]) {

  int sockfd, portno, n,loops,sleeptime,successres=0,numOfTimeouts=0;
  float totalresponsetime=0,elapsed;
  struct timeval tvreq;
  struct timeval tvres;
  struct timeval tvloopstart;
  struct timeval tvloopend;

  struct timeval timeout; // Define the timeout structure
  timeout.tv_sec = 0.1;   // Set the timeout to 5 seconds (adjust as needed)
  timeout.tv_usec = 0;



  struct sockaddr_in serv_addr; //Socket address structure
  struct hostent *server; //return type of gethostbyname

  char buffer[1024]; //buffer for message

  if (argc < 6) {
    fprintf(stderr, "usage %s hostname port <c_file> loopnum sleeptimeinsec <new/status> <token>\n", argv[0]);
    exit(0);
  }
  
  portno = atoi(argv[2]); // 2nd argument of the command is port number
  char *c_file = argv[3];
  loops=atoi(argv[4]);
  sleeptime=atoi(argv[5]);
  char *check=argv[6];

  char *token;
  if(argv[7]){
   token=argv[7];
  //  printf("TOKEN",token);
  }else{
    token="NA";
  }
  
  /* create socket, get sockfd handle */



  gettimeofday(&tvloopstart, NULL);

  for(int i=0;i<loops;i++){

  sockfd = socket(AF_INET, SOCK_STREAM, 0); //create the half socket. 
  //AF_INET means Address Family of INTERNET. SOCK_STREAM creates TCP socket (as opposed to UDP socket)

  if (sockfd < 0)
    error("ERROR opening socket");

  /* fill in server address in sockaddr_in datastructure */

  server = gethostbyname(argv[1]);
  //finds the IP address of a hostname. 
  //Address is returned in the 'h_addr' field of the hostend struct

  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr)); // set server address bytes to zero

  serv_addr.sin_family = AF_INET; // Address Family is IP

  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
/*Copy server IP address being held in h_addr field of server variable
to sin_addr.s_addr of serv_addr structure */

//convert host order port number to network order
  serv_addr.sin_port = htons(portno);

  /* connect to server 
  First argument is the half-socket, second is the server address structure
  which includes IP address and port number, third is size of 2nd argument
  */

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR connecting");



  int fd = open(c_file, O_RDONLY);

  bzero(buffer, 1024);

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    error("ERROR setting socket options");
  }
  
  int bytesread = read(fd, buffer, 1024);

      
  // read time just b4 sending req
  gettimeofday(&tvreq, NULL);


  n = write(sockfd, buffer, bytesread); 

  close(fd);
 

  if (n < 0)
    error("ERROR writing to socket");
  bzero(buffer, 1024);

  /* read reply from server 
  First argument is socket, 2nd is string to read into, 3rd is number of bytes to read
  */

  // client gets tokenId to check status
  n = read(sockfd, buffer, 1024);
  printf("Server response: %s\n", buffer);

  if((strcmp(check, "new") == 0)){
      n = write(sockfd, "new", 3); 
  }else{
    char my_token[50];
    strcpy(my_token, token);
    n = write(sockfd, my_token, sizeof(my_token)); 
    bzero(buffer, 1024);
    n = read(sockfd, buffer, 1024);
    printf("Server status response: %s\n", buffer);

  }


  if (n < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        // Handle the timeout here (e.g., print a message)
        printf("Timeout occurred\n");
        numOfTimeouts++;
        continue;
      
      } else {
        error("ERROR reading from socket");
        continue;
      }
    }
  
  if(n!=0){
    successres++;
  }
  // read the currenttime just after getting the response
  gettimeofday(&tvres, NULL);

  if (n < 0)
    error("ERROR reading from socket");
 

  elapsed=timedifference_msec(tvreq, tvres);
  printf("response time: %f msec\n",elapsed );
  totalresponsetime+=elapsed;

  sleep(sleeptime);
  close(sockfd);
}

gettimeofday(&tvloopend, NULL);
elapsed=timedifference_msec(tvloopstart, tvloopend);

printf("AVG response time: %f msec\n", totalresponsetime/loops);
printf("Successful Response: %d\n", successres);
printf("Time taken for completing loop : %f msec\n",elapsed);
return 0;

}


