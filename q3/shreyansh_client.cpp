#include <stdio.h>
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

/////////////////////////////
#include <pthread.h>
#include <iostream>
#include <semaphore.h>
#include <assert.h>
#include <queue>
#include <vector>
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
const LL MOD = 1000000007;
#define part cout << "-----------------------------------" << endl;
#define pb push_back
#define debug(x) cout << #x << " : " << x << endl

///////////////////////////////
#define SERVER_PORT 8002
////////////////////////////////////

const LL buff_sz = 1048576;

vector < pair <int,string> > vec;
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
///////////////////////////////////////////////////

string str_err_1 = "Insertion Sucessful!";
string str_err_2 = "Key already exists!";
string str_err_3 = "No such key exists!";
string str_err_4 = "Deletion Sucessful!";
string str_err_5 = "Concat failed as at least one of the keys does not exist!";


pair<string, int> read_string_from_socket(int fd, int bytes)
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    // debug(bytes_received);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. Seems server has closed socket\n";
        // return "
        exit(-1);
    }

    // debug(output);
    output[bytes_received] = 0;
    output.resize(bytes_received);

    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)
{
    // cout << "We are sending " << s << endl;
    int bytes_sent = write(fd, s.c_str(), s.length());
    // debug(bytes_sent);
    // debug(s);
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA on socket.\n";
        // return "
        exit(-1);
    }

    return bytes_sent;
}

int get_socket_fd(struct sockaddr_in *ptr)
{
    struct sockaddr_in server_obj = *ptr;

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("Error in socket creation for CLIENT");
        exit(-1);
    }
    int port_num = SERVER_PORT;
    memset(&server_obj, 0, sizeof(server_obj)); // Zero out structure
    server_obj.sin_family = AF_INET;
    server_obj.sin_port = htons(port_num); //convert to big-endian order
    if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0)
    {
        perror("Problem in connecting to the server");
        exit(-1);
    }
    return socket_fd;
}

void * begin_process(void * ptr)
{
    long long int indx = (long long int)ptr;
    sleep(vec[indx].first);
    struct sockaddr_in server_obj;
    int socket_fd = get_socket_fd(&server_obj);
    // cout<<"indx = "<<indx<<'\n';
    // cout << "Connection to server successful" << endl;
    string to_send;
    // cout << "Enter msg: "<<vec[indx].second;
    // temp = vec[indx].second;
    string temp = vec[indx].second;
    // cout<<"Temp = "<<temp<<'\n';
    send_string_on_socket(socket_fd, temp);
    int num_bytes_read;
    string output_msg;
    tie(output_msg, num_bytes_read) = read_string_from_socket(socket_fd, buff_sz);

    cout<<indx<<":"<< output_msg << '\n';
    fflush(stdout);
    cout << "====" << endl;
    return NULL;
}

int main(int argc, char *argv[])
{

    int i, j, k, t, n,m;
    pthread_t client_thread[1000];
    char str[500];
    string val2;
    int val1;
    cin>>m;
    for(int i=0;i<m;i++)
    {
        cin>>val1;
        cin.getline(str,100);
        vec.pb({val1,str});


    }
    // cout<<"reach here\n";
    for(int i=0;i<m;i++)
    {
        // cout<<"Inside]n";
        pthread_create(&client_thread[i],NULL,begin_process,(void *)(long long int)i);
    }
    for(int i=0;i<m;i++)
    {
        pthread_join(client_thread[i],NULL);
    }
    // begin_process();
    return 0;
}