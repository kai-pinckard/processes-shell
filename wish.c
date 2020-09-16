#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>

/*
This function takes a line that has been read with getline and
parses it into its tokens. An array of char arrays is returned with
the length of each sub array being the length of the longest argument.
*/
char** parse_line()
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
    getline(&line_buffer, &buffer_size, stdin);

    printf("%s\n", line_buffer);

    int i = 0;
    args[0] = strtok(line_buffer, " \n");
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
    }
    else if(strcmp(args[0], "path") == 0)
    {
        printf("builtin path called:\n");
    }
    else
    {
        printf("not built in cmd\n");
        printf("WIP --- attempting to execute command.\n");

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
        }
        else
        {
            // This is the child
            printf("child here\n");
            //run execv

            /* //there may be an extra \n in the last argument
            for(int j = 0; j <= i ; j++)
            {
                printf("arg %d: %s\n", j, args[j]);
            } 
            */
            printf("child calling execv\n");

            // previously &(args[1])
            execv(args[0], args);
            printf("unexpected return execv error occured.\n");
            exit(1);
        }
    }
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



int main(int argc, char* argv[])
{
    char** args;
    // update this to also include user bin
    //char path[256] = "/bin/";

    if(argc < 2)
    {
        printf("less than two args.\n");
        printf("interactive mode: \n");

        while(true)
        {
            printf("wish> ");
            args = parse_line();
            print_args(args);
            handle_command(args);
        }


    }
    else
    {
        printf("more than two args.\n");
        printf("running from file %s\n", argv[1]);
    }

    //free(args);

    return 0;
}