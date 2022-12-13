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

int parse_and_process_user_input(void);
void process_pipes(char *all_args[], int all_args_index, char *command_args[]);
int process_redirection(char *input_args[], int write_fd1, int read_fd1, int write_fd2, int read_fd2);
void separate_args(char *sec_of_command, char *command_args[]);
int parse_redirection(char *symbol, char *input_args[], int i, int is_redirect, char *prev_args[], int indices[], int index_of_indices);
int run_input_output_redir(int indices[], int index, int input_index, char *prev_args[], char *input_args[]);
int run_output_redir(int indices[], int index, char *prev_args[], char *input_args[], int read_fd2);
void handle_middle_pipes(int j, char *command_args[], int *write_fd1, int *read_fd1, int *write_fd2, int *read_fd2);


int main(int argc, char *argv[])
{
    for(;;){
        parse_and_process_user_input();
    }

    return 0;
}

/*
* Get the user input and process it
*/
int parse_and_process_user_input()
{
    char original_user_input[INPUT_MAX_LENGTH];
    char *edited_user_input;
    //array of all things in between each |
    static char *all_args[MAX_NUMBER_ARGS];
    int all_args_index;
    static char *command_args[MAX_NUMBER_ARGS];
    //section being one part the command on either side of the pipe (only one section w/o pipes)
    char *sec_of_command;

    //print prompt and get user input
    printf("$ ");

    // Get user input text and check if EOF or control D
    if(fgets(original_user_input, INPUT_MAX_LENGTH, stdin) == NULL){
        exit(0);
    }

    //get rid of \n character
    edited_user_input = strtok(original_user_input, "\n");

    if(edited_user_input == NULL){
        return -1;
    }

    //check if command is exit
    if((strcmp(edited_user_input, "exit") == 0)){
        exit(0);
    }

    //fill an array of strings to the left and right of each |
    all_args[0] = strtok(edited_user_input, "|");
    all_args_index = 1;
    while((all_args[all_args_index] = strtok(NULL, "|")) != NULL){
        all_args_index += 1;
    }

    //IF THERE ARE NO PIPES
    if(all_args_index == 1){
        sec_of_command = all_args[0];

        separate_args(sec_of_command, command_args);

        //handle <, >, and >> and exec arguments
        process_redirection(command_args , -1, -1, -1, -1);

    }
    //IF THERE ARE PIPES
    else{
        process_pipes(all_args, all_args_index, command_args);
    }

    return 0;
}

/*
* Process each section on either side of the pipes
*/
void process_pipes(char *all_args[], int all_args_index, char *command_args[])
{
    //index to clear command_args for each index
    int i;
    //var to iterate through sections of commands
    int j;
    //section being one part the command on either side of the pipe (only one section w/o pipes)
    char *sec_of_command;
    //write and read ends of pipe
    int read_fd1;
    int write_fd1;
    int read_fd2;
    int write_fd2;
    int pipe_fds[2];

    // initialized here because only set when more than 1 pipe is provided
    read_fd2 = -1;
    write_fd2 = -1;

    //HANDLE FIRST LEFT SIDE OF PIPE
    //CREATE STRING ARRAY FOR LEFTMOST SIDE OF PIPE
    sec_of_command = all_args[0];

    separate_args(sec_of_command, command_args);

    //pipe to create write and read end
    if(pipe(pipe_fds) == -1){
        perror("pipe");
        exit(1);
    }
    read_fd1 = pipe_fds[0];
    write_fd1 = pipe_fds[1];

    //Find input redirection and exec first command, only continue if this exits successfully
    if(process_redirection(command_args, write_fd1, read_fd1, -1, -1) == 0){
        //clear argument array
        for(i = 0; i < MAX_NUMBER_ARGS; i++){
            command_args[i] = NULL;
        }

        //FOR LOOP TO HANDLE MIDDLE PIPES
        for(j = 1; j < all_args_index - 1; j++){
            //get arg array
            sec_of_command = all_args[j];
            
            separate_args(sec_of_command, command_args);

            handle_middle_pipes(j, command_args, &write_fd1, &read_fd1, &write_fd2, &read_fd2);

            //clears the command args array
            for(i = 0; i < MAX_NUMBER_ARGS; i++){
                command_args[i] = NULL;
            }
        }

        //HANDLE LAST RIGHT SIDE OF PIPE
        //get arg array
        sec_of_command = all_args[all_args_index-1];
        
        separate_args(sec_of_command, command_args);

        //if last one is even we use fd1 (if given odd number of pipes)
        if((all_args_index % 2) == 0){
            if(all_args_index > 2){
                if(close(read_fd2) == -1){
                    perror("close6");
                    exit(1);
                }
            }

            if(close(write_fd1) == -1){
                perror("close7");
                exit(1);
            }
            // parse user input for > or >>
            process_redirection(command_args, -1, -1, write_fd1, read_fd1);

            // close ends of pipes
            if(close(read_fd1) == -1){
                perror("close8");
                exit(1);
            }

        }
        //if last one is odd we use fd2 (if given even number of pipes)
        else{
            if(close(read_fd1) == -1){
                perror("close9");
                exit(1);
            }

            if(close(write_fd2) == -1){
                perror("close10");
                exit(1);
            }
            // parse user input for > or >>
            process_redirection(command_args, -1, -1, write_fd2, read_fd2);

            // close ends of pipe
            if(close(read_fd2) == -1){
                perror("close11");
                exit(1);
            }
        }
        
    }
    else{
        // close appropriate fds
        if(all_args_index > 2){
            if(close(read_fd2) == -1){
                perror("close12");
                exit(1);
            }
            if(close(write_fd2) == -1){
                perror("close13");
                exit(1);
            }
        }
        if(close(read_fd1) == -1){
            perror("close14");
            exit(1);
        }
        if(close(write_fd1) == -1){
            perror("close15");
            exit(1);
        }
    }
    
}

/*
* Parse the sections of the input for redirection (<, >, >>) and run the commands. 
*/
int process_redirection(char *input_args[], int write_fd1, int read_fd1, int write_fd2, int read_fd2)
{
    int i;
    int fd_in;
    // array for storing index of redirection symbols
    //      indices[0]: index of <, indices[1]: index of >, indices[2]: index of >>
    int indices[3] = {-1,-1,-1};
    int is_redirect;
    int exit_value;
    static char *prev_args[MAX_NUMBER_ARGS];
    int input_index;

    //clear prev_args array
    for(i = 0; i < MAX_NUMBER_ARGS; i++){
        prev_args[i] = '\0';
    }

    i = 0;
    is_redirect = 0;

    //parse input_args and fill the indices array to know which redirections were provided 
    while(input_args[i] != NULL){

        //store index of input redirection carrot
        if(parse_redirection("<", input_args, i, is_redirect, prev_args, indices, 0) == 1){
            is_redirect = 1;
        }
        
        //store index of output redirection carrot
        if(parse_redirection(">", input_args, i, is_redirect, prev_args, indices, 1) == 1){
            is_redirect = 1;
        }

        //store index of output redirection carrot
        if(parse_redirection(">>", input_args, i, is_redirect, prev_args, indices, 2) == 1){
            is_redirect = 1;
        }

        i++;
    }

    //if < provided
    if(indices[0] != -1){
        input_index = indices[0];
        //if < and > provided 
        if(indices[1] != -1){ 

            if(run_input_output_redir(indices, 1, input_index, prev_args, input_args) == -1){
                return -1;
            }
        }
        //if < and >> provided
        else if(indices[2] != -1){

            if(run_input_output_redir(indices, 2, input_index, prev_args, input_args) == -1){
                return -1;
            }
        }
        //if only < provided
        else{
            fd_in = open(input_args[input_index+1], O_RDONLY);
            
            if(fd_in == -1){
                perror("open");
                return -1;
            }

            //process is the child
            if(fork() == 0){
                if(dup2(fd_in, 0) == -1){
                    perror("dup2");
                    exit(1);
                }

                if(write_fd1 != -1){
                    if((dup2(write_fd1, 1)) == -1){
                        perror("dup2");
                        exit(1);
                    }

                    if(close(read_fd1) == -1){
                        perror("close18");
                        exit(1);
                    }
                }

                if(execvp(prev_args[0], prev_args) == -1){
                    perror("evecvp");
                    exit(1);
                }
            }
            //process is the parent
            else{
                if(wait(&exit_value) == -1){
                    perror("wait");
                    exit(1);
                }
                if(WEXITSTATUS(exit_value) != 0){
                    return -1;
                }
            }
            
            if(close(fd_in) == -1){
                perror("close19");
                exit(1);
            }
        }
    }
    //if no < provided
    else{
        //if >
        if(indices[1] != -1){
            
            if(run_output_redir(indices, 1, prev_args, input_args, read_fd2) == -1){
                return -1;
            }
        }
        //if >>
        else if(indices[2] != -1){

            if(run_output_redir(indices, 2, prev_args, input_args, read_fd2) == -1){
                return -1;
            }
        }
    }

    //if no redirection
    if(is_redirect == 0){
        //process is the child
        if(fork() == 0){
            if(write_fd1 != -1){

                if((dup2(write_fd1, 1)) == -1){
                    perror("dup2");
                    exit(1);
                }
            }

            if(read_fd2 != -1){
                if((dup2(read_fd2, 0)) == -1){
                    perror("dup2");
                    exit(1);
                }
            }

            if(execvp(input_args[0], input_args) == -1){
                perror("execvp1");
                exit(1);
            }
        }
        //process is the parent
        else{

            if(wait(&exit_value) == -1){
                perror("wait");
                exit(1);
            }
            if(WEXITSTATUS(exit_value) != 0){
                return -1;
            } 
        }
    }
    return 0;
}

/*
* Separate arguments via spaces
*/
void separate_args(char *sec_of_command, char *command_args[])
{
    int arg_index;

    arg_index = 0;

    //seperates each argument
    command_args[arg_index] = strtok(sec_of_command, " ");
    arg_index += 1;

    while((command_args[arg_index] = strtok(NULL, " ")) != NULL){
        arg_index += 1;
    }
}

/*
* Check if current index i in input_args is the specified redirection symbol. 
* If it is, update the indices array to indicate where that symbol is in input_args. 
*/
int parse_redirection(char *symbol, char *input_args[], int i, int is_redirect, char *prev_args[], int indices[], int index_of_indices)
{
    int j; 

    //store index of input redirection carrot
    if(strcmp(input_args[i], symbol) == 0){
        indices[index_of_indices] = i;
        //if this is the first redirect, process the commands
        if(is_redirect == 0){
            // Process all arguments until the redirection
            for(j = 0; j < i; j++){
                prev_args[j] = input_args[j];
            }
            is_redirect = 1;
        }
    }
    return is_redirect;
}

/*
* Executes commands with both input and output redirection (< and > or < and >>)
*/
int run_input_output_redir(int indices[], int index, int input_index, char *prev_args[], char *input_args[])
{
    int fd_out;
    int fd_in;
    int output_index;
    int exit_value;
    
    output_index = indices[index];

    if(index == 2){
        fd_out = open(input_args[output_index+1], O_CREAT | O_WRONLY | O_APPEND, 0666);
    }
    else{
        fd_out = open(input_args[output_index+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    }
    
    
    if(fd_out == -1){
        perror("open");
        exit(1);
    }

    fd_in = open(input_args[input_index+1], O_RDONLY);
    
    if(fd_in == -1){
        perror("open");
        exit(1);
    }

    //process is the child
    if(fork() == 0){

        if(dup2(fd_in, 0) == -1){
            perror("dup2");
            exit(1);
        }

        if(dup2(fd_out, 1) == -1){
            perror("dup2");
            exit(1);
        }

        if(execvp(prev_args[0], prev_args) == -1){
            perror("evecvp");
            exit(1);
        }
    } 
    else{
        if(wait(&exit_value) == -1){
            perror("wait");
            exit(1);
        }
        if(WEXITSTATUS(exit_value) != 0){
            return -1;
        }
    }
    if(close(fd_in) == -1){
        perror("close16");
        exit(1);
    }
    if(close(fd_out) == -1){
        perror("close17");
        exit(1);
    }
    return 0;
}

/*
* Executes commands with just output redirection (> or >>)
*/
int run_output_redir(int indices[], int index, char *prev_args[], char *input_args[], int read_fd2)
{
    int fd_out;
    int output_index;
    int exit_value;

    output_index = indices[index];

    if(index == 2){
        fd_out = open(input_args[output_index+1], O_CREAT | O_WRONLY | O_APPEND, 0666);
    }
    else{
        fd_out = open(input_args[output_index+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    }
    
    if(fd_out == -1){
        perror("open");
        return -1;
    }

    //process is the child
    if(fork() == 0){
        if(dup2(fd_out, 1) == -1){
            perror("dup2");
            exit(1);
        }
        
        if(read_fd2 != -1){
            if((dup2(read_fd2, 0)) == -1){
                perror("dup2");
                exit(1);
            }
        }

        if(execvp(prev_args[0], prev_args) == -1){
            perror("evecvp");
            exit(1);
        }
    }
    //process is the parent
    else{
        if(wait(&exit_value) == -1){
            perror("wait");
            exit(1);
        }
        if(WEXITSTATUS(exit_value) != 0){
            return -1;
        }
    }
    
    if(close(fd_out) == -1){
        perror("close21");
        exit(1);
    }

    return 0;
}

/*
* Handle the arguments in the middle of pipes
*/
void handle_middle_pipes(int j, char *command_args[], int *write_fd1, int *read_fd1, int *write_fd2, int *read_fd2)
{
    int pipe_fds[2];

    if((j % 2) == 0){
                
        //close previous fds
        if(j >= 2){
            if(close(*read_fd1) == -1){
                perror("close1");
                exit(1);
            }

            if(close(*write_fd2) == -1){
                perror("close2");
                exit(1);
            }
        }
        if(pipe(pipe_fds) == -1){
            perror("pipe");
            exit(1);
        }
        *read_fd1 = pipe_fds[0];
        *write_fd1 = pipe_fds[1];
        
        process_redirection(command_args, *write_fd1, *read_fd1, *write_fd2, *read_fd2);
    }
    //if j is odd replace contents of fd2
    else{
        //close previous fds
        if(j >= 2){
            if(close(*read_fd2) == -1){
                perror("close3");
                exit(1);
            }
        }
        
        // //close write end, so we don't have to close in child
        if(close(*write_fd1) == -1){
            perror("close5");
            exit(1);
        }

        if(pipe(pipe_fds) == -1){
            perror("pipe");
            exit(1);
        }
        *read_fd2 = pipe_fds[0];
        *write_fd2 = pipe_fds[1];

        process_redirection(command_args, *write_fd2, -1, -1, *read_fd1);

    }
}