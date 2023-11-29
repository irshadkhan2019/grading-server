// server.cpp

#include "server.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <queue>
#include <cstring>
#include <ctime>
#include <sstream>
#include <fstream>
#include <map>
using namespace std;

pthread_mutex_t file_mutex;
pthread_mutex_t queue_mutex;
pthread_cond_t queue_cond;
std::queue<std::string> request_queue;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

std::string generateUniqueName(int id, long int timestamp) {
    std::ostringstream oss;
    oss << timestamp << "_" << id;
    return oss.str();
}

void storeKeys(std::string location, const std::string in_progress = "1", const std::string q_pos = "0", const std::string is_completed = "0", const std::string compiler_err = "0", const std::string runtime_err = "0", const std::string output_err = "0", const std::string pass = "0") {
    std::map<std::string, std::string> keyValuePairs;
    keyValuePairs["q_pos"] = q_pos;
    keyValuePairs["in_progress"] = in_progress;
    keyValuePairs["is_completed"] = is_completed;
    keyValuePairs["compiler_err"] = compiler_err;
    keyValuePairs["runtime_err"] = runtime_err;
    keyValuePairs["output_err"] = output_err;
    keyValuePairs["pass"] = pass;

    std::ofstream file(location);

    if (file.is_open()) {
        for (const auto &entry : keyValuePairs) {
            file << entry.first << "=" << entry.second << "\n";
        }
        file.close();
        std::cout << "Key-value pairs written to file.\n";
    } else {
        std::cerr << "Unable to open the file.\n";
    }
}

std::string store_users_file(std::string client_token) {
    std::string client_files_dir = "./Submissions/" + client_token + "/";
    std::string program_file = client_files_dir + "file" + client_token + ".c";
    std::string make_client_dir = "mkdir ./Submissions/" + client_token;
    system(make_client_dir.c_str());
    return program_file;
}

int check_user_status(std::string client_token, std::string key) {
    std::string client_files_dir = "./Submissions/" + client_token + "/";
    std::string status_file = client_files_dir + "status.txt";

    FILE *file = fopen(status_file.c_str(), "r");

    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    char line[100];
    while (fgets(line, sizeof(line), file) != NULL) {
        char *token = strtok(line, "=");

        if (token != NULL && strcmp(token, key.c_str()) == 0) {
            token = strtok(NULL, "=");

            if (token != NULL) {
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

    
    string client_files_dir="./Submissions/"+client_token+"/";


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
    std::string client_token;
    while (true) {
        pthread_mutex_lock(&queue_mutex);
        while (request_queue.empty()) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        client_token = request_queue.front();
        request_queue.pop();
        pthread_mutex_unlock(&queue_mutex);
        handle_client_req(client_token);
    }
}

