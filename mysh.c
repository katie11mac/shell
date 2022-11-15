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

void get_user_input();
void parse_user_input(char *input_args[]); 

int main(int argc, char *argv[])
{

    for(;;)
    {
        get_user_input();
    }

    return 0;
}

void get_user_input()
{
    char original_user_input[INPUT_MAX_LENGTH];
    char *edited_user_input;

    static char *all_args[MAX_NUMBER_ARGS];
    int all_args_index;

    static char *left_command_args[MAX_NUMBER_ARGS];
    static char *right_command_args[MAX_NUMBER_ARGS];
    int arg_index_l; // index of the argument in the array
    int arg_index_r; // index of the arguments in the second array (other side of pipe)

    int exit_value;
    int i;
    
    int j;

    char *pipe_left_arg;
    char *pipe_right_arg;
    int pipe_fds[2];

    //print prompt and get user input
    printf("$ ");
    fgets(original_user_input, INPUT_MAX_LENGTH, stdin);

    //get rid of \n character
    edited_user_input = strtok(original_user_input, "\n");

    //check if command is exit
    if(strcmp(edited_user_input, "exit") == 0)
    {
        exit(0);
    }

    //fill an array of strings to the left and right of each |
    all_args[0] = strtok(edited_user_input, "|");
    all_args_index = 1;
    while((all_args[all_args_index] = strtok(NULL, "|")) != NULL)
    {
        all_args_index += 1; 
    }

    printf("first arg = %s\n", all_args[0]);
    printf("second arg = %s\n", all_args[1]);

    //IF THERE ARE NO PIPES
    if(all_args_index == 1)
    {
        pipe_left_arg = all_args[0];

        arg_index_l = 0;

        left_command_args[arg_index_l] = strtok(pipe_left_arg, " "); //seperates each argument
        arg_index_l += 1; 

        while((left_command_args[arg_index_l] = strtok(NULL, " ")) != NULL)
        {
            arg_index_l += 1; 
        }

        parse_user_input(left_command_args);

    }
    //IF THERE ARE PIPES
    //iterate through each pair of arguments on either side of the pipe
    for(j = 0; j < (all_args_index - 1); j++)
    {
        pipe_left_arg = all_args[j];
        pipe_right_arg = all_args[j+1];

        arg_index_l = 0; 
        arg_index_r = 0;

        //1. CREATE STRING ARRAY FOR LEFT SIDE OF PIPE
        left_command_args[arg_index_l] = strtok(pipe_left_arg, " "); //seperates each argument
        arg_index_l += 1; 

        while((left_command_args[arg_index_l] = strtok(NULL, " ")) != NULL)
        {
            arg_index_l += 1; 
        }
        
        //2. CREATE STRING ARRAY FOR RIGHT SIDE OF PIPE
        right_command_args[arg_index_r] = strtok(pipe_right_arg, " "); //seperates each argument
        arg_index_r += 1; 

        while((right_command_args[arg_index_r] = strtok(NULL, " ")) != NULL)
        {
            arg_index_r += 1; 
        }

        printf("first input = %s\n", left_command_args[0]);
        printf("second input = %s\n", right_command_args[0]);

        //pipe to create write and read end
        if(pipe(pipe_fds) == -1)
        {
            perror("pipe");
        }

        //fork and wait twice to run both commands
        if(fork() == 0)
        {
            if((dup2(pipe_fds[1], 1)) == -1)
            {
                perror("dup2");
            }

            if(execvp(left_command_args[0], left_command_args) == -1)
            {
                perror("evecvp");
            }
        
        }
        if(fork() == 0)
        {
            if((dup2(pipe_fds[0], 0)) == -1)
            {
                perror("dup2");
            }

            if(execvp(right_command_args[0], right_command_args) == -1)
            {
                perror("evecvp");
            }
        }

        //parent waits
        for(i=0; i<2; i++)
        {
            wait(&exit_value);
        }
  
    }

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
