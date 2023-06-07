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

int print(char arr[3][1024]) {
    std::cout << arr[1] << std::endl;
    std::cin >> arr[1];
    return 0;
}

int main(void) {
    char b[3][1024];
    char a[1024];
    std::cin >> a;
    std::strcpy(b[1], a);
    print(b);
    std::cout << "2: " << b[1];
    return 0;
}


