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
#include <chrono>
#include <thread>

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

    // ERROR: Sending two messages together.  Resolved by sleep above.  Can setsockopt to disable packing, but should probably instead embrace fixed size messages

    // Loop and receive packets until FIN is received
    char buffer[100];
    int len;
    fd_set fds;
    FD_ZERO(&fds);
    // What the heck this should be present to add the sock to the fd... but adding it in breaks everything
    FD_SET(sock, &fds);

    struct timeval tv;
    tv.tv_usec = 100000;
    int timer = 0;
    while(true) {
        // Select to check for FIN
        select(sock + 1, &fds, NULL, NULL, &tv);
        if(FD_ISSET(sock, &fds)) {
            len = recv(sock, buffer, 100, 0);
            if(len == 4 && buffer[0] == 'F' && buffer[1] == 'I' && buffer[2] == 'N') { // Can simplify and look for F
                break;
            } else {
                std::cout << "Received message (" << len << "): " << buffer << std::endl;
            } 
        } else {
            FD_SET(sock, &fds);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        timer = ++timer % 10;
        if(timer == 0) {
            char message[10] = "DroneData";
            send(sock, message, strlen(message) + 1, 0);
        }
    }
    std::cout << "FIN RECEIEVED(" << len << "): " << buffer << std::endl;

    #ifdef _WIN32
    closesocket(sock);
    #else
    close(sock);
    #endif

    std::cout << "Program terminated" << std::endl;
	return 0;
}
