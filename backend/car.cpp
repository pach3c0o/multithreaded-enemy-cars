#include <cstdlib>
#include "car.h"

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

int EnemyCar::getVariant() {
    return this->variant;
}

bool EnemyCar::isFinished() {
    return this->finished;
}

void EnemyCar::setFinished() {
    this->finished = true;
}

void EnemyCar::moveForward() {
    this->y = this->y + this->speed;
}
