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

// TODO: Use this?  Or not?  Not sure yet since it has to also be used in the hardware, and is easy enough to hardcode as 3
#define BUTTONS_PER_STATION 3

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

// TODO: GameData struct for gameOver, activeStation, currentCycle, subdivided into DroneData, PlayerData, and StationData for the relevant processing comamnds
void processConnection(SOCKET_TYPE sock, bool* gameOver, int* activeStation, bool* stationComplete);
void processDrone(SOCKET_TYPE sock, bool* gameOver, int droneID);
void processPlayer(SOCKET_TYPE sock, bool* gameOver);
void processStation(SOCKET_TYPE sock, bool* gameOver, int* activeStation, bool* stationComplete, int stationID);

void stationControl(bool* gameOver, int* activeStation, bool* stationComplete);
void playerControl(bool* gameOver);
void droneControl(bool* gameOver);

#define NUM_DRONES 0
#define NUM_STATIONS 2
#define NUM_PLAYERS 0

#define NUM_CYCLES 3 // Total cycles (number of times each station much be activated in order) to end the game

int main() {
    #ifdef _WIN32
        WSADATA wsa;
        printf("\nInitializing Winsock...");
        if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
            printf("Failed. Error Code : %d", WSAGetLastError());
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

    std::vector<std::thread*> control_threads;
    std::vector<std::thread*> threads;
    std::vector<SOCKET_TYPE> sockets;
    bool gameOver = true;
    int activeStation = 0;
    int currentCycle = 0;
    bool stationComplete[NUM_STATIONS];
    for(int i = 0; i < NUM_STATIONS; ++i) {
        stationComplete[i] = false;
    }

    // NOTE: Hard coded to look for exactly a certain number of connections; in the future could be modified to be an open lobby of connecting devices
    for(int i = 0; i < NUM_DRONES + NUM_PLAYERS + NUM_STATIONS; ++i) {
        // Thread figures out whether sender is Player, Station, or Drone on connect
        socklen_t sen_len = sizeof(sen_addr);
        SOCKET_TYPE newsock = accept(listeningSocket, (struct sockaddr *)&sen_addr, &sen_len);
        threads.push_back(new std::thread(processConnection, newsock, &gameOver, &activeStation, stationComplete));
        sockets.push_back(newsock);
    }

    // Initialize Control Threads
    control_threads.push_back(new std::thread(stationControl, &gameOver, &activeStation, stationComplete));
    control_threads.push_back(new std::thread(droneControl, &gameOver));
    control_threads.push_back(new std::thread(playerControl, &gameOver));

    // Sleeping for 1 sec to ensure all threads get to their spinlocking, and so the press to start shows up last
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "Press Enter to Start...";
    std::string temp;
    std::getline(std::cin, temp);

    // Game Start!
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "Game Start!" << std::endl;
    gameOver = false;

    while(!gameOver) {
        // spinlock until stationControl calls the game to a close
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Game Over!" << std::endl;
    std::cout << "TIME: " << std::endl;
    std::cout << ((end-start).count()) / 1000000000.0 << " seconds " << std::endl;  // PRINTS SECONDS

    // Join threads here, after the game has ended
    for(int i = 0; i < threads.size(); ++i) {
        (*threads[i]).join();
        delete threads[i];
        // TODO: Close socket as thread is getting deleted; so probably have to keep track of those as well
        
        #ifdef _WIN32
            shutdown(sockets[i], SD_BOTH);
        #else
            shutdown(sockets[i], SHUT_RDWR);
        #endif
    }

    for(int i = 0; i < control_threads.size(); ++i) {
        (*control_threads[i]).join();
        delete control_threads[i];
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

void processConnection(SOCKET_TYPE sock, bool* gameOver, int* activeStation, bool* stationComplete) {
    std::cout << "Thread to process incoming connection" << std::endl;
    char buffer[100];
    int len = recv(sock, buffer, 100, 0);

    if(len == 1) {
        switch(buffer[0]) {
            case 'D':
                std::cout << "I just connected a drone!" << std::endl;
                static int droneID = 0;
                processDrone(sock, gameOver, droneID++);
                break;
            case 'P':
                std::cout << "I just connected a player!" << std::endl;
                processPlayer(sock, gameOver);
                break;
            case 'S':
                std::cout << "I just connected a station!" << std::endl;
                static int stationID = 0;
                processStation(sock, gameOver, activeStation, stationComplete, stationID++);
                break;
            default:
                std::cout << "Unexpected identifier as first message: '" << buffer[0] << "'" << std::endl;
        }
    } else {
        std::cout << "Unexpected byte count from first message: " << len << std::endl;
    }
}

void processDrone(SOCKET_TYPE sock, bool* gameOver, int droneID) {
    // NOTE: Not yet implemented to actually work with tello.
    std::cout << "Drone " << droneID << " Connected" << std::endl;
    char idMessage[] = "TELLO-F1AFF9";
    send(sock, idMessage, strlen(idMessage) + 1, 0);
    while (*gameOver) {
        // Spinlock until gameOver is disabled aka until game has begun
    }

    char takeoff[] = "takeoff";
    char rotate[] = "rotate";
    char somethi[] = "somethi";

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    send(sock, takeoff, strlen(takeoff) + 1, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    send(sock, somethi, strlen(somethi) + 1, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    send(sock, rotate, strlen(rotate) + 1, 0);
}

void processStation(SOCKET_TYPE sock, bool* gameOver, int* activeStation, bool* stationComplete, int stationID) {
    std::cout << "Station " << stationID << " Connected" << std::endl;
    while (*gameOver) {
        // Spinlock until gameOver is disabled aka until game has begun
    }
    int len;
    char buffer[100];
    buffer[0] = 'E';
    while(!*gameOver && *activeStation != stationID) {
        // Spinlock until the active station is this ID, and enable the relevant station when necessary
    }
    send(sock, buffer, 1, 0);
    // Reusing buffer after; note how it's set by recv further down so we don't need to reset it here

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    struct timeval tv;
    tv.tv_usec = 100000;
    
    while(!(*gameOver)) {
        select(sock + 1, &fds, NULL, NULL, &tv);
        if(FD_ISSET(sock, &fds)) {
            len = recv(sock, buffer, 100, 0);
            buffer[len] = '\0';
            if(len % 3 != 0) {
                // TODO: If byte length -1, connection lost, so don't spam this message
                std::cout << "Unexpected byte length(" << len << "), full message is " << buffer << std::endl;
            } else {
                for(int i = 0; i < len - 2; i += 3) {
                    // std::cout << "Station " << stationID << "\tB1: " << buffer[i] << "\tB2: " << buffer[i+1] << "\tB3: " << buffer[i+2] << std::endl;
                    if(buffer[i] == '1' && buffer[i+1] == '1' && buffer[i+2] == '1') {
                        std::cout << "ALL BUTTONS PRESSED!  :)" << std::endl;
                        stationComplete[stationID] = true; // Letting the head thread know that this station has completed its task
                        
                        while(stationComplete[stationID]) {
                            // First spinlock until head thread gest the chance to disable stationComplete (and modify activeStation so this isn't a passthrough)
                        }
                        
                        while(!*gameOver && *activeStation != stationID) {
                            // Spinlock until the active station is this ID, (waiting for head thread to tell me it's time to roll!)
                        }
                        buffer[0] = 'E';
                        send(sock, buffer, 1, 0);
                    }
                }
            }
        } else {
            FD_SET(sock, &fds);
        }
    }
}

void processPlayer(SOCKET_TYPE sock, bool* gameOver) {
    // NOTE: Not yet implemented to work with player vests because we haven't yet built them lol
}

void stationControl(bool* gameOver, int* activeStation, bool* stationComplete) {
    std::cout << "stationControl READY!" << std::endl;
    while (*gameOver) {
        // Spinlock until gameOver is disabled aka until game has begun
    }

    for(int currentCycle = 1; currentCycle <= NUM_CYCLES; ++currentCycle) {
        std::cout << "Current Cycle: " << currentCycle << "/" << NUM_CYCLES << std::endl;
        *activeStation = 0;
        while(*activeStation < NUM_STATIONS) {
        // NOTE: This loop does the job, but it seems a little less clean
            std::cout << "Active station: " << *activeStation << std::endl;
            while(stationComplete[*activeStation] == false) {
                // Spinlock until activeStation is properly pressed
            }
            *activeStation = *activeStation + 1;
            stationComplete[*activeStation-1] = false; // Signaling to thread that activestation has been changed
        }
    }

    *gameOver = true;
}

void droneControl(bool* gameOver) {
    std::cout << "droneControl READY!" << std::endl;
    while (*gameOver) {
        // Spinlock until gameOver is disabled aka until game has begun
    }

    // TODO: Somehow swarm the drones
}

void playerControl(bool* gameOver) {
    std::cout << "playerControl READY!" << std::endl;
    while (*gameOver) {
        // Spinlock until gameOver is disabled aka until game has begun
    }
}