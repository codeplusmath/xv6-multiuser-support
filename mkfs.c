#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

#define NINODES 200

//Default mode for file inode i.e. read, write & execute for everyone
#define DEFAULT_MODE 0777

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

int nbitmap = FSSIZE/(BSIZE*8) + 1;
int ninodeblocks = NINODES / IPB + 1;
int nlog = LOGSIZE;
int nmeta;    // Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks;  // Number of data blocks

int fsfd;
struct superblock sb;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;

// gloabal inode variables for directries
uint root_inode_no;
uint dev_inode_no;
uint bin_inode_no;
uint home_inode_no;
uint super_user_inode_no;
uint etc_inode_no;


void balloc(int);
void wsect(uint, void*);
void winode(uint, struct dinode*);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type, uint mode); // assigns a disk inode to a newly created file & it requires mode of file
void iappend(uint inum, void *p, int n);

// convert to intel byte order
ushort
xshort(ushort x)
{
  ushort y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint
xint(uint x)
{
  uint y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

// create required default directories with all permisions
void
make_directories(void)
{
	struct dirent dr;

	// for mount point directory i.e. /
	// ROOTINO is 1
	root_inode_no = ialloc(T_DIR, DEFAULT_MODE);
	assert(root_inode_no == ROOTINO);
	
	// writes 0 to inode
	bzero(&dr, sizeof(dr));
	dr.inum = xshort(root_inode_no);
	strcpy(dr.name, ".");
	iappend(root_inode_no, &dr, sizeof(dr));

	bzero(&dr, sizeof(dr));
        dr.inum = xshort(root_inode_no);
        strcpy(dr.name, "..");
        iappend(root_inode_no, &dr, sizeof(dr));

	// for dev directory i.e. /dev
	dev_inode_no = ialloc(T_DIR, DEFAULT_MODE);

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(dev_inode_no);
        strcpy(dr.name, ".");
        iappend(dev_inode_no, &dr, sizeof(dr));

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(root_inode_no);
        strcpy(dr.name, "..");
        iappend(dev_inode_no, &dr, sizeof(dr));

	bzero(&dr, sizeof(dr));
        dr.inum = xshort(dev_inode_no);
        strcpy(dr.name, "dev");
        iappend(root_inode_no, &dr, sizeof(dr));


	// for bin directory i.e. /bin
        bin_inode_no = ialloc(T_DIR, DEFAULT_MODE);

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(bin_inode_no);
        strcpy(dr.name, ".");
        iappend(bin_inode_no, &dr, sizeof(dr));

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(root_inode_no);
        strcpy(dr.name, "..");
        iappend(bin_inode_no, &dr, sizeof(dr));
	
	bzero(&dr, sizeof(dr));
        dr.inum = xshort(bin_inode_no);
        strcpy(dr.name, "bin");
        iappend(root_inode_no, &dr, sizeof(dr));

	// for home directory i.e. /home
        home_inode_no = ialloc(T_DIR, DEFAULT_MODE);

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(home_inode_no);
        strcpy(dr.name, ".");
        iappend(home_inode_no, &dr, sizeof(dr));

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(root_inode_no);
        strcpy(dr.name, "..");
        iappend(home_inode_no, &dr, sizeof(dr));

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(home_inode_no);
        strcpy(dr.name, "home");
        iappend(root_inode_no, &dr, sizeof(dr));

	// for root user root directory i.e. /root
        super_user_inode_no = ialloc(T_DIR, DEFAULT_MODE);

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(super_user_inode_no);
        strcpy(dr.name, ".");
        iappend(super_user_inode_no, &dr, sizeof(dr));

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(home_inode_no);
        strcpy(dr.name, "..");
        iappend(super_user_inode_no, &dr, sizeof(dr));

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(super_user_inode_no);
        strcpy(dr.name, "root");
        iappend(home_inode_no, &dr, sizeof(dr));

	// for etc directory i.e. /etc
        etc_inode_no = ialloc(T_DIR, DEFAULT_MODE);

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(etc_inode_no);
        strcpy(dr.name, ".");
        iappend(etc_inode_no, &dr, sizeof(dr));

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(root_inode_no);
        strcpy(dr.name, "..");
        iappend(etc_inode_no, &dr, sizeof(dr));

        bzero(&dr, sizeof(dr));
        dr.inum = xshort(etc_inode_no);
        strcpy(dr.name, "etc");
        iappend(root_inode_no, &dr, sizeof(dr));
}


int
main(int argc, char *argv[])
{
  int i, cc, fd;
  uint dir_inode_no, inum;
  struct dirent de;
  char buf[BSIZE];
  char * short_name;


  static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

  if(argc < 2){
    fprintf(stderr, "Usage: mkfs fs.img files...\n");
    exit(1);
  }

  assert((BSIZE % sizeof(struct dinode)) == 0);
  assert((BSIZE % sizeof(struct dirent)) == 0);	

  // read & write permissions
  fsfd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fsfd < 0){
    perror(argv[1]);
    exit(1);
  }

  // 1 fs block = 1 disk sector
  nmeta = 2 + nlog + ninodeblocks + nbitmap;
  nblocks = FSSIZE - nmeta;

  sb.size = xint(FSSIZE);
  sb.nblocks = xint(nblocks);
  sb.ninodes = xint(NINODES);
  sb.nlog = xint(nlog);
  sb.logstart = xint(2);
  sb.inodestart = xint(2+nlog);
  sb.bmapstart = xint(2+nlog+ninodeblocks);

  printf("nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
         nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

  freeblock = nmeta;     // the first free block that we can allocate

  for(i = 0; i < FSSIZE; i++)
    wsect(i, zeroes);

  memset(buf, 0, sizeof(buf));
  memmove(buf, &sb, sizeof(sb));
  wsect(1, buf);

  make_directories();
  
  for(i = 2; i < argc; i++){
	// skipping 'user'
	if(strncmp(argv[i], "user/", 5) == 0) 
		short_name = argv[i] + 5;
	else
		short_name = argv[i];

    assert(index(argv[i], '/') == 0);

    if((fd = open(argv[i], 0)) < 0){
      perror(argv[i]);
      exit(1);
    }

    // Skip leading _ in name when writing to file system.
    // The binaries are named _rm, _cat, etc. to keep the
    // build operating system from trying to execute them
    // in place of system binaries like rm and cat.
    if(short_name[0] == '_'){
	// all binaries get copied to /bin & other to /home
    	short_name = short_name + 1;
	dir_inode_no = bin_inode_no;
    }
    else if(strncmp(argv[i], "passwd", 6) == 0){
    	dir_inode_no = etc_inode_no;
    }

    inum = ialloc(T_FILE, dir_inode_no == etc_inode_no ? 0644: DEFAULT_MODE);

    bzero(&de, sizeof(de));
    de.inum = xshort(inum);
    strncpy(de.name, short_name, DIRSIZ);
    iappend(dir_inode_no, &de, sizeof(de));

    while((cc = read(fd, buf, sizeof(buf))) > 0)
      iappend(inum, buf, cc);

    close(fd);
  }

  balloc(freeblock);
  exit(0);
}

void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(write(fsfd, buf, BSIZE) != BSIZE){
    perror("write");
    exit(1);
  }
}

void
winode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}

void
rinode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, BSIZE) != BSIZE){
    perror("read");
    exit(1);
  }
}

uint
ialloc(ushort type, uint mode)
{
  uint inum = freeinode++;
  struct dinode din;

  bzero(&din, sizeof(din));
  din.type = xshort(type);
  din.nlink = xshort(1);
  din.size = xint(0);
  din.uid = xint(0); // To allocate a new inode (for example, when creating a file), xv6 calls ialloc
  din.mode = xint(mode);
  winode(inum, &din);
  return inum;
}

void
balloc(int used)
{
  uchar buf[BSIZE];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < BSIZE*8);
  bzero(buf, BSIZE);
  for(i = 0; i < used; i++){
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  printf("balloc: write bitmap block at sector %d\n", sb.bmapstart);
  wsect(sb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void
iappend(uint inum, void *xp, int n)
{
  char *p = (char*)xp;
  uint fbn, off, n1;
  struct dinode din;
  char buf[BSIZE];
  uint indirect[NINDIRECT];
  uint x;

  rinode(inum, &din);
  off = xint(din.size);
  // printf("append inum %d at off %d sz %d\n", inum, off, n);
  while(n > 0){
    fbn = off / BSIZE;
    assert(fbn < MAXFILE);
    if(fbn < NDIRECT){
      if(xint(din.addrs[fbn]) == 0){
        din.addrs[fbn] = xint(freeblock++);
      }
      x = xint(din.addrs[fbn]);
    } else {
      if(xint(din.addrs[NDIRECT]) == 0){
        din.addrs[NDIRECT] = xint(freeblock++);
      }
      rsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      if(indirect[fbn - NDIRECT] == 0){
        indirect[fbn - NDIRECT] = xint(freeblock++);
        wsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      }
      x = xint(indirect[fbn-NDIRECT]);
    }
    n1 = min(n, (fbn + 1) * BSIZE - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * BSIZE), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}
