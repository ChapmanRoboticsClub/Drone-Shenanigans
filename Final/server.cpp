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
void processDrone(SOCKET_TYPE sock, bool* gameOver);
void processPlayer(SOCKET_TYPE sock, bool* gameOver);
void processStation(SOCKET_TYPE sock, bool* gameOver, int* activeStation, bool* stationComplete, int stationID);

void stationControl(bool* gameOver, int* activeStation, bool* stationComplete);
void playerControl();
void droneControl();

#define NUM_DRONES 0
#define NUM_STATIONS 2
#define NUM_PLAYERS 0

#define NUM_CYCLES 3 // Not yet implemented, but total cycles of all stations required to end the game

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
    bool gameOver = true;
    int activeStation = 0;
    int currentCycle = 0;
    bool stationComplete[NUM_STATIONS];
    for(int i = 0; i < NUM_STATIONS; ++i) {
        stationComplete[i] = false;
    }

    // NOTE: Hard coded to look for exactly a certain number of connections; in the future could be modified to be an open lobby of connecting devices
    for(int i = 0; i < NUM_DRONES + NUM_PLAYERS + NUM_STATIONS; ++i) {
        // Have threads figure out whether sender is Player, Station, or Drone on connect.  Then, process separately in separate functions dependent on that (Switch statement!)
        // Accept a connection request
        socklen_t sen_len = sizeof(sen_addr);
        SOCKET_TYPE newsock = accept(listeningSocket, (struct sockaddr *)&sen_addr, &sen_len);
        threads.push_back(new std::thread(processConnection, newsock, &gameOver, &activeStation, stationComplete));
        sockets.push_back(newsock);
    }

    threads.push_back(new std::thread(stationControl, &gameOver, &activeStation, stationComplete));

    // Sleeping for 1 sec to ensure press enter comes at the bottom
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // TODO: Need synchronization on game start so first station can't be pressed early.
    // Game Start!
    std::cout << "Game Start!" << std::endl;
    gameOver = false;

    // TODO: Stopwatch start here

    while(!gameOver) {
        // spinlock until stationControl calls the game to a close
    }

    // TODO: Stopwatch end here, display time

    std::cout << "Game Over!" << std::endl;

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
                processDrone(sock, gameOver);
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

void processDrone(SOCKET_TYPE sock, bool* gameOver) {
    // NOTE: Not yet implemented to actually work with tello.  Commented out below is original sim test code
    /* int len;
    char buffer[100];
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    struct timeval tv;
    tv.tv_usec = 100000;
    
    while(!(*gameOver)) {
        // Spin lock; will block on recv
        select(sock + 1, &fds, NULL, NULL, &tv);
        if(FD_ISSET(sock, &fds)) {
            len = recv(sock, buffer, 100, 0);
            std::cout << "Received message (" << len << "): " << buffer << std::endl;
        } else {
            FD_SET(sock, &fds);
        }
    } */
}

// TODO: Instead of processing everything in processStation, have processStation modify array of booleans simply stating "done" or "not done".  Do logic in main thread
void processStation(SOCKET_TYPE sock, bool* gameOver, int* activeStation, bool* stationComplete, int stationID) {
    std::cout << "Station ID Connected is: " << stationID << std::endl;
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
                    std::cout << "Station " << stationID << "\tB1: " << buffer[i] << "\tB2: " << buffer[i+1] << "\tB3: " << buffer[i+2] << std::endl;
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
    
    /* while(!(*gameOver)) {
        // Spin lock
        char message[100] = "WAITING...";
        send(sock, message, strlen(message) + 1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } */
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
            std::cout << "Active station: " << *activeStation << std::endl;
        // NOTE: This loop does the job, but it seems a little less clean
            while(stationComplete[*activeStation] == false) {
                // Spinlock until activeStation is properly pressed
                // NOTE: When implmenting drones/players, we'll probably want 3 master threads, one for all drones, one for all stations, and one for all players
            }
            *activeStation = *activeStation + 1;
            stationComplete[*activeStation-1] = false; // Signaling to thread that activestation has been changed
        }
    }

    *gameOver = true;
}
