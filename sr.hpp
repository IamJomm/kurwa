#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <unistd.h>

#include <fstream>

using std::string, std::to_string, std::stoi, std::min, std::ifstream,
    std::ofstream, std::ios;

class clsSock {
   public:
    int sock;
    SSL *ssl;

    void send(const string &msg) {
        short msgSize = msg.size();
        SSL_write(ssl, &msgSize, sizeof(msgSize));
        SSL_write(ssl, msg.c_str(), msgSize);
    }
    string recv() {
        short msgSize;
        SSL_read(ssl, (char *)&msgSize, sizeof(msgSize));
        string res;
        char buffer[1024];
        while (msgSize) {
            memset(buffer, 0, sizeof(buffer));
            msgSize -=
                SSL_read(ssl, buffer, min((short)sizeof(buffer), msgSize));
            res.append(buffer);
        }
        return res;
    }

    void sendFile(const string &path,
                  void (*callback)(const long &, const long &) = nullptr) {
        ifstream input(path);
        input.seekg(0, ios::end);
        long fileSize = input.tellg();
        input.seekg(0, ios::beg);
        SSL_write(ssl, &fileSize, sizeof(fileSize));
        char buffer[1024];
        long bytesLeft = fileSize;
        while (bytesLeft) {
            short bytesToSend = min(bytesLeft, (long)sizeof(buffer));
            input.read(buffer, bytesToSend);
            SSL_write(ssl, buffer, bytesToSend);
            bytesLeft -= bytesToSend;
            memset(buffer, 0, sizeof(buffer));
            if (callback) callback(fileSize - bytesLeft, fileSize);
        }
        input.close();
    }
    void recvFile(const string &path,
                  void (*callback)(const long &, const long &) = nullptr) {
        ofstream output(path);
        char buffer[1024];
        long fileSize;
        SSL_read(ssl, (char *)&fileSize, sizeof(fileSize));
        long bytesLeft = fileSize;
        while (bytesLeft) {
            memset(buffer, 0, sizeof(buffer));
            int bytesRecved =
                SSL_read(ssl, buffer, min(bytesLeft, (long)sizeof(buffer)));
            output.write(buffer, bytesRecved);
            bytesLeft -= bytesRecved;
            if (callback) callback(fileSize - bytesLeft, fileSize);
        }
        output.close();
    }

    void close() {
        SSL_shutdown(ssl);
        ::close(sock);
        SSL_free(ssl);
    }
    clsSock(int sock, SSL *ssl) : sock(sock), ssl(ssl) {
        SSL_set_fd(ssl, sock);
        SSL_accept(ssl);
    }
    clsSock(SSL *ssl) : sock(socket(AF_INET, SOCK_STREAM, 0)), ssl(ssl) {
        sockaddr_in servAddr{};
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(8080);
        inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr);
        connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));
        SSL_set_fd(ssl, sock);
        SSL_connect(ssl);
    }
};
