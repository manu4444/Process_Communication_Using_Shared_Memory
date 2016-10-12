/************************************
Name: 			Manu Khandelwal
Student #ID: 	5272109
CSE Lab Login: 	khand055
Assignment No: 	1-PartB
**************************************/



#include <stdlib.h> 	/* for random integer */
#include <stdio.h>		/* for standard I/O */
#include <unistd.h>		/* for fork */
#include <sys/types.h> 	/*pid_t defined*/
#include <signal.h>    	/* for Signal Handling */
#include <sys/shm.h>   	/* For Shared Memory */
#include <sys/wait.h>

#define BUFFER_SIZE 1024
void signal_handler(int);
FILE *fps;
FILE *fpd;
char *source_filename;//="source-file";
char *destination_filename = "destination-file";
int pid_parent, pid_child;


int shmem_id_buffer, shmem_id_count;       	 	/* shared memory identifier for buffer and for count */
int *shmem_ptr_buffer, *shmem_ptr_count;     	/* pointer to shared segment */

key_t key_buffer = 4455, key_count = 4466;   	/* A key to access shared memory segments */
int flag = 1023, status;           						/* Controls things like r/w permissions */


int main(int argc, char *argv[]) {

	if( argc <=1 ){
		printf("Please provide the file name\n");
		exit(0);
	}
    source_filename = argv[1];
    if( signal( SIGUSR1, signal_handler) == SIG_ERR  ) {
        printf("Parent: Unable to create handler for SIGUSR1\n");
    }

    if( signal( SIGUSR2, signal_handler) == SIG_ERR  ) {
        printf("Child: Unable to create handler for SIGUSR2\n");
    }


    /* Creating Shared memory segment for the buffer data*/
    shmem_id_buffer = shmget (key_buffer, sizeof(char)*BUFFER_SIZE, flag);
    if (shmem_id_buffer == -1)
    {
        perror ("shmget failed");
        exit (1);
    }

    /* Creating sharede memory segment for the count */
    shmem_ptr_buffer = shmat (shmem_id_buffer, (void *) NULL, flag);
    if (shmem_ptr_buffer == (void *) -1)
    {
        perror ("shmat failed");
        exit (2);
    }

    /* Attaching the shared memory segment for buffer to address space */
    shmem_id_count = shmget (key_count, sizeof(int), flag);
    if (shmem_id_count == -1)
    {
        perror ("shmget failed");
        exit (1);
    }

    /* Attaching the shared memory segment for count to address space */
    shmem_ptr_count = shmat (shmem_id_count, (void *) NULL, flag);
    if (shmem_ptr_count == (void *) -1)
    {
        perror ("shmat failed");
        exit (2);
    }


    printf( "Parent pid = %d\n", pid_parent=getpid() );
    remove(destination_filename);

    if( (pid_child = fork()) == 0 ) {
        printf("Child process started\n" );
        pid_child = getpid();
        for( ;; );


    } else if ( pid_child > 0 ) { // Parent process

        printf("Parent (%d) process started\n", pid_parent);
        
        fps=fopen(source_filename, "rb");
        shmem_ptr_count[0] = fread(shmem_ptr_buffer, 1, BUFFER_SIZE, fps);
 
        printf("Parent (%d) is sending signal to child (%d) to start copying %d characters\n", pid_parent, pid_child, shmem_ptr_count[0]);
        kill( pid_child, SIGUSR2 );
        //for( ;; );
        waitpid(pid_child, &status, 0);
        if( WIFEXITED( status ) ) {
            printf("Parent is exiting !! \n");
            fclose(fps);
            exit(0);
        }


    } else {
        // fork failed
        printf("fork() failed!\n");
        return 1;
    }

    printf("Control has come to the very end\n");
}


void signal_handler(int signo)
{
    /* signo contains the signal number that was received */
    switch( signo )
    {
    /* Signal is a SIGUSR1 */
    case SIGUSR1:
        if( pid_parent == getpid() ) { /* it is the parent */
            printf("\nParent (%d) has received the signal from the child \n", pid_parent );

                if( feof(fps) ){
                    shmem_ptr_count[0] = EOF;

                } else {
                    shmem_ptr_count[0] = fread(shmem_ptr_buffer, 1, BUFFER_SIZE, fps);
                }

                kill( pid_child, SIGUSR2 );
        } 
        break;

    /* Signal is a SIGUSR2 */
    case SIGUSR2:

        if( pid_parent != getpid() ) { // It is a child process

            if( shmem_ptr_count[0] == EOF ) {
                
                printf("Child is exiting !! \n");
                exit(0);
                //kill( pid_parent, SIGUSR2 );
            }

            printf("Child (%d) has received the signal from the parent (%d) and started to copy %d characters \n", getpid(), pid_parent, shmem_ptr_count[0] );
            fpd=fopen(destination_filename, "ab");

            fwrite (shmem_ptr_buffer , sizeof(char), shmem_ptr_count[0], fpd);

            fclose(fpd);
            printf("\nChild (%d) sending signal to parent (%d) about finished task copying %d characters\n", getpid(), pid_parent, shmem_ptr_count[0]);
            shmem_ptr_count[0] = 0;

            kill( pid_parent, SIGUSR1 );
        } 
        break;

	defaut:
        break;
    }
}
