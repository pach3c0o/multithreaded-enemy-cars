#include <chrono>
#include <cstdlib>
#include "car_manager.h"

const int SPAWN_INTERVAL_MS = 1200;
const int SPEED_MIN = 3;
const int SPEED_RANGE = 4;  // car speed = SPEED_MIN .. SPEED_MIN + SPEED_RANGE - 1 (px/step)
const int STEP_MS = 16; 

CarManager::CarManager(int windowHeight) {
    this->windowHeight = windowHeight;
    this->lastSpawnMs = 0;
    this->stopFlag = false;
}

CarManager::~CarManager() {
    stop();

    for (int i = 0; i < (int)cars.size(); i++) {
        delete cars[i];
    }
}

// Design 3: spawn/cleanup stay on one thread; one more thread per car
// variant moves only the cars of its own variant.
void CarManager::start() {
    spawnThread = std::thread(&CarManager::spawnLoop, this);

    for (int variant = 0; variant < NUM_VARIANTS; variant++) {
        moveThreads[variant] = std::thread(&CarManager::moveLoop, this, variant);
    }
}

void CarManager::stop() {
    carMutex.lock();
    stopFlag = true;
    carMutex.unlock();

    if (spawnThread.joinable()) {
        spawnThread.join();
    }
    for (int variant = 0; variant < NUM_VARIANTS; variant++) {
        if (moveThreads[variant].joinable()) {
            moveThreads[variant].join();
        }
    }
}


void CarManager::spawnLoop() {
      carMutex.lock();
      bool stop = stopFlag;
      carMutex.unlock();

      while (stop == false) {
          spawnCars();
          removeFinishedCars();

          std::this_thread::sleep_for(std::chrono::milliseconds(STEP_MS));

          carMutex.lock();
          stop = stopFlag;
          carMutex.unlock();
      }
}


void CarManager::moveLoop(int variant) {
      carMutex.lock();
      bool stop = stopFlag;
      carMutex.unlock();
  
      while (stop == false) {
          moveVariant(variant);

          std::this_thread::sleep_for(std::chrono::milliseconds(STEP_MS));

          carMutex.lock();
          stop = stopFlag;
          carMutex.unlock();
      }
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

void CarManager::spawnCars() {
    long now = nowMs();
    if (now - lastSpawnMs >= SPAWN_INTERVAL_MS) {
        createCar();
        lastSpawnMs = now;
    }
}

void CarManager::createCar() {
    carMutex.lock();

    int lane = pickFreeLane();
    if (lane >= 0) {
        int speed = SPEED_MIN + (rand() % SPEED_RANGE);
        EnemyCar* car = new EnemyCar(lane, speed);
        cars.push_back(car);
    }

    carMutex.unlock();
}


void CarManager::moveVariant(int variant) {
    carMutex.lock();

    for (int i = 0; i < (int)cars.size(); i++) {
        EnemyCar* car = cars[i];
        bool mine = car->getVariant() == variant && car->isFinished() == false;

        if (mine == true) {
            bool blocked = false;
            for (int j = 0; j < (int)cars.size(); j++) {
                if (blocked == false) {
                    EnemyCar* other = cars[j];
                    if (other != car && other->getLane() == car->getLane()) {
                        int gap = other->getY() - car->getY();
                        if (gap > 0 && gap < CAR_GAP) {
                            blocked = true;
                        }
                    }
                }
            }

            if (blocked == false) {
                car->moveForward();
            }

            if (car->getY() >= windowHeight) {
                car->setFinished();
            }
        }
    }

    carMutex.unlock();
}

void CarManager::removeFinishedCars() {
    std::vector<EnemyCar*> doneCars;

    carMutex.lock();

    int i = 0;
    while (i < (int)cars.size()) {
        if (cars[i]->isFinished() == true) {
            doneCars.push_back(cars[i]);
            cars.erase(cars.begin() + i);
        }
        else {
            i = i + 1;
        }
    }

    carMutex.unlock();

    for (int j = 0; j < (int)doneCars.size(); j++) {
        delete doneCars[j];
    }
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
