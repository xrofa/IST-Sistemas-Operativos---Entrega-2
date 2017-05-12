/*
// Projeto SO - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2016-17
*/

#include "commandlinereader.h"
#include "contas.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#define COMANDO_DEBITAR "debitar"
#define COMANDO_CREDITAR "creditar"
#define COMANDO_LER_SALDO "lerSaldo"
#define COMANDO_SIMULAR "simular"
#define COMANDO_SAIR "sair"
#define COMANDO_ARG_SAIR_AGORA "agora"
#define COMANDO_STATUS "status"
#define NUM_TRABALHADORAS 3
#define CMD_BUFFER_DIM  (NUM_TRABALHADORAS * 2)
#define C 3
#define OP_DEBITAR 0
#define OP_CREDITAR 1
#define OP_LER_SALDO 2
#define COMANDO_PRINT "print"
#define MAXARGS 3
#define BUFFER_SIZE 100
#define COMANDO_GO "go"

#define MAXFILHOS 20

  typedef struct
     {
       int operacao;
       int idConta;
       int valor;
} comando_t;

comando_t cmd_buffer[CMD_BUFFER_DIM];
int buff_write_idx = 0, buff_read_idx = 0;
pthread_t tid[NUM_TRABALHADORAS];
pthread_mutex_t  mutex; /* Variavel mutex */
extern pthread_mutex_t  mutexcontas[NUM_CONTAS]; /* Variavel mutex */
int estado = 1;
sem_t semaforoescrita, semaforoleitura;
int returnthread;
int comandosexecutados=0;
int comandosemexecucao=0;

/*
	Terminar as threads
*/
	void terminarthreads(){
		int i;
		int ptr[NUM_TRABALHADORAS];
		printf("O i-banco vai terminar\n--\n");
		comandosemexecucao++;
		for(i=0; i<NUM_TRABALHADORAS; i++){
			if (pthread_join(tid[i], NULL) != 0){
          	printf("\nErro ao finalizar thread %d\n", errno);
          	exit(1);
          	}/*
          	else{
          		printf("thread[%d]%d: return value from thread is [%d]\n",tid[i],i, ptr[i]);
          	}*/
    	}
    	pthread_mutex_destroy(&mutex);
    	
    	for(i=0; i<NUM_CONTAS; i++){
    		pthread_mutex_destroy(&mutexcontas[i]);
    	}
    	sem_destroy(&semaforoescrita);
    	sem_destroy(&semaforoleitura);
    	comandosemexecucao--;
    	printf("\n--\nO i-banco terminou\n--\n");
    	pthread_exit(&returnthread);
    	
	} 

/*
	Inicializar semaforos
*/
	void iniciarsemaforos(){
	
		sem_init(&semaforoescrita, 0, 0);
		sem_init(&semaforoleitura, 0, 0);
	
	
	}

/*
	Coloca os pedidos no buffer
*/  
  void produtor(int id, int value, int op){
  	
  	
  	if(buff_write_idx == (NUM_TRABALHADORAS * 2) && buff_read_idx == (NUM_TRABALHADORAS * 2)){
  		buff_write_idx=0;
  		buff_read_idx=0;
  	}
  	
  	comandosemexecucao++;
  	sem_wait(&semaforoescrita);
  	pthread_mutex_lock(&mutex);
  	cmd_buffer[buff_write_idx].operacao= op;
 	cmd_buffer[buff_write_idx].idConta = id;
    cmd_buffer[buff_write_idx].valor = value;
  	buff_write_idx++; /* Aumenta o index de escrita para o proximo */
  	
  	comandosemexecucao--;
  	pthread_mutex_unlock(&mutex);
  	sem_post(&semaforoescrita);
  	
  	/*printf("Stored in Buffer as: %d, idConta= %d, Valor= %d // buff_write_idx= %d // buff_read_idx=%d\n", op, id, value, buff_write_idx, buff_read_idx );*/
      			
  
  }
  
/*
	Recebe o próximo pedido disponivel no buffer - PTHREAD FUNC
*/
  void *consumidor(){
  	sleep(1);
  	while(estado==1){
  	
  	/*printf("Id: %d // consumidor():buff_write_idx: %d\n",(unsigned int)pthread_self(), buff_write_idx);*/
  
  		if(buff_write_idx>0){
  			int id, valor, op,i;
  			
  			
  			pthread_mutex_lock(&mutex);
  			sem_wait(&semaforoleitura);
  			
  			i=buff_read_idx;
  			/*printf("buff_read_idx: %d\n", i);*/
  			op=cmd_buffer[i].operacao;
  			id=cmd_buffer[i].idConta;
  		
  		
  		
  			if(op!=2){
  				valor=cmd_buffer[i].valor;
  			}
  		
  			
  		
  		
  			if(op==0 && valor!= 0){
  				comandosemexecucao++;
  				debitar (id, valor);
  				
  				printf("%s(%d, %d): OK\n\n", COMANDO_DEBITAR, id, valor);
  				buff_read_idx++;
  				comandosemexecucao--;
  			}
  			else if(op==1){
  				comandosemexecucao++;
  				creditar(id, valor);
  				
  				printf("%s(%d, %d): OK\n\n", COMANDO_CREDITAR, id, valor);
  				buff_read_idx++;
  				comandosemexecucao--;
  			}
  			else if(op==2){
  				comandosemexecucao++;
  				int saldo = lerSaldo (id);
  				
  				printf("%s(%d): O saldo da conta é %d.\n\n", COMANDO_LER_SALDO, id, saldo);
  				buff_read_idx++;
  				comandosemexecucao--;
  			}
  			
  			pthread_mutex_unlock(&mutex);
  			sem_post(&semaforoleitura);

		}
	}	
  	pthread_exit(&returnthread);
}

/*
	Recebe comando e chama o produtor para colocar o pedido no buffer
*/
	void recebecomando(){
		
		while(1){
			
			int numargs;
			char *args[MAXARGS + 1];
    		char buffer[BUFFER_SIZE];
    		
    		int idConta, valor;
    		
    		numargs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE);
    		
    		/*printf("numargs:%d\n", numargs);	*/
    		
    		if (strcmp(args[0], COMANDO_DEBITAR) == 0){
    			idConta = atoi(args[1]);
      			valor = atoi(args[2]);
      			
      			comandosexecutados++;
      			produtor(idConta, valor, OP_DEBITAR);
      			
      			/*printf("idConta: %d , valor:%d\n", idConta, valor);*/
    		}
    		else if (strcmp(args[0], COMANDO_CREDITAR) == 0){
    			idConta = atoi(args[1]);
      			valor = atoi(args[2]);
      			
      			comandosexecutados++;
      			produtor(idConta, valor, OP_CREDITAR);
    		}
    		else if (strcmp(args[0], COMANDO_LER_SALDO) == 0){
    			idConta = atoi(args[1]);
      			
      			comandosexecutados++;
      			produtor(idConta, 0, OP_LER_SALDO);
    		}
    		else if(strcmp(args[0], COMANDO_SAIR) == 0){
    			printf("\n-- \n");
    			estado=0;
    			comandosexecutados++;
    			terminarthreads();
    			
    		}
    		
    		else if(strcmp(args[0], COMANDO_STATUS) == 0){
    		
    			printf("\n--\nComandos em execucao: %d\n--\n", comandosemexecucao);
    			printf("\n--\nComandos executados: %d\n--\n", comandosexecutados);
    		
    		
    		
    		}
    		else if(strcmp(args[0], COMANDO_PRINT) == 0){
    			int cena;
    			cena= atoi(args[1]);
    	
   			 	printf("PRINT: CMD_BUFFER:\n OP: %d, idConta: %d, Valor: %d\n", cmd_buffer[cena].operacao,cmd_buffer[cena].idConta, cmd_buffer[cena].valor );
  
    		}
    		else if(strcmp(args[0], COMANDO_GO) == 0){
    			/*printf("\n Before consumidor(), buff_write_idx: %d\n", buff_write_idx);*/
    			/*printf("\n Before consumidor(), buff_read_idx: %d\n", buff_read_idx);*/
    			consumidor();
    			/*printf("\n After consumidor(), buff_write_idx: %d\n", buff_write_idx);*/
    			/*printf("\n After consumidor(), buff_read_idx: %d\n", buff_read_idx);*/
    		}
    		else
    			printf("Comando desconhecido!\n");
		
		}
	
	}

/*
	Inicializar as threads
*/
  void thread_init(int threadnumber){
  int z;

  
  for(z=0;z<threadnumber; z++){
  	if(pthread_create(&tid[z], NULL, consumidor, NULL) != 0){
  		printf("Erro na criacao da tarefa\n");
  		exit(1);
  	}
  	else{}
  		/*printf("Criada a thread consumidor %d\n", (int)tid[z]);*/
  }
  
  } 


/*
	MAIN
*/
int main (int argc, char** argv) {
  

  printf("Bem-vinda/o ao i-banco\n\n");
  inicializarContas();
  thread_init(NUM_TRABALHADORAS);
  pthread_mutex_init(&mutex,NULL);
  iniciarsemaforos();
  recebecomando();
  
  
  
  
  /* Associa o signal SIGUSR1 'a funcao trataSignal.
     Esta associacao e' herdada pelos processos filho que venham a ser criados.
     Alternativa: cada processo filho fazer esta associacao no inicio da
     funcao simular; o que se perderia com essa solucao?
     Nota: este aspeto e' de nivel muito avancado, logo
     so' foi exigido a projetos com nota maxima  
  */
  if (signal(SIGUSR1, trataSignal) == SIG_ERR) {
    perror("Erro ao definir signal.");
    exit(EXIT_FAILURE);
  }
  
 
}
