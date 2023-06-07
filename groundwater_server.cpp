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

#define MAX_CONNECTIONS 100
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
int ClientSorter(int sector, int indexes_of_disconnections[100], int count_n_dis);
int ClientSender(char msgs[100][BUFFER_SIZE], int senders_indexes[100], int subsector, int* breaker);

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
    char buffer[BUFFER_SIZE], msgs[100][BUFFER_SIZE];
    int senders_indexes[100];
    int indexes_of_disconnections[100];
    while(true) {
        int count_ind_dis = 0;
        int breaker = 0;
        for (int j = 0; j < 100; j++) {senders_indexes[j] = -25;}
        //!!
        std::thread hand_1(ClientSender, msgs, senders_indexes, 1, &breaker);
        std::thread hand_2(ClientSender, msgs, senders_indexes, 2, &breaker);
        //!!
        for (int i = sector - 100; i < counter; i++) {
            int socket_status = recv(connections[i], buffer, BUFFER_SIZE, 0);

            // Есть сообщение, передаем ф-ции потока чтения чтения
            if (socket_status > 0) {
                std::strcpy(msgs[i - (sector - 100)], buffer);
                senders_indexes[i - (sector - 100)] = i;
            } else {
                // Сокет закрыт на другом конце соединения, поэтому закрываем здесь
                if (socket_status == 0) {
                    std::cout << "Client №" << i + 1 << " LEFT! ID of connection: " << connections[i] << std::endl;
                }
                // Ошибка чтения сокета, закрываем сокет
                if (socket_status < 0) {
                    std::cout << "Client №" << i + 1 << " IS NOT READABLE! ID of connection: " << connections[i] << std::endl;
                }
                close(connections[i]);
                indexes_of_disconnections[count_ind_dis] = i;
                count_ind_dis++;
            }  
        }
        breaker = 1;
        hand_1.join();
        hand_2.join();

        ClientSorter(sector, indexes_of_disconnections, count_ind_dis);
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

int ClientSorter(int sector, int indexes_of_disconnections[100], int count_ind_dis) {
    /*Сортирует массив подключений от закрытых сокетов*/
    int copy_arr[100];
    int c = 0;
    for (int i = sector - 100; i < sector; i++) {
        int fl = 0;
        for (int j = 0; j < count_ind_dis; j++) {
            if (i == indexes_of_disconnections[j]) {
                fl = 1;
            }
        }
        if (fl == 0) {
            copy_arr[c] == connections[i];
            c++;
        }
    }

    for (int i = sector - 100; i < (sector - 100 + c); i++) {
        connections[i] = copy_arr[i];
    }
    return 0;
}

int ClientSender(char msgs[100][BUFFER_SIZE], int senders_indexes[100], int subsector, int* breaker) {
    /* Отправляет сообщения выбранных клиентов на участке, до 50 клиентов вкл*/
    int index;
    int index_edge;

    if (subsector == 1) {
        index = 0;
        index_edge = 50;
    } else {
        index = 50;
        index_edge = 100;
    }

    for (index; index < index_edge; index++) {
        // тормозит процесс пока не получит сообщение или останову, проверяю по индексу отправителя, если его нет то стоит -25
        while (-25 == senders_indexes[index] && *breaker != 1) {}

        // не заканчиваю алгоритм раньше, тк менеджер может закончить раньше отправителя
        if (-25 == senders_indexes[index]) {
            for (int i = 0; i < counter; i++) {
                // Отправляем прочитанное всем кроме отправившего
                if (i != senders_indexes[index]) {
                    send(connections[i], msgs[index], BUFFER_SIZE, 0);
                }
            }
        }
    }
    return 0;
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
