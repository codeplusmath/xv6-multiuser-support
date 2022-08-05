#include"types.h"
#include"stack.h"
#include"user.h"
#include"fs.h"

void id(void){
    int euid = myproc()->euid;
    printf("uid = %d (%s)",euid,uid_to_user(euid));
    exit();
}
