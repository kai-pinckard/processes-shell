#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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
    char* args[20][20];

    fgets(cmd, 20, stdin);
    printf("%s", cmd);
    args[0] = strtok(cmd, " ");
    int i = 1;
    while(val != NULL)
    {
        printf("%s\n", val);
        args[] = strtok(NULL, " ");
        i += 1;
    }
    
    child = fork();

    if(child != 0)
    {
        // This is not the child
        printf("%d\n", child);
    }
    else
    {
        // This is the child
        printf("child here\n");
        //run execv
    }

    return 0;
}