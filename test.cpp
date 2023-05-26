#include <iostream>
#include <cstring>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <thread>

void msg_format() {
    char n;
    char a[1024];

    std::cin >> n;
    for (int i = 0; i < 1024; i++) {
        a[i] = n;
    }
    

    for (int i = 0; i < 1024; i++) {
        std::cout << '\n' << a;
    }
    std::cout << std::endl;
}

int main(void) {
    msg_format();
    return 0;
}


