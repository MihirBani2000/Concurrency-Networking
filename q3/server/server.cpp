#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
// #include <sys/stat.h>
#include <netinet/in.h>
// #include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// #include <fcntl.h>
#include <iostream>
// #include <assert.h>
#include <tuple>
#include <queue>

using namespace std;
typedef long long LL;

//Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"
#define debug(x) cout << #x << " : " << x << endl
#define part cout << "-----------------------------------" << endl;

#define MAX_CLIENTS 10
#define SERVERPORT 8001
#define BUFFER_SIZE 1048576
#define MAX_THREADS 20
#define MAX_KEYS 101
#define ARG_LIMIT 3

// const int initial_msg_len = 256;
int thread_num;
pthread_t thread_pool[MAX_THREADS];
pthread_mutex_t q_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dic_mutex[MAX_KEYS];
pthread_cond_t q_condvar = PTHREAD_COND_INITIALIZER;
string dictionary[MAX_KEYS];
queue<int *> my_q;

// function declarations
void perr_exit(const string &msg);
void cout_exit(const string &msg);
pair<string, int> read_string_from_socket(const int &fd, int bytes);
int send_string_on_socket(int fd, const string &s);
void *handle_connection(void *client_fd_ptr);
void *handle_thread_pool(void *args);
int tokenize_inputs(char *input_string, string commands[]);
string handle_requests(string command[], int argc);
string handle_insert(string command[], int argc);
string handle_delete(string command[], int argc);
string handle_update(string command[], int argc);
string handle_concat(string command[], int argc);
string handle_fetch(string command[], int argc);

int tokenize_inputs(char *input_string, string commands[])
{
    // char *delim = "\t ";
    char *temp_in = strtok(input_string, "\t ");
    int cnt = 0;
    while (temp_in != NULL)
    {
        // printf("inside tokenize_inputs %s \n", temp_in);
        commands[cnt++] = temp_in;
        temp_in = strtok(NULL, "\t ");
    }
    return cnt;
}

string handle_requests(string command[], int argc)
{
    string output = "Invalid commands by the user.";
    if ((argc != 2) && (argc != 3))
    {
        cout << "HIII" << argc << endl;
        return output;
    }

    int key = atoi(command[1].c_str());
    if ((key < 0) || (key > 100))
    {
        output = "Invalid commands by the user. Key must be in range [0,100]";
        return output;
    }

    if (argc == 2)
    {
        if (command[0] == "delete")
            output = handle_delete(command, argc);
        else if (command[0] == "fetch")
            output = handle_fetch(command, argc);
        else if (command[0] == "concat")
            output = handle_concat(command, argc);
    }
    else
    {
        if (command[0] == "insert")
            output = handle_insert(command, argc);
        else if (command[0] == "update")
            output = handle_update(command, argc);
        else if (command[0] == "concat")
            output = handle_concat(command, argc);
    }

    return output;
}

string handle_insert(string command[], int argc)
{
    string output;
    int key = atoi(command[1].c_str());
    pthread_mutex_lock(&dic_mutex[key]);
    string value = command[2];
    if (dictionary[key] == "\0") // new key added
    {
        dictionary[key] = value;
        output = "Insertion successful";
    }
    else // key already present
        output = "Key already exists";
    pthread_mutex_unlock(&dic_mutex[key]);
    return output;
}

string handle_delete(string command[], int argc)
{
    string output;
    int key = atoi(command[1].c_str());
    pthread_mutex_lock(&dic_mutex[key]);
    if (dictionary[key] == "\0") // no key present
        output = "No such key exists";
    else // key already present
    {
        dictionary[key] = "\0";
        output = "Deletion successful";
    }
    pthread_mutex_unlock(&dic_mutex[key]);
    return output;
}

string handle_update(string command[], int argc)
{
    string output;
    int key = atoi(command[1].c_str());
    pthread_mutex_lock(&dic_mutex[key]);
    string value = command[2];
    if (dictionary[key] == "\0") // no key present
        output = "Key does not exist";
    else // key already present
    {
        dictionary[key] = value;
        output = value;
    }
    pthread_mutex_unlock(&dic_mutex[key]);
    return output;
}

string handle_concat(string command[], int argc)
{
    string output;
    int key1 = atoi(command[1].c_str()), key2;
    pthread_mutex_lock(&dic_mutex[key1]);
    if (argc == 2)
        key2 = key1;
    else
    {
        key2 = atoi(command[2].c_str());
        pthread_mutex_lock(&dic_mutex[key2]);
    }

    if ((dictionary[key1] == "\0") || (dictionary[key2] == "\0")) // no keys present
        output = "Concat failed as at least one of the keys does not exist";
    else // keys present
    {
        string temp1 = dictionary[key1], temp2 = dictionary[key2];
        dictionary[key1] += temp2;
        if (key1 != key2)
            dictionary[key2] += temp1;
        output = dictionary[key2];
    }

    pthread_mutex_unlock(&dic_mutex[key1]);
    if (key1 != key2)
        pthread_mutex_unlock(&dic_mutex[key2]);

    return output;
}

string handle_fetch(string command[], int argc)
{
    string output;
    int key = atoi(command[1].c_str());
    pthread_mutex_lock(&dic_mutex[key]);
    if (dictionary[key] == "\0") // no key present
        output = "Key does not exist";
    else // key already present
        output = dictionary[key];
    pthread_mutex_unlock(&dic_mutex[key]);
    return output;
}

void *handle_connection(void *client_fd_ptr)
{
    int client_fd = *((int *)client_fd_ptr);
    string command[ARG_LIMIT];
    int argc, received_num = -1;

    while (true)
    {
        string recv_msg, send_msg;
        tie(recv_msg, received_num) = read_string_from_socket(client_fd, BUFFER_SIZE);
        // debug(ret_val);
        // printf("Read something\n");
        if (received_num <= 0)
        {
            // perror("Error read()");
            printf("Server could not read msg sent from client\n");
            break;
        }
        cout << "Client sent : " << recv_msg << endl;
        if (recv_msg == "exit")
        {
            cout << "Exit pressed by client" << endl;
            break;
        }
        argc = tokenize_inputs(&recv_msg[0], command);
        send_msg = handle_requests(command, argc);

        int sent_to_client = send_string_on_socket(client_fd, send_msg);
        // debug(sent_to_client);
        if (sent_to_client == EXIT_FAILURE)
        {
            perror("Error while writing to client. Seems socket has been closed");
            break;
        }
    }

    close(client_fd);
    printf(BRED "Disconnected from client" ANSI_RESET "\n");
    return NULL;
}

void *handle_thread_pool(void *args)
{
    while (1)
    {
        pthread_mutex_lock(&q_mutex);
        while (my_q.empty())
        {
            pthread_cond_wait(&q_condvar, &q_mutex);
        }
        int *client_fd = my_q.front();
        my_q.pop();
        pthread_mutex_unlock(&q_mutex);
        handle_connection(client_fd);
    }
    return NULL;
}

int main(int argc, char *argv[])
{

    if (argc != 2)
        cout_exit("Insufficient arguments. Mention the number of threads.");

    thread_num = atoi(argv[1]);
    if (thread_num < 1)
        cout_exit("Enter valid number of threads in pool.");

    int listen_fd, client_fd, port_num;
    socklen_t client_len;
    struct sockaddr_in serv_addr_obj, client_addr_obj;

    // initiating the thread pool
    for (int i = 0; i < thread_num; i++)
    {
        pthread_create(&thread_pool[i], NULL, handle_thread_pool, NULL);
    }

    // initializing the dictionary and mutex
    for (int i = 0; i < MAX_KEYS; i++)
    {
        dictionary[i] = "\0";
        if (pthread_mutex_init(&dic_mutex[i], NULL))
        {
            perr_exit("mutex error: unable to initialize mutex lock");
        }
    }

    // creating a socket that welcomes some initial contact
    // from a client process running on an arbitrary host
    // get listening socket
    // get ip, port
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perr_exit("ERROR creating listening socket");

    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_num = SERVERPORT;
    serv_addr_obj.sin_family = AF_INET;
    serv_addr_obj.sin_addr.s_addr = htonl(INADDR_ANY); // server: INADDR_ANY will bind the port to all available interfaces.
    serv_addr_obj.sin_port = htons(port_num);          // process specifies port

    /* bind socket to this port number on this machine */
    /* When a socket is created with socket(2), it exists in a name space
       (address family) but has no address assigned to it.  bind() assigns
       the address specified by addr to the socket referred to by the file
       descriptor wel_sock_fd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  */
    if (bind(listen_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
        perr_exit("bind: error on binding listen socket");

    /* listen for incoming connection requests */
    if ((listen(listen_fd, MAX_CLIENTS)) < 0)
        perr_exit("listen: error on listening socket");

    cout << "Server has started listening on the LISTEN PORT" << endl;
    client_len = sizeof(client_addr_obj);

    while (1)
    {
        // accept a new request, create a client_fd
        // During the three-way handshake, the client process knocks on the welcoming door
        // of the server process. When the server “hears” the knocking, it creates a new door—
        // more precisely, a new socket that is dedicated to that particular client.
        // accept is a blocking call
        printf("Waiting for a new client to request for a connection at port %d\n", port_num);

        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr_obj, &client_len);
        if (client_fd < 0)
            perr_exit("accept: ERROR occurred in SERVER");
        // *client_fd_ptr = client_fd;

        printf(BGRN "New client connected from port number %d and IP %s\n" ANSI_RESET,
               ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));

        pthread_mutex_lock(&q_mutex);
        my_q.push(&client_fd);
        pthread_cond_signal(&q_condvar);
        pthread_mutex_unlock(&q_mutex);
    }

    close(listen_fd);
    return 0;
}

// functions
void perr_exit(const string &msg)
{
    perror(msg.c_str());
    exit(EXIT_FAILURE);
}

void cout_exit(const string &msg)
{
    cout << msg << endl;
    exit(EXIT_FAILURE);
}

pair<string, int> read_string_from_socket(const int &fd, int bytes)
{
    string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    // debug(output);
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