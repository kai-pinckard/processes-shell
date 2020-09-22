#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
//include linux limits


// This is a space delimited list of directories.
const int MAX_PATH_SIZE = 4096;
char path[4096] = "/bin /usr/bin";
char error_message[30] = "An error has occurred\n";

/*
Need to parse line into separate commands. The starting
pointer to line will always be saved so it can be deallocated.
Then a separate pointer will be stored to track the current location
in the line we are in terms of how many commands have been parsed already.
curr_command_ptr points to the start of the next command in the line to process.
When calling get_command 
*/

void generic_error()
{
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

void set_args_to_null(char** args, int start_index)
{
    for(int i = start_index; args[i] != NULL; i++)
    {
        args[i] = NULL;
    }
    return;
}

/*
This function searches for > and &s and inserts a space before and
after them for easier parsing.
*/
char* preprocess_line(char* line)
{
    //printf("%s\n", line);
    int extra_spaces = 0; 
    for(int i = 0; i < strlen(line); i++)
    {
        if(line[i] == '>' || line[i] ==  '&')
        {
            extra_spaces += 2;
        }
    }

    char* line_cp = (char*) calloc(strlen(line) + extra_spaces + 1, sizeof(char));
    line_cp[0] = '\0';

    int line_cp_pos = 0;
    for(int i = 0; i < strlen(line); i++)
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
    //printf("line %s\n", line);
    //printf("linecp %s\n", line_cp);
    free(line);
    return line_cp;

    //free(line);
}

void print_args(char** args)
{
    //printf("------------Printing---------------\n");
    for(int i = 0; args[i] != NULL; i++)
    {
        //printf("%s\n", args[i]);
        //free(args[i]);
    }
    //printf("------------end printing------------\n\n");
}

/*
This function takes a line that has been read with getline and
parses it into its tokens. An array of char arrays is returned with
the length of each sub array being the length of the longest argument.
*/
char** parse_line(FILE* input_file)
{
    int MAX_ARGS = 100;
    // getline vars
    char* line_buffer = NULL;
    size_t buffer_size = 0;

    // array of args
    char** args = (char**) calloc(MAX_ARGS, sizeof(char*));

    if(args == NULL)
    {
        //printf("calloc failed to allocate requested memory.\n");
        generic_error();
        exit(1);
    }

    int chars_read = -1;
    // need to free memory allocated by getline
    if ((chars_read = getline(&line_buffer, &buffer_size, input_file)) == -1)
    {
        //printf("end of file reached\n");
        //generic_error();
        exit(0);
    }
    if(chars_read == 0)
    {
        //printf("ran\n");
        exit(1);
    }

    line_buffer = preprocess_line(line_buffer);
    int i = 0;
    args[0] = strtok(line_buffer, " \n\t");

    if(args[0] == NULL)
    {
        //printf("error invalid input empty");
        //generic_error();
        return NULL;
    }

    while(args[i] != NULL)
    {
        //printf("setting %s\n", args[i]);
        i += 1;

        if(i > MAX_ARGS)
        {
            //printf("Too many arguments in line. (limit %d)\n", MAX_ARGS);
            generic_error();
            exit(1);
        }

        args[i] = strtok(NULL, " \n\t");
    }
    return args;
}

void handle_path(char** args)
{
    // set the first byte to \0 so that concats can be used
    bzero(path, 1);
    for(int i = 1; args[i] != NULL; i++)
    {
        //printf("%s\n", args[i]);
        strcat(path, args[i]);
        strcat(path, " ");
        //free(args[i]);
    }
}

void handle_cd(char** args)
{
    if(args[0] != NULL && args[1] != NULL && args[2] == NULL)
    {
        //print_args(args);
        if(chdir(args[1]) != 0)
        {
            //perror("error invalid path not able to cd.");
            generic_error();
        }
    }
    else
    {
        //perror("invalid number of arguments for cd\n");
        generic_error();
        return;
    }
}

/*
    This function recieves a list of char** containing all of the
    args from a single line. It is responsible for returning a new list of
    char** that contains all of the arguments in a single command. It must
    also update the pointer next to hold the location in the original args 
    after the replaced so that & so that on subsequent calls the next 
    commands can be obtained. ^^
*/
char** get_next_command(char** args, char*** next)
{
    //possibly need to free somewhere.
    //printf("called now\n");
    *next = NULL;
    for(int i = 0; args[i] != NULL; i++)
    {
        //printf("arg %d: %s\n", i, args[i]);

        // if the current arg is an & then replace ampersand with NULL
        // return pointer to args. 
        if(strcmp(args[i], "&") == 0)
        {
            if(i != 0)
            {
                args[i] = NULL;
            }
            
            // if there is another command to parse after &
            if(args[i+1] != NULL)
            {
                //printf("args: %p\n next_cur: %p\nnext_new: %p\n", args, next, args+i+1);
                *next = args + i + 1;
            }
            break;
        }
    }
    return args;
}
void handle_external_command(char** args)
{
     // create a modifiable copy of path
    int path_len = strlen(path) + 1;
    char path_cp[path_len];
    strncpy(path_cp, path, path_len);

    char* directory = strtok(path_cp, " ");
    bool found_file = false;

    while(directory != NULL)
    {
        // concat the directory with args[0]
        //printf("direct: %s\n", directory);
        // +2 1 for termination char and 1 for /
        int file_path_length = strlen(directory) + strlen(args[0]) + 2;
        char file_path[file_path_length];
        strncpy(file_path, directory, file_path_length);
        strcat(file_path, "/");
        strcat(file_path, args[0]);
        //printf("path %s\n", file_path);

        //printf("filepath: %s\n", file_path);
        if (access(file_path, X_OK) == 0)
        {
            // a valid executable has been found
            // //printf("attempting running program at %s\n", file_path);
            found_file = true;
            pid_t child;
            child = fork();

            /*
            Need to have a loop to fork all commands before reaching this point.
            */

            if(child != 0)
            {
                // parent returns
                /* // This is not the child
                int wstatus = -1;
                if(waitpid(child, &wstatus, 0) == -1)
                {
                    // //printf("waitpid failed.\n");
                    generic_error();
                }
                //printf("wstatus: %d\n", wstatus);
                //printf("parent here: %d\n", child); */
                return;
            }
            else
            {
                //printf("child calling execv\n");

                // check if redirection is present
                int redirection_file = -1;
                char* redirect_char_loc = NULL;

                for(int i = 0; args[i] != NULL; i++)
                {
                    //printf("%s\n", args[i]);
                    // this requires a space which is a problem.
                    // the argument is a >
                    if(strcmp(args[i], ">") == 0)
                    {
                        //printf("attempting redirection occured\n");
                        if(args[i+1] != NULL && args[i+2] == NULL)
                        {
                            //printf("redirection occuring\n");
                            redirection_file = open(args[i+1], O_CREAT|O_TRUNC|O_WRONLY, 0666);
                            if (redirection_file < 0)
                            {
                                //  perror("unable to open redirection file\n");
                                generic_error();
                                return;
                            }
                            // remove all args after and including >
                            set_args_to_null(args, i);
                        }
                        else
                        {
                            generic_error();
                            return;
                        }
                        break;
                    }
                    //The argument contains a >
                    else if((redirect_char_loc = strchr(args[i], '>')) != NULL)
                    {
                        //printf("here");
                        if(redirect_char_loc[1] != '\0')
                        {
                            redirection_file = open((redirect_char_loc+1), O_CREAT|O_TRUNC|O_WRONLY, 0666);
                            if (redirection_file < 0)
                            {
                                //  perror("unable to open redirection file\n");
                                generic_error();
                                return;
                            }
                            // remove all args after and including >
                            set_args_to_null(args, i);
                            // terminate string at redirect char
                            *redirect_char_loc = '\0';
                            //printf("%s\n", args[i]);
                        }
                        else
                        {
                            generic_error();
                            return;
                        }
                    }
                }

                // configure output and error redirection
                dup2(redirection_file, 1);
                dup2(redirection_file, 2);


                // need to be able to prepend to 
                //printf("excv\n");
                execv(file_path, args);
                //printf("unexpected return execv error occured.\n");
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




void handle_command(char** args)
{
    if(args[0] == 0)
    {
        // check that this is the right error message
        //printf("error uninitialized");
        generic_error();
    }
    else if(strcmp(args[0], "&") == 0)
    {
        return;
    }

    if(strcmp(args[0], "exit") == 0)
    {
        //printf("builtin exit called:\n");
        if(args[1] != NULL)
        {
            generic_error();
        }
        exit(0);
    }
    else if(strcmp(args[0], "cd") == 0)
    {
        //printf("builtin cd called:\n");
        handle_cd(args);
    }
    else if(strcmp(args[0], "path") == 0)
    {
        //printf("builtin path called:\n");
        handle_path(args);
    }
    else
    {
        //printf("not built in cmd\n");
        //printf("WIP --- attempting to execute command.\n");

        //search_for_program(args);
        handle_external_command(args);
    }
}

/*
Need something like start all commands then wait for commands to finish.
*/

/* void thinking(args)
{
    args = parse_line(1);
    command_args = get_next_command(args);
    pid_t children[];
    while (command_args != NULL)
    {
        execute_command();
    }
    wait_for_all_commands();
} */


int main(int argc, char* argv[])
{
    char** args;
    FILE *fp = fdopen(0, "r");
    if(argc > 1)
    {
        //printf("more than two args.\n");
        //printf("running from file %s\n", argv[1]);
        fp = fopen(argv[1], "r");
        //printf("her3e");
        if(fp == NULL)
        {
            //printf("here");
            generic_error();
            exit(1);
        }
        if(fgetc(fp) == 0 || argc > 2)
        {
            generic_error();
            exit(1);
        } 
        rewind(fp);
    }
    while(true)
    {
        char** next = NULL;
        args = parse_line(fp);
        if(args != NULL)
        {
            // possible memory leaks
            get_next_command(args, &next);
            //print_args(args);
            handle_command(args);
            args = next;
            while(next != NULL)
            {
                //printf("main next: %p\n", next);
                get_next_command(args, &next);
                handle_command(args);
                //args is modified by get next command
                args = next;
            }
            while(wait(NULL) > 0);
        }
    }

    //free(args);

    return 0;
}