#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "ws_server.h"

// Some platforms (macOS) do not define MSG_NOSIGNAL. On Linux (the Docker image)
// it exists and stops send() from raising SIGPIPE when a client disconnects.
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

// ---------------------------------------------------------------------------
// Handshake helpers (SHA1 + base64). This is standard WebSocket boilerplate:
// the server must answer the client key with base64(sha1(key + magic guid)).
// ---------------------------------------------------------------------------

static void sha1(const unsigned char* data, size_t len, unsigned char out[20]) {
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    size_t paddedLen = ((len + 8) / 64 + 1) * 64;
    std::vector<unsigned char> msg(paddedLen, 0);
    memcpy(msg.data(), data, len);
    msg[len] = 0x80;

    uint64_t bitLen = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) {
        msg[paddedLen - 1 - i] = (unsigned char)(bitLen >> (8 * i));
    }

    for (size_t block = 0; block < paddedLen; block += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)msg[block + 4 * i] << 24) |
                   ((uint32_t)msg[block + 4 * i + 1] << 16) |
                   ((uint32_t)msg[block + 4 * i + 2] << 8) |
                   ((uint32_t)msg[block + 4 * i + 3]);
        }
        for (int i = 16; i < 80; i++) {
            uint32_t v = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (v << 1) | (v >> 31);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;

        for (int i = 0; i < 80; i++) {
            uint32_t f = 0;
            uint32_t k = 0;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            }
            else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t tmp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = tmp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    uint32_t parts[5] = { h0, h1, h2, h3, h4 };
    for (int i = 0; i < 5; i++) {
        out[4 * i] = (unsigned char)(parts[i] >> 24);
        out[4 * i + 1] = (unsigned char)(parts[i] >> 16);
        out[4 * i + 2] = (unsigned char)(parts[i] >> 8);
        out[4 * i + 3] = (unsigned char)(parts[i]);
    }
}

static std::string base64(const unsigned char* data, size_t len) {
    const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = (uint32_t)data[i] << 16;
        if (i + 1 < len) {
            n = n | ((uint32_t)data[i + 1] << 8);
        }
        if (i + 2 < len) {
            n = n | (uint32_t)data[i + 2];
        }
        out += table[(n >> 18) & 63];
        out += table[(n >> 12) & 63];
        out += (i + 1 < len) ? table[(n >> 6) & 63] : '=';
        out += (i + 2 < len) ? table[n & 63] : '=';
    }
    return out;
}

static bool doHandshake(int fd) {
    char buffer[2048];
    int n = recv(fd, buffer, sizeof(buffer) - 1, 0);
    bool ok = false;

    if (n > 0) {
        buffer[n] = '\0';
        std::string request(buffer);
        std::string header = "Sec-WebSocket-Key: ";
        size_t pos = request.find(header);

        if (pos != std::string::npos) {
            size_t start = pos + header.size();
            size_t end = request.find("\r\n", start);
            std::string key = request.substr(start, end - start);

            std::string toHash = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            unsigned char hash[20];
            sha1((const unsigned char*)toHash.c_str(), toHash.size(), hash);
            std::string accept = base64(hash, 20);

            std::string response =
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";

            send(fd, response.c_str(), response.size(), MSG_NOSIGNAL);
            ok = true;
        }
    }
    return ok;
}

static std::string encodeTextFrame(const std::string& payload) {
    std::string frame;
    frame += (char)0x81;   // FIN bit + text opcode

    size_t len = payload.size();
    if (len < 126) {
        frame += (char)len;
    }
    else if (len < 65536) {
        frame += (char)126;
        frame += (char)((len >> 8) & 0xFF);
        frame += (char)(len & 0xFF);
    }
    else {
        frame += (char)127;
        for (int i = 7; i >= 0; i--) {
            frame += (char)((len >> (8 * i)) & 0xFF);
        }
    }

    frame += payload;
    return frame;
}

// ---------------------------------------------------------------------------
// WsServer
// ---------------------------------------------------------------------------

WsServer::WsServer(int port) {
    listenFd = socket(AF_INET, SOCK_STREAM, 0);

    int reuse = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    int bound = bind(listenFd, (struct sockaddr*)&address, sizeof(address));
    if (bound < 0) {
        std::cerr << "[ws] could not bind port " << port << std::endl;
        exit(1);
    }

    int listening = listen(listenFd, 8);
    if (listening < 0) {
        std::cerr << "[ws] could not listen on port " << port << std::endl;
        exit(1);
    }

    std::cout << "[ws] listening on port " << port << std::endl;
}

void WsServer::start() {
    acceptThread = std::thread(&WsServer::acceptLoop, this);
}

void WsServer::acceptLoop() {
    while (true) {
        int clientFd = accept(listenFd, NULL, NULL);
        if (clientFd >= 0) {
            bool ok = doHandshake(clientFd);
            if (ok == true) {
                clientsMutex.lock();
                clients.push_back(clientFd);
                clientsMutex.unlock();
                std::cout << "[ws] client connected" << std::endl;
            }
            else {
                close(clientFd);
            }
        }
    }
}

void WsServer::broadcast(const std::string& text) {
    std::string frame = encodeTextFrame(text);

    clientsMutex.lock();
    int i = 0;
    while (i < (int)clients.size()) {
        int fd = clients[i];
        int sent = send(fd, frame.c_str(), frame.size(), MSG_NOSIGNAL);
        if (sent < 0) {
            close(fd);
            clients.erase(clients.begin() + i);
            std::cout << "[ws] client disconnected" << std::endl;
        }
        else {
            i = i + 1;
        }
    }
    clientsMutex.unlock();
}
