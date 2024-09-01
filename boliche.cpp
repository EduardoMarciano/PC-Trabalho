#include <iostream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

//Constantes
#define NumeroPistas 3
#define NumeroCadeiras 5
#define NumeroClientes 20

//Variáveis
int pistasParaLevantarPinos[NumeroPistas] = {0};

//Semafaros
// Semafaro para controlar o acesso as cadeiras de espera
sem_t cadeiras_espera;
// Semafaro para indicar quando o funcionário deve checar os pinos
sem_t funcionario_levanta_pinos;
// Semafaros para controlar o acesso as pistas
sem_t semafaros_pistas[NumeroPistas];
// Semafaros para controlar o estado dos pinos de uma dada pista
sem_t semafaros_pistas_pinos[NumeroPistas];

//Variáveis de Condição
// Variável de Condição para não causar espera ocupada do cliente que está na cadeira de espera.
pthread_cond_t espera_pista = PTHREAD_COND_INITIALIZER;

//Locks
// Lock para organizar os prints.
pthread_mutex_t lock_print = PTHREAD_MUTEX_INITIALIZER;
// Lock acessar a variável de condição para esperar uma pista.
pthread_mutex_t lock_espera_pista = PTHREAD_MUTEX_INITIALIZER;
// Lock para uma dupla esperar a outra usar a pista para ela usar.
pthread_mutex_t lock_acesso_dupla[NumeroPistas] = PTHREAD_MUTEX_INITIALIZER;
// Lock para acessar a variável pistasParaLevantarPinos.
pthread_mutex_t lock_avisa_funcionario = PTHREAD_MUTEX_INITIALIZER;

void* funcionario(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock_print);
            std::cout <<"Funcionário iniciou seu expediente." <<std::endl;
        pthread_mutex_unlock(&lock_print);
        
        while (1) {
        //Espera algum cliente sinalizar que sua pista precisa levantar os seus pinos
        sem_wait(&funcionario_levanta_pinos);

        //Percorre Todas as Pistas verificando qual precisa ter seus pinos levantados.
        for (int i = 0; i < NumeroPistas; i++) {
            pthread_mutex_lock(&lock_avisa_funcionario);
            if(pistasParaLevantarPinos[i]){
                //Retorna a condição da pista para Zero. Não precisa levantar os pinos.
                pistasParaLevantarPinos[i] = 0;
                //Incrementa em um o Semafaro da Pista para indicar que os pinos estão disponíveis.
                sem_post(&semafaros_pistas_pinos[i]);

                pthread_mutex_lock(&lock_print);
                    std::cout << "Funcionário Levanta os pinos da pista de número: "<< i <<"."<< std::endl;
                pthread_mutex_unlock(&lock_print);
            }
            pthread_mutex_unlock(&lock_avisa_funcionario);
        }
        
        sleep(1);
        }
    }
}

void utilizaPista(int id, int i){
    pthread_mutex_lock(&lock_acesso_dupla[i]);

        //Espera ter pinos disponíveis.
        sem_wait(&semafaros_pistas_pinos[i]);
        
        pthread_mutex_lock(&lock_print);
            std::cout << "Cliente de id: " << id << " faz primeiro lançamento na pista de número: "<< i <<"."<< std::endl;
        pthread_mutex_unlock(&lock_print);
        
        pthread_mutex_lock(&lock_avisa_funcionario);
            //Post para avisar o funcionário que há pinos a serem levantados.
            sem_post(&funcionario_levanta_pinos);
            //Vaŕiavel que indicará qual pista o funcionário deverá incrementar o semáfaro para "Levantar os Pinos"
            pistasParaLevantarPinos[i] = 1;
        pthread_mutex_unlock(&lock_avisa_funcionario);

        //Espera ter pinos disponíveis.
        sem_wait(&semafaros_pistas_pinos[i]);

        pthread_mutex_lock(&lock_print);
            std::cout << "Cliente de id: " << id << " faz segundo lançamento na pista de número: "<< i <<"."<< std::endl;
        pthread_mutex_unlock(&lock_print);

        pthread_mutex_lock(&lock_avisa_funcionario);
            //Post para avisar o funcionário que há pinos a serem levantados.
            sem_post(&funcionario_levanta_pinos);
            //Vaŕiavel que indicará qual pista o funcionário deverá incrementar o semáfaro para "Levantar os Pinos"
            pistasParaLevantarPinos[i] = 1;
        pthread_mutex_unlock(&lock_avisa_funcionario);
    
    pthread_mutex_unlock(&lock_acesso_dupla[i]);
    sleep(2);
}

void* cliente(void* arg) {
    while (1) {
        
        int id = *((int *)arg);

        //Cliente tenta pegar uma cadeira de espera
        if (sem_trywait(&cadeiras_espera) == 0) {
            //Cenário em que o Cliente consegue alguma cadeira de espera
            pthread_mutex_lock(&lock_print);
                std::cout << "Cliente de id: " << id << " conseguiu pegar uma cadeira" << std::endl;
            pthread_mutex_unlock(&lock_print);
                
            bool conseguiuPista = false;
            
            //Percorre todas as pistas tentando pegar uma
            while (!conseguiuPista) {
                for (int i = 0; i < NumeroPistas; i++) {

                    //Cliente tenta pegar uma Pista
                    if (sem_trywait(&semafaros_pistas[i]) == 0) {

                        //Cenário em que consegue pegar uma Pista
                        conseguiuPista = true;
                        
                        //Libera uma cadeira de espera
                        sem_post(&cadeiras_espera);

                        pthread_mutex_lock(&lock_print);
                            std::cout << "Cliente de id: " << id << " liberou uma cadeira de espera." << std::endl;
                            std::cout << "Cliente de id: " << id << " está usando a pista " << i << std::endl;
                        pthread_mutex_unlock(&lock_print);

                        utilizaPista(id, i);

                        //Libere um espaço na pista e Avisa um cliente nas cadeiras de espera que o espaço está vago.
                        sem_post(&semafaros_pistas[i]);
                        pthread_mutex_lock(&lock_espera_pista);
                            // Envia um sinal para os clientes nas cadeiras de espera
                            pthread_cond_signal(&espera_pista);
                        pthread_mutex_unlock(&lock_espera_pista);

                        pthread_mutex_lock(&lock_print);
                            std::cout << "Cliente de id: " << id << " liberou a pista " << i << std::endl;
                        pthread_mutex_unlock(&lock_print);
                        break;
                    }
                }
                if(!conseguiuPista){
                    //Cenário em que Cliente não consegue a pista, irá eseprar até ser "avisado" por outro cliente
                    pthread_mutex_lock(&lock_espera_pista);
                        pthread_cond_wait(&espera_pista,&lock_espera_pista);
                        
                        pthread_mutex_lock(&lock_print);
                            std::cout << "Cliente de id: " << id << " acordou "<< std::endl;
                        pthread_mutex_unlock(&lock_print);
                        
                    pthread_mutex_unlock(&lock_espera_pista);
                }
            }
        }
        else{
            //Cénario Onde Cliente não consegue uma cadeira e vai embora.
            pthread_mutex_lock(&lock_print);
                std::cout <<"Cliente de id: " <<  id << " não conseguiu uma cadeira" << std::endl;
            pthread_mutex_unlock(&lock_print);
        }
        sleep(4);
    }
}

int main() {
    int i;
    int id[NumeroClientes];

    pthread_t thr_clientes[NumeroClientes], thr_funcionario;

    //Inicialização dos semáforos para as pistas de boliche
    for (i = 0; i < NumeroPistas; i++) {
        sem_init(&(semafaros_pistas[i]), 0, 2);
        sem_init(&(semafaros_pistas_pinos[i]), 0, 1);
    }
    
    //Inicialização do semáforo para as cadeiras de espera
    sem_init(&cadeiras_espera, 0, NumeroCadeiras);

    //Criação das multiplas threads
    for (i = 0; i < NumeroClientes; i++) {
        id[i] = i;
        pthread_create(&thr_clientes[i], NULL, cliente, (void*) &id[i]);
    }

    //Joins em cada threads gerada.
    pthread_create(&thr_funcionario, NULL, funcionario, NULL);

    for (i = 0; i < NumeroClientes; i++) {
        pthread_join(thr_clientes[i], NULL);
    }

    pthread_join(thr_funcionario, NULL);
    return 0;
}
