// server.h

#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <queue>
#include <map>
#include <string>
#include <pthread.h>
using namespace std;

extern pthread_mutex_t file_mutex;
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t queue_cond;
extern std::queue<std::string> request_queue;

void error(const char *msg);

std::string store_users_file(std::string client_token);

void storeKeys(std::string location, const std::string in_progress, const std::string q_pos, const std::string is_completed,const std::string compiler_err, const std::string runtime_err, const std::string output_err, const std::string pass);

std::string generateUniqueName(int id, long int timestamp);

void *handle_client_req(std::string client_token);

int check_user_status(std::string client_token, std::string key);

void *thread_function(void *arg);

#endif // SERVER_H

