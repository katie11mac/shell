#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define INPUT_MAX_LENGTH 4096
#define MAX_NUMBER_ARGS 10

char **get_user_input();
void run_program(char *input_args[]);

int main(int argc, char *argv[])
{
    char **input_args;

    for(;;)
    {
        input_args = get_user_input();
        run_program(input_args);
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
