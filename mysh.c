#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define INPUT_MAX_LENGTH 4096
#define MAX_NUMBER_ARGS 10

char **get_user_input();
void parse_user_input(char *input_args[]); 
void run_program(char *input_args[]);

int main(int argc, char *argv[])
{
    char **input_args;

    for(;;)
    {
        input_args = get_user_input();
        parse_user_input(input_args);
    }

    return 0;
}

char **get_user_input()
{
    char original_user_input[INPUT_MAX_LENGTH];
    char *edited_user_input;
    static char *input_args[MAX_NUMBER_ARGS];
    int counter; 

    printf("$ ");
    fgets(original_user_input, INPUT_MAX_LENGTH, stdin);
    edited_user_input = strtok(original_user_input, "\n");

    counter = 0; 
    // ASSUME ONLY GIVE ONE ARGUMENT
    //trailing spaces on exit? 
    input_args[counter] = strtok(edited_user_input, " "); 
    counter += 1; 
    while((input_args[counter] = strtok(NULL, " ")) != NULL)
    {
        counter += 1; 
    }

    if(strcmp(input_args[0],"exit") == 0)
    {
        exit(0);
    }

    return input_args;
}

void parse_user_input(char *input_args[])
{
    int i, j;
    int fd;
    int is_redirect;
    char *prev_args[MAX_NUMBER_ARGS];

    is_redirect = 0; 

    for(i = 0; i < INPUT_MAX_LENGTH; i++)
    {
        if(strcmp(input_args[i], ">") == 0)
        {
            if((fd = open(input_args[i+1], O_CREAT | O_RDWR | O_TRUNC)) == -1)
            {
                perror("open");
            }

            if(dup2(fd, 1) == -1)
            {
                perror("dup2");
            }

            is_redirect = 1; 

            break; 
        }
    }

    if(is_redirect)
    {
        for(j = 0; j < i; j++)
        {
            prev_args[j] = input_args[j]; 
        }

        run_program(prev_args);
        
        close(1); 
    }
    else
    {
        run_program(input_args);
    }
    

}

void run_program(char *input_args[])
{
    int exit_value;
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
