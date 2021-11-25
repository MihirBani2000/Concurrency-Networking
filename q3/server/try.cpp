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
#include <iostream>
#include <assert.h>
#include <tuple>
using namespace std;
typedef long long int ll;

#define err_exit(x) \
    perror(x);      \
    exit(EXIT_FAILURE);

int main(int argc, char *argv[])
{
    string a = "mihir", b = "bani";
    string temp1 = a, temp2 = b;
    a += temp2;
    b += temp1;
    cout << a << endl
         << b << endl;
    return 0;
}