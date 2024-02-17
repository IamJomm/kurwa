#include <sys/socket.h>

#include <string>

using std::string, std::to_string, std::stoi, std::min;

class clsSock {
   public:
    int sock;
    void send(const string& msg) {
        int msgSize = msg.size();
        const char* bufferSize = to_string(msgSize).c_str();
        ::send(sock, bufferSize, 1024, 0);
        const char* buffer = msg.c_str();
        ::send(sock, buffer, msgSize, 0);
    }
    string recv() {
        string res;
        int msgSize;
        char buffer[1024];
        ::recv(sock, buffer, 1024, 0);
        msgSize = stoi(buffer);
        while (msgSize != 0) {
            msgSize -= ::recv(sock, buffer, min(1024, msgSize), 0);
            res.append(buffer);
        }
        return res;
    }
    clsSock() : sock(socket(AF_INET, SOCK_STREAM, 0)) {}
    clsSock(int x) : sock(x) {}
};
