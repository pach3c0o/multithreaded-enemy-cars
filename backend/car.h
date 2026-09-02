#ifndef CAR_H
#define CAR_H

const int NUM_LANES = 3;
const int NUM_VARIANTS = 5;
const int CAR_GAP = 130;

class EnemyCar {
    private:
    int id;
    int lane;
    int y;
    int speed;
    int variant;

    bool finished;

    static int nextId;

    public:
    EnemyCar(int lane, int speed);

    int getId();
    int getLane();
    int getY();
    int getVariant();

    bool isFinished();
    void setFinished();
    void moveForward();
};

#endif
