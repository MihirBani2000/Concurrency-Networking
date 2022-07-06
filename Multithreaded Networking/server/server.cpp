#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
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

#define MAX_CLIENTS 4
#define SERVERPORT 8001
// #define MAX_THREADS 100
#define MAX_KEYS 101
#define ARG_LIMIT 4

// const int initial_msg_len = 256;
const LL BUFFER_SIZE = 1048576;
pthread_mutex_t q_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dic_mutex[MAX_KEYS];
pthread_cond_t q_condvar = PTHREAD_COND_INITIALIZER;
string dictionary[MAX_KEYS];
queue<int> my_q;

// function declarations
// void perr_exit(const string &msg);
// void cout_exit(const string &msg);
// pair<string, int> read_string_from_socket(const int &fd, int bytes);
// int send_string_on_socket(int fd, const string &s);
// void handle_connection(void *client_fd_ptr);
// void *handle_thread_pool(void *args);
// int tokenize_inputs(char *input_string, string commands[]);
// string handle_requests(string command[], int argc);
// string handle_insert(string command[], int argc);
// string handle_delete(string command[], int argc);
// string handle_update(string command[], int argc);
// string handle_concat(string command[], int argc);
// string handle_fetch(string command[], int argc);

// functions
void perr_exit(const string &msg)
{
    perror(msg.c_str());
    exit(-1);
}

void cout_exit(const string &msg)
{
    cout << msg << endl;
    exit(-1);
}

pair<string, int> read_string_from_socket(const int &fd, LL bytes)
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

string handle_insert(string command[], int argc)
{
    string output;
    int key = atoi(command[1].c_str());
    pthread_mutex_lock(&dic_mutex[key]);
    string value = command[2];
    if (dictionary[key] == "") // no key present
    {
        dictionary[key] = value;
        output = BGRN "Insertion successful" ANSI_RESET;
    }
    else // key already present
        output = BRED "Key already exists" ANSI_RESET;
    pthread_mutex_unlock(&dic_mutex[key]);
    return output;
}

string handle_delete(string command[], int argc)
{
    string output;
    int key = atoi(command[1].c_str());
    pthread_mutex_lock(&dic_mutex[key]);
    if (dictionary[key] == "") // no key present
        output = BRED "No such key exists" ANSI_RESET;
    else // key already present
    {
        dictionary[key] = "";
        output = BGRN "Deletion successful" ANSI_RESET;
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
    if (dictionary[key] == "") // no key present
        output = BRED "Key does not exist" ANSI_RESET;
    else // key already present
    {
        dictionary[key] = value;
        output = BGRN + value + ANSI_RESET;
    }
    pthread_mutex_unlock(&dic_mutex[key]);
    return output;
}

string handle_concat(string command[], int argc)
{
    string output;
    int key1 = atoi(command[1].c_str());
    int key2 = atoi(command[2].c_str());
    if (key1 > key2)
    {
        int temp = key1;
        key1 = key2;
        key2 = temp;
    }
    // locking the smaller key first
    pthread_mutex_lock(&dic_mutex[key1]);
    pthread_mutex_lock(&dic_mutex[key2]);

    if ((dictionary[key1] == "") || (dictionary[key2] == "")) // no keys present
        output = BRED "Concat failed as at least one of the keys does not exist" ANSI_RESET;
    else // keys present
    {
        string temp1 = dictionary[key1], temp2 = dictionary[key2];
        dictionary[key1] += temp2;
        dictionary[key2] += temp1;
        output = BGRN + dictionary[key2] + ANSI_RESET;
    }

    pthread_mutex_unlock(&dic_mutex[key2]);
    pthread_mutex_unlock(&dic_mutex[key1]);

    return output;
}

string handle_fetch(string command[], int argc)
{
    string output;
    int key = atoi(command[1].c_str());
    pthread_mutex_lock(&dic_mutex[key]);
    if (dictionary[key] == "") // no key present
        output = BRED "Key does not exist" ANSI_RESET;
    else // key already present
        output = BGRN + dictionary[key] + ANSI_RESET;
    pthread_mutex_unlock(&dic_mutex[key]);
    return output;
}

string handle_requests(string command[], int argc)
{
    string output = BRED "Invalid commands by the user." ANSI_RESET;
    if ((argc != 2) && (argc != 3))
    {
        return output;
    }

    int key = atoi(command[1].c_str());
    if ((key < 0) || (key > 100))
    {
        output = BRED "Invalid commands by the user. Key must be in range [0,100]" ANSI_RESET;
        return output;
    }

    if (argc == 2)
    {
        if (command[0] == "delete")
            output = handle_delete(command, argc);
        else if (command[0] == "fetch")
            output = handle_fetch(command, argc);
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

// void handle_connection(void *client_fd_ptr)
void handle_connection(int client_fd)
{
    // int client_fd = *((int *)client_fd_ptr);
    string command[ARG_LIMIT];
    string recv_msg, send_msg, send_msg_final;
    int argc, received_num, ret_val = 1;
    int sent_to_client;

    tie(recv_msg, received_num) = read_string_from_socket(client_fd, BUFFER_SIZE);
    ret_val = received_num;
    if (ret_val <= 0)
    {
        printf("Server could not read msg sent from client\n");
        goto exit_from_here;
    }
    cout << "Client sent : " << recv_msg << endl;

    argc = tokenize_inputs(&recv_msg[0], command);
    send_msg = handle_requests(command, argc);
    send_msg_final = to_string(pthread_self()) + ":" + send_msg;

    // sleep before sending back to client
    sleep(2);
    sent_to_client = send_string_on_socket(client_fd, send_msg_final);
    if (sent_to_client == EXIT_FAILURE)
    {
        perror("Error while writing to client. Seems socket has been closed");
        goto exit_from_here;
    }

exit_from_here:
    close(client_fd);
    printf(BRED "Disconnected from client" ANSI_RESET "\n");
    return;
}

void *handle_thread_pool(void *args)
{
    printf("Thread %ld created.\n", pthread_self());
    while (1)
    {
        pthread_mutex_lock(&q_mutex);
        while (my_q.empty())
        {
            // cout << "size" << my_q.size() << "\t" << pthread_self() << endl;
            pthread_cond_wait(&q_condvar, &q_mutex);
        }
        int client_fd = my_q.front();
        // cout << "before size" << my_q.size() << endl;
        my_q.pop();
        cout << pthread_self() << " popped fd " << client_fd << endl;
        part;
        // cout << "after size" << my_q.size() << endl;
        pthread_mutex_unlock(&q_mutex);
        handle_connection(client_fd);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int thread_num;
    if (argc != 2)
        cout_exit("Insufficient arguments. Mention the number of threads.");

    thread_num = atoi(argv[1]);
    if (thread_num < 1)
        cout_exit("Enter valid number of threads in pool.");

    // initializing the dictionary and mutex
    for (int i = 0; i < MAX_KEYS; i++)
    {
        dictionary[i] = "";
        if (pthread_mutex_init(&dic_mutex[i], NULL))
            perr_exit("mutex error: unable to initialize mutex lock");
    }

    // initiating the thread pool
    pthread_t thread_pool[thread_num];
    for (int i = 0; i < thread_num; i++)
    {
        pthread_create(&thread_pool[i], NULL, handle_thread_pool, NULL);
    }
    // to wait for all threads to be created
    sleep(2);

    struct sockaddr_in serv_addr_obj, client_addr_obj;
    int listen_fd, client_fd, port_num;
    socklen_t client_len;

    // creating a socket that welcomes some initial contact
    // from a client process running on an arbitrary host
    // get listening socket
    // get ip, port
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perr_exit("ERROR creating listening socket");

    // This is to lose the pesky "Address already in use" error message
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) // SOL_SOCKET is the socket layer itself
        perr_exit("setsockopt");

    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    port_num = SERVERPORT;
    serv_addr_obj.sin_family = AF_INET;
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY; // server: INADDR_ANY will bind the port to all available interfaces.
    serv_addr_obj.sin_port = htons(port_num);   // process specifies port

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
        printf("Waiting for a new client to request for a connection at port %d\n",
               port_num);

        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr_obj, &client_len);
        pthread_mutex_lock(&q_mutex);
        if (client_fd < 0)
            perr_exit("accept: ERROR occurred in SERVER");

        printf(BGRN "New client connected from port number %d and IP %s\n" ANSI_RESET,
               ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));

        my_q.push(client_fd);
        // cout << "myq size " << my_q.size() << endl;
        pthread_cond_signal(&q_condvar);
        pthread_mutex_unlock(&q_mutex);
    }

    close(listen_fd);
    return 0;
}
