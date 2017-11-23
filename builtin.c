/*   $Id: builtin.c,v 1.24 2017/06/01 22:37:44 leslier4 Exp $   */

/*  builtin.c
 *
 *  Robert Leslie
 *  6/1/2017
 *  CSCI 352
 *  Assignment 6
 *
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proto.h"
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <time.h>

/* global variables */
extern int shift;
extern int glArgc;
extern int extVal;

void builtin(char **argv, int *argcp, int infd, int outfd, int errfd){
  /*if user entered exit, perform proper exit action*/
  if(strcmp(argv[0], "exit") == 0){
    if(*argcp > 1){
      exit(atoi(argv[1]));
    }else{
      exit(0);
    }
  }
  
  int i;
  /*if user entered aecho, print proper aecho output*/
  if(strcmp(argv[0], "aecho") == 0){
    if(*argcp == 1){
      *argcp = 0;
      dprintf(outfd, "\n");
      return;
    }
    if(strcmp(argv[1], "-n") == 0){
      for(i=2; i < *argcp; i++){
	dprintf(outfd, "%s", argv[i]);
	if(i < *argcp - 1){
	  dprintf(outfd, " ");
	}
      }
    }else{
      for(i=1; i < *argcp; i++){
	dprintf(outfd, "%s", argv[i]);
	if(i < *argcp - 1){
	  dprintf(outfd, " ");
	}
      }
    }
    if(strcmp(argv[1], "-n") != 0){
      dprintf(outfd, "\n");
    }
    *argcp = 0;
    extVal = 0;
  }

  /*if user enters envset, change environment var to value entered*/
  if(strcmp(argv[0], "envset") == 0){
    if(*argcp == 3){
      setenv(argv[1], argv[2], strlen(argv[2]));
      extVal = 0;
    }else{
      dprintf(errfd, "Wrong number of args\n");
      extVal = 1;
    }
    *argcp = 0;
  }

  /*if user enters envunset, unset the environment var*/
  if(strcmp(argv[0], "envunset") == 0){
    if(*argcp == 2){
      unsetenv(argv[1]);
      extVal = 0;
    }else{
      dprintf(errfd, "Wrong number of args\n");
      extVal = 1;
    }
    *argcp = 0;
  }

  /*if user enters cd, chdir to given dir, if none entered use HOME*/
  if(strcmp(argv[0], "cd") == 0){
    if(*argcp == 1){
      if(getenv("HOME") != NULL){
	chdir(getenv("HOME"));
	extVal = 0;
      }else{
	dprintf(errfd, "No directory was given\n");
	extVal = 1;
      }
    }else{
      if(chdir(argv[1]) == -1){
	dprintf(errfd, "No such directory\n");
	extVal = 1;
      }else{
	extVal = 0;
      }
    }
    *argcp = 0;
  }

  /*if user enters shift, try to shift args*/
  if(strcmp(argv[0], "shift") == 0){
    if(*argcp > 1){
      int n = atoi(argv[1]);
      if(n < (glArgc-shift) && n > 0){
	shift = shift + n;
	extVal = 0;
      }else{
	dprintf(errfd, "Can't shift that far\n");
	extVal = 1;
      }
    *argcp = 0;
    }
    else if(*argcp == 1){
      if(1 < (glArgc-shift)){
	shift = shift + 1;
	extVal = 0;
	*argcp = 0;
      }
    }
  }

  /*if user enters unshift, try to unshift args*/
  if(strcmp(argv[0], "unshift") == 0){
    if(*argcp > 1){
      int n = atoi(argv[1]);
      if(n < shift && n > 0){
	shift = shift - n;
	extVal = 0;
      }else{
	dprintf(errfd, "Can't unshift that far\n");
	extVal = 1;
      }
    }else{
      shift = 1;
      extVal = 0;
    }
    *argcp = 0;
  }

  /*if user enters sstat, print file info for each file entered*/
  if(strcmp(argv[0], "sstat") == 0){
    int ix = 1;
    struct stat buf;
    struct passwd *pwd;
    struct group *grp;
    struct tm *time;
    while(ix < *argcp){
      if(stat(argv[ix], &buf) == -1){
	ix++;
	extVal = 1;
	continue;
      }else{
	extVal = 0;
      }
      pwd = getpwuid(buf.st_uid);
      grp = getgrgid(buf.st_gid);
      time = localtime(&buf.st_mtime);
      if(pwd != NULL && grp != NULL){
	dprintf(outfd, "%s %s %s ", argv[ix], pwd->pw_name, grp->gr_name);
      }else if(pwd == NULL && grp != NULL){
	dprintf(outfd, "%s %d %s ", argv[ix], buf.st_uid, grp->gr_name);
      }else if(pwd != NULL && grp == NULL){
	dprintf(outfd, "%s %s %d ", argv[ix], pwd->pw_name, buf.st_gid);
      }else{
	dprintf(outfd, "%s %d %d ", argv[ix], buf.st_uid, buf.st_gid);
      }
	
      dprintf(outfd, (S_ISDIR(buf.st_mode)) ? "d" : "-");
      dprintf(outfd, (buf.st_mode & S_IRUSR) ? "r" : "-");
      dprintf(outfd, (buf.st_mode & S_IWUSR) ? "w" : "-");
      dprintf(outfd, (buf.st_mode & S_IXUSR) ? "x" : "-");
      dprintf(outfd, (buf.st_mode & S_IRGRP) ? "r" : "-");
      dprintf(outfd, (buf.st_mode & S_IWGRP) ? "w" : "-");
      dprintf(outfd, (buf.st_mode & S_IXGRP) ? "x" : "-");
      dprintf(outfd, (buf.st_mode & S_IROTH) ? "r" : "-");
      dprintf(outfd, (buf.st_mode & S_IWOTH) ? "w" : "-");
      dprintf(outfd, (buf.st_mode & S_IXOTH) ? "x" : "-");
      dprintf(outfd, " %lu %ld ", buf.st_nlink, buf.st_size);
      dprintf(outfd, "%s", (asctime(time)));
	      
      ix++;
    }
    *argcp = 0;
  }

  /*if user enters read and a variable name, get a line of input
   *and set the environment variable to the line of input
   *else print usage message
   */
  if(strcmp(argv[0], "read") == 0){
    if(*argcp == 2){
      char buffer[100];
      read(infd, buffer, 100);
      int len = strlen(buffer);
      if(buffer[len-1] == '\n'){
	buffer[len-1] = 0;
      }
      setenv(argv[1], buffer, len);
      extVal = 0;
    }else{
      dprintf(errfd, "Usage: read variable-name\n");
      extVal = 1;
    }
    *argcp = 0;
  }
}
