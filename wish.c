#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

/*
This function will return the first sub command in the string
*/
char* chunk()
{
    char* test;
    return test;
}


/*
This function will return an array of strings that contains each of the arguments in
a sub command
*/
char** getargs()
{
    char** test;
    return test;
}




int main()
{
    pid_t child;
    char cmd[20];
    char* args[20];

    while(true)
    {
        printf("wish> ");
        fgets(cmd, 20, stdin);
        int i = 0;
        args[i] = strtok(cmd, " \n");
        while(args[i] != NULL)
        {
            /* printf("%s\n", args[i]); */
            i += 1;
            args[i] = strtok(NULL, " \n");
        }
        

        /* for(int j = 0; j <= i ; j++)
        {
            printf("arg %d: %s\n", j, args[j]);
        } */


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
            int status = execv(args[0], args);
            printf("unexpected return execv error occured.\n");
        }

    }

    return 0;
}