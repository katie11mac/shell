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
    int arg_index; // index of the argument in the array
    
    int exit_value;
    int i;

    int x, k; //index for checking for <
    int fd; //fd for < and >>
    static char *prev_args[MAX_NUMBER_ARGS]; //arg array for <

    int is_redirect;
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

        parse_user_input(command_args); //handle <,>, and >>

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
        if(pipe(pipe_fd1) == -1)
        {
            perror("pipe");
        }

        // CHECK FOR <
        x = 0;
        is_redirect = 0;

        while(command_args[x] != NULL && is_redirect == 0)
        {
            // INPUT REDIRECTION
            if(strcmp(command_args[x], "<") == 0)
            {
                printf("INPUT REDIRECTION \n");
                fd = open(command_args[x+1], O_RDONLY);
                
                if(fd == -1)
                {
                    perror("open");
                }

                // Process all arguments until the redirection
                for(k = 0; k < x; k++)
                {
                    prev_args[k] = command_args[k];
                }

                //process is the child
                if(fork() == 0)
                {
                    //dup for input redirection
                    if(dup2(fd, 0) == -1)
                    {
                        perror("dup2");
                    }

                    //dup for piping
                    if((dup2(pipe_fd1[1], 1)) == -1)
                    {
                        perror("dup2");
                    }

                    close(pipe_fd1[0]);

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
            x++;
        }

        if(is_redirect == 0) //if not redirected, normal
        {
            printf("first arg = %s\n", command_args[0]);

            //1c. fork for left side
            if(fork() == 0)
            {
                if((dup2(pipe_fd1[1], 1)) == -1)
                {
                    perror("dup2");
                }

                close(pipe_fd1[0]);

                if(execvp(command_args[0], command_args) == -1)
                {
                    perror("evecvp");
                }
            
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
            printf("all_args_index = %d\n", all_args_index);

            if(all_args_index > 2)
            {
                close(pipe_fd2[0]);
                close(pipe_fd2[1]);
            }

            // CHECK FOR >
            x = 0;
            is_redirect = 0;

            while(command_args[x] != NULL && is_redirect == 0) // change these
            {
                // OUTPUT REDIRECTION
                if((strcmp(command_args[x], ">") == 0) || (strcmp(command_args[x], ">>") == 0)) //or >>
                {
                    printf("OUTPUT REDIRECTION\n");
                    
                    // put a >> check here so dont truncate
                    if(strcmp(command_args[x], ">") == 0)
                    {
                        fd = open(command_args[x+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
                    }
                    else
                    {
                        fd = open(command_args[x+1], O_CREAT | O_WRONLY | O_APPEND, 0666);
                    }
                    
                    if(fd == -1)
                    {
                        perror("open");
                    }

                    // Process all arguments until the redirection
                    for(k = 0; k < x; k++)
                    {
                        prev_args[k] = command_args[k];
                    }

                    //process is the child
                    if(fork() == 0)
                    {
                        if(dup2(fd, 1) == -1)
                        {
                            perror("dup2");
                        }

                        if((dup2(pipe_fd1[0], 0)) == -1)
                        {
                            perror("dup2");
                        }

                        close(pipe_fd1[1]);

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
                x++;
            }

            if(is_redirect == 0)
            {
                printf("NO OUTPUT REDIRECTION\n");
            
                if(fork() == 0)
                {
                    if((dup2(pipe_fd1[0], 0)) == -1)
                    {
                        perror("dup2");
                    }

                    close(pipe_fd1[1]);

                    if(execvp(command_args[0], command_args) == -1)
                    {
                        perror("evecvp");
                    }
                }
            }
        }
        else //if last one is odd we use fd2
        {
            printf("odd\n");
            
            close(pipe_fd1[0]);
            close(pipe_fd1[1]);

            // CHECK FOR >
            x = 0;
            is_redirect = 0;

            while(command_args[x] != NULL && is_redirect == 0) // change these
            {
                
                // OUTPUT REDIRECTION
                if((strcmp(command_args[x], ">") == 0) || (strcmp(command_args[x], ">>") == 0)) //or >>
                {
                    // put a >> check here so dont truncate
                    if(strcmp(command_args[x], ">") == 0)
                    {
                        fd = open(command_args[x+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
                    }
                    else
                    {
                        fd = open(command_args[x+1], O_CREAT | O_WRONLY | O_APPEND, 0666);
                    }
                    
                    if(fd == -1)
                    {
                        perror("open");
                    }

                    // Process all arguments until the redirection
                    for(k = 0; k < x; k++)
                    {
                        prev_args[k] = command_args[k];
                    }

                    //process is the child
                    if(fork() == 0)
                    {
                        if(dup2(fd, 1) == -1)
                        {
                            perror("dup2");
                        }

                        if((dup2(pipe_fd2[0], 0)) == -1)
                        {
                            perror("dup2");
                        }

                        close(pipe_fd2[1]);

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
                x++;
            }
            
            if(is_redirect == 0)
            {
                if(fork() == 0)
                {
                    if((dup2(pipe_fd2[0], 0)) == -1)
                    {
                        perror("dup2");
                    }

                    close(pipe_fd2[1]);

                    if(execvp(command_args[0], command_args) == -1)
                    {
                        perror("evecvp");
                    }
                }
            }
            
        }

        //4. close ends of pipe
        close(pipe_fd1[0]);
        close(pipe_fd1[1]);
        close(pipe_fd2[0]);
        close(pipe_fd2[1]);
        
        //5. wait for each child
        for(j = 0; j < all_args_index; j++)
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

