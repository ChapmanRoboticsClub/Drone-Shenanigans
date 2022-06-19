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

    // Step 1: Create a socket
    SOCKET_TYPE sock = socket(AF_INET , SOCK_STREAM , 0);

    // Step 2: Set the destination information
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(struct sockaddr_in));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr("127.0.0.1");
	dest.sin_port = htons(9090);

    // Step 3: Connect to the server
	connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));

    // Step 4: Tell server that I'm a station
    char message = 'S';
    send(sock, &message, 1, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    bool b1 = false;
    bool b2 = false;
    bool b3 = false;    
    char buffer[3];
    bool enabled = false;
    int len;

    len = recv(sock, buffer, 100, 0);
    if (len != 1) {
        std::cout << "ERROR Receiving Enable" << std::endl;
    }
    enabled = true;

    char input = 't';
    while(true) {
        if(enabled) {
            sprintf(buffer, "%d%d%d", b1, b2, b3);
            len = send(sock, buffer, 3, 0);

            if (len == -1) {
                std::cout << "Error sending current state to server" << std::endl;
            }
            std::cout << "Current State: " << b1 << b2 << b3 << std::endl;
            
            if (b1 && b2 && b3) {
                enabled = false;
                b1 = false;
                b2 = false;
                b3 = false;
                continue;
            }

            std::cout << "Which button would you like to change (1/2/3) or quit (q)?\n" << std::endl; 
            std::cin >> input;
            if (input == 'q' || input == 'Q') {
                break;
            } else if (input == '1') {
                b1 = !b1;
            } else if (input == '2') {
                b2 = !b2;
            } else if (input == '3') {
                b3 = !b3;
            } else {
                std::cerr << "ERROR: Unknown character '" << input << "'" << std::endl;
            }

        } else {
            std::cout << "Disabled until enable sent through" << std::endl;
            len = recv(sock, buffer, 100, 0);
            if (len != 1) {
                std::cout << "ERROR Receiving Enable" << std::endl;
            }
            enabled = true;
        }
    }
}