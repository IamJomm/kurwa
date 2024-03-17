#include <sys/socket.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
using std::string, std::to_string, std::stoi, std::min, std::cout, std::endl,
    std::ifstream, std::ofstream, std::ios;

class clsSock {
   public:
    int sock;

    void send(const string& msg) {
        short msgSize = msg.size();
        ::send(sock, &msgSize, sizeof(msgSize), 0);
        ::send(sock, msg.c_str(), msgSize, 0);
    }
    string recv() {
        string res;
        char buffer[1024];
        ::recv(sock, buffer, sizeof(short), 0);
        short msgSize;
        memcpy(&msgSize, buffer, sizeof(short));
        while (msgSize) {
            memset(buffer, 0, sizeof(buffer));
            msgSize -=
                ::recv(sock, buffer, min((short)sizeof(buffer), msgSize), 0);
            res.append(buffer);
        }
        return res;
    }

    void sendFile(const string& path,
                  void (*callback)(const long&, const long&) = nullptr) {
        ifstream input(path);
        input.seekg(0, ios::end);
        long fileSize = input.tellg();
        input.seekg(0, ios::beg);
        ::send(sock, &fileSize, sizeof(fileSize), 0);
        char buffer[1024];
        long bytesLeft = fileSize;
        while (bytesLeft) {
            short bytesToSend = min(bytesLeft, (long)sizeof(buffer));
            input.read(buffer, bytesToSend);
            ::send(sock, buffer, bytesToSend, 0);
            bytesLeft -= bytesToSend;
            memset(buffer, 0, sizeof(buffer));
            if (callback) callback(fileSize - bytesLeft, fileSize);
        }
    }

    void recvFile(const string& path,
                  void (*callback)(const long&, const long&) = nullptr) {
        ofstream output(path);
        char buffer[1024];
        long fileSize;
        ::recv(sock, (char*)&fileSize, sizeof(fileSize), 0);
        long bytesLeft = fileSize;
        while (bytesLeft) {
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived =
                ::recv(sock, buffer, min(bytesLeft, (long)sizeof(buffer)), 0);
            output.write(buffer, bytesReceived);
            bytesLeft -= bytesReceived;
            if (callback) callback(fileSize - bytesLeft, fileSize);
        }
    }

    clsSock() : sock(socket(AF_INET, SOCK_STREAM, 0)) {}
    clsSock(int x) : sock(x) {}
};
