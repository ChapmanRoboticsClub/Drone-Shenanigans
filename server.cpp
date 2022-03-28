// NOTE: Windows needs to be compiled with -lWS2_32 for the WS2_32 library

#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>

// Wanted to use SOCKET but that conflicted with mingW.  Unsure if SOCKET is defined in posix
typedef unsigned int SOCKET_TYPE;
#else
// Currently assuming non-windows is POSIX, which may not be a safe assumption
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

typedef int SOCKET_TYPE;
#endif


#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

void processConnection(SOCKET_TYPE sock, bool* gameOver);
void processDrone(SOCKET_TYPE sock, bool* gameOver);
void processPlayer(SOCKET_TYPE sock, bool* gameOver);
void processStation(SOCKET_TYPE sock, bool* gameOver);

extern int errno;

int main() {
    #ifdef _WIN32
        WSADATA wsa;
        printf("\nInitializing Winsock...");
        if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
            printf("Failed. Error Code : %d",WSAGetLastError());
            return 1;
        }
        printf("Initialized.\n");
	#endif

    struct sockaddr_in my_addr, sen_addr;

    // Step 1: Create socket
    SOCKET_TYPE listeningSocket = socket(AF_INET , SOCK_STREAM , 0);

    // Step 2: Bind to a port number
    memset(&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(9090);
	bind(listeningSocket, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));

    // Step 3: Listen for connections
    listen(listeningSocket, 5);

    std::vector<std::thread*> threads;
    std::vector<SOCKET_TYPE> sockets;
    bool gameOver = false;

    // NOTE: Hard coded to look for exactly 3 connections; needs to change to look for a certain number of drones, players, stations, or to just wait for a "START GAME" input
    for(int i = 0; i < 3; ++i) {
        // Have threads figure out whether sender is Player, Station, or Drone on connect.  Then, process separately in separate functions dependent on that (Switch statement!)
        // Accept a connection request
        socklen_t sen_len = sizeof(sen_addr);
        SOCKET_TYPE newsock = accept(listeningSocket, (struct sockaddr *)&sen_addr, &sen_len);
        threads.push_back(new std::thread(processConnection, newsock, &gameOver));
        sockets.push_back(newsock);
    }

    // Sleeping for 1 sec to ensure press enter comes at the bottom
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Game Start!
    std::cout << "Press Enter!" << std::endl;
    std::string temp;
    std::getline(std::cin, temp);

    // Game End
    gameOver = true;


    // Join threads here, after the game has ended
    for(int i = 0; i < threads.size(); ++i) {
        (*threads[i]).join();
        delete threads[i];
        // TODO: Close socket as thread is getting deleted; so probably have to keep track of those as well
        #ifdef _WIN32
            closesocket(sockets[i]);
        #else
            close(sockets[i]);
        #endif
    }

    // Cleanup and close sockets as necessary
    #ifdef _WIN32
        closesocket(listeningSocket);
    #else
        close(listeningSocket);
    #endif

    std::cout << "Program terminated" << std::endl;
	return 0;
}

void processConnection(SOCKET_TYPE sock, bool* gameOver) {
    std::cout << "Thread to process incoming connection" << std::endl;
    char buffer[100];
    int len = recv(sock, buffer, 100, 0);

    if(len == 1) {
        switch(buffer[0]) {
            case 'D':
                std::cout << "I just connected a drone!" << std::endl;
                processDrone(sock, gameOver);
                break;
            case 'P':
                std::cout << "I just connected a player!" << std::endl;
                processPlayer(sock, gameOver);
                break;
            case 'S':
                std::cout << "I just connected a station!" << std::endl;
                processStation(sock, gameOver);
                break;
            default:
                std::cout << "Unexpected identifier as first message: '" << buffer[0] << "'" << std::endl;
        }
    } else {
        std::cout << "Unexpected byte count from first message: " << len << std::endl;
    }
}

void processDrone(SOCKET_TYPE sock, bool* gameOver) {
    char buffer[100];
    int len;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    struct timeval tv;
    tv.tv_usec = 100000;
    while(!(*gameOver)) {
        // Spin lock

        // Use select before recv to prevent blocking
        select(sock + 1, &fds, NULL, NULL, &tv);
        if(FD_ISSET(sock, &fds)) {
            len = recv(sock, buffer, 100, 0);
            std::cout << "Received message (" << len << "): " << buffer << std::endl;   
        } else {
            FD_SET(sock, &fds);
        }
    }
    // Send FIN packet
    char message[4] = "FIN";
    send(sock, message, strlen(message) + 1, 0);
}

void processPlayer(SOCKET_TYPE sock, bool* gameOver) {
    while(!(*gameOver)) {
        // Spin lock
        char message[100] = "WAITING...";
        send(sock, message, strlen(message) + 1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // Send FIN packet
    char message[4] = "FIN";
    send(sock, message, strlen(message) + 1, 0);
}

void processStation(SOCKET_TYPE sock, bool* gameOver) {
    while(!(*gameOver)) {
        // Spin lock
        char message[100] = "WAITING...";
        send(sock, message, strlen(message) + 1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // Send FIN packet
    char message[4] = "FIN";
    send(sock, message, strlen(message) + 1, 0);
}
