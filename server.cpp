#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdio>

int main() {
    int servSock = socket(AF_INET, SOCK_STREAM, 0), clientSock, opt = 1;
    setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
               sizeof(opt));
    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(8080);
    socklen_t addrLen = sizeof(servAddr);
    bind(servSock, (sockaddr*)&servAddr, sizeof(servAddr));
    listen(servSock, 5);
    clientSock = accept(servSock, (sockaddr*)&servAddr, &addrLen);
    char buffer[1024];
    recv(clientSock, &buffer, sizeof(buffer), 0);
    char msg[1024];
    snprintf(msg, sizeof(msg), "Hello, %s!!!", buffer);
    send(clientSock, &msg, sizeof(msg), 0);
    close(servSock);
}
