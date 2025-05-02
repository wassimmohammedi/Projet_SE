#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    pthread_mutex_t mutex;
    char direction;
    int count_x;
    int count_y;
    int waiting_x;
    int waiting_y;
    sem_t x_sem;
    sem_t y_sem;
    int turn;
} Tunnel;

typedef struct {
    int bus_id;
    char city;
    Tunnel* tunnel;
} BusArgs;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_tunnel(Tunnel* tunnel) {
    pthread_mutex_init(&tunnel->mutex, NULL);
    tunnel->direction = '\0';
    tunnel->count_x = 0;
    tunnel->count_y = 0;
    tunnel->waiting_x = 0;
    tunnel->waiting_y = 0;
    tunnel->turn = 0;
    sem_init(&tunnel->x_sem, 0, 0);
    sem_init(&tunnel->y_sem, 0, 0);
}

void destroy_tunnel(Tunnel* tunnel) {
    pthread_mutex_destroy(&tunnel->mutex);
    sem_destroy(&tunnel->x_sem);
    sem_destroy(&tunnel->y_sem);
}

void enter_from_x(Tunnel* tunnel) {
    pthread_mutex_lock(&tunnel->mutex);
    tunnel->waiting_x++;

    while ((tunnel->direction == 'Y') || 
           (tunnel->count_y > 0) || 
           (tunnel->turn == 1 && tunnel->waiting_y > 0)) {
        pthread_mutex_unlock(&tunnel->mutex);
        sem_wait(&tunnel->x_sem);
        pthread_mutex_lock(&tunnel->mutex);
    }

    tunnel->waiting_x--;
    tunnel->count_x++;
    tunnel->direction = 'X';
    pthread_mutex_unlock(&tunnel->mutex);
}

void exit_from_x(Tunnel* tunnel) {
    pthread_mutex_lock(&tunnel->mutex);
    tunnel->count_x--;
    if (tunnel->count_x == 0) {
        tunnel->direction = '\0';
        tunnel->turn = 1;
        for (int i = 0; i < tunnel->waiting_y; i++) sem_post(&tunnel->y_sem);
    }
    pthread_mutex_unlock(&tunnel->mutex);
}

void enter_from_y(Tunnel* tunnel) {
    pthread_mutex_lock(&tunnel->mutex);
    tunnel->waiting_y++;

    while ((tunnel->direction == 'X') || 
           (tunnel->count_x > 0) || 
           (tunnel->turn == 0 && tunnel->waiting_x > 0)) {
        pthread_mutex_unlock(&tunnel->mutex);
        sem_wait(&tunnel->y_sem);
        pthread_mutex_lock(&tunnel->mutex);
    }

    tunnel->waiting_y--;
    tunnel->count_y++;
    tunnel->direction = 'Y';
    pthread_mutex_unlock(&tunnel->mutex);
}

void exit_from_y(Tunnel* tunnel) {
    pthread_mutex_lock(&tunnel->mutex);
    tunnel->count_y--;
    if (tunnel->count_y == 0) {
        tunnel->direction = '\0';
        tunnel->turn = 0;
        for (int i = 0; i < tunnel->waiting_x; i++) sem_post(&tunnel->x_sem);
    }
    pthread_mutex_unlock(&tunnel->mutex);
}

void* bus_behavior(void* arg) {
    BusArgs* args = (BusArgs*)arg;
    int bus_id = args->bus_id;
    char city = args->city;
    Tunnel* tunnel = args->tunnel;
    free(arg);

    for (int i = 1; i <= 10; i++) {
        char dest = (city == 'X') ? 'Y' : 'X';

        pthread_mutex_lock(&print_mutex);
        printf("Bus %d de ville %c : %c -> %c (Trajet aller %d)\n", bus_id, city, city, dest, i);
        pthread_mutex_unlock(&print_mutex);

        if (city == 'X') enter_from_x(tunnel);
        else enter_from_y(tunnel);

        int delay = 1000000 + rand() % 500001;
        usleep(delay);

        if (city == 'X') exit_from_x(tunnel);
        else exit_from_y(tunnel);

        pthread_mutex_lock(&print_mutex);
        printf("Bus %d de ville %c : %c -> %c (Trajet retour %d)\n", bus_id, city, dest, city, i);
        pthread_mutex_unlock(&print_mutex);

        if (dest == 'X') enter_from_x(tunnel);
        else enter_from_y(tunnel);

        delay = 1000000 + rand() % 500001;
        usleep(delay);

        if (dest == 'X') exit_from_x(tunnel);
        else exit_from_y(tunnel);
    }

    return NULL;
}

int main() {
    srand(time(NULL));
    Tunnel tunnel;
    init_tunnel(&tunnel);

    pthread_t threads[9];
    for (int i = 0; i < 9; i++) {
        BusArgs* args = malloc(sizeof(BusArgs));
        args->bus_id = (i < 5) ? i + 1 : i - 4;
        args->city = (i < 5) ? 'X' : 'Y';
        args->tunnel = &tunnel;
        pthread_create(&threads[i], NULL, bus_behavior, args);
    }

    for (int i = 0; i < 9; i++) pthread_join(threads[i], NULL);

    destroy_tunnel(&tunnel);
    pthread_mutex_destroy(&print_mutex);
    return 0;
}
