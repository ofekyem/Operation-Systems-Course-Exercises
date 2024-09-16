
#include <stdio.h>  
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <unistd.h>


int main(int argc ,char *argv[]) { 

    // first we check if there is a correct number of arguments
    if(argc != 5 ){
        fprintf(stderr, "Usage: %s <parent_message> <child1_message> <child2_message> <count>", argv[0]);
        return 1;
    }
    
    // Lets took the arguments of messeges and put them in variables and also save the number of times each msg should be printed.
    char *parent_msg = argv[1];
    char *child1_msg = argv[2];
    char *child2_msg = argv[3];
    int times = atoi(argv[4]); 

    // Lets write to the a new/existing file named output.txt if there is a problem return error.
    FILE *file = fopen("output.txt", "w");
    if (file == NULL) {
        perror("fopen");
        return 1;
    } 

    // Lets create child1 process. if there is an error, notify about it.
    pid_t pid1;
    pid1 = fork();
    if (pid1 < 0) {
        perror("The action has failed!");
        return 1;
    }  

    // Lets create child2 process. if there is an error, notify about it.
    pid_t pid2;
    if (pid1 > 0){
        pid2 = fork();
        if (pid2 < 0) {
            perror("The action has failed!");
            return 1;
        }
    } 

    // If we enter here, that means we are in child1 process so lets print its msg.
    if (pid1 == 0){
        for(int i = 0; i < times; i++){
            fprintf(file, "%s\n", child1_msg);
        }
        fclose(file);
        exit(0);
    } 

    sleep(5); 

    // If we enter here, that means we are in child2 process so lets print its msg.
    if (pid2 == 0){
        for(int i = 0; i < times; i++){
            fprintf(file, "%s\n", child2_msg);
        }
        fclose(file);
        exit(0);
    } 

    // If we enter here, that means we are in parent process so lets wait untill child1 and child2 are done and then print parent msg.
    else{
        wait(NULL);
        wait(NULL);
        for(int i = 0; i < times; i++){
            fprintf(file, "%s\n", parent_msg);
        }
        fclose(file);
        exit(0);
    } 

    fclose(file);
    return 0;
}