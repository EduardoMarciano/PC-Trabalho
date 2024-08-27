#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define NumeroPistas 12
#define NumeroCadeiras 10
#define NumeroClientes 100

sem_t cadeiras_epsera;
sem_t semafaros_pistas[NumeroPistas];
sem_t semafaros_pistas_pinos[NumeroPistas];

void *cliente (void *arg);
void *funcionario (void *arg);

int main () {
	int i;
	int *id;

    pthread_t thr_clientes[NumeroClientes], thr_funcionario;

	//semaforo para acesso a pista de boliche
    //semafaro para acesso 
	for (i=0; i<NumeroPistas; i++) {
		sem_init(&(semafaros_pistas[i]), 0, 2);
        sem_init(&(semafaros_pistas_pinos[i]), 0, 1);
	}
    // semafaro para cadeitas de espera
    sem_init(&cadeiras_epsera, 0, NumeroCadeiras);

    for (i = 0; i < NumeroClientes; i++) {
        id[i] = i;
        pthread_create(&thr_clientes[i], NULL, cliente, (void*) &id[i]);
    }
  
    for (i = 0; i < NumeroClientes; i++){
        pthread_join(thr_clientes[i], NULL);
    }

    pthread_create(&thr_funcionario, NULL, funcionario, NULL);
    pthread_join(thr_funcionario, NULL);  

	return 0;
}
