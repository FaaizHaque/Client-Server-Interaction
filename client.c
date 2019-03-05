#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define shsize 10000
#define BUFSIZE 100
int main(int argc, char** argv)
{
    	//Check if 3 arguments are specified (4 including program name)
    	if ( argc != 4 )
   	{
        	printf("Invalid Arguments Passed \n");
        	exit(0);
    	}

    	//Initialization of arguments
    	char* shm_name = argv[1];
    	char keyword[128];
   	char* sem_name = argv[3];
	
	for ( int i = 0; i < 128; i++ )
		keyword[i] = argv[2][i];


	sem_t * sem_mutex;
	char *mutex_name = strcat(sem_name, "_mutex");
	
	sem_mutex = sem_open(mutex_name, O_RDWR);
	if ( sem_mutex < 0 )
	{
		printf("Can not open semaphore mutex\n");
		exit(1);
	}

	//Structure of shared memory
	//--------------------------------------------
        struct requestQueueData{
                char word[128];
                int index;
        };

        struct requestQueue{
                struct requestQueueData rqdata[10];
                int back;
                int front;
                int count;
        };

        struct resultQueueData{
            int back; //Back of queue (variables go in from here)
            int front; //Front of queue(variables exit from here)
            long data[BUFSIZE];
            int count; //Num of current items
        };

        struct structure{
                struct requestQueue request;
                struct resultQueueData result[10];
                int queue_state[10];
        };
   
	//-------------------------------------------

	int fd;
	void *p;
	struct structure *sp;


	//Open shared memory created in server
	fd = shm_open(shm_name, O_RDWR, 0660);

	if ( fd < 0 )
	{
		printf("Can not access shared memory");
		exit(1);
	}

	//Map the shared memory object to address space
    p = mmap(NULL, sizeof(struct structure) + 1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	

		if ( p < 0 )
		{
			printf("Can not map shared memory");
			exit(1);
		}
		close(fd);
		
		sp = ( struct structure * ) p;

		int index;
		for ( index = 0; index < 10; index++ )
		{
			if ( sp->queue_state[index] == 0 )
			{
				break;
			}
			else if ( index == 9 )
			{
				index = 0;
			}
		}
		
		sp->queue_state[index] = 1;

		sp->request.back = (sp->request.back+1) % 10;
		sp->request.rqdata[sp->request.back].index = index;
	
	for ( int i = 0; i <128; i++ )
	 	sp->request.rqdata[sp->request.back].word[i] = keyword[i];
	
	sp->request.count++;

	//int flag = 0;
	long temp = 0; //Variables to print
	int counter = 0;
	while(1)
	{
		//Server finished sending outputs if temp is -1

		//Queue is empty, so remain in a waiting loop
		while ( sp->result[index].count == 0 )
		{
		}	
		//If reached here, queue is not empty no more
                	temp = sp->result[index].data[sp->result[index].front];
			sp->result[index].front = (sp->result[index].front + 1)% BUFSIZE;
			sem_wait(sem_mutex);
			sp->result[index].count--;
			sem_post(sem_mutex);

			if ( temp == -1 )
			{
				sem_wait(sem_mutex);
				sp->result[0].count--;
				sem_post(sem_mutex);
				break;
			}
			if ( temp != 0 )
			{
				printf("%ld\n", temp);
				counter++;		
			}
	}
	
//	sp->queue_state[index] = 0;
	
	//sem_close(sem_mutex);
	//sem_unlink(mutex_name);
	//shm_unlink(shm_name);
	
	printf("%d", counter);
	exit(1);

	return 0;		
}

