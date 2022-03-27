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


    // MAKE THREAD HERE!
    // Have threads figure out whether sender is Player, Station, or Drone on connect.  Then, process separately in separate functions dependent on that (Switch statement!)

    // Accept a connection request
	socklen_t sen_len = sizeof(sen_addr);
	SOCKET_TYPE newsock = accept(listeningSocket, (struct sockaddr *)&sen_addr, &sen_len);

    char buffer[100];
    int len = recv(newsock, buffer, 100, 0);

    if(len == 1) {
        switch(buffer[0]) {
            case 'D':
                std::cout << "I just connected a drone!" << std::endl;
                break;
            case 'P':
                std::cout << "I just connected a player!" << std::endl;
                break;
            case 'S':
                std::cout << "I just connected a station!" << std::endl;
                break;
            default:
                std::cout << "Unexpected identifier as first message: '" << buffer[0] << "'" << std::endl;
        }
    } else {
        std::cout << "Unexpected byte count from first message: " << len << std::endl;
    }

    // Game Start!

    // Join threads here, after the game has ended

    // Cleanup and close sockets as necessary
    #ifdef _WIN32
        closesocket(listeningSocket);
        closesocket(newsock);
    #else
        close(listeningSocket);
        close(newsock);
    #endif

    std::cout << "Program terminated" << std::endl;
	return 0;
}
