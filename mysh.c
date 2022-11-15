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
char **reset_array(char *array[]);

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

    //array of all things in between each |
    static char *all_args[MAX_NUMBER_ARGS];
    int all_args_index;

    static char *command_args[MAX_NUMBER_ARGS];
    static char *right_command_args[MAX_NUMBER_ARGS];
    int arg_index; // index of the argument in the array
    
    int exit_value;
    int i;
    int j;

    char *pipe_left_arg; //change this var name
    char *pipe_right_arg; //not using this atm
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

    // printf("first arg = %s\n", all_args[0]);
    // printf("second arg = %s\n", all_args[1]);

    //IF THERE ARE NO PIPES
    if(all_args_index == 1)
    {
        pipe_left_arg = all_args[0];

        arg_index = 0;

        command_args[arg_index] = strtok(pipe_left_arg, " "); //seperates each argument
        arg_index += 1; 

        while((command_args[arg_index] = strtok(NULL, " ")) != NULL)
        {
            arg_index += 1; 
        }

        parse_user_input(command_args);

    }
    //IF THERE ARE PIPES
    else
    {
        //1. HANDLE FIRST LEFT SIDE OF PIPE

        //a. CREATE STRING ARRAY FOR LEFTMOST SIDE OF PIPE
        pipe_left_arg = all_args[0];
        arg_index = 0; 

        command_args[arg_index] = strtok(pipe_left_arg, " "); //seperates each argument
        arg_index += 1; 

        while((command_args[arg_index] = strtok(NULL, " ")) != NULL)
        {
            arg_index += 1; 
        }

        //b. pipe to create write and read end
        if(pipe(pipe_fds) == -1)
        {
            perror("pipe");
        }

        printf("first arg = %s\n", command_args[0]);

        //1c. fork for left side
        if(fork() == 0)
        {
            if((dup2(pipe_fds[1], 1)) == -1)
            {
                perror("dup2");
            }

            close(pipe_fds[0]);

            if(execvp(command_args[0], command_args) == -1)
            {
                perror("evecvp");
            }
        
        }

        //1d. clear argument array
        for(i = 0; i < MAX_NUMBER_ARGS; i++) //CHECK BUGS
        {
            command_args[i] = NULL;
        }

        //2. FOR LOOP TO HANDLE MIDDLE PIPES
        for(j = 1; j < all_args_index - 1; j++)
        {
            
            //2a. get arg array
            pipe_left_arg = all_args[j];
            arg_index = 0; 

            command_args[arg_index] = strtok(pipe_left_arg, " "); //seperates each argument
            arg_index += 1; 

            while((command_args[arg_index] = strtok(NULL, " ")) != NULL)
            {
                arg_index += 1; 
            }

            printf("middle command = %s\n", command_args[0]);

            //2b. fork and dup twice
            if(fork() == 0)
            {
                if((dup2(pipe_fds[0], 0)) == -1)
                {
                    perror("dup2");
                }

                if((dup2(pipe_fds[1], 1)) == -1)
                {
                    perror("dup2");
                }

                close(pipe_fds[0]);
                close(pipe_fds[1]);

                if(execvp(command_args[0], command_args) == -1)
                {
                    perror("evecvp");
                }
            
            }

            for(i = 0; i < MAX_NUMBER_ARGS; i++)
            {
                command_args[i] = NULL;
            }

        }

        //3. HANDLE LAST RIGHT SIDE OF PIPE
        //3c. get arg array
        pipe_left_arg = all_args[all_args_index-1];
        arg_index = 0; 

        // printf("pipe left arg = %s\n", pipe_left_arg);

        command_args[arg_index] = strtok(pipe_left_arg, " "); //seperates each argument
        arg_index += 1; 

        while((command_args[arg_index] = strtok(NULL, " ")) != NULL)
        {
            arg_index += 1; 
        }

        printf("last arg = %s\n", command_args[0]);

        if(fork() == 0)
        {
            if((dup2(pipe_fds[0], 0)) == -1)
            {
                perror("dup2");
            }

            close(pipe_fds[1]);

            if(execvp(command_args[0], command_args) == -1)
            {
                perror("evecvp");
            }
        }


        //4. close ends of pipe
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        
        //5. wait for each child
        for(j = 0; j < all_args_index; j++)
        {
            wait(&exit_value);
        }
        
    }



    //iterate through each pair of arguments on either side of the pipe
    // for(j = 0; j < (all_args_index - 1); j++)
    // {
    //     pipe_left_arg = all_args[j];
    //     pipe_right_arg = all_args[j+1];

    //     arg_index_l = 0; 
    //     arg_index_r = 0;

    //     //1. CREATE STRING ARRAY FOR LEFT SIDE OF PIPE
    //     left_command_args[arg_index_l] = strtok(pipe_left_arg, " "); //seperates each argument
    //     arg_index_l += 1; 

    //     while((left_command_args[arg_index_l] = strtok(NULL, " ")) != NULL)
    //     {
    //         arg_index_l += 1; 
    //     }
        
    //     //2. CREATE STRING ARRAY FOR RIGHT SIDE OF PIPE
    //     right_command_args[arg_index_r] = strtok(pipe_right_arg, " "); //seperates each argument
    //     arg_index_r += 1; 

    //     while((right_command_args[arg_index_r] = strtok(NULL, " ")) != NULL)
    //     {
    //         arg_index_r += 1; 
    //     }

    //     printf("first input = %s\n", left_command_args[0]);
    //     printf("second input = %s\n", right_command_args[0]);

    //     //pipe to create write and read end
    //     if(pipe(pipe_fds) == -1)
    //     {
    //         perror("pipe");
    //     }

    //     // close the write and read ends of the pipes 
    //     if(fork() == 0)
    //     {
    //         if((dup2(pipe_fds[1], 1)) == -1)
    //         {
    //             perror("dup2");
    //         }

    //         close(pipe_fds[0]);

    //         if(execvp(left_command_args[0], left_command_args) == -1)
    //         {
    //             perror("evecvp");
    //         }
        
    //     }
    //     if(fork() == 0)
    //     {
    //         if((dup2(pipe_fds[0], 0)) == -1)
    //         {
    //             perror("dup2");
    //         }

    //         close(pipe_fds[1]);

    //         if(execvp(right_command_args[0], right_command_args) == -1)
    //         {
    //             perror("evecvp");
    //         }
    //     }

    //     close(pipe_fds[0]);
    //     close(pipe_fds[1]);
        
    //     wait(&exit_value);
    //     wait(&exit_value);
        
    //     // wait(&exit_value);
    //     // wait(NULL);
  
    // }

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

