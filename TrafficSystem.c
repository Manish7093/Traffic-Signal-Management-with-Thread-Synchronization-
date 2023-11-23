#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

pthread_attr_t attrA, attrB, attrC;
int ret;
int time1 = 50;
int time2 = 6;
int time3 = 1;
struct sched_param Sch1, Sch2, Sch3;

pthread_mutex_t TrafficPolice_mutex_lock;
pthread_cond_t TrafficPolice_Condn;

int Count_Car = 0;
char TrafficDirection[6];

sem_t Semaphore_Car;

struct car {
    int carID;
    char travelDirection;
    struct timespec CarArrived;
    struct timespec CarStarted;
    struct timespec CarStopped;
};

#define MAX_QUEUE_SIZE 100

struct Queue {
    struct car items[MAX_QUEUE_SIZE];
    int front, rear;
};

bool is_empty(struct Queue* queue) {
    return queue->front == -1;
}

bool is_full(struct Queue* queue) {
    return (queue->front == 0 && queue->rear == MAX_QUEUE_SIZE - 1) || (queue->front == queue->rear + 1);
}

void enqueue(struct Queue* queue, struct car item) {
    if (is_full(queue)) {
        fprintf(stderr, "Error: Queue is full.\n");
        exit(EXIT_FAILURE);
    }

    if (is_empty(queue)) {
        queue->front = 0;
        queue->rear = 0;
    } else if (queue->rear == MAX_QUEUE_SIZE - 1) {
        queue->rear = 0;
    } else {
        queue->rear += 1;
    }

    queue->items[queue->rear] = item;
}

struct car dequeue(struct Queue* queue) {
    struct car item;
    if (is_empty(queue)) {
        fprintf(stderr, "Error: Queue is empty.\n");
        exit(EXIT_FAILURE);
    }

    item = queue->items[queue->front];

    if (queue->front == queue->rear) {
        queue->front = -1;
        queue->rear = -1;
    } else if (queue->front == MAX_QUEUE_SIZE - 1) {
        queue->front = 0;
    } else {
        queue->front += 1;
    }

    return item;
}

struct Queue EastBuffer;
struct Queue WestBuffer;

int Sleep_thread(int seconds) {
    pthread_mutex_t mutex_lock;
    pthread_cond_t var;
    struct timespec Rem_time;

    if (pthread_mutex_init(&mutex_lock, NULL)) {
        perror("Error initializing mutex");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&var, NULL)) {
        perror("Error initializing condition variable");
        pthread_mutex_destroy(&mutex_lock);
        exit(EXIT_FAILURE);
    }

    Rem_time.tv_sec = (unsigned int)time(NULL) + seconds;
    Rem_time.tv_nsec = 0;

    int result = pthread_cond_timedwait(&var, &mutex_lock, &Rem_time);

    if (result != 0 && result != ETIMEDOUT) {
        perror("Error in pthread_cond_timedwait");
        pthread_mutex_destroy(&mutex_lock);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_destroy(&mutex_lock);

    return result;
}

void *produceEast(void *args) {
    printf("Producer thread in East direction started\n");
    struct timespec arrival;
    struct car newCar;

    while (1) {
        sem_wait(&Semaphore_Car);
        pthread_mutex_lock(&TrafficPolice_mutex_lock);

        while ((rand() % 10) < 8) {
            if ((rand() % 10) < 5 && !is_full(&EastBuffer)) {
                printf("Producing car in the East direction, LOCKS\n");
                Count_Car++;
                newCar.carID = Count_Car;
                newCar.travelDirection = 'N';
                arrival.tv_sec = (unsigned int)time(NULL);
                arrival.tv_nsec = 0;
                newCar.CarArrived = arrival;
                enqueue(&EastBuffer, newCar);

                int k = EastBuffer.rear - EastBuffer.front + 1;
                printf("Number of cars in EastBuffer: %d\n", k);

                Sleep_thread(1);
            } else {
                printf("Producing car in the West direction, mutex_lock LOCKS\n");
                Count_Car++;
                newCar.carID = Count_Car;
                newCar.travelDirection = 'S';
                arrival.tv_sec = (unsigned int)time(NULL);
                arrival.tv_nsec = 0;
                newCar.CarArrived = arrival;
                enqueue(&WestBuffer, newCar);

                int s = WestBuffer.rear - WestBuffer.front + 1;
                printf("Number of cars in WestBuffer: %d\n", s);

                Sleep_thread(1);
            }
        }

        printf("Sleeping for 20 seconds (East direction)\n");
        Sleep_thread(20);
        pthread_cond_signal(&TrafficPolice_Condn);
        pthread_mutex_unlock(&TrafficPolice_mutex_lock);
        printf("UNLOCKS\n");

        sem_post(&Semaphore_Car);
    }

    return NULL;
}

void ChangeDirection() {
    if (strcmp(TrafficDirection, "East") == 0) {
        strcpy(TrafficDirection, "West");
        printf("Switching traffic direction to West\n");
        printf("New Traffic Direction: %s\n", TrafficDirection);
    } else {
        strcpy(TrafficDirection, "East");
        printf("Switching traffic direction to East\n");
        printf("New Traffic Direction: %s\n", TrafficDirection);
    }
}

void SimulateCar() {
    struct car processedCar;

    FILE *carLog;
    if (strcmp(TrafficDirection, "East") == 0) {
        processedCar = dequeue(&EastBuffer);
        printf("Processing car from the East direction");
        Sleep_thread(1);
    } else {
        processedCar = dequeue(&WestBuffer);
        Sleep_thread(1);
    }

    printf("%-12d%c\n", processedCar.carID, processedCar.travelDirection);

    carLog = fopen("car.log", "a");
    fprintf(carLog, "%-12d%c\n", processedCar.carID, processedCar.travelDirection);
    fclose(carLog);
}

void TrafficPolice_Status() {
    FILE *TrafficPoliceLog;

    while (!is_empty(&EastBuffer) && !is_empty(&WestBuffer)) {
        printf("%-12ld - Traffic police sleeping\n", time(NULL));

        TrafficPoliceLog = fopen("TrafficPolice.log", "a");
        fprintf(TrafficPoliceLog, "%-12ld - Traffic police sleeping\n", time(NULL));
        fclose(TrafficPoliceLog);

        Sleep_thread(1);
        pthread_cond_wait(&TrafficPolice_Condn, &TrafficPolice_mutex_lock);
    }

    printf("%-12ld - Traffic police awake\n", time(NULL));

    TrafficPoliceLog = fopen("TrafficPolice.log", "a");
    fprintf(TrafficPoliceLog, "%-12ld - Traffic police awake\n", time(NULL));
    fclose(TrafficPoliceLog);
}

void *Car_consumer(void *args) {
    while (1) {
        printf("Entered consume function\n");

        pthread_mutex_lock(&TrafficPolice_mutex_lock);

        printf("Consumer not waiting: %s\n", TrafficDirection);

        if (strcmp(TrafficDirection, "East") == 0) {
            if (is_full(&WestBuffer) && !is_full(&EastBuffer)) {
                printf("Ready queue has more than 5 cars in the West. Changing direction to West.\n");
                ChangeDirection();
            } else if (is_empty(&EastBuffer) && is_full(&WestBuffer)) {
                printf("Changing direction to West as East buffer is empty and West buffer has 5 or more cars.\n");
                ChangeDirection();
            } else if (is_empty(&EastBuffer)) {
                printf("East buffer is empty. Initiating TrafficPolice_Status().\n");
                TrafficPolice_Status();
            } else {
                printf("Processing East car.\n");
                SimulateCar();
            }
        } else {
            if (is_full(&EastBuffer) && !is_full(&WestBuffer)) {
                printf("Ready queue has more than 5 cars in the East. Changing direction to East.\n");
                ChangeDirection();
            } else if (is_empty(&WestBuffer) && is_full(&EastBuffer)) {
                printf("Changing direction to East as West buffer is empty and East buffer has 5 or more cars.\n");
                ChangeDirection();
            } else if (is_empty(&WestBuffer)) {
                printf("West buffer is empty. Initiating TrafficPolice_Status().\n");
                TrafficPolice_Status();
            } else {
                printf("Processing West car.\n");
                SimulateCar();
            }
        }

        pthread_mutex_unlock(&TrafficPolice_mutex_lock);
    }

    return NULL;
}

int main() {
    strcpy(TrafficDirection, "East");

    pthread_t sTid, nTid, fTid;
    int pshared = 1;
    int semValue = 1;
    srand(time(NULL));

    FILE *carLogInitial, *TrafficPoliceLogInitial;
    carLogInitial = fopen("car.log", "w");
    fprintf(carLogInitial, "%-12s%-8s%-12s%-12s%-12s\n", "carID", "direction", "arrival-time", "start-time", "end-time");
    fclose(carLogInitial);
    TrafficPoliceLogInitial = fopen("TrafficPolice.log", "w");
    fprintf(TrafficPoliceLogInitial, "%-12s%-8s\n", "time", "state");
    fclose(TrafficPoliceLogInitial);

    if (pthread_mutex_init(&TrafficPolice_mutex_lock, NULL)) {
        perror("Error initializing mutex");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&TrafficPolice_Condn, NULL)) {
        perror("Error initializing condition variable");
        pthread_mutex_destroy(&TrafficPolice_mutex_lock);
        exit(EXIT_FAILURE);
    }
    if (0 != sem_init(&Semaphore_Car, pshared, semValue)) {
        perror("Error initializing semaphore");
        pthread_mutex_destroy(&TrafficPolice_mutex_lock);
        pthread_cond_destroy(&TrafficPolice_Condn);
        exit(EXIT_FAILURE);
    }

    ret = pthread_attr_init(&attrA);
    ret = pthread_attr_getschedparam(&attrA, &Sch1);
    Sch1.sched_priority = time1;

    ret = pthread_attr_init(&attrB);
    ret = pthread_attr_getschedparam(&attrB, &Sch2);
    Sch2.sched_priority = time2;

    ret = pthread_attr_init(&attrC);
    ret = pthread_attr_getschedparam(&attrC, &Sch3);
    Sch3.sched_priority = time3;

    pthread_create(&fTid, &attrA, Car_consumer, NULL);
    pthread_create(&nTid, &attrC, produceEast, NULL);

    while (1) {
        fflush(stdout);
        Sleep_thread(1);
    }

    sem_close(&Semaphore_Car);
    pthread_mutex_destroy(&TrafficPolice_mutex_lock);
    pthread_cond_destroy(&TrafficPolice_Condn);

    return 0;
}
