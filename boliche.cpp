#include <iostream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define NumeroPistas 12
#define NumeroCadeiras 10
#define NumeroClientes 100

//Semafaros
sem_t cadeiras_epsera;
sem_t semafaros_pistas[NumeroPistas];
sem_t semafaros_pistas_pinos[NumeroPistas];

//Variáveis de Condição
pthread_cond_t espera_pista = PTHREAD_COND_INITIALIZER;

//Locks
pthread_mutex_t lock_print = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_espera_pista = PTHREAD_MUTEX_INITIALIZER;

void* funcionario(void* arg) {
    pthread_mutex_lock(&lock_print);
        std::cout <<"Funcionário iniciou seu expediente." <<std::endl;
    pthread_mutex_unlock(&lock_print);
    
    sleep(1);
    
    pthread_mutex_lock(&lock_print);
        std::cout <<"Funcionário terminou seu expediente." <<std::endl;
    pthread_mutex_unlock(&lock_print);
}


void* cliente(void* arg) {
    int id = *((int *)arg);

    if (sem_trywait(&cadeiras_epsera) == 0) {
        pthread_mutex_lock(&lock_print);
        std::cout << "Cliente de id: " << id << " conseguiu pegar uma cadeira" << std::endl;
        pthread_mutex_unlock(&lock_print);
        
        sem_post(&cadeiras_epsera);
        
         bool conseguiuPista = false;
        
        // Percorre todas as pistas tentando pegar uma
        while (!conseguiuPista) {
            for (int i = 0; i < NumeroPistas; i++) {
                if (sem_trywait(&semafaros_pistas[i]) == 0) {
                    conseguiuPista = true;

                    pthread_mutex_lock(&lock_print);
                        std::cout << "Cliente de id: " << id << " está usando a pista " << i << std::endl;
                    pthread_mutex_unlock(&lock_print);

                    sleep(2);

                    sem_post(&semafaros_pistas[i]);

                    pthread_mutex_lock(&lock_espera_pista);
                        pthread_cond_signal(&espera_pista);
                    pthread_mutex_unlock(&lock_espera_pista);

                    pthread_mutex_lock(&lock_print);
                        std::cout << "Cliente de id: " << id << " liberou a pista " << i << std::endl;
                    pthread_mutex_unlock(&lock_print);
                    break;
                }
            }
            if(!conseguiuPista){
                pthread_mutex_lock(&lock_espera_pista);
                    pthread_cond_wait(&espera_pista,&lock_espera_pista);    
                pthread_mutex_unlock(&lock_espera_pista);
            }
        }

        pthread_mutex_lock(&lock_print);
            std::cout <<"Cliente de id: " <<  id << " liberou uma cadeira" << std::endl;
        pthread_mutex_unlock(&lock_print);
    }
    else{
        pthread_mutex_lock(&lock_print);
            std::cout <<"Cliente de id: " <<  id << " não conseguiu uma cadeira" << std::endl;
        pthread_mutex_unlock(&lock_print);
    }
}

int main() {
    int i;
    int id[NumeroClientes];

    pthread_t thr_clientes[NumeroClientes], thr_funcionario;

    // Inicialização dos semáforos para as pistas de boliche
    for (i = 0; i < NumeroPistas; i++) {
        sem_init(&(semafaros_pistas[i]), 0, 2);
        sem_init(&(semafaros_pistas_pinos[i]), 0, 1);
    }
    
    // Inicialização do semáforo para as cadeiras de espera
    sem_init(&cadeiras_epsera, 0, NumeroCadeiras);

    //Criação das multiplas threads
    for (i = 0; i < NumeroClientes; i++) {
        id[i] = i;
        pthread_create(&thr_clientes[i], NULL, cliente, (void*) &id[i]);
    }

    pthread_create(&thr_funcionario, NULL, funcionario, NULL);

    for (i = 0; i < NumeroClientes; i++) {
        pthread_join(thr_clientes[i], NULL);
    }

    pthread_join(thr_funcionario, NULL);
    return 0;
}
