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
#include <pthread.h>

#define ERROR_S "SERVER ERROR: "
#define DEFAULT_PORT 1604
#define BUFFER_SIZE 1024
#define NICK_SIZE 9
#define CLIENT_CLOSE_CONNECTION_SYMBOL '#'

//Хранение подключений
int connections[100];
int counter = 0;

void* ClientHandler(void *args);

int main(int argc, char const* argv[]) {
    int client;
    int server;
    // Ссылки на потоки каждого клиента
    pthread_t ID_threads[100];

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

    int ret = bind(client, reinterpret_cast < struct sockaddr*>(&server_address), sizeof(server_address));

    if (ret < 0) {
        std::cout << ERROR_S << "binding connection! Socket has already been establishing." << std::endl;
        return -1;
    }

    socklen_t size = sizeof(server_address);
    std::cout << "SERVER: " << "listening clients...\n" << std::endl;
    listen(client, SOMAXCONN);

    for (int i = 0; i <= 100; i++) {
        // Подключаем к сокету сервера клиент
        server = accept(client, reinterpret_cast <struct sockaddr*> (&server_address), &size);

        if (server <= 0) {
            std::cout << ERROR_S << "cant accepting client." << std::endl;
            i -= 1;
        } else {
            connections[i] = server;
            counter++;
            // j нужна от непредвиденного сдвига
            int j = i;
            // Создаем новый поток для контроля за подключением
            pthread_create(&ID_threads[i], NULL, ClientHandler, (void*)&j);
            pthread_detach(ID_threads[i]);

            std::cout << "Client N" << i + 1 << " Connected! ID of connection: " << connections[i] << std::endl;
        }
    }

    return 0;
}

void* ClientManger() {
    /* Смотрит за участком, то есть массивом подключений и отдает комманды */
    return 0;
}

void* ClientCleaner() {
    /*Сортирует массив подключений*/
    return 0;
}

void* ClientSender() {
    /* Отправляет сообщения выбранных клиентов на участке*/
    return 0;
}



void* ClientHandler(void *args) {
    // Распаковываем тк pthread_create принимает только void *args
    int index = *((int *) args);
    char buffer[BUFFER_SIZE];
    while(true) {
        //Читаем с сокета или ждем прочтения
        recv(connections[index], buffer, BUFFER_SIZE, 0);
        for (int i = 0; i < counter; i++) {
            if (i != index) {
                // Отправляем прочитанное всем кроме отправившего
                send(connections[i], buffer, BUFFER_SIZE, 0);
            }
        }
    }
}

//bySAO
