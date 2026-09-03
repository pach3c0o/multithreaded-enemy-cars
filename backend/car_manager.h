#ifndef CAR_MANAGER_H
#define CAR_MANAGER_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include "car.h"

const int POOL_SIZE = 4;

struct CarState {
    int id;
    int lane;
    int y;
    int variant;
};

class CarManager {
    private:
    std::mutex carMutex;    
    std::mutex queueMutex; 
    std::vector<EnemyCar*> cars;
    std::queue<EnemyCar*> taskQueue;
    int pendingTasks;  

    std::thread producerThread;
    std::thread workerThreads[POOL_SIZE];
    bool stopFlag;
    int windowHeight;
    long lastSpawnMs;

    int pickFreeLane();
    long nowMs();

    void producerLoop();
    void spawnCars();
    void createCar();
    void removeFinishedCars();
    void enqueueMoveTasks();
    void waitUntilTasksDone();

    void workerLoop();
    void moveCar(EnemyCar* car);

    public:
    CarManager(int windowHeight);
    ~CarManager();

    void start();
    void stop();
    std::vector<CarState> getSnapshot();
};

#endif
