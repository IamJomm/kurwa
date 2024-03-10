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
        ::send(sock, to_string(msgSize).c_str(), sizeof(msgSize), 0);
        ::send(sock, msg.c_str(), msgSize, 0);
    }
    string recv() {
        string res;
        short msgSize;
        char buffer[1024];
        ::recv(sock, buffer, sizeof(msgSize), 0);
        msgSize = stoi(buffer);
        while (msgSize) {
            memset(buffer, 0, sizeof(buffer));
            msgSize -= ::recv(sock, buffer, min((short)1024, msgSize), 0);
            res.append(buffer);
        }
        return res;
    }

    void sendFile(const string& path) {
        ifstream input(path);
        input.seekg(0, ios::end);
        long fileSize = input.tellg();
        input.seekg(0, ios::beg);
        ::send(sock, to_string(fileSize).c_str(), sizeof(fileSize), 0);
        char buffer[1024];
        while (input.read(buffer, sizeof(buffer))) {
            ::send(sock, buffer, input.gcount(), 0);
            memset(buffer, 0, sizeof(buffer));
        }
    }
    void recvFile(const string& path) {
        ofstream output(path);
        char buffer[1024];
        ::recv(sock, buffer, sizeof(long), 0);
        long fileSize = stoi(buffer);
        while (fileSize) {
            memset(buffer, 0, sizeof(buffer));
            int bytesRecvd = ::recv(sock, buffer, min((long)1024, fileSize), 0);
            fileSize -= bytesRecvd;
            output.write(buffer, bytesRecvd);
        }
    }

    clsSock() : sock(socket(AF_INET, SOCK_STREAM, 0)) {}
    clsSock(int x) : sock(x) {}
};
