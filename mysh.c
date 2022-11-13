#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>              /* Definition of O_* constants */
#include <unistd.h>

#define INPUT_MAX_LENGTH 4096
#define MAX_NUMBER_ARGS 10

char **get_user_input();
void parse_user_input(char *input_args[]); 

int main(int argc, char *argv[])
{

    for(;;)
    {
        get_user_input();
    }

    return 0;
}

char **get_user_input()
{
    char original_user_input[INPUT_MAX_LENGTH];
    char *edited_user_input;
    static char *input_args[MAX_NUMBER_ARGS];
    int arg_index; // index of the argument in the array

    char *pipe_args;
    int pipe_fds[2];

    pipe_fds[0] = 0;
    pipe_fds[1] = 1;

    printf("$ ");
    fgets(original_user_input, INPUT_MAX_LENGTH, stdin);
    edited_user_input = strtok(original_user_input, "\n");

    if(strcmp(edited_user_input, "exit") == 0)
    {
        exit(0);
    }

    pipe_args = strtok(edited_user_input, "|");

    while(pipe_args != NULL)
    {
        arg_index = 0; 
        input_args[arg_index] = strtok(pipe_args, " "); 
        arg_index += 1; 
        while((input_args[arg_index] = strtok(NULL, " ")) != NULL)
        {
            arg_index += 1; 
        }
        pipe(pipe_fds); // What if we need to fork for pipe
        parse_user_input(input_args); 

        pipe_args = strtok(NULL, "|"); // increment step
    }

    // strtok on | 
    // go through 

    return input_args;
}

void parse_user_input(char *input_args[])
{
    int i, j;
    int fd;
    int is_redirect;
    int exit_value;
    static char *prev_args[MAX_NUMBER_ARGS];

    is_redirect = 0;
    i = 0;

    while(input_args[i] != NULL && is_redirect == 0)
    {
        
        // OUTPUT REDIRECTION
        if((strcmp(input_args[i], ">") == 0) || (strcmp(input_args[i], ">>") == 0)) //or >>
        {
            // put a >> check here so dont truncate
            if(strcmp(input_args[i], ">") == 0)
            {
                fd = open(input_args[i+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
            }
            else
            {
                fd = open(input_args[i+1], O_CREAT | O_WRONLY | O_APPEND, 0666);
            }
            
            if(fd == -1)
            {
                perror("open");
            }

            // Process all arguments until the redirection
            for(j = 0; j < i; j++)
            {
                prev_args[j] = input_args[j];
            }

            //process is the child
            if(fork() == 0)
            {
                if(dup2(fd, 1) == -1)
                {
                    perror("dup2");
                }

                if(execvp(prev_args[0], prev_args) == -1)
                {
                    perror("evecvp");
                }
            }
            //process is the parent
            else
            {
                wait(&exit_value);
            }
            
            close(fd);

            is_redirect = 1;
            //CREATE ONE BIG FUCNTION AND THEN MAKE SMALLER FUNCTIONS AFTER
        }
        //if <, open file.txt (RDONLY, no mode), change fd to zero, set and send run_program prev_args

        // INPUT REDIRECTION
        if(strcmp(input_args[i], "<") == 0)
        {
            
            fd = open(input_args[i+1], O_RDONLY);
            
            if(fd == -1)
            {
                perror("open");
            }

            // Process all arguments until the redirection
            for(j = 0; j < i; j++)
            {
                prev_args[j] = input_args[j];
            }

            //process is the child
            if(fork() == 0)
            {
                if(dup2(fd, 0) == -1)
                {
                    perror("dup2");
                }

                if(execvp(prev_args[0], prev_args) == -1)
                {
                    perror("evecvp");
                }
            }
            //process is the parent
            else
            {
                wait(&exit_value);
            }
            
            close(fd);

            is_redirect = 1;
        }
        i++;
    }

    if(is_redirect == 0)
    {
        //process is the child
        if(fork() == 0)
        {
            if(execvp(input_args[0], input_args) == -1)
            {
                perror("evecvp");
            }
        }
        //process is the parent
        else
        {
            wait(&exit_value);
        }
    }
    

}
