#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

/////////////////////////////
#include <iostream>
#include <assert.h>
#include <tuple>
using namespace std;
/////////////////////////////

//Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef long long LL;

#define pb push_back
#define debug(x) cout << #x << " : " << x << endl
#define part cout << "-----------------------------------" << endl;

///////////////////////////////
#define MAX_CLIENTS 4
#define PORT_ARG 8002

const int initial_msg_len = 256;

////////////////////////////////////

const LL buff_sz = 1048576;

pair < string,int> map_val[105]; 

queue <int> qu;
int spawn_check_var = 0;
pthread_mutex_t spawn_check = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t qu_handler = PTHREAD_MUTEX_INITIALIZER;
pthread_t worker_threads[105];
pthread_mutex_t key_handler[105];
sem_t worker_thread_sem;
///////////////////////////////////////////////////
pair<string, int> read_string_from_socket(const int &fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    // cout<<"Output = "<<output<<'\n';
    debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)
{
    // debug(s.length());
    int bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}

void* handle_connection(void *)
{
   
    while(1)
    {
        vector <string> vec;
        sem_wait(&worker_thread_sem);
        int received_num, sent_num;
        int ret_val = 1;
        pthread_mutex_lock(&qu_handler);
        if(qu.empty())
        {
            pthread_mutex_unlock(&qu_handler);
            continue;
        }
        int client_socket_fd = qu.front();
        qu.pop();
        pthread_mutex_unlock(&qu_handler);
        string cmd;
        tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
        ret_val = received_num;

        if (ret_val <= 0)
        {
            printf("Server could not read msg sent from client\n");
            close(client_socket_fd);
            return NULL;
        }
        string tempr = "";
        for(int i=0;i<cmd.length();i++)
        {
            if(cmd[i]==' ')
            {
                if(tempr.size())
                vec.pb(tempr);
                tempr="";
                continue;
            }
            tempr+=cmd[i];
        }
        if(tempr.size())
        {
            vec.pb(tempr);
        }
        // for(int i=0;i<vec.size();i++)
        // {
        //     cout<<"Check\n";
        //     cout<<"vec = "<<vec[i]<<'\n';
        // }
        string msg_to_send_back="";
        uint a = pthread_self();
        string thread_name = to_string(a);
        thread_name+=":";
        msg_to_send_back+=thread_name;
        if(vec[0]=="insert")
        {
            int num1 = atoi(vec[1].c_str());
            pthread_mutex_lock(&key_handler[num1]);
            if(map_val[num1].second==0)
            {
                msg_to_send_back+="Insertion Sucessful";
                map_val[num1].first = vec[2];
                map_val[num1].second = 1;
            }
            else
            {
                msg_to_send_back+="Key already exists";
            }
            pthread_mutex_unlock(&key_handler[num1]);
        }
        else if (vec[0]=="delete")
        {
            int num1 = atoi(vec[1].c_str());
            pthread_mutex_lock(&key_handler[num1]);
            if(map_val[num1].second==1)
            {
                msg_to_send_back+="Deletion successful";
                map_val[num1].first = "";
                map_val[num1].second = 0;
            }
            else
            {
                msg_to_send_back+="No such key exists";
            }
            pthread_mutex_unlock(&key_handler[num1]);
        }
        else if(vec[0]=="update")
        {
            int num1 = atoi(vec[1].c_str());
            pthread_mutex_lock(&key_handler[num1]);
            if(map_val[num1].second==1)
            {
                msg_to_send_back+=vec[2];
                map_val[num1].first = vec[2];
            }
            else
            {
                msg_to_send_back+="Key does not exist";
            }
            pthread_mutex_unlock(&key_handler[num1]);
        }
        else if(vec[0]=="concat")
        {
            int num1 = atoi(vec[1].c_str());
            int num2 = atoi(vec[2].c_str());
            int key1,key2,smol,larg;
            bool flag = 0;
            smol = min(num1,num2);
            larg = max(num1,num2);
            pthread_mutex_lock(&key_handler[smol]);
            pthread_mutex_lock(&key_handler[larg]);
            if(map_val[num1].second==0)
            {
                flag=1;
            }
            if(map_val[num2].second==0)
            {
                flag=1;
            }
            if(flag)
            {
                msg_to_send_back+="Concat failed as at least one of the keys does not exist";
            }
            else
            {
                string str1,str2;
                str1 = map_val[num1].first;
                str2 = map_val[num2].first;
                map_val[num1].first = str1+str2;
                map_val[num2].first = str2+str1;
                msg_to_send_back+=map_val[num2].first;
            }
            pthread_mutex_unlock(&key_handler[larg]);
            pthread_mutex_unlock(&key_handler[smol]);
            
        }
        else if(vec[0]=="fetch")
        {
            int num1 = atoi(vec[1].c_str());
            pthread_mutex_lock(&key_handler[num1]);
            if(map_val[num1].second==1)
            {
                msg_to_send_back+=map_val[num1].first;
            }
            else
            {
                msg_to_send_back+="Key does not exist";
            }
            pthread_mutex_unlock(&key_handler[num1]);
        }
        else
        {
            msg_to_send_back+="Invalid request";
        }
        cout << "Client sent : " << cmd << endl;
        // msg_to_send_back+='\n';
        // if(cmd==)


        
        vec.clear();
        sleep(2);
        int sent_to_client = send_string_on_socket(client_socket_fd, msg_to_send_back);
        if (sent_to_client == -1)
        {
            perror("Error while writing to client. Seems socket has been closed");
        }
        close(client_socket_fd);


    }
    return NULL;

}

int main(int argc, char *argv[])
{

    int i, j, k, t, n,m;
    cin>>m;
    sem_init(&worker_thread_sem, 0, 0);
    for(int i=0;i<=100;i++)
    {
        key_handler[i] = PTHREAD_MUTEX_INITIALIZER; //Initializing mutex to handle map
        map_val[i] = {"",0};
    }
    for(int i=0;i<m;i++)
    {
        pthread_create(&worker_threads[i],NULL,handle_connection,NULL);
    }
    sleep(5);
    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;

    struct sockaddr_in serv_addr_obj, client_addr_obj;
    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_number = PORT_ARG;
    serv_addr_obj.sin_family = AF_INET;
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number);
    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    listen(wel_socket_fd, MAX_CLIENTS);
    cout << "Server has started listening on the LISTEN PORT" << endl;
    clilen = sizeof(client_addr_obj);
    while (1)
    {

        printf("Waiting for a new client to request for a connection\n");
        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        pthread_mutex_lock(&qu_handler);
            if (client_socket_fd < 0)
            {
                perror("ERROR while accept() system call occurred in SERVER");
                exit(-1);
            }
            printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));
        qu.push(client_socket_fd);
        sem_post(&worker_thread_sem);
        pthread_mutex_unlock(&qu_handler);
        // handle_connection();
    }

    close(wel_socket_fd);
    return 0;
}