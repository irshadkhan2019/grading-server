/* run using ./server <port> */
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

pthread_mutex_t queue_mutex;
pthread_mutex_t file_mutex;
pthread_cond_t queue_cond;

std::queue<string> request_queue;

std::string generateUniqueName(int id, long int timestamp) 
{
    // Concatenate thread ID and timestamp to create a unique ID
    std::ostringstream oss;
    oss << timestamp << "_" << id;

    return oss.str();
}

void storeKeys(string location,const string in_progress = "1",const string q_pos = "0",const string is_completed = "0",
                const string compiler_err = "0",const string runtime_err = "0",const string output_err="0",const string pass = "0"
){
     std::map<std::string, std::string> keyValuePairs;

    // Add some key-value pairs to the map
    keyValuePairs["q_pos"] = q_pos;
    keyValuePairs["in_progress"] = in_progress;
    keyValuePairs["is_completed"] = is_completed;
    keyValuePairs["compiler_err"] = compiler_err;
    keyValuePairs["runtime_err"] = runtime_err;
    keyValuePairs["output_err"] = output_err;
    keyValuePairs["pass"] = pass;
   

    // Open a file for writing
    std::ofstream file(location);

    if (file.is_open()) {
        // Iterate through the map and write key-value pairs to the file
        for (const auto& entry : keyValuePairs) {
            file << entry.first << "=" << entry.second << "\n";
        }

        // Close the file
        file.close();

        std::cout << "Key-value pairs written to file.\n";
    } else {
        std::cerr << "Unable to open the file.\n";
    }
}


void error(const char *msg) {
    perror(msg);
    exit(1);
}

string store_users_file(string client_token){
     string client_files_dir="./Submissions/"+client_token+"/";
     string program_file = client_files_dir+"file" + client_token + ".c";
     string make_client_dir = "mkdir ./Submissions/"+client_token;
     system(make_client_dir.c_str());


     return program_file;
}

int check_user_status(string client_token,string key){
    string client_files_dir="./Submissions/"+client_token+"/";
    string status_file=client_files_dir+"status.txt";
    // return status_file;

    FILE *file = fopen(status_file.c_str(), "r");
    
    if (file == NULL) {
        perror("Error opening file");
        return -1; // Return -1 to indicate an error
    }

    char line[100]; // Adjust the size based on the expected length of lines in the file

    // Iterate through each line in the file
    while (fgets(line, sizeof(line), file) != NULL) {
        // Tokenize the line based on '='
        char *token = strtok(line, "=");

        // Check if the token is the key we are looking for
        if (token != NULL && strcmp(token, key.c_str()) == 0) {
            // Move to the value part
            token = strtok(NULL, "=");

            if (token != NULL) {
                // Convert the value to an integer and return it
                int value = atoi(token);
                fclose(file);
                return value;
            }
        }
    }

    fclose(file);

    return -1;
}

void *handle_client_req(string client_token) 
{
    // int newsockfd = *((int *)arg);
    cout<<"Serving by thread::"<<client_token<<endl;
    int n;
    char buffer[1024];
    
    int thread_id = pthread_self();
    time_t now = time(nullptr);
    // string unique_id = generateUniqueName(client_token, now); 
  
    // cout<<"client Token:"<<client_token<<endl;
    
    string client_files_dir="./Submissions/"+client_token+"/";
    // string make_client_dir = "mkdir ./Submissions/"+client_token;
    // system(make_client_dir.c_str());

    // string client_files_dir="./Submissions/"+client_token+"/";

    string create_status_file_command="touch "+client_files_dir+"status.txt";
    system(create_status_file_command.c_str());

    string status_file_location=client_files_dir+"status.txt";
    storeKeys(status_file_location.c_str());

    string program_file = client_files_dir+"file" + client_token + ".c";
    string error_output_file =client_files_dir+"err" + client_token + ".txt";
    string executable = client_files_dir+"exec" + client_token ; 
    string diff_output_file =client_files_dir+"diff" + client_token + ".txt";
    string output_file =client_files_dir+"out" + client_token + ".txt";

    cout<<"programFile"<<program_file<<endl;
    cout<<"output_file"<<output_file<<endl;

    string compile_cmd = "g++ -o " + executable + " " + program_file ;
    string run_cmd =  executable + " > " + output_file; 
    string diff_cmd = "diff expected_output.txt " + output_file + " > " + diff_output_file;

    cout<<"COMPILE COMMAND"<<compile_cmd<<endl;
    cout<<"RUN COMMAND"<<run_cmd<<endl;
    write(1,"Serving request",15);
    bzero(buffer, 1024); // set buffer to zero

    // int fd = open(program_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
   
   
        int stderr_fd = open(error_output_file.c_str(), O_WRONLY | O_CREAT, 0666);
        cout<<"STD_ERR"<<stderr_fd<<endl;
        if (stderr_fd < 0)
            error("ERROR opening error_output.txt");

        // Redirect stderr to the file
        if (dup2(stderr_fd, STDERR_FILENO) == -1)
            error("ERROR redirecting stderr");

        cout<<"STD_ERR2"<<stderr_fd<<endl;    
        // Close the original stderr file descriptor
        close(stderr_fd);

        if (n < 0)
            error("ERROR reading from socket");

         cout<<"compile_status 0"<<endl;  
        int compile_status = system(compile_cmd.c_str());
        cout<<"compile_status"<<compile_status<<endl;

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
                    lseek(fd_out, 0, SEEK_SET);
                    ftruncate(fd_out, 0);
                    write(fd_out, "\nPASS : ", strlen("\nPASS : "));
                    write(fd_out, out_buffer, out_bytesread);
                    close(fd_out);
                    fd_out = open(output_file.c_str(), O_RDONLY);
                    out_bytesread = read(fd_out, out_buffer, 1024);
                    // write(newsockfd, out_buffer, out_bytesread);
//string location,const string in_progress = "1",const string q_pos = "0",const string is_completed = "0",
               // const string compiler_err = "0",const string runtime_err = "0",const string pass = "0"
                    storeKeys(status_file_location.c_str(),"0","0","1","0","0","0","1");
                    close(fd_out);
                } else {
                    // OUTPUT ERROR i.e. diff returns a non-zero value
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
                    // write(newsockfd, out_buffer, out_bytesread);
                    //string location,const string in_progress = "1",const string q_pos = "0",const string is_completed = "0",
               // const string compiler_err = "0",const string runtime_err = "0",const string pass = "0"
                    storeKeys(status_file_location.c_str(),"0","0","1","0","0","1","0");
                    close(fd_out);
                }
            } else {
                // IF RUNTIME ERROR
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
                // write(newsockfd, out_buffer, out_bytesread);
                storeKeys(status_file_location.c_str(),"0","0","1","0","1","0","0");
                close(fd_out);
            }
        } else {
            // IF COMPILATION ERROR
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
            // write(newsockfd, out_buffer, out_bytesread);
            storeKeys(status_file_location.c_str(),"0","0","1","1","0","0","0");
            close(fd_out);
        }
        
        if (n < 0)
            error("ERROR writing to socket");

        write(1,"Served request",15);    
        // close(newsockfd);
        write(1,"---------------",15);  
        
        return NULL;
}


void *thread_function(void *arg) {
    string client_token;
    while (true) {
        pthread_mutex_lock(&queue_mutex);
        while (request_queue.empty()) {
            // printf("Thread %d is waiting for requests to come\n", thread_id);
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        client_token = request_queue.front();
        request_queue.pop();
        pthread_mutex_unlock(&queue_mutex);
        // printf("Thread %d is processing a request no %d \n", thread_id,newsockfd);
        cout<<"Client token in thread fn::"<<client_token;
        handle_client_req(client_token);
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
            int in_progress=check_user_status(status,"in_progress");
            int is_completed=check_user_status(status,"is_completed");
            int pass=check_user_status(status,"pass");
            int compiler_err=check_user_status(status,"compiler_err");
            int runtime_err=check_user_status(status,"runtime_err");
            int output_err=check_user_status(status,"output_err");

            cout<<compiler_err<<in_progress<<is_completed<<output_err<<pass<<runtime_err<<endl;
            // char my_response[50];
            // strcpy(my_response, response.c_str());
            // char buffer[20];  // Adjust the size based on your needs
            // bzero(buffer,20);
            // // Convert the integer to a string
            // sprintf(buffer, "%d", in_progress);
            // write(*newsockfd,buffer, sizeof(buffer));

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
