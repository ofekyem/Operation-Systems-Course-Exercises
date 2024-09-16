
#include <stdio.h>  
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

// The function we got in the task
void write_message(const char *message, int count) {
    for (int i = 0; i < count; i++) {
        printf("%s\n", message);
        usleep((rand() % 100) * 1000); // Random delay between 0 and 99 milliseconds
    }
}

int main(int argc, char *argv[]) { 

    // first we check if there is a correct number of arguments
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <message1> <message2> ... <count>\n", argv[0]);
        return 1;
    }
    // Lets create the lockfile and clean it. then we take the arguments information: 
    // The number of messeges that indicates the number of procceses that will run, 
    // And the number of times to print each msg.
    const char *lockfile = "lockfile.lock";
    unlink(lockfile);
    int times = atoi(argv[argc - 1]);
    int msg_num = argc - 2;
     
    // Lets create an array of pids to parent and his children procceses. 
    pid_t pids[msg_num];
    
    // Lets go in a loop to run on all the procceses
    for (int i = 0; i < msg_num; i++) {

        pids[i] = fork(); 
        // If the fork failed, bring an error.
        if (pids[i] < 0) {
            perror("The action has failed!");
            return 1;
        } 

        // If we enter here we're in the i-child process.
        if (pids[i] == 0) { 

            // Lets save the i-child msg into a variable.
            const char *msg = argv[i+1]; 
            // Lets run in a loop untill the lock is released
            while (1) { 
                // Lets attempt to create and open a new lockfile to the current process 
                int file = open(lockfile, O_CREAT | O_EXCL, 0666); 
                // When it cant open a new file meaning that another process is in the lock right now
                if (file != -1) {
                    close(file);
                    break;
                } 
                // Wait and try again to open the lock
                usleep(1000); 
            }
            // Lets use the function in order to print the current procces messege in the number of times we got from user
            write_message(msg, times);  
            // Lets clean the buffer 
            fflush(stdout);  
            // Lets delete the lock file
            unlink(lockfile);
            exit(0);
        }
        sleep(3);
    }
    // The parent process wait all children procceses to end.
    for (int i = 0; i < msg_num; i++) {
        waitpid(pids[i], NULL, 0);
    }
    return 0;
}
