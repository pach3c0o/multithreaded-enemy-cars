#include <thread>
#include <chrono>
#include <cstdlib>
#include "car.h"
#include "car_manager.h"

int EnemyCar::nextId = 1;

EnemyCar::EnemyCar(int lane, int speed) {
    this->id = nextId;
    nextId = nextId + 1;

    this->lane = lane;
    this->speed = speed;
    this->y = 0;
    this->variant = rand() % NUM_VARIANTS;
    this->finished = false;
}

int EnemyCar::getId() {
    return this->id;
}

int EnemyCar::getLane() {
    return this->lane;
}

int EnemyCar::getY() {
    return this->y;
}

int EnemyCar::getSpeed() {
    return this->speed;
}

int EnemyCar::getVariant() {
    return this->variant;
}

bool EnemyCar::isFinished() {
    std::unique_lock<std::mutex> lock(this->finishedMutex);
    bool value = this->finished;
    lock.unlock();
    return value;
}

void EnemyCar::setFinished() {
    std::unique_lock<std::mutex> lock(this->finishedMutex);
    this->finished = true;
    lock.unlock();
}

void EnemyCar::moveForward() {
    this->y = this->y + this->speed;
}

void EnemyCar::run(CarManager* manager, int yLimit) {
    bool done = false;
    while (done == false) {
        done = manager->updateCar(this, yLimit);
        if (done == false) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
}
