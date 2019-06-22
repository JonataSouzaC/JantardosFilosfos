#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "string.h"
#include "sys/times.h"
#define N 5
#define PENSAR 0
#define FOME 1
#define COMER 2
#define ESQUERDA (nfilosofo+4)%N //agarrar garfo
                                 //da esquerda
#define DIREITA (nfilosofo+1)%N  //agarrar garfo
                                 //da direita
void *filosofo(void *num);
void agarraGarfo(int);
void deixaGarfo(int);
void testar(int);

struct shared_data { // porcao da memoria responsavel por determinar acessos, ela deve ser compartilhada entre os processosa traves da mmap(..)
    sem_t spoon; // regiao critica/mutex
    sem_t filosofo[N+1]; // mutex de cada filosofo
    int state[N+1]; // estado de cada filosofo
};

struct shared_data *shared; // memoria a ser compartilhada

void initialize_shared(); // inicia a memoria compartilhada
void finalize_shared();   // finaliza a memoria compartilhada

void *filosofo(void *num)
{

   while(1)
   {

      int *i = num;
      sleep(1);
      agarraGarfo(*i);
      sleep(1);
      deixaGarfo(*i);

   }
}

void agarraGarfo(int nfilosofo)
{
   sem_wait(&shared->spoon);
   shared->state[nfilosofo] = FOME;
   printf("Filosofo %d tem fome.\n", nfilosofo+1);
   //+1 para imprimir filosofo 1 e nao filosofo 0
   testar(nfilosofo);
   sem_post(&shared->spoon);
   sem_wait(&shared->filosofo[nfilosofo]);
   sleep(1);
}

void deixaGarfo(int nfilosofo)
{
   sem_wait(&shared->spoon);
   shared->state[nfilosofo]=PENSAR;
   printf("Filosofo %d deixou os garfos %d e %d.\n", nfilosofo+1, ESQUERDA+1, nfilosofo+1);
   printf("Filosofo %d esta a pensar.\n", nfilosofo+1);
   testar(ESQUERDA);
   testar(DIREITA);
   sem_post(&shared->spoon);
}

void testar(int nfilosofo)
{
   if( shared->state[nfilosofo]==FOME &&  shared->state[ESQUERDA]
 !=COMER &&  shared->state[DIREITA]!=COMER)
   {
       shared->state[nfilosofo]=COMER;
      sleep(2);
      printf("Filosofo %d agarrou os garfos %d e %d.\n", nfilosofo+1, ESQUERDA+1, nfilosofo+1);
      printf("Filosofo %d esta a comer.\n", nfilosofo+1);
      sem_post(&shared->filosofo[nfilosofo]);
   }
}

int main() {
    int i;
    pid_t pid, pids[N]; 
    initialize_shared();

    for(i=0;i<N;++i)
    {
        pid = fork();
        if(pid==0)
        {
            // filhos
            filosofo(&i);
            _exit(0);
        }
        else if(pid>0)
        {
            //outros pids
            pids[i] = pid;
            printf("pids[%d]=%d\n",i,pids[i]);
        }
        else
        {
            perror("fork");
            _exit(0);
        }
    }
    // espera processos filhos terminarem
    for(i=0;i<N;++i) waitpid(pids[i],NULL,0);

    finalize_shared();
    return (0);

}

void initialize_shared() // cria memora compartilhada entre processos pesados
{
    int i;
    int prot=(PROT_READ|PROT_WRITE); // representa que ele pode ser lido ou escrito
    int flags=(MAP_SHARED|MAP_ANONYMOUS); // representa que eh uma memoria compartilhada anonima
        shared=mmap(0,sizeof(*shared),prot,flags,-1,0); // funcao para criar a memoria compartilhada
        memset(shared,'\0',sizeof(*shared));
    sem_init(&shared->spoon,1,1);
    for(i=0;i<N;++i) sem_init(&shared->filosofo[i],1,1);
}

void finalize_shared()
{
    int i;
    for(i=0;i<N;++i) sem_destroy(&shared->filosofo[i]); // destroi os semaforos
    munmap(shared, sizeof(*shared)); // destroi a memoria compartilhada
}


