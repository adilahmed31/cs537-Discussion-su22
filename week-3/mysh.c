#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){
        int x = 5;
        int rc = fork();
        if (rc == 0) { //child process
                x++;
                printf("this is child with pid %d\n %d\n", getpid(),x);
                char *cmd_argv[10];
                cmd_argv[0] = "./main";
                cmd_argv[1] = "CS537";
                cmd_argv[2] = NULL;

                //some set-up work
                int fd = open("output.txt",  O_WRONLY | O_CREAT | O_TRUNC, 0666);
                dup2(fd, fileno(stdout));
                close(fd);
                execv(cmd_argv[0], cmd_argv);

                //if successful, will never return
                printf("Failure!\n");

        }
        else if (rc > 0){ //parent
                wait(NULL);
                printf("this is parent with pid %d\n %d\n", getpid(),x);
        }
        else{                //failure
        }
        printf("hello!\n");
}
