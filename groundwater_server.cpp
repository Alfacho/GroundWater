#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define ERROR_S "SERVER ERROR: "
#define DEFAULT_PORT 1601
#define BUFFER_SIZE 1024
#define CLIENT_CLOSE_CONNECTION_SYMBOL '#'

bool is_client_connection_close(const char* msg);

int main(int argc, char const* argv[]) {
    int client;
    int server;

    struct sockaddr_in server_address;

    client = socket(AF_INET, SOCK_STREAM, 0);

    if (client < 0) {
        std::cout << ERROR_S << "establishing socket error!" << std::endl;
        exit(0);
    }

    std::cout << "SERVER: Socket for server was succefully created" << std::endl;

    server_address.sin_port = htons(DEFAULT_PORT);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htons(INADDR_ANY);

    int ret = bind(client, reinterpret_cast < struct sockaddr*>(&server_address), sizeof(server_address) < 0);

    if (ret < 0) {
        std::cout << ERROR_S << "binding connection! Socket has already been establishing." << std::endl;
        return -1;
    }

    socketlen_t size = sizeof(server_address);
    std::cout << "SERVER: " << "listening clients..." << std::endl;
    listen(client, 1);

    server = accept(client, reinterpret_cast < struct sockaddr* > (&server_address), &size);
    if (server < 0) {
        std::cout << ERROR_S << "cant accepting client." << std::endl;
    }

    char buffer(BUFFER_SIZE);
    bool isExit = false;
    while (server > 0) {
        strcpy(buffer, "=> Server connected.\n");
        send(server, buffer, BUFFER_SIZE, 0);
        std::cout << "=> Connected to the client #1" << std::endl << "Enter " << CLIENT_CLOSE_CONNECTION_SYMBOL << " to end the connection\n\n";

        std::cout << "Client: ";
        recv(server, buffer, BUFFER_SIZE, 0);
        std::cout << buffer << std::endl;

        if (is_client_connection_close(buffer)) {
            isExit = true;
        }

        while (isExit) {
            std::cout << "Server: ";
            std::cin.getline(buffer, BUFFER_SIZE);
            send(server, buffer, BUFFER_SIZE, 0);
            if(is_client_connection_close(buffer)) {
                break;
            }
            std::cout << "Client: ";
            recv(server,buffer, BUFFER_SIZE, 0);
            std::cout << buffer << std::endl;
            if (is_client_connection_close(buffer)) {
                break;
            }
        }
        std::cout << "\n Good Bye..." << std::endl;
        isExit = false;
        exit(1);
    }
    return 0;
}

bool is_client_connection_close(const char* msg) {
    for (int i = 0; i < strlen(msg); ++i) {
        if (msg[i] == CLIENT_CLOSE_CONNECTION_SYMBOL) {
            return true;
        }
    }
    return false;
}

//18:20