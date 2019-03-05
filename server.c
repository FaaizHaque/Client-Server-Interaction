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
#include <ctype.h>
#include <semaphore.h>
#include <pthread.h>

//Shared memory size
#define shsize 10000
#define BUFSIZE 100

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char* searchWord(char* buff, char* keyword){
        int i, j, val;
        int filelength = strlen(buff);
        int wordlength = strlen(keyword);
        int lineNo = 1;
	char* retstr = "";
	char numstr[1000];
	int temp;
	temp = -1;
        for(i = 0; i < filelength-wordlength; i++){
                if(buff[i] == '\n'){
                        lineNo = lineNo + 1;
                }
                if(buff[i] == keyword[0]){
                        val = 1;
                        for(j = 0; j < wordlength; j++ ){
                                if(buff[i+j] == '\0'){
                                       break;
                                }
                                if(buff[i + j] != keyword[j]){
                                        val = 0;
                                }
                        }
                        if((val == 1) & (lineNo != temp)){
				sprintf(numstr, "%d ", lineNo);
				retstr = concat(retstr, numstr);
				temp = lineNo;
                        }
                }
        }
	return retstr;
}
               

int main(int argc, char ** argv)
{
    //Check if 3 arguments are specified (4 including program name)
    if ( argc != 4 )
    {
        printf("Invalid Arguments Passed \n");
        exit(1);
    }

	
    //Initialization of arguments
    char* shm_name = argv[1];
    char* inputfilename = argv[2];
    char* sem_name = argv[3];
	
	sem_t * sem_mutex;
	char *mutex_name = strcat(sem_name, "_mutex");

	//Clean up any shms with the same name
	shm_unlink(shm_name);

	sem_unlink(mutex_name);

	//SEMAPHORES

	sem_mutex = sem_open(mutex_name, O_RDWR | O_CREAT, 0660, 1);
	if ( sem_mutex < 0 )
	{
		printf("Can not create sempahore mutex\n");
		exit(1);
	}

	
	/* create the text file buffer */
	FILE* filepointer;
        filepointer = fopen(inputfilename, "r");
	fseek( filepointer, 0L, SEEK_END );
        size_t size = ftell(filepointer);
        fseek( filepointer, 0L, SEEK_SET );
        char* buffer = malloc( size );
        fread( buffer, 1, size, filepointer );
      	//printf("%s\n", buffer);
	/*-------------------------*/





    //Structure of shared memory

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
    
 
    //Pointer to the shared data
    struct structure *sp;
    void *p;
    int fd;

    //Create shared memory
    //S_IROTH :read permission, others; S_IWOTH : write permission, others; to allow client to read and write the shm

    fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);

    //ftruncate allocates size of shm
    if ( fd == -1 || ftruncate(fd, sizeof(struct structure) + 1000) == -1 )
    {
        printf("Shared memory segment couldn't be created due to errors\n");
        exit(1);
    }

    //Map the shared memory object to address space
    p = mmap(NULL, sizeof(struct structure) + 1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if ( p == MAP_FAILED )
    {
        printf("Shared memory wasn't mapped to address space");
        exit(1);
    }

	//Pointer to shared data
   	 sp = ( struct structure * ) p;
	
	//Initialize shm
	for ( int i = 0; i < 10; i++ )
	{
		sp->queue_state[i] = 0;
		sp->result[i].front = 0;
		sp->result[i].back = -1;
		sp->result[i].count = 0;
	}
		sp->request.front = 0;
		sp->request.back = -1;
		sp->request.count = 0;


void executeServer( char* buffer, int index, char * word )
{
        char *temp;
       // while (1)
       // {

                        temp = searchWord(buffer, word);
                        char *z = temp;
                        //While there is more characters to parse       
                        while ( *z  )
                        {
                                if ( isdigit(*z))
                                {
                                        //Val takes values of the line 
                                        //numbers one by one as it goes in in the loop
                                        long val = strtol(z, &z, 10);
 
                                        //Queue is full, so remain in a waiting loop 
                                        while ( sp->result[index].count == BUFSIZE - 1 )
                                        {
                                        }
                                        //If reached here, queue is not full so now send val to result queue
      
                                        //If queue is empty, we add only one element, so both
                                        //front and rear become the first index (0)



                                                sp->result[index].back = (sp->result[index].back + 1) % BUFSIZE;

						sp->result[index].data[sp->result[index].back] = val;
                                                sem_wait(sem_mutex);
                                                sp->result[index].count++;
                                                sem_post(sem_mutex);
                                }
                                else
                                {
                                     //If we encounter a space we reach here and move on
                                     z++;
                                }
                        }
                        //We finished parsing all integers
                        //Now we must tell client we are finished
                        //So we send a -1       
                        while ( sp->result[index].count == BUFSIZE - 1 )
                        {}
                        sp->result[index].back = (sp->result[index].back + 1) % BUFSIZE;
                        sp->result[index].data[sp->result[index].back] = -1;
                        sem_wait(sem_mutex);
                        sp->result[index].count++;
                        sem_post(sem_mutex);
                        return;

       // }

}



	int index2;
	char * temp2;
	//Looks for requests
	while(1)
	{
		index2 = 0;
		temp2 = "";
		while ( sp->request.count == 0 )
		{}
		
		index2 = sp->request.rqdata[sp->request.front].index;
		temp2 = sp->request.rqdata[sp->request.front].word;
		sp->request.front = (sp->request.front +1) % 10;

	//	sp->queue_state[index2] = 1;
	
		executeServer(buffer, index2, temp2);
		sp->request.count--;
		
		
//		sp->queue_state[index2] = 0;
		
	}
	
	

    return 0;
}

