// main.cpp

#include "server.h"

#include <iostream>
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
#include <map>
using namespace std;


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
        time_t now = time(nullptr);

        if (*newsockfd < 0) {
            free(newsockfd);
            error("ERROR on accept");
        }


        pthread_mutex_lock(&queue_mutex);
        printf("Pushing request %d conn %d\n", req,*newsockfd); 
        req++;
        
     

        // read data
        // close lient conection and send him response
        string client_token = generateUniqueName(*newsockfd, now); 

        // save his program
        string program_file=store_users_file(client_token);

        cout<<program_file;
        char userbuffer[1024];
        bzero(userbuffer,1024);

        int fd = open(program_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
        int n = read(*newsockfd, userbuffer, 1024);

        cout<<userbuffer;
        cout<<"FILE DESCRIPTOR"<<fd;
        cout<<"write::"<<write(fd, userbuffer, n);
        close(fd);  

 
        // send him msg
        string msg="Your request have been submitted .Use this id to check status:"+client_token+"\n";
        write(*newsockfd, msg.c_str(), 100);

        char status[50];
        bzero(status,50);
        n=read(*newsockfd,status,sizeof(status));
        cout<<"STATUS SEND BY CLIENT"<<status<<endl;

        if(strcmp(status, "new") == 0){
            request_queue.push(client_token.c_str());      
            pthread_cond_signal(&queue_cond);
            pthread_mutex_unlock(&queue_mutex);
            close(*newsockfd);
        }
        else{
            cout<<"STATUS Other and TOken"<<status<<endl;  
            int in_progress = check_user_status(status,"in_progress");
            int is_completed = check_user_status(status,"is_completed");
            int pass = check_user_status(status,"pass");
            int compiler_err = check_user_status(status,"compiler_err");
            int runtime_err = check_user_status(status,"runtime_err");
            int output_err = check_user_status(status,"output_err");

            cout<<compiler_err<<in_progress<<is_completed<<output_err<<pass<<runtime_err<<endl;

            if(is_completed && pass){
                string client_files_dir="./Submissions/"+string(status)+"/";
                string output_file =client_files_dir+"out" + status + ".txt";

                    int fd_out = open(output_file.c_str(), O_RDWR);
                    char out_buffer[1024];
                    int out_bytesread = read(fd_out, out_buffer, 1024);
                    lseek(fd_out, 0, SEEK_SET);
                    ftruncate(fd_out, 0);
                    write(fd_out, "\nPASS : ", strlen("\nPASS : "));
                    write(fd_out, out_buffer, out_bytesread);
                    close(fd_out);
                    fd_out = open(output_file.c_str(), O_RDONLY);
                    out_bytesread = read(fd_out, out_buffer, 1024);
                    write(*newsockfd, out_buffer, out_bytesread);
                    close(fd_out);
            }
            else if(is_completed && compiler_err){
                string client_files_dir="./Submissions/"+string(status)+"/";
                string error_output_file =client_files_dir+"err" + status + ".txt";

                int fd_out = open(error_output_file.c_str(), O_RDWR);
                char out_buffer[1024];
                int out_bytesread = read(fd_out, out_buffer, 1024);
                lseek(fd_out, 0, SEEK_SET);
                ftruncate(fd_out, 0);
                write(fd_out, "\nCOMPILATION ERROR :\n ", strlen("\nCOMPILATION ERROR :\n "));
                write(fd_out, out_buffer, out_bytesread);
                close(fd_out);
                fd_out = open(error_output_file.c_str(), O_RDONLY);
                out_bytesread = read(fd_out, out_buffer, 1024);
                write(*newsockfd, out_buffer, out_bytesread);
                close(fd_out);
            }
            else if(is_completed && runtime_err){
                string client_files_dir="./Submissions/"+string(status)+"/";
                string error_output_file =client_files_dir+"err" + status + ".txt";

                int fd_out = open(error_output_file.c_str(), O_RDWR);
                char out_buffer[1024];
                int out_bytesread = read(fd_out, out_buffer, 1024);
                lseek(fd_out, 0, SEEK_SET);
                ftruncate(fd_out, 0);
                write(fd_out, "\nRUNTIME ERROR :\n ", strlen("\nRUNTIME ERROR :\n "));
                write(fd_out, out_buffer, out_bytesread);
                close(fd_out);
                fd_out = open(error_output_file.c_str(), O_RDONLY);
                out_bytesread = read(fd_out, out_buffer, 1024);
                write(*newsockfd, out_buffer, out_bytesread);
                close(fd_out);
            }
            else if ( is_completed && output_err)
            {
                string client_files_dir="./Submissions/"+string(status)+"/";
                string diff_output_file =client_files_dir+"diff" + status + ".txt";

                int fd_out = open(diff_output_file.c_str(), O_RDWR);
                char out_buffer[1024];
                int out_bytesread = read(fd_out, out_buffer, 1024);
                lseek(fd_out, 0, SEEK_SET);
                ftruncate(fd_out, 0);
                write(fd_out, "\nOUTPUT ERROR :\n", strlen("\nOUTPUT ERROR :\n"));
                write(fd_out, out_buffer, out_bytesread);
                close(fd_out);
                fd_out = open(diff_output_file.c_str(), O_RDONLY);
                out_bytesread = read(fd_out, out_buffer, 1024);
                write(*newsockfd, out_buffer, out_bytesread);
                close(fd_out);
            }
            pthread_mutex_unlock(&queue_mutex);
            close(*newsockfd);
        }
      
     
    }

    return 0;
}

