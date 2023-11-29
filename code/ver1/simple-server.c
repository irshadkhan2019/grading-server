/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>       
#include <sys/stat.h>       
#include <fcntl.h>
#include <netinet/in.h>


void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) 
{
   int sockfd, //the listen socket descriptor (half-socket)
   newsockfd, //the full socket after the client connection is made
   portno; //port number at which server listens

  socklen_t clilen; //a type of an integer for holding length of the socket address
  char buffer[1024]; //buffer for reading and writing the messages
  struct sockaddr_in serv_addr, cli_addr; //structure for holding IP addresses
  int n;

  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  /* create socket */

  sockfd = socket(AF_INET, SOCK_STREAM, 0); 
  //AF_INET means Address Family of INTERNET. SOCK_STREAM creates TCP socket (as opposed to UDP socket)
 // This is just a holder right now, note no port number either. It needs a 'bind' call


  if (sockfd < 0)
    error("ERROR opening socket");

 
  bzero((char *)&serv_addr, sizeof(serv_addr)); // initialize serv_address bytes to all zeros
  
  serv_addr.sin_family = AF_INET; // Address Family of INTERNET
  serv_addr.sin_addr.s_addr = INADDR_ANY;  //Any IP address. 

//Port number is the first argument of the server command
  portno = atoi(argv[1]);
  serv_addr.sin_port = htons(portno);  // Need to convert number from host order to network order
  
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  /* listen for incoming connection requests */

  listen(sockfd, 2); // 2 means 2 connection requests can be in queue. 
  //now server is listening for connections


  clilen = sizeof(cli_addr);  //length of struct sockaddr_in


  /* accept a new request, now the socket is complete.
  Create a newsockfd for this socket.
  First argument is the 'listen' socket, second is the argument 
  in which the client address will be held, third is length of second
  */
  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  while(1){

  if (newsockfd < 0)
    error("ERROR on accept");

  /* read message from client */

  bzero(buffer, 1024); //set buffer to zero

  //issue read call on the socket, read 255 bytes.
  int fd = open("c_code_server.c", O_WRONLY | O_TRUNC | O_CREAT, 0666); 
  n = read(newsockfd, buffer, 1024);
  
  write(fd, buffer, n);
  close(fd);
  
  int stderr_fd = open("error_output.txt", O_WRONLY | O_CREAT, 0666);
  if (stderr_fd < 0)
      error("ERROR opening error_output.txt");

  // Redirect stderr to the file
  if (dup2(stderr_fd, STDERR_FILENO) == -1)
      error("ERROR redirecting stderr");

  // Close the original stderr file descriptor
  close(stderr_fd);
  
  if (n < 0)
      error("ERROR reading from socket");
  
  const char *command1 = "/usr/bin/gcc c_code_server.c -o c_code_server";
  int status1 = system(command1);
  if(status1 == 0)
  {   //if no compiler error
      const char *command2 = "./c_code_server > out_gen.txt"; 
      int status2 = system(command2);
    
      if(status2 == 0 )
      { //no runtime error
          const char *command3 = "/usr/bin/diff expected_output.txt out_gen.txt > diff_out.txt";
          int status3 = system(command3);
          if(status3 == 0 )
          {
              //PASS both files are same for diff then returns 0
              int fd_out = open("out_gen.txt", O_RDWR);
              char out_buffer[1024];
              int out_bytesread = read(fd_out, out_buffer, 1024);
              lseek(fd, 0, SEEK_SET);
              ftruncate(fd, 0);
              write(fd_out, "\nPASS : ", strlen("\nPASS : "));
              write(fd_out, out_buffer, out_bytesread );
              close(fd_out);
              fd_out = open("out_gen.txt", O_RDONLY);
              out_bytesread = read(fd_out, out_buffer, 1024);
              write(newsockfd, out_buffer, out_bytesread);
              close(fd_out);
          }
          else
          {
              //OUTPUT ERROR i.e diff returns non zero value
              int fd_out = open("diff_out.txt", O_RDWR);
              char out_buffer[1024];
              int out_bytesread = read(fd_out, out_buffer, 1024);
              lseek(fd, 0, SEEK_SET);
              ftruncate(fd, 0);
              write(fd_out, "\nOUTPUT ERROR :\n", strlen("\nOUTPUT ERROR :\n"));
              write(fd_out, out_buffer, out_bytesread );
              close(fd_out);
              fd_out = open("diff_out.txt", O_RDONLY);
              out_bytesread = read(fd_out, out_buffer, 1024);
              write(newsockfd, out_buffer, out_bytesread);
              close(fd_out);
          }
      }
      else
      {   
          //IF RUNTIME ERROR
          int fd_out = open("error_output.txt", O_RDWR);
          char out_buffer[1024];
          int out_bytesread = read(fd_out, out_buffer, 1024);
          lseek(fd, 0, SEEK_SET);
          ftruncate(fd, 0);
          write(fd_out, "\nRUNTIME ERROR :\n ", strlen("\nRUNTIME ERROR :\n "));
          write(fd_out, out_buffer, out_bytesread );
          close(fd_out);
          fd_out = open("error_output.txt", O_RDONLY);
          out_bytesread = read(fd_out, out_buffer, 1024);
          write(newsockfd, out_buffer, out_bytesread);
          close(fd_out);
      }
   } 
   else
   {
    	//IF COMPILATION ERROR
        int fd_out = open("error_output.txt", O_RDWR);
        char out_buffer[1024];
        int out_bytesread = read(fd_out, out_buffer, 1024);
        lseek(fd, 0, SEEK_SET);
        ftruncate(fd, 0);
        write(fd_out, "\nCOMPILATION ERROR :\n ", strlen("\nCOMPILATION ERROR :\n "));
        write(fd_out, out_buffer, out_bytesread );
        close(fd_out);
        fd_out = open("error_output.txt", O_RDONLY);
        out_bytesread = read(fd_out, out_buffer, 1024);
        write(newsockfd, out_buffer, out_bytesread);
        close(fd_out);
   }
  
 
  
  sleep(2);
  if (n < 0)
    error("ERROR writing to socket");

  if(n==0)
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    }
  return 0;
}
