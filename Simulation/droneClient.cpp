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

    // Step 4: Tell server that I'm a drone
    char message = 'D';
    send(sock, &message, 1, 0);     // Adding 1 to message.length() to allow for the null byte to be sent through
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Loop and receive packets until FIN is received
    char buffer[100];
    int len;

    while(true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        char message[100] = "Drone Data Sent...";
        len = send(sock, message, strlen(message) + 1, 0);
        if(len == -1) {
            std::cout << "Connection failed" << std::endl;
            break;
        }
    }

    #ifdef _WIN32
    closesocket(sock);
    #else
    close(sock);
    #endif

    std::cout << "Program terminated" << std::endl;
	return 0;
}