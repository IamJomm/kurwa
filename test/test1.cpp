#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;

class client {
   public:
    class clsSock {
       public:
        int sock;
        SSL *ssl;

        void send(const string &msg) {
            short msgSize = msg.size();
            //::send(sock, &msgSize, sizeof(msgSize), 0);
            SSL_write(ssl, &msgSize, sizeof(msgSize));
            //::send(sock, msg.c_str(), msgSize, 0);
            SSL_write(ssl, msg.c_str(), msgSize);
        }
        string recv() {
            short msgSize;
            //::recv(sock, (char *)&msgSize, sizeof(msgSize), 0);
            SSL_read(ssl, (char *)&msgSize, sizeof(msgSize));
            string res;
            char buffer[1024];
            while (msgSize) {
                memset(buffer, 0, sizeof(buffer));
                /* msgSize -= ::recv(sock, buffer,
                                  min((short)sizeof(buffer), msgSize), 0); */
                msgSize -=
                    SSL_read(ssl, buffer, min((short)sizeof(buffer), msgSize));
                res.append(buffer);
            }
            return res;
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
        clsSock(SSL *ssl) : sock(socket(AF_INET, SOCK_STREAM, 0)), ssl(ssl) {}
    } sock;

    void testServer() {
        string msg = sock.recv();
        cout << msg << endl;
        sock.send(msg);
    }
    void testClient() {
        string msg = "Hello, world!";
        sock.send(msg);
        msg = "";
        msg = sock.recv();
        cout << msg << endl;
    }

    client(SSL *ssl) : sock(ssl) {}
    client(int sock, SSL *ssl) : sock(sock, ssl) {}
};

void handleClient(client client) {
    client.testServer();
    client.sock.close();
}
void serverSock() {
    SSL_library_init();
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());

    SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(server_fd, 5);
    while (true) {
        client client(accept(server_fd, NULL, NULL), SSL_new(ctx));
        thread(handleClient, client).detach();
    }

    SSL_CTX_free(ctx);
}

void clientSock() {
    SSL_library_init();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

    SSL_CTX_load_verify_locations(ctx, "server.crt", NULL);

    client client(SSL_new(ctx));
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    connect(client.sock.sock, (struct sockaddr *)&server_addr,
            sizeof(server_addr));

    SSL_set_fd(client.sock.ssl, client.sock.sock);
    SSL_connect(client.sock.ssl);

    client.testClient();

    client.sock.close();
    SSL_CTX_free(ctx);
}

int main() {
    thread serverThread(serverSock);
    this_thread::sleep_for(chrono::seconds(1));

    int numClients = 1;
    vector<thread> clientThreads;
    for (int i = 0; i < numClients; i++)
        clientThreads.push_back(thread(clientSock));

    serverThread.join();
    for (auto &t : clientThreads) t.join();
}
