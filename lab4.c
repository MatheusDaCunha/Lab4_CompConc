#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "timer.h"

#define NUMERO_DE_ELEMENTOS 0
#define NUMERO_DE_THREADS 0


/* Estruturas e Tipos */
typedef struct{
    int *vetorEntrada;
    double *vetorDeSaida;
    long long int dimensao;
    int nThreads;
}ArgumentosDaThread;


/* Declaração de funções do programa */
void inicializarVetor(int *, long long int);
void resetarVetores(double *, double *, long long int);
int ehPrimo(int);
void processarPrimos_Sequencial(int *, double *, long long int);
void processarPrimos_Concorrente(int *, double *, long long int, int);
void *verificarPrimalidade(void *);
void compararResultados(double *, double *, long long int);
void exibirTemposDeExecucao();


/* Variáveis Globais */
long long int indicadorGlobal = 0; /* Controlador para divisão dinâmica de tarefas entre threads */
double inicio, fim, tempoSequencial, tempoConcorrente;
pthread_mutex_t lock;


/* Função principal - Documentação */
int main(int argc, char *argv[]){
    /* Variáveis locais de 'main' */
    long long int numeroDeElementos = NUMERO_DE_ELEMENTOS;
    int numeroDeThreads = NUMERO_DE_THREADS;
    int *vetorDeEntrada;
    double *vetorDeSaidaSequencial, *vetorDeSaidaConcorrente;
    pthread_mutex_init(&lock, NULL);


    /* Garantia de confiabilidade dos parâmetros de entrada */
    if(argc < 3){
        fprintf(stderr, "Digite: %s <número de elementos> <número de threads>\n", argv[0]);
        return 1;
    }
    numeroDeElementos = atoll(argv[1]);
    numeroDeThreads = atoi(argv[2]);
    if(numeroDeThreads > numeroDeElementos) numeroDeThreads = numeroDeElementos;


    /* Alocação e verificação de memória para os vetores de entrada e saída */
    vetorDeEntrada = (int *)malloc(sizeof(int) * numeroDeElementos);
    if(vetorDeEntrada == NULL){
        fprintf(stderr, "ERRO: Memória insuficiente. Tente com um vetor menor.\n");
        return 2;
    }
    vetorDeSaidaSequencial = (double *)malloc(sizeof(double) * numeroDeElementos);
    if(vetorDeSaidaSequencial == NULL){
        fprintf(stderr, "ERRO: Memória insuficiente.  Tente com um vetor menor.\n");
        return 2;
    }
    vetorDeSaidaConcorrente = (double *)malloc(sizeof(double) * numeroDeElementos);
    if(vetorDeSaidaConcorrente == NULL){
        fprintf(stderr, "ERRO: Memória insuficiente.  Tente com um vetor menor.\n");
        return 2;
    }


    inicializarVetor(vetorDeEntrada, numeroDeElementos);
    resetarVetores(vetorDeSaidaSequencial, vetorDeSaidaConcorrente, numeroDeElementos);
    processarPrimos_Sequencial(vetorDeEntrada, vetorDeSaidaSequencial, numeroDeElementos);
    processarPrimos_Concorrente(vetorDeEntrada, vetorDeSaidaConcorrente, numeroDeElementos, numeroDeThreads);
    compararResultados(vetorDeSaidaSequencial, vetorDeSaidaConcorrente, numeroDeElementos);
    exibirTemposDeExecucao();


    /* Liberação da memória usada pelo programa */
    pthread_mutex_destroy(&lock);
    free(vetorDeEntrada);
    free(vetorDeSaidaSequencial);
    free(vetorDeSaidaConcorrente);

    return 0;
}


/* Documentação */
void inicializarVetor(int *vetor, long long int dimensao){
    for(long long int i = 0; i < dimensao; i++)
        vetor[i] = (rand() % 1000000); /* Gera um inteiro aleatório. */
    return;
}

/* Documentação */
void resetarVetores(double *vetorA, double *vetorB, long long int dimensao){
    for(long long int i = 0; i < dimensao; i++){
        vetorA[i] = 0;
        vetorB[i] = 0;
    }
    return;
}

/* Documentação */
int ehPrimo(int numero){
    if((numero <= 1) || (numero == 2) || (numero % 2 == 0)) return 0;
    for(int i = 3; i < sqrt(numero) + 1; i += 2){
        if(numero % i == 0) return 0;
    }
    return 1;
}

/* Documentação */
void processarPrimos_Sequencial(int *vetorDeEntrada, double *vetorDeSaida, long long int dimensao){
    GET_TIME(inicio);
    for(long long int i = 0; i < dimensao; i++){
        if(ehPrimo(vetorDeEntrada[i]))
            vetorDeSaida[i] = sqrt(vetorDeEntrada[i]);
        else
            vetorDeSaida[i] = vetorDeEntrada[i];
    }
    GET_TIME(fim);
    tempoSequencial = fim - inicio;
    return;
}

/* Documentação */
void processarPrimos_Concorrente(int *vetorEntrada, double *vetorDeSaida, long long int dimensao, int nThreads){

    pthread_t *tid; /* Identificadores que o sistema usa para as threads  */
    ArgumentosDaThread *argumentos;

    /* Alocação e verificação de memória para os ponteiros de uso local em 'processarPrimos_Concorrente' */
    tid = (pthread_t *)malloc(sizeof(pthread_t) * nThreads);
    if(tid == NULL){
        fprintf(stderr, "ERRO: Memória insuficiente. Tente um número menor de threads.\n");
        exit(2);
    }
    argumentos = (ArgumentosDaThread *)malloc(sizeof(ArgumentosDaThread) * nThreads);
    if(argumentos == NULL){
        fprintf(stderr, "ERRO: Memória insuficiente. Tente um número menor de threads.\n");
        exit(2);
    }

    GET_TIME(inicio);
    /* Criação de cada thread */
    for(int i = 0; i < nThreads; i++){
        (argumentos + i)->vetorEntrada = vetorEntrada;
        (argumentos + i)->vetorDeSaida = vetorDeSaida;
        (argumentos + i)->dimensao = dimensao;
        (argumentos + i)->nThreads = nThreads;
        if(pthread_create((tid + i) ,NULL, verificarPrimalidade, (void *)(argumentos + i))){
            fprintf(stderr, "ERRO: Falha ao criar a thread %d\n", i);
            exit(3);
        }
    }
    /* Espera o término de todas as threads para prosseguir o fluxo principal. */
    for(int i = 0; i < nThreads; i++){
        if(pthread_join(*(tid + i), NULL)){
            fprintf(stderr, "ERRO: Falha ao terminar a thread %d\n", i);
            exit(4);
        }
    }
    GET_TIME(fim);
    tempoConcorrente = fim - inicio;

    free(tid);
    free(argumentos);
    return;
}

/* Documentação */
void *verificarPrimalidade(void *arg){
    ArgumentosDaThread *argumentos = (ArgumentosDaThread *)arg;
    int id_Local;

    pthread_mutex_lock(&lock);
    id_Local = indicadorGlobal;
    indicadorGlobal++;
    pthread_mutex_unlock(&lock);

    while(indicadorGlobal < argumentos->dimensao + 1){
        if(ehPrimo(argumentos->vetorEntrada[id_Local]))
            argumentos->vetorDeSaida[id_Local] = sqrt(argumentos->vetorEntrada[id_Local]);
        else
            argumentos->vetorDeSaida[id_Local] = argumentos->vetorEntrada[id_Local];

        pthread_mutex_lock(&lock);
        id_Local = indicadorGlobal;
        indicadorGlobal++;
        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
}

/* Documentação */
void compararResultados(double *vetorSequencial, double *vetorConcorrente, long long int dimensao){
    for(long long int i = 0; i < dimensao; i++){
        if(vetorSequencial[i] != vetorConcorrente[i]){
            printf("Resultado: Vetores DIFERENTES\n");
            return;
        }
    }
    printf("Resultado: Vetores IGUAIS\n");
    return;
}

/* Documentação */
void exibirTemposDeExecucao(){
    printf("Tempo SEQUENCIAL: %lf\n", tempoSequencial);
    printf("Tempo CONCORRENTE: %lf\n", tempoConcorrente);
    printf("Aceleração: %lf\n", tempoSequencial / tempoConcorrente);
    return;
}
