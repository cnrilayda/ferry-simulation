#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

// Project requirements
#define FERRY_CAPACITY 20
#define TOTAL_CARS 12        // Project requirement
#define TOTAL_MINIBUSES 10   // Project requirement  
#define TOTAL_TRUCKS 8       // Project requirement
#define CAR_UNIT 1
#define MINIBUS_UNIT 2
#define TRUCK_UNIT 3
#define TOTAL_VEHICLES (TOTAL_CARS + TOTAL_MINIBUSES + TOTAL_TRUCKS)

// CPU intensive parameters - MAXIMUM CPU USAGE
#define WORK_ITERATIONS 1000000  // Much higher for intense CPU work
#define PROCESSING_DELAY 1000    // Minimal sleep - mostly busy waiting
#define BUSY_WAIT_CYCLES 500000  // For busy waiting instead of sleep

typedef struct {
    char type[10];
    int size;
    int id;
    int start_side;
    int current_side;
    int toll_used[2];
    int completed;
    int returned_home;
    int work_cycles;  // Track work done
} Vehicle;

// Global variables
int ferry_side;
int current_load = 0;
int vehicle_count = 0;
int completed_vehicles = 0;
int active_threads = 0;
int stop_ferry = 0;
int total_departures = 0;
int vehicles_at_start = TOTAL_VEHICLES;

// Mutexes and conditions
pthread_mutex_t tolls[4];
pthread_mutex_t ferry_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ferry_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t side_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t side_cond = PTHREAD_COND_INITIALIZER;

// Function prototypes
void pass_toll(Vehicle *v, int side);
void* ferry_manager(void* arg);
void* vehicle_thread(void *arg);
int get_random_toll_for_side(int side);
void print_vehicle_status(Vehicle *v, const char* action, int side);
void simulate_cpu_work(Vehicle *v, const char* task);
double calculate_complex_math(int iterations);

int main() {
    pthread_t threads[TOTAL_VEHICLES];
    pthread_t ferry_thread;
    srand(time(NULL));
    
    // Initialize ferry at random side
    ferry_side = rand() % 2;
    printf("=== FERRY TOUR SIMULATION STARTED ===\n");
    printf("Ferry starts at side %d\n", ferry_side);
    printf("Total vehicles: %d (Cars: %d, Minibuses: %d, Trucks: %d) - Project Spec\n", 
           TOTAL_VEHICLES, TOTAL_CARS, TOTAL_MINIBUSES, TOTAL_TRUCKS);
    printf("Ferry capacity: %d units\n", FERRY_CAPACITY);
    printf("CPU intensive mode: ON\n");
    printf("=====================================\n\n");

    // Initialize toll mutexes
    for (int i = 0; i < 4; i++) {
        pthread_mutex_init(&tolls[i], NULL);
    }
    
    // Create ferry manager thread
    pthread_create(&ferry_thread, NULL, ferry_manager, NULL);

    int t = 0;
    
    // Create car threads
    for (int i = 0; i < TOTAL_CARS; i++) {
        Vehicle *v = malloc(sizeof(Vehicle));
        strcpy(v->type, "car");
        v->size = CAR_UNIT;
        v->id = i + 1;
        v->start_side = rand() % 2;
        v->current_side = v->start_side;
        v->completed = 0;
        v->returned_home = 0;
        v->work_cycles = 0;
        printf("Car %d starts at side %d\n", v->id, v->start_side);
        pthread_create(&threads[t++], NULL, vehicle_thread, v);
        usleep(10000); // Keep minimal for fast thread creation
    }
    
    // Create minibus threads
    for (int i = 0; i < TOTAL_MINIBUSES; i++) {
        Vehicle *v = malloc(sizeof(Vehicle));
        strcpy(v->type, "minibus");
        v->size = MINIBUS_UNIT;
        v->id = i + 1;
        v->start_side = rand() % 2;
        v->current_side = v->start_side;
        v->completed = 0;
        v->returned_home = 0;
        v->work_cycles = 0;
        printf("Minibus %d starts at side %d\n", v->id, v->start_side);
        pthread_create(&threads[t++], NULL, vehicle_thread, v);
        usleep(1000);
    }
    
    // Create truck threads
    for (int i = 0; i < TOTAL_TRUCKS; i++) {
        Vehicle *v = malloc(sizeof(Vehicle));
        strcpy(v->type, "truck");
        v->size = TRUCK_UNIT;
        v->id = i + 1;
        v->start_side = rand() % 2;
        v->current_side = v->start_side;
        v->completed = 0;
        v->returned_home = 0;
        v->work_cycles = 0;
        printf("Truck %d starts at side %d\n", v->id, v->start_side);
        pthread_create(&threads[t++], NULL, vehicle_thread, v);
        usleep(1000);
    }

    printf("\nAll vehicles created. Starting CPU intensive simulation...\n\n");

    // Wait for all vehicle threads to complete
    for (int i = 0; i < t; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Signal ferry to stop
    pthread_mutex_lock(&ferry_mutex);
    stop_ferry = 1;
    pthread_cond_broadcast(&ferry_cond);
    pthread_mutex_unlock(&ferry_mutex);
    
    // Wait for ferry thread with timeout
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 2;
    
    int result = pthread_timedjoin_np(ferry_thread, NULL, &timeout);
    if (result != 0) {
        printf("Ferry thread timeout, cancelling...\n");
        pthread_cancel(ferry_thread);
    }
    
    printf("\n=== SIMULATION COMPLETED ===\n");
    printf("All vehicles have returned to their starting positions!\n");
    printf("Total ferry departures: %d\n", total_departures);
    
    // Cleanup
    for (int i = 0; i < 4; i++) {
        pthread_mutex_destroy(&tolls[i]);
    }
    
    return 0;
}

// CPU intensive work simulation - HEAVY COMPUTATION
void simulate_cpu_work(Vehicle *v, const char* task) {
    // Multiple heavy computations to max out CPU
    double result1 = calculate_complex_math(WORK_ITERATIONS);
    double result2 = calculate_complex_math(WORK_ITERATIONS / 2);
    double result3 = calculate_complex_math(WORK_ITERATIONS / 3);
    
    v->work_cycles++;
    
    // Heavy string and hash operations
    char buffer[2000];
    volatile int hash = 0; // volatile prevents optimization
    
    for (int i = 0; i < 5000; i++) {
        sprintf(buffer, "[%s %d] %s - cycle %d - results: %.4f %.4f %.4f - hash: %d", 
                v->type, v->id, task, v->work_cycles, result1, result2, result3, hash);
        
        // Intensive hash calculation
        for (int j = 0; buffer[j]; j++) {
            hash = hash * 31 + buffer[j];
            hash = hash ^ (hash >> 16); // Additional bit operations
        }
        
        // More math operations
        for (volatile int k = 0; k < 1000; k++) {
            hash += k * k + k / 3;
        }
    }
    
    // Store result to prevent compiler optimization
    v->work_cycles += (hash % 10);
}

// Complex mathematical calculations - MAXIMUM CPU LOAD
double calculate_complex_math(int iterations) {
    volatile double result = 1.0; // volatile prevents optimization
    volatile double temp1, temp2, temp3;
    
    for (volatile int i = 1; i < iterations; i++) {
        // Multiple floating point operations
        temp1 = 1.0 / (i * i);
        temp2 = 1.0 / (i * i * i);  
        temp3 = 1.0 / (i * i * i * i);
        
        result += temp1 + temp2 + temp3;
        
        // Additional trigonometric operations (CPU intensive)
        if (i % 100 == 0) {
            volatile double sin_val = result * 0.001;
            volatile double cos_val = result * 0.002;
            // Prevent overflow with more operations
            result = result * 0.999999 + sin_val + cos_val;
        }
        
        // Bit operations for more CPU work
        volatile int bit_work = i;
        bit_work = bit_work ^ (bit_work << 1);
        bit_work = bit_work ^ (bit_work >> 1);
        result += bit_work * 0.0001;
    }
    
    return result;
}

int get_random_toll_for_side(int side) {
    if (side == 0) {
        return rand() % 2;
    } else {
        return 2 + rand() % 2;
    }
}

// Busy waiting function instead of sleep
void busy_wait(int cycles) {
    volatile int waste_cpu = 0;
    for (volatile int i = 0; i < cycles; i++) {
        waste_cpu += i * i + i / 3;
        waste_cpu = waste_cpu ^ (waste_cpu >> 1);
    }
}

void print_vehicle_status(Vehicle *v, const char* action, int side) {
    printf("[%s %d] %s at side %d (work cycles: %d)\n", 
           v->type, v->id, action, side, v->work_cycles);
}

void pass_toll(Vehicle *v, int side) {
    int toll_index = get_random_toll_for_side(side);
    v->toll_used[side] = toll_index;
    
    pthread_mutex_lock(&tolls[toll_index]);
    
    // HEAVY CPU work during toll processing
    simulate_cpu_work(v, "processing toll payment");
    simulate_cpu_work(v, "verifying payment");
    simulate_cpu_work(v, "updating records");
    
    printf("[%s %d] passing through toll %d on side %d (work cycles: %d)\n", 
           v->type, v->id, toll_index, side, v->work_cycles);
    
    // Replace sleep with busy waiting for max CPU usage
    busy_wait(BUSY_WAIT_CYCLES);
    pthread_mutex_unlock(&tolls[toll_index]);
}

void* ferry_manager(void* arg) {
    int wait_cycles = 0;
    int empty_ferry_cycles = 0;
    int manager_work_cycles = 0;
    
    while (!stop_ferry) {
        // Simulate ferry management work
        manager_work_cycles++;
        double management_result = calculate_complex_math(WORK_ITERATIONS / 2);
        
        pthread_mutex_lock(&ferry_mutex);
        
        // Reduced timeout for more active checking
        while (current_load == 0 && !stop_ferry && completed_vehicles < TOTAL_VEHICLES) {
            struct timespec timeout;
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += 1; // Reduced from 3 seconds
            
            int wait_result = pthread_cond_timedwait(&ferry_cond, &ferry_mutex, &timeout);
            
            if (wait_result == ETIMEDOUT) {
                empty_ferry_cycles++;
                printf(">>> Ferry timeout waiting for vehicles (cycle %d, mgmt work: %d)\n", 
                       empty_ferry_cycles, manager_work_cycles);
                break;
            } else {
                empty_ferry_cycles = 0;
            }
            
            wait_cycles = 0;
        }
        
        if (stop_ferry || completed_vehicles >= TOTAL_VEHICLES) {
            pthread_mutex_unlock(&ferry_mutex);
            break;
        }
        
        // Ferry departure decision logic
        int should_depart = 0;
        wait_cycles++;
        
        printf("Ferry status: side=%d, load=%d/%d, active=%d, completed=%d/%d, cycles=%d, mgmt_work=%d\n", 
               ferry_side, current_load, FERRY_CAPACITY, active_threads, 
               completed_vehicles, TOTAL_VEHICLES, wait_cycles, manager_work_cycles);
        
        if (current_load == 0 && empty_ferry_cycles >= 1) { // More aggressive
            should_depart = 1;
            printf(">>> EMPTY Ferry departing (empty_cycles=%d)\n", empty_ferry_cycles);
        } else if (current_load >= FERRY_CAPACITY) {
            should_depart = 1;
            printf(">>> Ferry departing: Full capacity\n");
        } else if (current_load > 0) {
            pthread_mutex_unlock(&ferry_mutex);
            
            // Shorter waits, more CPU activity
            if (wait_cycles <= 2) {
                usleep(500000); // 0.5 seconds
            } else if (wait_cycles <= 4) {
                usleep(300000); // 0.3 seconds
            } else {
                usleep(100000); // 0.1 seconds
            }
            
            pthread_mutex_lock(&ferry_mutex);
            
            // More aggressive departure
            if (current_load >= 15 || wait_cycles >= 3) {
                should_depart = 1;
                printf(">>> Ferry departing: Load=%d, cycles=%d\n", current_load, wait_cycles);
            }
        }
        
        if (should_depart) {
            total_departures++;
            wait_cycles = 0;
            empty_ferry_cycles = 0;
            
            printf(">>> Ferry departing from side %d with %d/%d units\n", 
                   ferry_side, current_load, FERRY_CAPACITY);
            
            // Simulate ferry travel work
            for (int i = 0; i < 5; i++) {
                calculate_complex_math(WORK_ITERATIONS / 10);
                usleep(400000); // Distributed travel time
            }
            
            current_load = 0;
            ferry_side = 1 - ferry_side;
            
            printf("<<< Ferry arrived at side %d\n", ferry_side);
            
            pthread_cond_broadcast(&side_cond);
            pthread_cond_broadcast(&ferry_cond);
        }
        
        pthread_mutex_unlock(&ferry_mutex);
        usleep(50000); // Very fast checks for high activity
    }
    
    return NULL;
}

void* vehicle_thread(void *arg) {
    Vehicle* v = (Vehicle*)arg;
    
    pthread_mutex_lock(&count_mutex);
    active_threads++;
    pthread_mutex_unlock(&count_mutex);
    
    // Simulate initial vehicle preparation work
    simulate_cpu_work(v, "vehicle startup");
    
    // First journey
    int destination_side = 1 - v->current_side;
    
    int boarded_first = 0;
    while (!boarded_first && !stop_ferry) {
        // Simulate waiting/planning work
        simulate_cpu_work(v, "planning journey");
        
        pthread_mutex_lock(&ferry_mutex);
        
        if (ferry_side == v->current_side && 
            current_load + v->size <= FERRY_CAPACITY) {
            
            pthread_mutex_unlock(&ferry_mutex);
            pass_toll(v, v->current_side);
            pthread_mutex_lock(&ferry_mutex);
            
            if (ferry_side == v->current_side && 
                current_load + v->size <= FERRY_CAPACITY) {
                
                current_load += v->size;
                v->current_side = destination_side;
                
                printf("[%s %d] boarded ferry from side %d. Load: %d/%d\n", 
                       v->type, v->id, ferry_side, current_load, FERRY_CAPACITY);
                
                boarded_first = 1;
                pthread_cond_signal(&ferry_cond);
            }
        }
        
        pthread_mutex_unlock(&ferry_mutex);
        
        if (!boarded_first) {
            // ACTIVE WAITING with MAXIMUM CPU work instead of sleep
            simulate_cpu_work(v, "waiting for ferry - planning");
            simulate_cpu_work(v, "waiting for ferry - calculating");
            busy_wait(BUSY_WAIT_CYCLES / 10); // Busy wait instead of usleep
        }
    }
    
    if (stop_ferry) {
        pthread_mutex_lock(&count_mutex);
        active_threads--;
        pthread_mutex_unlock(&count_mutex);
        free(v);
        return NULL;
    }
    
    // Wait for arrival with CONTINUOUS work
    while (ferry_side != v->current_side && !stop_ferry) {
        simulate_cpu_work(v, "traveling on ferry - processing");
        simulate_cpu_work(v, "traveling on ferry - planning arrival");
        busy_wait(BUSY_WAIT_CYCLES / 20);
        
        pthread_mutex_lock(&side_mutex);
        if (ferry_side == v->current_side) {
            pthread_mutex_unlock(&side_mutex);
            break;
        }
        pthread_mutex_unlock(&side_mutex);
    }
    
    printf("[%s %d] arrived at side %d\n", v->type, v->id, v->current_side);
    
    // MAXIMUM work at destination
    for (int i = 0; i < 10; i++) {
        simulate_cpu_work(v, "working at destination - task 1");
        simulate_cpu_work(v, "working at destination - task 2"); 
        busy_wait(BUSY_WAIT_CYCLES / 15);
    }
    
    // Return journey (similar pattern with CPU work)
    destination_side = v->start_side;
    
    int boarded_return = 0;
    while (!boarded_return && !stop_ferry) {
        simulate_cpu_work(v, "planning return");
        
        pthread_mutex_lock(&ferry_mutex);
        
        if (ferry_side == v->current_side && 
            current_load + v->size <= FERRY_CAPACITY) {
            
            pthread_mutex_unlock(&ferry_mutex);
            pass_toll(v, v->current_side);
            pthread_mutex_lock(&ferry_mutex);
            
            if (ferry_side == v->current_side && 
                current_load + v->size <= FERRY_CAPACITY) {
                
                current_load += v->size;
                v->current_side = destination_side;
                
                printf("[%s %d] boarded return ferry. Load: %d/%d\n", 
                       v->type, v->id, current_load, FERRY_CAPACITY);
                
                boarded_return = 1;
                pthread_cond_signal(&ferry_cond);
            }
        }
        
        pthread_mutex_unlock(&ferry_mutex);
        
        if (!boarded_return) {
            for (int i = 0; i < 5; i++) {
                simulate_cpu_work(v, "waiting return ferry");
                usleep(100000);
            }
        }
    }
    
    if (stop_ferry) {
        pthread_mutex_lock(&count_mutex);
        active_threads--;
        pthread_mutex_unlock(&count_mutex);
        free(v);
        return NULL;
    }
    
    // Wait for return arrival
    pthread_mutex_lock(&side_mutex);
    while (ferry_side != v->current_side && !stop_ferry) {
        pthread_mutex_unlock(&side_mutex);
        simulate_cpu_work(v, "returning home");
        usleep(200000);
        pthread_mutex_lock(&side_mutex);
    }
    pthread_mutex_unlock(&side_mutex);
    
    // Final work before completion
    simulate_cpu_work(v, "completing journey");
    
    pthread_mutex_lock(&count_mutex);
    v->returned_home = 1;
    completed_vehicles++;
    active_threads--;
    vehicles_at_start--;
    pthread_mutex_unlock(&count_mutex);
    
    printf("[%s %d] returned home (total work cycles: %d). Completed: %d/%d\n", 
           v->type, v->id, v->work_cycles, completed_vehicles, TOTAL_VEHICLES);
    
    pthread_cond_signal(&ferry_cond);
    
    free(v);
    return NULL;
}
