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
#include <fcntl.h>

#define MAX_CONNECTIONS 1000
#define ERROR_S "SERVER ERROR: "
#define DEFAULT_PORT 1605
#define BUFFER_SIZE 1024
#define NICK_SIZE 9
#define CLIENT_CLOSE_CONNECTION_SYMBOL '#'

//Хранение подключений
int connections[MAX_CONNECTIONS];
int indexes_of_ready_connections[MAX_CONNECTIONS];
int counter = 0;

void* ClientHandler(void *args);
void UnblockingSocket(int n_connection);
int ClientSorter(int sector);
void ClientSender(char* buffer);

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
            UnblockingSocket(i);
            counter++;
            // j нужна от непредвиденного сдвига
            int j = i;
            // Создаем новый поток для контроля за подключением
            pthread_create(&ID_threads[i], NULL, ClientHandler, (void*)&j);
            pthread_detach(ID_threads[i]);

            std::cout << "Client №" << i + 1 << " CONNECTED! ID of connection: " << connections[i] << std::endl;
        }
    }

    return 0;
}

void* ClientManger(int sector) {
    /* Смотрит за участком, то есть массивом подключений и отдает комманды, до 100 подключений вкл */
    char buffer[BUFFER_SIZE], msgs_1[50][BUFFER_SIZE], msgs_2[50][BUFFER_SIZE];
    int sender_indexes_1[50], sender_indexes_2[50];
    int indexes_of_disconnections[100];
    int count_n_dis = 0;
    while(true) {
        //!!
        std::thread hand_1(ClientSender, &msgs_1);
        std::thread hand_2(ClientSender, &msgs_2);
        //!!
        for (int i = sector - 100; i < counter; i++) {
            int socket_status = recv(connections[i], buffer, BUFFER_SIZE, 0);

            // Есть сообщение, передаем ф-ции потока чтения чтения
            if (socket_status > 0) {
                if (i < 50) {
                    std::strcpy(msgs_1[i], buffer);
                } else {
                    std::strcpy(msgs_2[i], buffer);
                }
            } else {
                // Сокет закрыт на другом конце соединения
                if (socket_status == 0) {
                    std::cout << "Client №" << i + 1 << " LEFT! ID of connection: " << connections[i] << std::endl;
                }
                // Ошибка чтения сокета, закрываем сокет
                if (socket_status < 0) {
                    std::cout << "Client №" << i + 1 << " IS NOT READABLE! ID of connection: " << connections[i] << std::endl;
                }
                close(connections[i]);
                indexes_of_disconnections[count_n_dis] = i;
                count_n_dis++;
            }  
        }
        hand_1.join();
        hand_2.join();
        ClientSorter(sector);
    }
    return 0;
}

void UnblockingSocket(int n_connection) {
    /*Устанавливает неблокирующий режим чтения для сокетов на секторе*/
        int nonBlocking = 1;
        if (fcntl(n_connection, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
            std::cout << "FAILED TO SET NON-BLOCKING SOCKET №" << n_connection << std::endl;
        }
}

int ClientSorter(int sector) {
    /*Сортирует массив подключений от закрытых сокетов*/
    return 0;
}

void ClientSender(char* buffer) {
    /* Отправляет сообщения выбранных клиентов на участке, до 50 клиентов вкл*/
}



void* ClientHandler(void *args) {
    // Распаковываем тк pthread_create принимает только void *args
    int index = *((int *) args);
    char buffer[BUFFER_SIZE];
    while(true) {
        //Читаем с сокета или ждем прочтения
        if (recv(connections[index], buffer, BUFFER_SIZE, 0) > 0) {
            for (int i = 0; i < counter; i++) {
                if (i != index) {
                    // Отправляем прочитанное всем кроме отправившего
                    send(connections[i], buffer, BUFFER_SIZE, 0);
                }
            }
        } else {
            std::cout << "Client №" << index + 1 << " LEFT! ID of connection: " << connections[index] << std::endl;
            close(connections[index]);
            return 0;
        }
    }
}

//bySAO
