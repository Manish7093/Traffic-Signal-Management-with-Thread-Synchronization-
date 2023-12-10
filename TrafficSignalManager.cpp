#include <stdio.h>
#include <semaphore.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include<cstring>
#include <queue>
#include <iomanip>
#include<sched.h>
using namespace std;

// Thread attributes for different threads A, B, and C
pthread_attr_t attrA, attrB, attrC;

// Return value for pthread_create function
int ret;

// Time variables representing different durations for threads A, B, and C
int time1 = 50; // Time duration for thread A
int time2 = 6;  // Time duration for thread B
int time3 = 1;  // Time duration for thread C

// Scheduler parameters for threads A, B, and C
sched_param Sch1, Sch2, Sch3;

// Initialize mutex and condition variable for traffic police synchronization
pthread_mutex_t TrafficPolice_mutex_lock;
pthread_cond_t TrafficPolice_Condn;

// Keep track of the total number of cars that have been created
int Count_Car = 0;

// Store the direction from which incoming cars are arriving
// The array size is set to 6 to accommodate directions like "north" and "south"
char TrafficDirection[6];

// Semaphore to control access to the shared car resources
sem_t Semaphore_Car;

struct car {
  int carID;                         // Unique identifier for each car
  char travelDirection;               // Direction in which the car is traveling (e.g., 'E' for east, 'W' for west)
  struct timespec CarArrived;         // Time when the car appears on the road
  struct timespec CarStarted;         // Time when the car starts its journey through the construction zone
  struct timespec CarStopped;         // Time when the car completes its journey through the construction zone
};

queue<car> EastBuffer;                // Queue to store cars traveling from the East direction
queue<car> WestBuffer;                // Queue to store cars traveling from the West direction

// Sleeps the calling thread for the specified number of seconds.
// Returns 0 on successful sleep, -1 on error.
int Sleep_thread(int seconds) {
  pthread_mutex_t mutex_lock;  // Mutex variable for thread synchronization
  pthread_cond_t var;           // Condition variable for thread synchronization
  struct timespec Rem_time;     // Remaining time variable for timed wait
  
  // Initialize the mutex
  if (pthread_mutex_init(&mutex_lock, NULL)) {
    return -1;  // Return -1 in case of mutex initialization failure
  }
  
  // Initialize the condition variable
  if (pthread_cond_init(&var, NULL)) {
    return -1;  // Return -1 in case of condition variable initialization failure
  }
  
  // Calculate the absolute time when the thread should wake up
  Rem_time.tv_sec = (unsigned int)time(NULL) + seconds;
  Rem_time.tv_nsec = 0;
  
  // Wait for the specified time or until a signal is received on the condition variable
  // pthread_cond_timedwait suspends the calling thread until either the specified time 
  // elapses or a signal is received on the condition variable
  return pthread_cond_timedwait(&var, &mutex_lock, &Rem_time);
}

// Car producer thread for the East direction
void *produceEast(void *args) {
    cout <<"Producer thread in East direction started" << endl;
    struct timespec arrival;
    struct car newCar;
    while (1) {
        sem_wait(&Semaphore_Car); // Wait for available car slot
        pthread_mutex_lock(&TrafficPolice_mutex_lock); // Lock the traffic police mutex

        // Generate cars with 80% probability for North and 20% probability for South
        while ((rand() % 10) < 8) {
            // Produce car in the East direction
            if ((rand() % 10) < 5 && (EastBuffer.size() < 5)) {
                cout << "Producing car in the East direction, LOCKS" << endl;
                Count_Car++;
                newCar.carID = Count_Car;
                newCar.travelDirection = 'E';
                arrival.tv_sec = (unsigned int)time(NULL);
                arrival.tv_nsec = 0;
                newCar.CarArrived = arrival;
                EastBuffer.push(newCar);

                int k = EastBuffer.size();
                cout << "Number of cars in EastBuffer: " << k << endl;

                Sleep_thread(1); // Simulate car production time
            } 
            else 
            {
                // Produce car in the West direction
                cout << "Producing car in the West direction, mutex_lock LOCKS" << endl;
                Count_Car++;
                newCar.carID = Count_Car;
                newCar.travelDirection = 'W';
                arrival.tv_sec = (unsigned int)time(NULL);
                arrival.tv_nsec = 0;
                newCar.CarArrived = arrival;
                WestBuffer.push(newCar);

                int s = WestBuffer.size();
                cout << "Number of cars in WestBuffer: " << s << endl;

                Sleep_thread(1); // Simulate car production time
            }
        }
        
        cout << "Sleeping for 20 seconds (East direction)" << endl;
        Sleep_thread(20); // Sleep for 20 seconds before producing more cars
        pthread_cond_signal(&TrafficPolice_Condn); // Signal the traffic police thread to resume
        pthread_mutex_unlock(&TrafficPolice_mutex_lock); // Unlock the traffic police mutex
        cout << "UNLOCKS" << endl;

        sem_post(&Semaphore_Car); // Release the car production semaphore
    }

    return 0;
}

// Car producer thread in the West direction
void *produceWest(void *args) {
  cout << "in west " << endl;
  struct timespec arrival;
  struct car newCar;
  while (1) {
    sem_wait(&Semaphore_Car);
    pthread_mutex_lock(&TrafficPolice_mutex_lock);
    while ((rand() % 10) < 8) {
      cout << "Producing car in the west direction, MUTEX LOCKS" << endl;
      Count_Car++;
      newCar.carID = Count_Car;
      newCar.travelDirection = 'W';
      arrival.tv_sec = (unsigned int)time(NULL);
      arrival.tv_nsec = 0;
      newCar.CarArrived = arrival;
      WestBuffer.push(newCar);
      int k = WestBuffer.size();
      cout << "Number of cars in WestBuffer: " << k << endl;
      Sleep_thread(1); // Simulate car production time
    }
    cout << "sleep 20 S" << endl;
    Sleep_thread(5);
    pthread_cond_signal(&TrafficPolice_Condn);
    pthread_mutex_unlock(&TrafficPolice_mutex_lock);
    cout << "UNLOCKS" << endl;
    sem_post(&Semaphore_Car); // Release the car production semaphore
  }
  return 0;
}




// Function to change the direction of traffic allowed through the construction zone
void ChangeDirection() {
  // Check if the current traffic direction is East
  if (strcmp(TrafficDirection, "East") == 0) {
    // If the current direction is East, switch it to West
    strcpy(TrafficDirection, "West");
    cout << "Switching traffic direction to West" << endl;
    cout << "New Traffic Direction: " << TrafficDirection << endl;
  } else {
    // If the current direction is West, switch it to East
    strcpy(TrafficDirection, "East");
    cout << "Switching traffic direction to East" << endl;
    cout << "New Traffic Direction: " << TrafficDirection << endl;
  }
  return;
}

// Removes a car from the queue
// Simulates a car passing through the construction zone
void SimulateCar() {
  struct car processedCar; // Structure to hold information about the processed car

  ofstream carLog; // Output stream for logging car details

  // Check the traffic direction
  if (strcmp(TrafficDirection, "East") == 0) {
    processedCar = EastBuffer.front(); // Retrieve the car from the East queue
    cout << "Processing car from the East direction"; // Display processing message
    Sleep_thread(1); // Simulate processing time
    EastBuffer.pop(); // Remove the car from the East queue

  } else {
    processedCar = WestBuffer.front(); // Retrieve the car from the West queue
    Sleep_thread(1); // Simulate processing time
    WestBuffer.pop(); // Remove the car from the West queue
  }

  // Display processed car details on the console
  cout << left << setw(12) << processedCar.carID << processedCar.travelDirection << endl;

  carLog.open("car.log", ios_base::app); // Open the log file in append mode
  // Log processed car details in the car.log file
  carLog << left << setw(12) << processedCar.carID << processedCar.travelDirection << "\n";
  carLog.close(); // Close the log file
  return;
}

// Determines whether the traffic police officer should be awake or asleep
void TrafficPolice_Status() {
  ofstream TrafficPoliceLog;

  // Keep checking both East and West buffers until both are empty
  while (!EastBuffer.empty() && !WestBuffer.empty()) {
    cout << left << setw(12) << time(NULL) << " - Traffic police sleeping" << endl;

    // Log the sleeping state to the TrafficPolice.log file
    TrafficPoliceLog.open("TrafficPolice.log", ios_base::app);
    TrafficPoliceLog << left << setw(12) << time(NULL) << " - Traffic police sleeping\n";
    TrafficPoliceLog.close();

    Sleep_thread(1); // Sleep for 1 second
    pthread_cond_wait(&TrafficPolice_Condn, &TrafficPolice_mutex_lock); // Wait for condition signal to wake up
  }

  // Traffic police officer is awake
  cout << left << setw(12) << time(NULL) << " - Traffic police awake" << endl;

  // Log the waking up state to the TrafficPolice.log file
  TrafficPoliceLog.open("TrafficPolice.log", ios_base::app);
  TrafficPoliceLog << left << setw(12) << time(NULL) << " - Traffic police awake\n";
  TrafficPoliceLog.close();
}

// Car consumer thread function
void *Car_consumer(void *args) {
  while (1) {
    cout << "Entered consume function" << endl;

    pthread_mutex_lock(&TrafficPolice_mutex_lock); // Lock the mutex to access shared resources

    cout << "Consumer not waiting: " << TrafficDirection << endl;

    if (strcmp(TrafficDirection, "East") == 0) {
      // Check conditions for changing traffic direction to West
      if ((WestBuffer.size() >= 5) && (EastBuffer.size() < 5)) {
        cout << "Ready queue has more than 5 cars in the West. Changing direction to West." << endl;
        ChangeDirection();
      } else if (EastBuffer.empty() && WestBuffer.size() >= 5) {
        cout << "Changing direction to West as East buffer is empty and West buffer has 5 or more cars." << endl;
        ChangeDirection();
      } else if (EastBuffer.empty()) {
        cout << "East buffer is empty. Initiating TrafficPolice_Status()." << endl;
        TrafficPolice_Status();
      } else {
        cout << "Processing East car." << endl;
        SimulateCar();
      }
    } else {
      // Check conditions for changing traffic direction to East
      if (EastBuffer.size() >= 5 && WestBuffer.size() < 5) {
        cout << "Ready queue has more than 5 cars in the East. Changing direction to East." << endl;
        ChangeDirection();
      } else if (WestBuffer.empty() && EastBuffer.size() >= 5) {
        cout << "Changing direction to East as West buffer is empty and East buffer has 5 or more cars." << endl;
        ChangeDirection();
      } else if (WestBuffer.empty()) {
        cout << "West buffer is empty. Initiating TrafficPolice_Status()." << endl;
        TrafficPolice_Status();
      } else {
        cout << "Processing West car." << endl;
        SimulateCar();
      }
    }

    pthread_mutex_unlock(&TrafficPolice_mutex_lock); // Unlock the mutex after processing
  }

  return 0;
}
int main() {
  // Initialize traffic direction to "East"
  strcpy(TrafficDirection, "East");

  // Declare thread IDs and other variables
  pthread_t nTid, fTid;
  int pshared = 1;
  int semValue = 1; // Value is 1 because this is a lock
  srand(time(NULL));

  // Begin log files
  ofstream carLogInitial, TrafficPoliceLogInitial;
  carLogInitial.open("car.log");
  carLogInitial << left << setw(12) << "carID" << "direction" << "arrival-time" << "start-time" << "end-time\n";
  carLogInitial.close();
  TrafficPoliceLogInitial.open("TrafficPolice.log");
  TrafficPoliceLogInitial << left << setw(12) << "time" << "state\n";
  TrafficPoliceLogInitial.close();

  // Initialize and check mutex, condition variable, and semaphore
  if (pthread_mutex_init(&TrafficPolice_mutex_lock, NULL)) {
    perror("Error initializing mutex");
    return -1;
  }
  if (pthread_cond_init(&TrafficPolice_Condn, NULL)) {
    perror("Error initializing condition variable");
    pthread_mutex_destroy(&TrafficPolice_mutex_lock); // Clean up mutex
    return -1;
  }
  if (0 != sem_init(&Semaphore_Car, pshared, semValue)) {
    perror("Error initializing semaphore");
    pthread_mutex_destroy(&TrafficPolice_mutex_lock); // Clean up mutex
    pthread_cond_destroy(&TrafficPolice_Condn); // Clean up condition variable
    return -1;
  }

  // Set thread priorities and create threads
  ret = pthread_attr_init(&attrA);
  ret = pthread_attr_getschedparam(&attrA, &Sch1);
  Sch1.sched_priority = time1;

  ret = pthread_attr_init(&attrB);
  ret = pthread_attr_getschedparam(&attrA, &Sch2);
  Sch2.sched_priority = time2;

  ret = pthread_attr_init(&attrC);
  ret = pthread_attr_getschedparam(&attrA, &Sch3);
  Sch3.sched_priority = time3;

  // Create thread for flag person consuming the cars
  pthread_create(&fTid, &attrA, Car_consumer, NULL);

  // Create thread for car producer in the West direction
  // Uncomment the following line when implementing produceWest function
   pthread_create(&sTid, &attrB, produceWest, NULL);

  // Create thread for car producer in the East direction
  pthread_create(&nTid, &attrC, produceEast, NULL);

  // Main loop to keep the program running
  while (1) {
    fflush(stdout);
    Sleep_thread(1); // Sleep for 1 second
  }

  // Clean up resources
  sem_close(&Semaphore_Car);
  pthread_mutex_destroy(&TrafficPolice_mutex_lock);
  pthread_cond_destroy(&TrafficPolice_Condn);
  
  return 0;
}
