#include"types.h"
#include"stack.h"
#include"user.h"
#include"fs.h"

void whomi(void){
    int uid = myproc()->uid;
    printf("%s",uid_to_user(uid));
    exit();
}
