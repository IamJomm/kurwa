#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>

using std::string, std::cout, std::endl, std::cin;

int main() {
    int clientSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr.s_addr);
    servAddr.sin_port = htons(8080);
    connect(clientSock, (sockaddr*)&servAddr, sizeof(servAddr));
    char buffer[1024];
    cin >> buffer;
    send(clientSock, &buffer, sizeof(buffer), 0);
    recv(clientSock, &buffer, sizeof(buffer), 0);
    cout << buffer;
    close(clientSock);
}
