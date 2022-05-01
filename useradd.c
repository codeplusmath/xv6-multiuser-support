#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

// Checking given tag
#define STATUS_UNDEFINED 0
#define STATUS_DIR 1
#define STATUS_UID 2
#define STATUS_NAME 3

void
insert_string(char* destination, int pos, char* seed)
{
	char * str_Cp;
	str_Cp = (char*)malloc(strlen(destination)+strlen(seed)+1);
	strncpy(str_Cp,destination,pos);
	str_Cp[pos] = '\0';
	strcat(str_Cp,seed);
	strcat(str_Cp, destination+pos);
	strcpy(destination,str_Cp);
	free(str_Cp);
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("useradd: user is required\n");
		exit();
	}

	int uid = -1, password_file_descp, i;
	char dir[64];
	char *name, *user;

	int Status = STATUS_UNDEFINED;

	for (i = 1; i < argc; i ++)
	{
		char *current_argument = argv[i];
		// Status token or username:
		if (Status == STATUS_UNDEFINED) {
	    		if (strstr(current_argument, "-d"))
				Status = STATUS_DIR;
	    		else if (strstr(current_argument, "-u"))
				Status = STATUS_UID;
	    		else if (strstr(current_argument, "-c"))
				Status = STATUS_NAME;
			else if (strstr(current_argument, "-")) {
				printf("useradd: invalid command\n");
				exit();
	    		}
		else
			user = current_argument;

		} 
		else {
			if (Status == STATUS_DIR)
				strcpy(dir, current_argument);
			else if (Status == STATUS_UID)
				uid = atoi(current_argument);
			else if (Status == STATUS_NAME)
				name = current_argument;

			Status = STATUS_UNDEFINED;
		}
	}

	if (user == NULL) {
		printf("useradd: user is required\n");
		exit();
	}
	if((password_file_descp = open("/etc/passwd", O_RDWR)) < 0){
		printf("useradd: cannot open etc/passwd\n");
		exit();
	}

	// uid validation:
	if (uid == -1)
		uid = next_uid();
	else if (validate_uid(uid) < 0) {
		printf("useradd: uid already exists\n");
		exit();
	}

	// Directory name building:
	if (strlen(dir) == 0) {
		strcat(dir, "/home/");
		strcat(dir, user);
		strcat(dir, "\0");
	} else
		insert_string(dir, 0, "/home/\0");

	if (mkdir(dir) < 0) {
		printf("useradd: cannot create user directory\n");
		exit();
	}

	chown(dir, uid);

	int n;
	char Password_Buffer[256];
	char _uid[10];

	strcat(Password_Buffer, user);
	strcat(Password_Buffer, ":_:");
	strcat(Password_Buffer, itoa(uid, _uid, 10));
	strcat(Password_Buffer, ":");
	strcat(Password_Buffer, name == NULL ? "_" : name);
	strcat(Password_Buffer, ":");
	strcat(Password_Buffer, dir);
	strcat(Password_Buffer, "\n\0");

	// Reading the file just to move offset to the end:
	read(password_file_descp, 0, passwd_len());

	if ((write(password_file_descp, Password_Buffer, strlen(Password_Buffer))) <= 0) {
		printf("useradd: cannot write into passwd\n");
		exit();
	}
	
	close(password_file_descp);
	exit();
}
