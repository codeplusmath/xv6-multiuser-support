#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

char*
uid_to_user(uint uid)
{
	int fd, i, count;
	char bufferer[2048];
	char * usr = NULL;

	if((fd = open("/etc/passwd", 0)) < 0){
                printf("UserID to User: cannot open /etc/passwd\n");
                return NULL;
        }
	
	if((i = read(fd, bufferer, sizeof(bufferer))) <= 0) return null;
	char* token = strtok(bufferer, ":");
	// reading user & its UID
	while(token != NULL){
		if(count == 0){
			usr = malloc(strlen(token)+1);
			strcpy(usr, token);
		}
		if (count == 2) {
                        uint _uid = atoi(token);
                        if (_uid == uid) {
                                close(fd);
                                return usr;
                        }
                }
                if (count == 4) token = strtok(NULL, "\n");
                else token = strtok(NULL, ":");
                count ++;
                if (count == 6) count = 0;
	}
	close(fd);
	return NULL;
}

uint
nextuid()
{
	int fd, i, count=0;
	uint min_uid = 1000;
	char * token =NULL;
	char bufferer[2048];
        if((fd = open("/etc/passwd", 0)) < 0){
                printf("nextuid: cannot open /etc/passwd\n");
                return NULL;
        }
        if ((i = read(fd, bufferer, sizeof(bufferer))) <= 0) return NULL;
	token = strtok(bufferer, ":");
        // reading uid
	while(token != NULL) {
                if (count == 2) {
                        int curr_uid = atoi(token);

                        if (curr_uid >= 1000 && curr_uid == min_uid)
                                min_uid ++;
                }

                if (count == 4)
                        token = strtok(NULL, "\n");
                else
                        token = strtok(NULL, ":");

                count ++;
                if (count == 6) {
                        count = 0;
                }
        }
    	close(fd);
        return min_uid;

}


uint
validate_uid(uint uid)
{
        int fd, i, count = 0;
        char buffer[2048];
        
        if((fd = open("/etc/passwd", 0)) < 0){
                printf("nextuid: cannot open /etc/passwd\n");
                return NULL;
        }

        if ((i = read(fd, buffer, sizeof(buffer))) <= 0) return NULL;
        char* token = strtok(buffer, ":");
        while(token != NULL) {
                // Read UID:
                if (count == 2) {
                        if (uid == atoi(token)) {
                                close(fd);
                                return -1;
                        }
                }
                if (count == 4) token = strtok(NULL, "\n");
                else token = strtok(NULL, ":");
                count ++;
                if (count == 6) count = 0;
        }

        close(fd);
        return 0;
}


int
validate_user_pass(uint uid, char* pass)
{
        int fd, i;
        if((fd = open("/etc/passwd", 0)) < 0){
                printf("uidtouser: cannot open /etc/passwd\n");
                return NULL;
        }
        char buffer[2048];

        if ((i = read(fd, buffer, sizeof(buffer))) <= 0)
                return NULL;

        char* token = strtok(buffer, ":");
        int count = 0;
        char* _pass = NULL;
        while(token != NULL) {
                // read pass
                if (count == 1) {
                        _pass = malloc(strlen(token)+1);
                        strcpy(_pass, token);
                }
                // read UID
                if (count == 2) {
                        uint _uid = atoi(token);
                        if (_uid == uid) {
                                int same = strcmp(pass, _pass);
                                close(fd);
                                return same == 0;
                        }
                }

                if (count == 4) token = strtok(NULL, "\n");
                else token = strtok(NULL, ":");

                count ++;
                if (count == 6) count = 0;
        }
        close(fd);
        return 0;
}



char *
uid_to_pass(uint uid)
{
        int fd, i;
        if((fd = open("/etc/passwd", 0)) < 0){
                printf("uidtopass: cannot open /etc/passwd\n");
                return NULL;
        }
        char buffer[2048];
        if ((n = read(fd, buffer, sizeof(buffer))) <= 0) return NULL;
        char* token = strtok(buffer, ":");
        int count = 0;
        char* pass = NULL;
        while(token != NULL) {
                // read pass
                if (count == 1) {
                        pass = malloc(strlen(token)+1);
                        strcpy(pass, token);
                }
                // read uid
                if (count == 2) {
                        uint _uid = atoi(token);
                        if (_uid == uid) {
                                close(fd);
                                return pass;
                        }
                }

                if (count == 4) token = strtok(NULL, "\n");
                else token = strtok(NULL, ":");
                count ++;
                if (count == 6) count = 0;
        }
        close(fd);
        return NULL;
}


uint
user_to_uid(char* name)
{
        int fd,i;
        if((fd = open("/etc/passwd", 0)) < 0){
                printf("uidtopass: cannot open /etc/passwd\n");
                return NULL;
        }
        
        char buffer[2048];
        if ((i = read(fd, buffer, sizeof(buffer))) <= 0) return NULL;
        char* token = strtok(buffer, ":");
        int count = 0;
        int found = 0;
        while(token != NULL) {
                // read user
                if (count == 0) {
                        if (strcmp(name, token) == 0)
                                found = 1;
                }
                // read UID
                if (count == 2) {
                        uint uid = atoi(token);
                        if (found) {
                                close(fd);
                                return uid;
                        }
                }

                if (count == 4) token = strtok(NULL, "\n");
                else token = strtok(NULL, ":");

                count ++;
                if (count == 6) count = 0;
        }
        close(fd);
        return -1;
}

uint
passwd_len()
{
        int fd, i;
        if((fd = open("/etc/passwd", 0)) < 0){
                printf("passwdlen: cannot open /etc/passwd\n");
                return NULL;
        }
        char buffer[2048];
        if ((i = read(fd, buffer, sizeof(buffer))) <= 0) return NULL;
        close(fd);
        return strlen(buffer);
}

