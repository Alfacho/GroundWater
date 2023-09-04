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

// Сейчас вся архитектура построена только на 1 менеджере, то есть на 100 подключениях, дальше нужно перерабатывать
#define MAX_CONNECTIONS 100
#define ERROR_S "SERVER ERROR: "
#define DEFAULT_PORT 1605 // 9
#define BUFFER_SIZE 1024
#define NICK_SIZE 9
#define CLIENT_CLOSE_CONNECTION_SYMBOL '#'

//Хранение подключений
int connections[MAX_CONNECTIONS];
int indexes_of_ready_connections[MAX_CONNECTIONS];
int counter = 0;

void* ClientManger(void *args);
void* ClientHandler(void *args);
void UnblockingSocket(int n_connection);
int ClientSorter(int sector, int indexes_of_disconnections[100], int disconnection_counter);
int ClientSender(char msgs[100][BUFFER_SIZE], int sender_indexes[100], int subsector, int* breaker, int* counter_of_ready_hands);

int main(int argc, char const* argv[]) {
    // Приветствие
    system("clear");
    system("figlet -f slant \"GroundWater\"");

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

    while (true) {
        if (counter < MAX_CONNECTIONS) {
            // Подключаем к сокету сервера клиент
            server = accept(client, reinterpret_cast <struct sockaddr*> (&server_address), &size);

            if (server <= 0) {
                std::cout << ERROR_S << "cant accepting client." << std::endl;
            } else {
                connections[counter] = server;
                UnblockingSocket(counter);
                // Создаем новый поток для контроля за подключением
                if ((counter + 1)% 100 == 0) {
                    // создаем для непредвиденного сдвига
                    // сектора начинаются со 100 и так далее, 1 сектор контролирует 1 менеджер
                    int sector = counter + 100;
                    pthread_create(&ID_threads[counter/100], NULL, ClientManger, (void*)&sector);
                    pthread_detach(ID_threads[counter/100]);
                }

                std::cout << "Client №" << counter + 1 << " CONNECTED! ID of connection: " << connections[counter] << std::endl;
                counter++;

                if ((counter + 1) == 100) {std::cout << "MAXIMUM CONNECTIONS RECHED: " << (counter + 1) << '/' << MAX_CONNECTIONS << std::endl;}
            }
        } else {
            usleep(1000);
        }
    }

    return 0;
}


// Новейшее ядро v0.2
void* ClientManger(void *args) {
    /* Смотрит за участком, то есть массивом подключений и отдает комманды, до 100 подключений вкл */

    // распаковка значения контролируемого сектора
    int sector = *((int *) args);
    // временный буфер и массив для сообщений 
    char buffer[BUFFER_SIZE], msgs[100][BUFFER_SIZE];
    // массив индексов отправителей
    int sender_indexes[100];
    for (int j = 0; j < 100; j++) {
        sender_indexes[j] = -25;
    }
    int indexes_of_disconnections[100];
    // блокировщик, который указывает потокам, что цикл потока менеджера завершён
    int breaker = 1;
    // счетчик завершенных циклов потоков отправителей
    int counter_of_ready_hands = 2;

    std::thread hand_1(ClientSender, msgs, sender_indexes, 1, &breaker, &counter_of_ready_hands);
    std::thread hand_2(ClientSender, msgs, sender_indexes, 2, &breaker, &counter_of_ready_hands);
    hand_1.detach();
    hand_2.detach();

    while(true) {
        // проверяем завершен ли прошлый цикл отправителей
        if (counter_of_ready_hands == 2) {
            counter_of_ready_hands = 0;
            int message_and_sender_counter = 0;
            int disconnection_counter = 0;
            // разблокировываем потоки сендеров
            breaker = 0;

            // Устанавливаем край работы менеджера
            int edge = sector;
            if (sector > counter) {
                edge = counter;
            }
            for (int i = sector - 100; i < edge; i++) {
                int socket_status = recv(connections[i], buffer, BUFFER_SIZE, 0);

                // Есть сообщение, передаем ф-ции потока чтения чтения
                if (socket_status > 0) {
                    // !! БАГ, если соединение живо, то не хначит, что есть сообщение !!
                    std::strcpy(msgs[message_and_sender_counter], buffer);
                    sender_indexes[message_and_sender_counter] = i;
                    message_and_sender_counter++;
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
                    counter--;
                    indexes_of_disconnections[disconnection_counter] = i;
                    disconnection_counter++;
                }  
            }
            // даем команду потокам сендеров на завершение
            breaker = 1;

            ClientSorter(sector, indexes_of_disconnections, disconnection_counter);
        } else {
            // спаааать 
            usleep(10);
            // Можно снизить задержку и вставив считыватель комманд серверной консоли
        }
    }
    return 0;
}

void UnblockingSocket(int n_connection) {
    /*Устанавливает неблокирующий режим чтения для сокетов на секторе*/
        int nonBlocking = 1;
        if (fcntl(n_connection, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
            std::cout << "FAILED TO SET NON-BLOCKING SOCKET №" << n_connection << std::endl;
            // нужно отключить клиента из сессии
        }
}

int ClientSorter(int sector, int indexes_of_disconnections[100], int disconnection_counter) {
    /*Сортирует массив подключений от закрытых сокетов*/
    int copy_arr[100];
    int c = 0;
    int c1 = 0;
    for (int i = sector - 100; i < sector; i++) {
        int fl = 0;
        for (int j = 0; j < disconnection_counter; j++) {
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
        connections[i] = copy_arr[c1];
        c1++;
    }
    return 0;
}

// bag
int ClientSender(char msgs[100][BUFFER_SIZE], int sender_indexes[100], int subsector, int* breaker, int* counter_of_ready_hands) {
    /* Отправляет сообщения выбранных клиентов на участке, до 50 клиентов вкл*/
    // Подсектор это до 50 подключений входящих в сектор менеджера, в отличии от секторов имеют свои индексы
    int subsector_start = 0;
    int subsector_finish = 50;
    if (subsector == 2) {
        subsector_start += 50;
        subsector_finish += 50;
    }

    while (true) {
        // ждем пока не дана команда начать цикл 
        if (*breaker == 0) {
            // каждый цикл начинается с начала подсектора, обновляем его
            int index_subsector = subsector_start;
            // пока (нет остановы или есть сообщения) и не достигли края подсектора читаем сообщения
            while (((*breaker == 0) || (sender_indexes[index_subsector] != -25))  && (index_subsector < subsector_finish)) {
                // то что нет остановы и нет края, не значит, что есть сообщение
                if (sender_indexes[index_subsector] != -25) {
                    for (int i = 0; i < counter; i++) {
                        if (i != sender_indexes[index_subsector]) {
                            send(connections[i], msgs[index_subsector], BUFFER_SIZE, 0);
                        }
                    }
                    // очищаем массив, тк подключение больше не ждет прочтения, ведь сообщение не актульно
                    sender_indexes[index_subsector] = -25;
                    // сдвигаемся, если сообщение отправлено
                    // !! ИНДЕКСАЦИЯ MSG И SENDER_INDEXES СООТВЕТСТВУЕТ ДРУГ ДРУГУ И ПОСЛЕДОВАТЕЛЬНА !!
                    index_subsector++;
                } else {
                    std::cout << "none" << std::endl;
                }
                // ошибка в цикле
                usleep(1000);
                // ошибка в цикле
            }
            std::cout << "THE END!" << std::endl;
            *counter_of_ready_hands++;
        } else {
            usleep(2);
        }
    }
    return 0;
}


// старенькое ядро сервера
// устарело тк расходует целый поток на 1 подключение
void* ClientHandler(void *args) {
    // Распаковываем тк pthread_create принимает только void *args
    int index = *((int *) args);
    char buffer[BUFFER_SIZE];
    while(true) {
        //Читаем из сокета или ждем прочтения
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

// война с ящерами
//bySAO