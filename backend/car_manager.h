#ifndef CAR_MANAGER_H
#define CAR_MANAGER_H

#include <vector>
#include <thread>
#include <mutex>
#include "car.h"


struct CarState {
    int id;
    int lane;
    int y;
    int variant;
};

class CarManager {
    private:
    std::mutex carMutex;
    std::vector<EnemyCar*> cars;
    std::thread updateThread;
    bool stopFlag;
    int windowHeight;
    long lastSpawnMs;

    int pickFreeLane();
    long nowMs();

    void updateLoop();
    void spawnCars();
    void createCar();
    void updateAll();
    void removeFinishedCars();

    public:
    CarManager(int windowHeight);
    ~CarManager();

    void start();
    void stop();
    std::vector<CarState> getSnapshot();
};

#endif
