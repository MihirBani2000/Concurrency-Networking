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

void prints(void *a)
{
    string my = *((string *)a);
    my += "lol";
    cout << my << endl;
}

int main(int argc, char *argv[])
{
    int i = 0;
    string c = "mihir", b = "bani", a;
    // while (i++ < 2)
    // {
    //     getline(cin, a);
    //     // string temp1 = a, temp2 = b;
    //     // a += temp2;
    //     // b += temp1;
    //     // cout << a << endl
    //     //      << b << endl;
    //     cout << a << endl;
    //     prints(&a);
    //     cout << a << endl;
    // }
    a = to_string(pthread_self()) + c;
    cout << a << endl;
    return 0;
}