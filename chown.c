#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("chown: invalid arg count\n");
        exit();
    }

    int i;
    char* user = NULL;
     
    if (strlen(argv[1]) <= 1) {
        printf("chown: invalid args\n");
        exit();
    }

    // No colon so only user was provided:
    if (strstr(argv[1], ":") == NULL) {
        user = argv[1];
    }
    else {
        int arglen = strlen(argv[1]);
        char* token = strtok(argv[1], ":");

        // User and colon after him:
        if (strlen(token) + 1 == arglen) {
            user = token;
         } 
        // We have both the user  
        else {
            user = token;
            token = strtok(NULL, ":");             
        }
    }

    uint uid = -1, gid = -1;
    if (user != NULL) {
        uid = atoi(user);

        if (uid == -1)
            uid = user_to_uid(user);
    }
    for (i = 2; i < argc; i ++)
        chown(argv[i], uid );
    
	exit();
}
