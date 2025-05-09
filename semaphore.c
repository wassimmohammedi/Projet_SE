#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

// Structure représentant le tunnel partagé
typedef struct {
    pthread_mutex_t mutex;  // Mutex pour protéger l'accès aux variables partagées
    char direction;        // Direction actuelle du tunnel ('X', 'Y' ou '\0' si vide)
    int count_x;           // Nombre de bus venant de X actuellement dans le tunnel
    int count_y;           // Nombre de bus venant de Y actuellement dans le tunnel
    int waiting_x;         // Nombre de bus venant de X en attente
    int waiting_y;         // Nombre de bus venant de Y en attente
    sem_t x_sem;           // Sémaphore pour les bus venant de X
    sem_t y_sem;           // Sémaphore pour les bus venant de Y
    int turn;              // Tour (0 pour X, 1 pour Y) pour éviter la famine
} Tunnel;

// Arguments passés à chaque thread bus
typedef struct {
    int bus_id;            // Identifiant du bus
    char city;             // Ville d'origine ('X' ou 'Y')
    Tunnel* tunnel;        // Pointeur vers le tunnel partagé
} BusArgs;

// Mutex pour protéger les sorties console
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialisation du tunnel
void init_tunnel(Tunnel* tunnel) {
    pthread_mutex_init(&tunnel->mutex, NULL);
    tunnel->direction = '\0';  // Aucune direction initialement
    tunnel->count_x = 0;
    tunnel->count_y = 0;
    tunnel->waiting_x = 0;
    tunnel->waiting_y = 0;
    tunnel->turn = 0;          // Commence par le tour de X
    sem_init(&tunnel->x_sem, 0, 0);  // Sémaphores initialisés à 0
    sem_init(&tunnel->y_sem, 0, 0);
}

// Destruction propre du tunnel
void destroy_tunnel(Tunnel* tunnel) {
    pthread_mutex_destroy(&tunnel->mutex);
    sem_destroy(&tunnel->x_sem);
    sem_destroy(&tunnel->y_sem);
}

// Fonction pour entrer dans le tunnel depuis X
void enter_from_x(Tunnel* tunnel) {
    pthread_mutex_lock(&tunnel->mutex);
    tunnel->waiting_x++;  // Incrémente le compteur d'attente

    // Attend que les conditions soient favorables :
    // 1. Le tunnel ne doit pas être en direction Y
    // 2. Il ne doit pas y avoir de bus Y dans le tunnel
    // 3. Si c'est le tour de Y et qu'il y a des bus Y en attente, on attend
    while ((tunnel->direction == 'Y') || 
        (tunnel->count_y > 0) || 
        (tunnel->turn == 1 && tunnel->waiting_y > 0)) {
        pthread_mutex_unlock(&tunnel->mutex);
        sem_wait(&tunnel->x_sem);  // Attend le signal
        pthread_mutex_lock(&tunnel->mutex);
    }

    tunnel->waiting_x--;    // Décrémente l'attente
    tunnel->count_x++;      // Incrémente le compteur dans le tunnel
    tunnel->direction = 'X'; // Définit la direction
    pthread_mutex_unlock(&tunnel->mutex);
}

// Fonction pour sortir du tunnel depuis X
void exit_from_x(Tunnel* tunnel) {
    pthread_mutex_lock(&tunnel->mutex);
    tunnel->count_x--;  // Décrémente le compteur dans le tunnel
    
    // Si plus de bus X dans le tunnel
    if (tunnel->count_x == 0) {
        tunnel->direction = '\0';  // Réinitialise la direction
        tunnel->turn = 1;          // Passe le tour à Y
        // Réveille tous les bus Y en attente
        for (int i = 0; i < tunnel->waiting_y; i++) sem_post(&tunnel->y_sem);
    }
    pthread_mutex_unlock(&tunnel->mutex);
}

// Fonction pour entrer dans le tunnel depuis Y (symétrique à enter_from_x)
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

// Fonction pour sortir du tunnel depuis Y (symétrique à exit_from_x)
void exit_from_y(Tunnel* tunnel) {
    pthread_mutex_lock(&tunnel->mutex);
    tunnel->count_y--;
    
    if (tunnel->count_y == 0) {
        tunnel->direction = '\0';
        tunnel->turn = 0;  // Passe le tour à X
        // Réveille tous les bus X en attente
        for (int i = 0; i < tunnel->waiting_x; i++) sem_post(&tunnel->x_sem);
    }
    pthread_mutex_unlock(&tunnel->mutex);
}

// Comportement d'un bus (thread)
void* bus_behavior(void* arg) {
    BusArgs* args = (BusArgs*)arg;
    int bus_id = args->bus_id;
    char city = args->city;
    Tunnel* tunnel = args->tunnel;
    free(arg);  // Libère la mémoire allouée pour les arguments

    // Chaque bus effectue 10 allers-retours
    for (int i = 1; i <= 10; i++) {
        char dest = (city == 'X') ? 'Y' : 'X';  // Destination opposée

        // Affichage du départ
        pthread_mutex_lock(&print_mutex);
        printf("Bus %d de ville %c : %c -> %c (Trajet aller %d)\n", bus_id, city, city, dest, i);
        pthread_mutex_unlock(&print_mutex);

        // Entrée dans le tunnel selon la ville d'origine
        if (city == 'X') enter_from_x(tunnel);
        else enter_from_y(tunnel);

        // Temps aléatoire dans le tunnel (1.0 à 1.5 secondes)
        int delay = 1000000 + rand() % 500001;
        usleep(delay);

        // Sortie du tunnel
        if (city == 'X') exit_from_x(tunnel);
        else exit_from_y(tunnel);

        // Affichage de l'arrivée
        pthread_mutex_lock(&print_mutex);
        printf("Bus %d de ville %c : %c -> %c (Trajet retour %d)\n", bus_id, city, dest, city, i);
        pthread_mutex_unlock(&print_mutex);

        // Entrée pour le retour
        if (dest == 'X') enter_from_x(tunnel);
        else enter_from_y(tunnel);

        // Temps aléatoire dans le tunnel pour le retour
        delay = 1000000 + rand() % 500001;
        usleep(delay);

        // Sortie pour le retour
        if (dest == 'X') exit_from_x(tunnel);
        else exit_from_y(tunnel);
    }

    return NULL;
}

int main() {
    srand(time(NULL));  // Initialisation du générateur aléatoire
    Tunnel tunnel;
    init_tunnel(&tunnel);  // Initialisation du tunnel

    // Création des threads (5 bus de X et 4 de Y)
    pthread_t threads[9];
    for (int i = 0; i < 9; i++) {
        BusArgs* args = malloc(sizeof(BusArgs));
        args->bus_id = (i < 5) ? i + 1 : i - 4;  // IDs 1-5 pour X, 1-4 pour Y
        args->city = (i < 5) ? 'X' : 'Y';
        args->tunnel = &tunnel;
        pthread_create(&threads[i], NULL, bus_behavior, args);
    }

    // Attente de la fin de tous les threads
    for (int i = 0; i < 9; i++) pthread_join(threads[i], NULL);

    // Nettoyage
    destroy_tunnel(&tunnel);
    pthread_mutex_destroy(&print_mutex);
    return 0;
}