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
    std::vector<std::thread> threads;
    int windowHeight;
    long lastSpawnMs;

    int pickFreeLane();                  
    long nowMs();

    public:
    CarManager(int windowHeight);

    void createCar();
    void spawnCars();
    void removeFinishedCars();
    bool updateCar(EnemyCar* car, int yLimit);   
    std::vector<CarState> getSnapshot();
};

#endif
