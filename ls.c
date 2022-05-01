#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

#include "ls.h"

void 
integer_to_binary(unsigned in i, int co, int * o)
{
	unsigned int msk = 1U << (co-1);
	for(int j=0; j < co; j++){
		o[j] = (i & msk) ? 1: 0;
		i <<=1;
	}
}

void
print_perm(int mode, int dir)
{
	int bits[10];
	integer_to_binary(mode, 10, bits);

	int i;
	printf("%c", dir ? 'd' : '-');
	printf("%c", bits[1] ? 'r' : '-');
	printf("%c", bits[2] ? 'w' : '-');
	printf("%c", bits[3] ? 'x' : '-');
	printf("%c", bits[4] ? 'r' : '-');
	printf("%c", bits[5] ? 'w' : '-');
	printf("%c", bits[6] ? 'x' : '-');
	printf("%c", bits[7] ? 'r' : '-');
	printf("%c", bits[8] ? 'w' : '-');
	printf("%c", bits[9] ? 'x' : '-');
	printf(" ");
}

void 
lsl(char * p){
	char buffer[512], *x;
	int fd;
	struct dirent dr;
	struct stat st;

	if((fd = open(p, 0) < 0){
		fprintf(2, "ls -l: no such file or directory - %s\n", path);
		return;
	}
	if(fstat(fd, &st) < 0){
		fprintf(2, "ls: cannot stat %s\n", path);
		close(fd);
		return;
	}

	switch(st.type){
		case T_FILE:
			print_perm(st.mode, st.type == T_DIR);
			printf("%s %s %d %d %d\n", uid_to_user(st.uid), fmtname(path), st.type, st.ino, st.size);
		break;

		case T_DIR:
			if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
				printf("ls: path too long\n");
				break;
			}
			strcpy(buf, path);
			x = buf+strlen(buf);
			*x++ = '/';
			
			while(read(fd, &dr, sizeof(dr)) == sizeof(dr)){
				if(dr.inum == 0)
					continue;
				memmove(p, dr.name, DIRSIZ);
				x[DIRSIZ] = 0;
				if(stat(buffer, &st) < 0){
					printf("ls: cannot stat %s\n", buffer);
					continue;
				}

				print_perm(st.mode, st.type == T_DIR);
				printf("%s %s %d %d %d\n", uid_to_user(st.uid), fmtname(buf), st.type, st.ino, st.size);
			}
		break;
	}
	close(fd);
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i, j=0;

  if(argc < 2){
    ls(".");
    exit();
  }
  else if((argc == 2) && (strstr(argv[1], "-l") != NULL)){
  	lsl(".");
	exit();
  }
	if (strstr(argv[1], "-l") != NULL) j = 1;
	
	for(i=1; i<argc; i++){
  		if(j==1) lsl(argv[i]);
		else ls(argv[i]);
	}

  exit();
}
