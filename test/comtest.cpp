#include <arpa/inet.h>
#include <ncurses.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

const int numClients = 1;
string path;

void progressBar(const long &prog, const long &total) {
    int y, x;
    getyx(stdscr, y, x);
    const int len = 30;
    int percent = ceil(float(len) / total * prog);
    wchar_t str[len + 1];
    for (int i = 0; i < percent; i++) str[i] = L'\u2588';
    for (int i = percent; i < len; i++) str[i] = L'\u2592';
    str[len] = L'\0';
    mvprintw(y, 0, "|%ls|", str);
    if (prog == total) addch('\n');
    refresh();
}

class client {
   public:
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
    } sock;

    void testServer() {
        string msg = sock.recv();
        printw("%s\n", msg.c_str());
        sock.send(msg);
        sock.recvFile("/home/jomm/Documents/kurwa/client/test/Asakusa1.png");
        sock.sendFile("/home/jomm/Documents/kurwa/client/test/Asakusa1.png");
    }
    void testClient() {
        string msg = "Hello, world!";
        sock.send(msg);
        msg = "";
        msg = sock.recv();
        printw("%s\n", msg.c_str());
        sock.sendFile("/home/jomm/Documents/kurwa/client/test/Asakusa.png",
                      progressBar);
        sock.recvFile("/home/jomm/Documents/kurwa/client/test/Asakusa2.png",
                      progressBar);
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

    SSL_CTX_use_certificate_file(ctx, (path + "cert.pem").c_str(),
                                 SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, (path + "key.pem").c_str(),
                                SSL_FILETYPE_PEM);

    int servSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(8080);
    servAddr.sin_addr.s_addr = INADDR_ANY;
    socklen_t addrLen = sizeof(servAddr);
    bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr));

    listen(servSock, 5);
    for (int i = 0; i < numClients; i++)
        thread(handleClient,
               client(accept(servSock, (sockaddr *)&servAddr, &addrLen),
                      SSL_new(ctx)))
            .detach();

    SSL_CTX_free(ctx);
    close(servSock);
}

void clientSock() {
    SSL_library_init();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

    SSL_CTX_load_verify_locations(ctx, (path + "cert.pem").c_str(), NULL);

    client client(SSL_new(ctx));

    client.testClient();

    client.sock.close();
    SSL_CTX_free(ctx);
}

int main(int argc, char *argv[]) {
    path = fs::canonical(argv[0]).parent_path().string() + '/';

    setlocale(LC_ALL, "");
    initscr();
    scrollok(stdscr, TRUE);

    thread serverThread(serverSock);
    this_thread::sleep_for(chrono::seconds(1));

    vector<thread> clientThreads;
    for (int i = 0; i < numClients; i++)
        clientThreads.push_back(thread(clientSock));

    serverThread.join();
    for (thread &t : clientThreads) t.join();

    getch();
    endwin();
}
