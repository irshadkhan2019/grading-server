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
#include <stdbool.h>
#include <queue>
#include <cstring>
#include <ctime>
#include <sstream>
#include <fstream>
using namespace std; 

pthread_mutex_t queue_mutex;
pthread_mutex_t file_mutex;
pthread_cond_t queue_cond;

std::queue<int> request_queue;

std::string generateUniqueName(int id, long int timestamp) 
{
    // Concatenate thread ID and timestamp to create a unique ID
    std::ostringstream oss;
    oss << timestamp << "_" << id;

    return oss.str();
}

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void *handle_client_req(void *arg) 
{
    int newsockfd = *((int *)arg);
    int n;
    char buffer[1024];
    
    int thread_id = pthread_self();
    time_t now = time(nullptr);

    string unique_id = generateUniqueName(thread_id, now); 
    
    string program_file = "file" + unique_id + ".c";
    string error_output_file = "err" + unique_id + ".txt";
    string executable = "exec" + unique_id ; 
    string diff_output_file = "diff" + unique_id + ".txt";
    string output_file = "out" + unique_id + ".txt";

    string compile_cmd = "g++ -o " + executable + " " + program_file ;
    string run_cmd = "./" + executable + " > " + output_file; 
    string diff_cmd = "diff expected_output.txt " + output_file + " > " + diff_output_file;

    write(1,"Serving request",15);
    bzero(buffer, 1024); // set buffer to zero

    int fd = open(program_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
    n = read(newsockfd, buffer, 1024);

    if (n == 0)
    {
        printf("Nothing received from client. Closing socket. Exiting Thread, id");
        close(newsockfd);
        return NULL;
     }

        write(fd, buffer, n);

        close(fd);       

        int stderr_fd = open(error_output_file.c_str(), O_WRONLY | O_CREAT, 0666);
        if (stderr_fd < 0)
            error("ERROR opening error_output.txt");

        // Redirect stderr to the file
        if (dup2(stderr_fd, STDERR_FILENO) == -1)
            error("ERROR redirecting stderr");

        // Close the original stderr file descriptor
        close(stderr_fd);

        if (n < 0)
            error("ERROR reading from socket");

        int compile_status = system(compile_cmd.c_str());
        if (compile_status == 0) {
            // if no compiler error
            int run_status = system(run_cmd.c_str());

            if (run_status == 0) {
                // no runtime error
                int diff_status = system(diff_cmd.c_str() );
            
                if (diff_status == 0) {
                    // PASS both files are the same for diff then return 0
                    int fd_out = open(output_file.c_str(), O_RDWR);
                    char out_buffer[1024];
                    int out_bytesread = read(fd_out, out_buffer, 1024);
                    lseek(fd, 0, SEEK_SET);
                    ftruncate(fd, 0);
                    write(fd_out, "\nPASS : ", strlen("\nPASS : "));
                    write(fd_out, out_buffer, out_bytesread);
                    close(fd_out);
                    fd_out = open(output_file.c_str(), O_RDONLY);
                    out_bytesread = read(fd_out, out_buffer, 1024);
                    write(newsockfd, out_buffer, out_bytesread);
                    close(fd_out);
                } else {
                    // OUTPUT ERROR i.e. diff returns a non-zero value
                    int fd_out = open(diff_output_file.c_str(), O_RDWR);
                    char out_buffer[1024];
                    int out_bytesread = read(fd_out, out_buffer, 1024);
                    lseek(fd, 0, SEEK_SET);
                    ftruncate(fd, 0);
                    write(fd_out, "\nOUTPUT ERROR :\n", strlen("\nOUTPUT ERROR :\n"));
                    write(fd_out, out_buffer, out_bytesread);
                    close(fd_out);
                    fd_out = open(diff_output_file.c_str(), O_RDONLY);
                    out_bytesread = read(fd_out, out_buffer, 1024);
                    write(newsockfd, out_buffer, out_bytesread);
                    close(fd_out);
                }
            } else {
                // IF RUNTIME ERROR
                int fd_out = open(error_output_file.c_str(), O_RDWR);
                char out_buffer[1024];
                int out_bytesread = read(fd_out, out_buffer, 1024);
                lseek(fd, 0, SEEK_SET);
                ftruncate(fd, 0);
                write(fd_out, "\nRUNTIME ERROR :\n ", strlen("\nRUNTIME ERROR :\n "));
                write(fd_out, out_buffer, out_bytesread);
                close(fd_out);
                fd_out = open(error_output_file.c_str(), O_RDONLY);
                out_bytesread = read(fd_out, out_buffer, 1024);
                write(newsockfd, out_buffer, out_bytesread);
                close(fd_out);
            }
        } else {
            // IF COMPILATION ERROR
            int fd_out = open(error_output_file.c_str(), O_RDWR);
            char out_buffer[1024];
            int out_bytesread = read(fd_out, out_buffer, 1024);
            lseek(fd, 0, SEEK_SET);
            ftruncate(fd, 0);
            write(fd_out, "\nCOMPILATION ERROR :\n ", strlen("\nCOMPILATION ERROR :\n "));
            write(fd_out, out_buffer, out_bytesread);
            close(fd_out);
            fd_out = open(error_output_file.c_str(), O_RDONLY);
            out_bytesread = read(fd_out, out_buffer, 1024);
            write(newsockfd, out_buffer, out_bytesread);
            close(fd_out);
        }
        
        if (n < 0)
            error("ERROR writing to socket");

        write(1,"Served request",15);    
        close(newsockfd);
        write(1,"---------------",15);  
        
        return NULL;
}


void *thread_function(void *arg) {
    int thread_id = *((int *)arg);
    while (true) {
        int newsockfd;
        pthread_mutex_lock(&queue_mutex);
        while (request_queue.empty()) {
            printf("Thread %d is waiting for requests to come\n", thread_id);
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        newsockfd = request_queue.front();
        request_queue.pop();
        pthread_mutex_unlock(&queue_mutex);
        printf("Thread %d is processing a request no %d \n", thread_id,newsockfd);
        handle_client_req(&newsockfd);
    }
}

int main(int argc, char *argv[]) {
    int sockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t t_id;
  
    pthread_mutex_init(&file_mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <port> <thread_pool_size>\n", argv[0]);
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
    int thread_pool_size = atoi(argv[2]);
    serv_addr.sin_port = htons(portno);

    if (thread_pool_size < 1) {
        fprintf(stderr, "Thread pool size must be at least 1\n");
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    /* listen for incoming connection requests */
    listen(sockfd, 10);
    clilen = sizeof(cli_addr);

    // create thread pool

    pthread_t thread_pool[thread_pool_size];
    int thread_ids[thread_pool_size];

    for(int i=0;i< thread_pool_size;i++){
        thread_ids[i] = i; 
        printf("creating thread with id %d \n",thread_ids[i] ); 
        if (pthread_create(&thread_pool[i], NULL, thread_function, &thread_ids[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    int req=1;
    while (1) {
        
        write(1,"New conn",8);
        int *newsockfd = (int *)malloc(sizeof(int));
        *newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (*newsockfd < 0) {
            free(newsockfd);
            error("ERROR on accept");
        }


        pthread_mutex_lock(&queue_mutex);
        printf("Pushing request %d conn %d\n", req,*newsockfd); 
        req++;
        
        request_queue.push(*newsockfd);
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);


    }

    return 0;
}
