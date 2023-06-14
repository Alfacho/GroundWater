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

#define SERVER_IP "127.0.0.1"
#define DEFAULT_PORT 1606
#define BUFFER_SIZE 1024
#define NICK_SIZE 9
#define SERVER_CLOSE_CONNECTION_SYMBOL '#'
#define ERROR_C "CLIENT ERROR: "

int client;

bool IsClientConnectionClose(const char* msg, int* client);
int msg_format(char* nick, int nick_str_size, char* buffer, int buffer_str_size, char* msg);
void nick_format(char* buffer, char* nick);

void ClientReader();

int main(int argc,char const* argv[]) {
    char buffer[BUFFER_SIZE];

    char nick[NICK_SIZE];
    nick_format(buffer, nick);
    
    struct sockaddr_in server_address;

    client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        std::cout << ERROR_C << "establishing socket error!" << std::endl;
        exit(0);
    }

    server_address.sin_port = htons(DEFAULT_PORT);
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);

    std::cout << "\n=> Client socket created." << std::endl;

    int ret = connect(client, reinterpret_cast<const struct sockaddr*>(&server_address), sizeof(server_address));
    if (ret == 0) {
        std::cout << "=> Connection to server." << inet_ntoa(server_address.sin_addr) << " with port number: " 
                    << DEFAULT_PORT << std::endl << std::endl;
    }

    std::thread Hand(ClientReader);
    Hand.detach();

    char msg[BUFFER_SIZE];
    while (true) {
        std::cin.getline(buffer, BUFFER_SIZE);
        std::cout << std::endl;
        while (msg_format(nick, strlen(nick), buffer, strlen(buffer), msg) == 1) {
            std::cin.getline(buffer, BUFFER_SIZE);
            std::cout << std::endl;
        }
        send(client, msg, BUFFER_SIZE, 0);

        if (IsClientConnectionClose(buffer, &client)) {
            return 0;
        }
    }

    return 0;
}

void ClientReader() {
    char buffer[BUFFER_SIZE];
    while (true) {
        recv(client, buffer, BUFFER_SIZE, 0);
        std::cout << buffer << std::endl << std::endl;
    }
}

bool IsClientConnectionClose(const char* msg, int* client) {
    for (int i = 0; i < strlen(msg); ++i) {
        if (msg[i] == SERVER_CLOSE_CONNECTION_SYMBOL) {
            close(*client);
            std::cout << "\n GoodBye..." << std::endl;
            return true;
        }
    }
    return false;
}

int msg_format(char* nick, int nick_str_size, char* buffer, int buffer_str_size, char* msg) {
    int f = 0;
    if (nick_str_size + buffer_str_size + 2 + 1 < 1024) {
        std::strcpy(msg, nick);
        std::strcat(msg, ": ");
        std::strcat(msg, buffer);
    } else {
        std::cout << "Limit " << BUFFER_SIZE - nick_str_size - 2 - 1 << " symbols!" << std::endl;
        f = 1;
    }
    return f;
}

void nick_format(char* buffer, char* nick) {
    std::cout << "Please enter your nickname to continue ( 8 characters, ENG ): ";
    std::cin.getline(buffer, BUFFER_SIZE);
    while (strlen(buffer) > NICK_SIZE - 1) {
        std::cout << "\nPlease enter your nickname to continue ( >> 8 CHARACTERS <<, ENG ): ";
        std::cin.getline(buffer, BUFFER_SIZE);
    }
    strcpy(nick, buffer);
}
