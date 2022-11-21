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
void parse_user_input(char *input_args[], int fd_dup, int fd_close, int fd_dup1, int fd_close1);

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
    int arg_index; // index of the argument in the array
    
    int exit_value;
    int i;
    int j;

    char *pipe_left_arg; //change this var name
    
    //write and read ends of pipe
    int pipe_fd1[2];
    int pipe_fd2[2];

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

        // parse_user_input(command_args); //handle <,>, and >>
        parse_user_input(command_args , -1, -1, -1, -1);

    }
    //IF THERE ARE PIPES
    else
    {
        //1. HANDLE FIRST LEFT SIDE OF PIPE

        //1a. CREATE STRING ARRAY FOR LEFTMOST SIDE OF PIPE
        pipe_left_arg = all_args[0];
        arg_index = 0; 

        command_args[arg_index] = strtok(pipe_left_arg, " "); //seperates each argument
        arg_index += 1; 

        while((command_args[arg_index] = strtok(NULL, " ")) != NULL)
        {
            arg_index += 1; 
        }

        //1b. pipe to create write and read end
        if(pipe(pipe_fd1) == -1)
        {
            perror("pipe");
        }

        printf("first arg = %s\n", command_args[0]);

        //1c. Find input redirection and exec first command
        parse_user_input(command_args, pipe_fd1[1], pipe_fd1[0], -1, -1);

        //1d. clear argument array
        for(i = 0; i < MAX_NUMBER_ARGS; i++)
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

            if((j % 2) == 0) //if j is even replace contents of fd1
            {
                if(j >= 2) //close previous fds
                {
                    close(pipe_fd1[0]);
                    close(pipe_fd1[1]);
                }
                if(pipe(pipe_fd1) == -1)
                {
                    perror("pipe");
                }

                if(fork() == 0)
                {
                    if((dup2(pipe_fd2[0], 0)) == -1)
                    {
                        perror("dup2");
                    }
                    
                    close(pipe_fd2[1]);

                    if((dup2(pipe_fd1[1], 1)) == -1)
                    {
                        perror("dup2");
                    }

                    close(pipe_fd1[0]);

                    printf("execing %s 132\n", command_args[0]);
                    if(execvp(command_args[0], command_args) == -1)
                    {
                        perror("evecvp");
                    }
                
                }
            }
            else //if j is odd replace contents of fd2
            {
                printf("Running %d argument: should be odd\n", j);
                if(j >= 2) //close previous fds
                {
                    close(pipe_fd2[0]);
                    close(pipe_fd2[1]);
                }
                
                if(pipe(pipe_fd2) == -1)
                {
                    perror("pipe");
                }

                if(fork() == 0)
                {
                    if((dup2(pipe_fd1[0], 0)) == -1) // read from read end
                    {
                        perror("dup2");
                    }
                    
                    close(pipe_fd1[1]); // close write end

                    if((dup2(pipe_fd2[1], 1)) == -1) // write to write end of pipe
                    {
                        perror("dup2");
                    }

                    close(pipe_fd2[0]);

                    if(execvp(command_args[0], command_args) == -1)
                    {
                        perror("evecvp");
                    }
                
                }
            }

            //clears the command args array
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

        if((all_args_index % 2) == 0) //if last one is even we use fd1
        {
            printf("even\n");
            printf("all_args_index = %d\n", all_args_index);

            if(all_args_index > 2)
            {
                close(pipe_fd2[0]);
                close(pipe_fd2[1]);
            }

            close(pipe_fd1[1]);
            // parse user input for > or >>
            parse_user_input(command_args, -1, -1, pipe_fd1[0], pipe_fd1[1]);

            // close ends of pipes
            close(pipe_fd1[0]);
            close(pipe_fd1[1]);
        }   
        else //if last one is odd we use fd2
        {
            printf("odd\n");
            
            close(pipe_fd1[0]);
            close(pipe_fd1[1]);

            close(pipe_fd2[1]);
            // parse user input for > or >>
            parse_user_input(command_args, -1, -1, pipe_fd2[0], pipe_fd2[1]);

            // close ends of pipe
            close(pipe_fd2[0]);
            close(pipe_fd2[1]);
        }
        
        // wait for each child
        for(j = 0; j < all_args_index; j++)
        {
            wait(&exit_value);
        }
    }
}

void parse_user_input(char *input_args[], int fd_dup, int fd_close, int fd_dup1, int fd_close1)
{
    int i, j;
    int fd_in, fd_out;
    int indices[3] = {-1,-1,-1};
    int is_redirect;
    int exit_value;
    static char *prev_args[MAX_NUMBER_ARGS];
    int input_index, output_index;

    is_redirect = 0;
    i = 0;

    while(input_args[i] != NULL)
    {
        //store index of input redirection carrot
        if(strcmp(input_args[i], "<") == 0)
        {
            indices[0] = i;
            //if this is the first redirect, process the commands
            if(is_redirect == 0)
            {
                // Process all arguments until the redirection
                for(j = 0; j < i; j++)
                {
                    prev_args[j] = input_args[j];
                }
            }
            is_redirect = 1;
        }
        
        //store index of output redirection carrot
        if((strcmp(input_args[i], ">") == 0)) 
        {
            indices[1] = i;
            if(is_redirect == 0)
            {
                // Process all arguments until the redirection
                for(j = 0; j < i; j++)
                {
                    prev_args[j] = input_args[j];
                }
            }
            is_redirect = 1;
        }
        //store index of output redirection carrot
        if(strcmp(input_args[i], ">>") == 0)
        {
            indices[2] = i;
            if(is_redirect == 0)
            {
                // Process all arguments until the redirection
                for(j = 0; j < i; j++)
                {
                    prev_args[j] = input_args[j];
                }
            }
            is_redirect = 1; 
        }
        
        i++;
    }

    //if <
    if(indices[0] != -1)
    {
        input_index = indices[0];
        //if < and >
        if(indices[1] != -1)
        { 
            output_index = indices[1];

            fd_out = open(input_args[output_index+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
            
            if(fd_out == -1)
            {
                perror("open");
            }

            fd_in = open(input_args[input_index+1], O_RDONLY);
            
            if(fd_in == -1)
            {
                perror("open");
            }

            //process is the child
            if(fork() == 0)
            {
                if(dup2(fd_in, 0) == -1)
                {
                    perror("dup2");
                }

                if(dup2(fd_out, 1) == -1)
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
            
            close(fd_in);
            close(fd_out);
        }
        //if < and >>
        else if(indices[2] != -1)
        {
            output_index = indices[2];

            fd_out = open(input_args[output_index+1], O_CREAT | O_WRONLY | O_APPEND, 0666);
            
            if(fd_out == -1)
            {
                perror("open");
            }

            fd_in = open(input_args[input_index+1], O_RDONLY);
            
            if(fd_in == -1)
            {
                perror("open");
            }

            //process is the child
            if(fork() == 0)
            {
                if(dup2(fd_in, 0) == -1)
                {
                    perror("dup2");
                }

                if(dup2(fd_out, 1) == -1)
                {
                    perror("dup2");
                }

                if(execvp(prev_args[0], prev_args) == -1)
                {
                    perror("evecvp");
                }
            }
        }
        //if only <
        else
        {
            fd_in = open(input_args[input_index+1], O_RDONLY);
            
            if(fd_in == -1)
            {
                perror("open");
            }

            //process is the child
            if(fork() == 0)
            {
                if(dup2(fd_in, 0) == -1)
                {
                    perror("dup2");
                }

                if(fd_dup != -1)
                {
                    if((dup2(fd_dup, 1)) == -1)
                    {
                        perror("dup2");
                    }

                    close(fd_close);  
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
            
            close(fd_in);
        }
    }
    //if not <
    else
    {
        //if >
        if(indices[1] != -1)
        {
            output_index = indices[1];

            fd_out = open(input_args[output_index+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
            
            if(fd_out == -1)
            {
                perror("open");
            }

            //process is the child
            if(fork() == 0)
            {

                if(dup2(fd_out, 1) == -1)
                {
                    perror("dup2");
                }

                if(fd_dup1 != -1)
                {
                    if((dup2(fd_dup1, 0)) == -1)
                    {
                        perror("dup2");
                    }

                    close(fd_close1);  
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
            
            close(fd_out);
        }
        //if >>
        else if(indices[2] != -1)
        {
            output_index = indices[2];

            fd_out = open(input_args[output_index+1], O_CREAT | O_WRONLY | O_APPEND, 0666);
            
            if(fd_out == -1)
            {
                perror("open");
            }

            //process is the child
            if(fork() == 0)
            {
                if(dup2(fd_out, 1) == -1)
                {
                    perror("dup2");
                }
               
                if(fd_dup1 != -1)
                {
                    if((dup2(fd_dup1, 0)) == -1)
                    {
                        perror("dup2");
                    }

                    close(fd_close1);  
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
            
            close(fd_out);
        }
    }

    //if no redirection
    if(is_redirect == 0)
    {
        printf("no redirection\n");
        //process is the child
        if(fork() == 0)
        {
            if(fd_dup != -1)
            {
                printf("fd_dup = %d and fd_close = %d\n", fd_dup, fd_close);

                if((dup2(fd_dup, 1)) == -1)
                {
                    perror("dup2");
                }

                close(fd_close);
            }

            if(fd_dup1 != -1)
            {
                printf("fd_dup1 = %d and fd_close1 = %d\n", fd_dup1, fd_close1);
                
                if((dup2(fd_dup1, 0)) == -1)
                {
                    perror("dup2");
                }

                close(fd_close1);
            }

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

