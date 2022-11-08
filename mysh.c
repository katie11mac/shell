#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define INPUT_MAX_LENGTH 4096
#define MAX_NUMBER_ARGS 10

char *print_prompt();
char **read_input(char *user_input);
void do_the_thing(char *input_args[]);

int main(int argc, char *argv[])
{
    char *user_input;
    char **input_args;

    for(;;)
    {
        user_input = print_prompt();
        input_args = read_input(user_input);
        do_the_thing(input_args);
    }
    return 0;
}

char *print_prompt()
{
    char *user_input;
    
    //THINK ABOUT WHERE TO FREE
    user_input = malloc(INPUT_MAX_LENGTH);
    
    printf("$ ");
    fgets(user_input, INPUT_MAX_LENGTH, stdin);

    return(user_input);
    
}

char **read_input(char *user_input)
{
    char **input_args;
    
    //THINK ABOUT WHERE TO FREE
    input_args = malloc(MAX_NUMBER_ARGS);

    input_args[0] = user_input;

    //RUN STRTOK and at the end add a null character to the array
    input_args[1] = NULL;

    return input_args;
    //REMEMBER EXIT CASES
}

void do_the_thing(char *input_args[])
{
    //process is the child
    if(fork() == 0)
    {
        execv(input_args[0], input_args);
    }
    //process is the parent
    else
    {
        wait(NULL);
    }
}
