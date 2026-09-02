#include <chrono>
#include <cstdlib>
#include "car_manager.h"

const int SPAWN_INTERVAL_MS = 1200;
const int SPEED_MIN = 3;
const int SPEED_RANGE = 4;   // car speed = SPEED_MIN .. SPEED_MIN + SPEED_RANGE - 1 (px/step)

CarManager::CarManager(int windowHeight) {
    this->windowHeight = windowHeight;
    this->lastSpawnMs = 0;
}

long CarManager::nowMs() {
    auto since = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(since).count();
}

int CarManager::pickFreeLane() {
    int freeLanes[NUM_LANES];
    int freeCount = 0;

    for (int lane = 0; lane < NUM_LANES; lane++) {
        bool occupied = false;
        for (int i = 0; i < (int)cars.size(); i++) {
            if (cars[i]->getLane() == lane && cars[i]->getY() < CAR_GAP) {
                occupied = true;
            }
        }
        if (occupied == false) {
            freeLanes[freeCount] = lane;
            freeCount = freeCount + 1;
        }
    }

    int chosen = -1;
    if (freeCount > 0) {
        chosen = freeLanes[rand() % freeCount];
    }
    return chosen;
}

void CarManager::createCar() {
    carMutex.lock();

    int lane = pickFreeLane();
    if (lane >= 0) {
        int speed = SPEED_MIN + (rand() % SPEED_RANGE);
        EnemyCar* car = new EnemyCar(lane, speed);
        cars.push_back(car);

        std::thread carThread(&EnemyCar::run, car, this, windowHeight);
        threads.push_back(std::move(carThread));
    }

    carMutex.unlock();
}

void CarManager::spawnCars() {
    long now = nowMs();
    if (now - lastSpawnMs >= SPAWN_INTERVAL_MS) {
        createCar();
        lastSpawnMs = now;
    }
}

void CarManager::removeFinishedCars() {
    std::vector<std::thread> doneThreads;
    std::vector<EnemyCar*> doneCars;

    carMutex.lock();

    int i = 0;
    while (i < (int)cars.size()) {
        if (cars[i]->isFinished() == true) {
            doneThreads.push_back(std::move(threads[i]));
            doneCars.push_back(cars[i]);
            cars.erase(cars.begin() + i);
            threads.erase(threads.begin() + i);
        }
        else {
            i = i + 1;
        }
    }

    carMutex.unlock();

    for (int j = 0; j < (int)doneThreads.size(); j++) {
        doneThreads[j].join();
        delete doneCars[j];
    }
}

bool CarManager::updateCar(EnemyCar* car, int yLimit) {
    carMutex.lock();

    bool blocked = false;
    for (int i = 0; i < (int)cars.size(); i++) {
        EnemyCar* other = cars[i];
        if (other != car && other->getLane() == car->getLane()) {
            int gap = other->getY() - car->getY();
            if (gap > 0 && gap < CAR_GAP) {
                blocked = true;
            }
        }
    }

    if (blocked == false) {
        car->moveForward();
    }

    bool reachedEnd = false;
    if (car->getY() >= yLimit) {
        reachedEnd = true;
    }

    carMutex.unlock();

    if (reachedEnd == true) {
        car->setFinished();
    }
    return reachedEnd;
}

std::vector<CarState> CarManager::getSnapshot() {
    carMutex.lock();

    std::vector<CarState> snapshot;
    for (int i = 0; i < (int)cars.size(); i++) {
        CarState state;
        state.id = cars[i]->getId();
        state.lane = cars[i]->getLane();
        state.y = cars[i]->getY();
        state.variant = cars[i]->getVariant();
        snapshot.push_back(state);
    }

    carMutex.unlock();
    return snapshot;
}
