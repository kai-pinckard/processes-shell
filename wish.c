#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>


// This is a space delimited list of directories.
const int MAX_PATH_SIZE = 4096;
char path[4096] = "/bidn/ /usr/bin/";


void set_args_to_null(char** args, int start_index)
{
    for(int i = start_index; args[i] != NULL; i++)
    {
        args[i] = NULL;
    }
    return;
}

void print_args(char** args)
{
    printf("------------Printing---------------\n");
    for(int i = 0; args[i] != NULL; i++)
    {
        printf("%s\n", args[i]);
        //free(args[i]);
    }
    printf("------------end printing------------\n\n");
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

    if( args == NULL)
    {
        printf("calloc failed to allocate requested memory.\n");
        exit(1);
    }

    // need to free memory allocated by getline
    if (getline(&line_buffer, &buffer_size, input_file) == -1)
    {
        printf("end of file reached\n");
        exit(0);
    }

    int i = 0;
    args[0] = strtok(line_buffer, " \n");

    if(args[0] == NULL)
    {
        //printf("error invalid input empty");
        return NULL;
    }

    while(args[i] != NULL)
    {
        //printf("setting %s\n", args[i]); 
        i += 1;

        if(i > MAX_ARGS)
        {
            printf("Too many arguments in line. (limit %d)\n", MAX_ARGS);
            exit(1);
        }

        args[i] = strtok(NULL, " \n");
    }

    /*  for(int i = 0; i < 2; i++)
    {
        printf("%s\n", args[i]);
    } */

    // line buffer stores all of the sub strings.
    // free line_buffer);
    // and or maybe free each indivudual pointer in args by looping through
    printf("returned\n");
    return args;
}


/*
    This function is responsible for seeing if an executable exists in the directories
    specified by path that corresponds to the entered arguments.
*/
void search_for_program(char** args)
{


    // create a modifiable copy of path
    int path_len = strlen(path) + 1;
    char path_cp[path_len];
    strncpy(path_cp, path, path_len);


    char* directory = strtok(path_cp, " ");

    while(directory != NULL)
    {
        // concat the directory with args[0]
        int file_path_length = strlen(directory) + strlen(args[0]) + 1;
        char file_path[file_path_length];
        strncpy(file_path, directory, file_path_length);
        strcat(file_path, args[0]);

        if (access(file_path, X_OK) == 0)
        {
            printf("program found at %s\n", file_path);
            return;
        }

        directory = strtok(NULL, " ");
    }
    printf("error no program found.\n");

}


void handle_path(char** args)
{
    // set the first byte to \0 so that concats can be used
    bzero(path, 1);
    for(int i = 1; args[i] != NULL; i++)
    {
        printf("%s\n", args[i]);
        strcat(path, args[i]);
        strcat(path, " ");
        //free(args[i]);
    }
}

void handle_cd(char** args)
{
    if(args[0] != NULL && args[1] != NULL && args[2] == NULL)
    {
        print_args(args);
        if(chdir(args[1]) != 0)
        {
            perror("error invalid path not able to cd.");
        }
    }
    else
    {
        perror("invalid number of arguments for cd\n");
        return;
    }
}

void handle_command(char** args)
{
    if(args[0] == 0)
    {
        // check that this is the right error message
        printf("error uninitialized");
    }

    if(strcmp(args[0], "exit") == 0)
    {
        printf("builtin exit called:\n");
        exit(0);
    }
    else if(strcmp(args[0], "cd") == 0)
    {
        printf("builtin cd called:\n");
        handle_cd(args);
    }
    else if(strcmp(args[0], "path") == 0)
    {
        printf("builtin path called:\n");
        handle_path(args);
    }
    else
    {
        printf("not built in cmd\n");
        printf("WIP --- attempting to execute command.\n");

        //search_for_program(args);

        // create a modifiable copy of path
        int path_len = strlen(path) + 1;
        char path_cp[path_len];
        strncpy(path_cp, path, path_len);

        char* directory = strtok(path_cp, " ");

        while(directory != NULL)
        {
            // concat the directory with args[0]
            printf("direct: %s\n", directory);
            int file_path_length = strlen(directory) + strlen(args[0]) + 1;
            char file_path[file_path_length];
            strncpy(file_path, directory, file_path_length);
            strcat(file_path, args[0]);

            printf("filepath: %s\n", file_path);
            if (access(file_path, X_OK) == 0)
            {
                // a valid executable has been found
                printf("attempting running program at %s\n", file_path);

                pid_t child;
                child = fork();

                if(child != 0)
                {
                    // This is not the child
                    int wstatus = -1;
                    if(waitpid(child, &wstatus, 0) == -1)
                    {
                        printf("waitpid failed.\n");
                    }
                    printf("wstatus: %d\n", wstatus);
                    printf("parent here: %d\n", child);
                    return;
                }
                else
                {
                    printf("child calling execv\n");

                    // check if redirection is present
                    int redirection_file = -1;

                    for(int i = 0; args[i] != NULL; i++)
                    {
                        printf("%s\n", args[i]);
                        // this requires a space which is a problem.
                        if(strcmp(args[i], ">") == 0)
                        {
                            printf("attempting redirection occured\n");
                            if(args[i+1] != NULL && args[i+2] == NULL)
                            {
                                printf("redirection occuring\n");
                                redirection_file = open(args[i+1], O_CREAT|O_TRUNC|O_WRONLY, 0666);
                                if (redirection_file < 0)
                                {
                                    perror("unable to open redirection file\n");
                                    return;
                                }
                                // remove all args after and including >
                                set_args_to_null(args, i);
                            }
                            break;
                        }
                    }

                    // configure output and error redirection
                    dup2(redirection_file, 1);
                    dup2(redirection_file, 2);


                    // need to be able to prepend to 
                    execv(file_path, args);
                    printf("unexpected return execv error occured.\n");
                    exit(1);
                }


            }

            directory = strtok(NULL, " ");
        }
    }
}


int main(int argc, char* argv[])
{
    char** args;

    // and start building against the unit tests
    if(argc < 2)
    {
        //printf("less than two args.\n");
        //printf("interactive mode: \n");

        while(true)
        {
            printf("wish> ");
            args = parse_line(stdin);
            if(args != NULL)
            {
                print_args(args);
                handle_command(args);
            }
        }

    }
    else
    {
        printf("more than two args.\n");
        printf("running from file %s\n", argv[1]);
        FILE *fp = fopen(argv[1], "r");

        while(true)
        {
            printf("wish> ");
            args = parse_line(fp);
            if(args != NULL)
            {
                print_args(args);
                handle_command(args);
            }
        }
    }

    //free(args);

    return 0;
}