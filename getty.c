#include "types.h"
#include "stat.h"
#include "user.h"

int User_Done = 0;
int Pass_Done = 0;

char user_name[30];
char password[32];

int
get_terminal(char *buf, int nbuf)
{
    if (!User_Done) {
        printf("getty: Enter your user_name: ");
        gets(user_name, sizeof(user_name));
        user_name[strlen(user_name) - 1] = '\0';
        User_Done = 1;
    } else {
        printf("getty: Enter current password: ");
        echoswitch();
        gets(password, sizeof(password));
        echoswitch();
        password[strlen(password) - 1] = '\0';
        Pass_Done = 1;
    }

	return 0;
}


char*
uid_to_dir(uint uid)
{
	int fd;

	if((fd = open("/etc/passwd", 0)) < 0){
		printf("uidtopass: cannot open /etc/passwd\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");

	int counter = 0;
	int found = 0;
	char* dir = NULL;

	while(tok != NULL) {
		// Read UID:
		if (counter == 2) {
			uint _uid = atoi(tok);
			if (_uid == uid) {
				found = 1;
			}
		}
		// Read pass:
		if (counter == 5) {
			if (found) {
				dir = malloc(strlen(tok)+1);
				strcpy(dir, tok);

				close(fd);
				return dir;
			}
		}

		if (counter == 4)
			tok = strtok(NULL, "\n");
		else
			tok = strtok(NULL, ":");

		counter ++;
		if (counter == 6) {
			counter = 0;
		}
	}

	close(fd);

	return NULL;
}

void show_terminal(int uid)
{    
	char *shargv[] = { "sh", 0 };
	int wpid;
	int n;
	char message_of_day[256];

	int fd;
	if ((fd = open("/etc/motd", 0)) < 0) {
		printf("getty: cannot open /etc/motd\n");
		exit();
	}

	if ((n = read(fd, motd, sizeof(message_of_day))) <= 0) {
	printf("getty: cannot read from /etc/motd\n");
		exit();
	}

	printf("\n%s", message_of_day);

	close(fd);

	int pid = fork();
	if (pid < 0) {
		printf("getty: fork failed\n");
		exit();
	}
	if (pid == 0) {
		printf("getty: starting sh with uid %d\n", uid);

		setuid(uid);
		chdir(uid_to_dir(uid));
		exec("/bin/sh", shargv);
		printf("getty: exec sh failed\n");
		exit();
	}
	while((wpid=wait()) >= 0 && wpid != pid)
		printf("zombie!\n");
	exit();
}


int
main(int argc, char *argv[])
{
	int uid;
	int n;
	static char buf[100];
	char issues[256];

	clrscr();

	int fd;
	if ((fd = open("/etc/issue", 0)) < 0) {
		printf("getty: cannot open /etc/issue\n");
	}

	if ((n = read(fd, issues, sizeof(issues))) <= 0) {
		printf("getty: cannot read from /etc/issue\n");
	}

	printf("Issues: %s\n\n", issues);

	while(get_terminal(buf, sizeof(buf)) >= 0){
		if (User_Done && Pass_Done) {

			uid = user_to_uid(user_name);
			if (uid < 0) {
				printf("getty: Username doesn't exist!\n");
				User_Done = 0;
				Pass_Done = 0;
				continue;
			}

			char* userpass = uid_to_pass(uid);
			if (strcmp(userpass, password) != 0) {
				printf("getty: Passwords don't match!\n");
				User_Done = 0;
				Pass_Done = 0;
				continue;
			}

			break;
		}
		wait();
	}
	show_terminal(uid);
}

