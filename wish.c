/*
    Project 1: simple shell
    Completed by: Kai Pinckard
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

// This is a space delimited list of directories.
char path[PATH_MAX] = "/bin /usr/bin";
char error_message[30] = "An error has occurred\n";

/*
A wrapper function to clean up the write calls.
*/
void generic_error()
{
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

/*
This function searches for '>' and '&'s and inserts a space before and
after them for easier parsing.
*/
char* preprocess_line(char* line)
{
    int extra_spaces = 0; 
    for(size_t i = 0; i < strlen(line); i++)
    {
        if(line[i] == '>' || line[i] ==  '&')
        {
            extra_spaces += 2;
        }
    }

    char* line_cp = (char*) calloc(strlen(line) + extra_spaces + 1, sizeof(char));
    line_cp[0] = '\0';
    int line_cp_pos = 0;

    for(size_t i = 0; i < strlen(line); i++)
    {
        if(line[i] == '>' || line[i] ==  '&')
        {
            line_cp[line_cp_pos] = ' ';
            line_cp_pos += 1;
            line_cp[line_cp_pos] = line[i];
            line_cp_pos += 1;
            line_cp[line_cp_pos] = ' ';
            line_cp_pos += 1;
        }
        else
        {
            line_cp[line_cp_pos] = line[i];
            line_cp_pos += 1;
        }
    }
    free(line);
    return line_cp;
}

/*
    This function reads a line from "input_file" and parses
    it, storing the results in an array of arguments (which are of type char*)
    that it returns. However, if there are no arguments to extract from the 
    current line it will return NULL instead. It updates line_buffer_ptr to hold
    the address of the memory allocated by getline, so that it can be
    deallocated elsewhere. 
*/
char** parse_line(FILE* input_file, char** line_buffer_ptr)
{
    size_t buffer_size = 0;
    char** args = (char**) calloc(_POSIX_ARG_MAX, sizeof(char*));

    if(args == NULL)
    {
        generic_error();
        exit(1);
    }

    if(getline(line_buffer_ptr, &buffer_size, input_file) == -1)
    {
        // No chars were read
        exit(0);
    }

    *line_buffer_ptr = preprocess_line(*line_buffer_ptr);

    args[0] = strtok(*line_buffer_ptr, " \n\t");
    if(args[0] == NULL)
    {
        // Invalid empty input
        free(args);
        return NULL;
    }

    // Store the rest of the arguments from the line into args
    for(int i = 1; (args[i] = strtok(NULL, " \n\t")) != NULL; i++);

    return args;
}

void handle_path(char** args)
{
    // Set first byte of path to \0 for strcat
    bzero(path, 1);
    for(int i = 1; args[i] != NULL; i++)
    {
        strcat(path, args[i]);
        strcat(path, " ");
    }
}

void handle_cd(char** args)
{
    if(args[0] != NULL && args[1] != NULL && args[2] == NULL)
    {
        if(chdir(args[1]) != 0)
        {
            generic_error();
        }
    }
    else
    {
        generic_error();
        return;
    }
}

/*
    This function recieves a list of char** containing all of the
    args from a single line. It is responsible for returning a new
    char** that contains all of the arguments in a single command. It
    updates the pointer "next" to hold the location in the original "args"
    after the replaced '&' so that on subsequent calls the next 
    commands can be obtained.
*/
char** get_next_command(char** args, char*** next)
{
    *next = NULL;
    for(int i = 0; args[i] != NULL; i++)
    {
        if(strcmp(args[i], "&") == 0)
        {
            if(i != 0)
            {
                args[i] = NULL;
            }
            // If there is another command to parse after &
            if(args[i+1] != NULL)
            {
                *next = args + i + 1;
            }
            break;
        }
    }
    return args;
}

/*
    This function sets all the char*s to NULL from start_index onwards
    until the first NULL char* is encountered. This allows
    arguments after > to be ignored (these arguments will have already
    been handled as output redirection is done in handle_externel_command.)
*/
void set_args_to_null(char** args, int start_index)
{
    for(int i = start_index; args[i] != NULL; i++)
    {
        args[i] = NULL;
    }
    return;
}

/*
    This function handles all commands that are not builtin to wish.
*/
void handle_external_command(char** args)
{
    // Create a modifiable copy of path
    int path_len = strlen(path) + 1;
    char path_cp[path_len];
    strncpy(path_cp, path, path_len);

    char* directory = strtok(path_cp, " ");
    bool found_file = false;

    while(directory != NULL)
    {
        // Combine the directory with args[0]
        int file_path_length = strlen(directory) + strlen(args[0]) + 2;
        char file_path[file_path_length];
        strncpy(file_path, directory, file_path_length);
        strcat(file_path, "/");
        strcat(file_path, args[0]);

        if (access(file_path, X_OK) == 0)
        {
            // A valid executable has been found
            found_file = true;
            pid_t child;
            child = fork();

            if(child != 0)
            {
                // The parent returns immediately
                return;
            }
            else
            {
                int redirection_file = -1;

                // Loop through all arguments to see if any are ">"
                for(int i = 0; args[i] != NULL; i++)
                {
                    if(strcmp(args[i], ">") == 0)
                    {
                        // Check that there is an argument after '>' and there is not more than one
                        if(args[i+1] != NULL && args[i+2] == NULL)
                        {
                            redirection_file = open(args[i+1], O_CREAT|O_TRUNC|O_WRONLY, 0666);
                            if (redirection_file < 0)
                            {
                                // Failed to open redirection file
                                generic_error();
                                return;
                            }
                            // Remove all args after and including > in the current command
                            set_args_to_null(args, i);
                        }
                        else
                        {
                            generic_error();
                            return;
                        }
                        // No need to continue loop after finding '>'
                        break;
                    }
                }

                // Configure output and error redirection
                dup2(redirection_file, 1);
                dup2(redirection_file, 2);

                // Run the specified executable
                execv(file_path, args);
                generic_error();
                exit(1);
            }
        }
        directory = strtok(NULL, " ");
    }
    if(!found_file)
    {
        generic_error();
    }
}

/*
    This function handles the current command. It returns
    a bool indicating if main should exit the program 
    after this function call
*/
bool handle_command(char** args)
{
    // Error checking
    if(args[0] == 0)
    {
        generic_error();
        return false;
    }
    else if(strcmp(args[0], "&") == 0)
    {
        return false;
    }

    // Checking for command type
    if(strcmp(args[0], "exit") == 0)
    {
        if(args[1] != NULL)
        {
            generic_error();
        }
        return true;
    }
    else if(strcmp(args[0], "cd") == 0)
    {
        handle_cd(args);
    }
    else if(strcmp(args[0], "path") == 0)
    {
        handle_path(args);
    }
    else
    {
        handle_external_command(args);
    }
    return false;
}

int main(int argc, char* argv[])
{
    char** initial_args;
    FILE *fp = fdopen(0, "r");
    char* line_buffer = NULL;
    if(argc > 1)
    {
        fclose(fp);
        fp = fopen(argv[1], "r");

        if(fp == NULL)
        {
            // Error occured opening file
            generic_error();
            exit(1);
        }
        if(fgetc(fp) == 0 || argc > 2)
        {
            // Check for an empty file or multiple files
            generic_error();
            exit(1);
        } 
        rewind(fp);
    }
    while(true)
    {
        char** next = NULL;

        // If we are in interactive mode print "wish> "
        if(argc < 2)
        {
            printf("wish> ");
        }
        // Initial args is used for freeing memory.
        initial_args = parse_line(fp, &line_buffer);
        char** args = initial_args;
        if(args != NULL)
        {
            get_next_command(args, &next);

            if(handle_command(args))
            {
                fclose(fp);
                free(initial_args);
                free(line_buffer);
                exit(0);
            }
            args = next;
            while(next != NULL)
            {
                get_next_command(args, &next);
                if(handle_command(args))
                {
                    fclose(fp);
                    free(initial_args);
                    free(line_buffer);
                    exit(0);
                }
                args = next;
            }
            // Wait for all children
            while(wait(NULL) > 0);
        }
        free(initial_args);
        free(line_buffer);
    }
}