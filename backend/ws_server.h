#ifndef WS_SERVER_H
#define WS_SERVER_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>

class WsServer {
    private:
    int listenFd;
    std::vector<int> clients;
    std::mutex clientsMutex;
    std::thread acceptThread;

    void acceptLoop();

    public:
    WsServer(int port);
    void start();
    void broadcast(const std::string& text);
};

#endif
