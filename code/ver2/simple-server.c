/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>       
#include <sys/stat.h>       
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>

pthread_mutex_t file_mutex;

void error(char *msg) {
    perror(msg);
    exit(1);
}

void *handle_client_req(void *arg) {
    int newsockfd = *((int *)arg);
    int n;
    char buffer[1024];

    while (1) {
        bzero(buffer, 1024); // set buffer to zero

        int fd = open("c_code_server.c", O_WRONLY | O_TRUNC | O_CREAT, 0666);
        n = read(newsockfd, buffer, 1024);

        if (n < 0){
        printf("ERROR reading from socket");
        continue;
        }

        if (n == 0){
        printf("Nothing received from client. Closing socket. Exiting Thread, id");
        close(newsockfd);
        return NULL;
     }

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
        if (status1 == 0) {
            // if no compiler error
            const char *command2 = "./c_code_server > out_gen.txt";
            int status2 = system(command2);

            if (status2 == 0) {
                // no runtime error
                const char *command3 = "/usr/bin/diff expected_output.txt out_gen.txt > diff_out.txt";
                int status3 = system(command3);
            
                if (status3 == 0) {
                    // PASS both files are the same for diff then return 0
                    int fd_out = open("out_gen.txt", O_RDWR);
                    char out_buffer[1024];
                    int out_bytesread = read(fd_out, out_buffer, 1024);
                    lseek(fd, 0, SEEK_SET);
                    ftruncate(fd, 0);
                    write(fd_out, "\nPASS : ", strlen("\nPASS : "));
                    write(fd_out, out_buffer, out_bytesread);
                    close(fd_out);
                    fd_out = open("out_gen.txt", O_RDONLY);
                    out_bytesread = read(fd_out, out_buffer, 1024);
                    write(newsockfd, out_buffer, out_bytesread);
                    close(fd_out);
                } else {
                    // OUTPUT ERROR i.e. diff returns a non-zero value
                    int fd_out = open("diff_out.txt", O_RDWR);
                    char out_buffer[1024];
                    int out_bytesread = read(fd_out, out_buffer, 1024);
                    lseek(fd, 0, SEEK_SET);
                    ftruncate(fd, 0);
                    write(fd_out, "\nOUTPUT ERROR :\n", strlen("\nOUTPUT ERROR :\n"));
                    write(fd_out, out_buffer, out_bytesread);
                    close(fd_out);
                    fd_out = open("diff_out.txt", O_RDONLY);
                    out_bytesread = read(fd_out, out_buffer, 1024);
                    write(newsockfd, out_buffer, out_bytesread);
                    close(fd_out);
                }
            } else {
                // IF RUNTIME ERROR
                int fd_out = open("error_output.txt", O_RDWR);
                char out_buffer[1024];
                int out_bytesread = read(fd_out, out_buffer, 1024);
                lseek(fd, 0, SEEK_SET);
                ftruncate(fd, 0);
                write(fd_out, "\nRUNTIME ERROR :\n ", strlen("\nRUNTIME ERROR :\n "));
                write(fd_out, out_buffer, out_bytesread);
                close(fd_out);
                fd_out = open("error_output.txt", O_RDONLY);
                out_bytesread = read(fd_out, out_buffer, 1024);
                write(newsockfd, out_buffer, out_bytesread);
                close(fd_out);
            }
        } else {
            // IF COMPILATION ERROR
            int fd_out = open("error_output.txt", O_RDWR);
            char out_buffer[1024];
            int out_bytesread = read(fd_out, out_buffer, 1024);
            lseek(fd, 0, SEEK_SET);
            ftruncate(fd, 0);
            write(fd_out, "\nCOMPILATION ERROR :\n ", strlen("\nCOMPILATION ERROR :\n "));
            write(fd_out, out_buffer, out_bytesread);
            close(fd_out);
            fd_out = open("error_output.txt", O_RDONLY);
            out_bytesread = read(fd_out, out_buffer, 1024);
            write(newsockfd, out_buffer, out_bytesread);
            close(fd_out);
        }
        
        if (n < 0)
            error("ERROR writing to socket");

        close(newsockfd);
        pthread_exit(NULL);
    }
}

int main(int argc, char *argv[]) {
    int sockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t t_id;
    int *newsockfd = (int *)malloc(sizeof(int));
    pthread_mutex_init(&file_mutex, NULL);

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    /* create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = atoi(argv[1]);
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    /* listen for incoming connection requests */
    listen(sockfd, 10);
    clilen = sizeof(cli_addr);

    while (1) {
      
    
        *newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (*newsockfd < 0) {
            free(newsockfd);
            error("ERROR on accept");
        }

  
        if (pthread_create(&t_id, NULL, &handle_client_req, (void *)newsockfd) != 0) {
            perror("pthread_create");
            free(newsockfd);
            exit(1);
        }

        write(1,"Thread exit",12);
       pthread_join(t_id, NULL);

    }

    return 0;
}
