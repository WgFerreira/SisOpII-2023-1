#include <stdio.h>
#include <pthread.h>
#include <string.h>

// Subservices
#include "include/discovery_subservice.h"
#include "include/monitoring_subservice.h"
#include "include/management_subservice.h"
#include "include/interface_subservice.h"

// Constants
#define TKN_MANAGER "manager"

// Global Variables
int type_station = 0;  // 0: Participant; 1: Manager;

// Functions
void init_station(char* arg);

int main(int argc, char *argv[]) {
    pthread_t th1, th2, th3, th4;

    init_station(argv[1]);
    printf("%d\n", type_station);

    pthread_create(&th1, NULL, &discovery, NULL);
    pthread_create(&th2, NULL, &monitoring, NULL);
    pthread_create(&th3, NULL, &management, NULL);
    pthread_create(&th4, NULL, &interface, NULL);

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    pthread_join(th3, NULL);
    pthread_join(th4, NULL);

    return 0;
}

void init_station(char* arg) {
    if (arg != NULL && strcmp(arg, TKN_MANAGER) == 0) {
        type_station = 1;
    }
}