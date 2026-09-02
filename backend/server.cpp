#include <csignal>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>

#include "car_manager.h"
#include "ws_server.h"

const int PORT = 5000;
const int WINDOW_HEIGHT = 840;
const int BROADCAST_MS = 33;


std::string buildJson(const std::vector<CarState>& cars) {
    std::string out = "{\"lanes\":" + std::to_string(NUM_LANES) +
                      ",\"height\":" + std::to_string(WINDOW_HEIGHT) +
                      ",\"cars\":[";
    for (int i = 0; i < (int)cars.size(); i++) {
        if (i > 0) {
            out = out + ",";
        }
        out = out + "{\"id\":" + std::to_string(cars[i].id) +
              ",\"lane\":" + std::to_string(cars[i].lane) +
              ",\"y\":" + std::to_string(cars[i].y) +
              ",\"variant\":" + std::to_string(cars[i].variant) + "}";
    }
    out = out + "]}";
    return out;
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    srand((unsigned int)time(NULL));

    CarManager manager(WINDOW_HEIGHT);
    manager.start();

    WsServer server(PORT);
    server.start();

    while (true) {
        std::vector<CarState> snapshot = manager.getSnapshot();
        std::string message = buildJson(snapshot);
        server.broadcast(message);

        std::this_thread::sleep_for(std::chrono::milliseconds(BROADCAST_MS));
    }

    return 0;
}
