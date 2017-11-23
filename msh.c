/*   $Id: msh.c,v 1.36 2017/06/01 23:13:56 leslier4 Exp $   */

/* CS 352 -- Mini Shell!  
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001 
 *   Modified January 6, 2003
 *
 *   Robert Leslie
 *   6/1/2017
 *   CSCI 352
 *   Assignment 6
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proto.h"
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Constants */ 

#define LINELEN 1024

/* global arguments */
int glArgc;
char **glArgv;
int shift = 1;
int extVal = 0;
FILE *fp;
const int WAIT = 1;
const int DONTWAIT = 0;
const int DONTEXPAND = 0;
const int EXPAND = 2;

/* Prototypes */

int processline (char *line, int infd, int outfd, int flags);
int findunquote(char *buffer, char *line);
void waiter();
void signalHandler(int signum);

/* Shell main */

int
main (int argc, char **argv)
{
    char   buffer [LINELEN];
    int    len;
    glArgc = argc;
    glArgv = argv;

    signal(SIGINT, signalHandler);
    
    /*check to see if file was given, and try to open, else use stdin*/
    if(argc > 1){
      fp = fopen(argv[1], "r");
      if(fp == NULL){
	perror("Can't open file");
	exit(127);
      }
    }else{
      fp = stdin;
    }
    
    while (1) {

       /* prompt if interative, and get line */
      if(argc == 1 && getenv("P1") == NULL){
	fprintf (stderr, "%% ");
      }else if(argc == 1){
	fprintf(stderr, "%s ", getenv("P1"));
      }
	if (fgets (buffer, LINELEN, fp) != buffer)
	  break;

        /* Get rid of \n at end of buffer. */
	len = strlen(buffer);
	if (buffer[len-1] == '\n')
	    buffer[len-1] = 0;

	//remove comments from line
	findunquote(buffer, "#");
	
	/* Run it ... */
	processline (buffer, 0, 1, WAIT | EXPAND);

    }
    /*close file and exit*/
    if(argc > 1){
      fclose(fp);
      return 0;
    }
    
    if (!feof(stdin))
        perror ("read");

    return 0;		/* Also known as exit (0); */
}


int processline (char *line, int infd, int outfd, int flags)
{
    pid_t  cpid;
    
    /*create new line to call expand, determine if expand was succcessful*/
    int newsize = 200021;
    char nline[newsize];
    if(flags & EXPAND){
      int success = expand(line, nline, newsize);
      if(success < 0){
	extVal = 127;
	return -1;
      }
    }else
      strcpy(nline, line);
    
    //Implement Pipelining, create a 2d array to store two pipes
    //once you find a pipe character, set it to end of string and call processline
    //determine proper fd to use based on how many pipe characters there were
    int ix = 0;
    int oldix = 0;
    if((ix = findunquote(nline, "|")) >= 0){
      oldix = ix;
      int fds[2][2];
      int i = 0;
      pipe(fds[i]);
      nline[ix] = '\0';
      processline(nline, infd, fds[i][1], DONTEXPAND | DONTWAIT);
      close(fds[i][1]);
      while((ix = findunquote(nline, "|")) >= 0){
	i = (i+1) % 2;
	pipe(fds[i]);
	nline[ix] = '\0';
	processline(&nline[oldix+1], fds[(i+1)%2][0], fds[i][1], DONTEXPAND | DONTWAIT);
	close(fds[(i+1)%2][0]);
	close(fds[i][1]);
	oldix = ix;
      }
      processline(&nline[oldix+1], fds[i][0], outfd, DONTEXPAND | (WAIT & flags));
      close(fds[i][0]);
      return 1;
    }

    //Implement redirection
    int cinfd = infd;
    int coutfd = outfd;
    int cerrfd = 2;
    ix = 0;
    char filename[255];
    int fdFlags;
    int openFlags;
    while((ix = findunquote(nline, "<>")) >= 0){
      //determine the type of redirection and set flags
      if(nline[ix] == '<'){
	nline[ix] = ' ';
	ix++;
	fdFlags = 0;
	openFlags = O_RDONLY; 
      }else{
	if((ix == 1 && nline[0] == '2') || (ix > 1 && nline[ix-1] == '2' && nline[ix-2] == ' ')){
	  nline[ix-1] = ' ';
	  fdFlags = 2;
	  if((ix+1) < strlen(nline) && nline[ix+1] == '>'){
	    openFlags = O_CREAT | O_APPEND | O_RDWR;
	  }else{
	    openFlags = O_CREAT | O_TRUNC | O_RDWR;
	  }
	  nline[ix] = ' ';
	  ix++;
	}else{
	  fdFlags = 1;
	  if((ix+1) < strlen(nline) && nline[ix+1] == '>'){
	    openFlags = O_CREAT | O_APPEND | O_RDWR;
	  }else{
	    openFlags = O_CREAT | O_TRUNC | O_RDWR;
	  }
	  nline[ix] = ' ';
	  ix++;
	}
      }
      //determine the filename
      if(nline[ix] == '>'){
	nline[ix] = ' ';
	ix++;
      }
      if(nline[ix] == ' '){
	ix++;
      }
      int nameIx = 0;
      while(nline[ix] != ' ' && nline[ix] != '<' && nline[ix] != '>' && nline[ix] != '\0'){
	if(nline[ix] == '\"'){
	  nline[ix] = ' ';
	  ix++;
	  while(nline[ix] != '\"' && nline[ix] != '\0'){
	    filename[nameIx] = nline[ix];
	    nline[ix] = ' ';
	    ix++;
	    nameIx++;
	  }
	  if(nline[ix] != '\0'){
	    nline[ix] = ' ';
	    ix++;
	  }
	}else{
	  filename[nameIx] = nline[ix];
	  nline[ix] = ' ';
	  ix++;
	  nameIx++;
	}
      }
      filename[nameIx] = '\0';
      //close fds if necessary
      if(cinfd != infd && fdFlags == 0){
	close(cinfd);
      }else if(coutfd != outfd && fdFlags == 1){
	close(coutfd);
      }else if(cerrfd != 2 && fdFlags == 2){
	close(cerrfd);
      }
      //open file and set proper cfd
      int fd = open(filename, openFlags, 0666);
      if(fdFlags == 0){
	cinfd = fd;
      }else if(fdFlags == 1){
	coutfd = fd;
      }else{
	cerrfd = fd;
      }
    }
    
    int argcp = 0;
    char** argv = arg_parse(nline, &argcp);
    /*if argcount is zero dont do any more work*/
    if(argcp == 0){
      return 0;
    }

    /*check to see if arg is a builtin, if so return for next input*/
    builtin(argv, &argcp, cinfd, coutfd, cerrfd);
    if(argcp == 0){
      if(cinfd != infd)
	close(cinfd);
      if(coutfd != outfd)
	close(coutfd);
      if(cerrfd != 2)
	close(cerrfd);
      memset(nline, 0, newsize);
      argv = NULL;
      free(argv);
      return 0;
    }
    
    /* Start a new process to do the job. */
    cpid = fork();
    if (cpid < 0) {
      perror ("fork");
      memset(nline, 0, newsize);
      argv = NULL;
      free(argv);
      extVal = 127;
      return -1;
    }
    
    /* Check for who we are! */
    if (cpid == 0) {
      /* We are the child! */
      if(cinfd != 0){
	dup2(cinfd, 0);
      }
      if(coutfd != 1){
	dup2(coutfd, 1);
      }
      if(cerrfd != 2){
	dup2(cerrfd, 2);
      }
      execvp (argv[0], argv);
      extVal = 127;
      perror ("exec");
      if(glArgc > 1){
	fclose(fp);
      }
      exit (127);
    }
    memset(nline, 0, newsize);
    argv = NULL;
    free(argv);
    if(flags & WAIT){
      waiter();
    }
    if(cinfd != infd)
      close(cinfd);
    if(coutfd != outfd)
      close(coutfd);
    if(cerrfd != 2)
      close(cerrfd);


    return cpid;

}

//makes anything past # unreachable
//as long as it isnt preceeded by a $ or surrounded by quotes
//modified commentRemove to findunquote
//now returns index of unquoted character found in line,
//it still handles comment removal in the function
int findunquote(char *buffer, char *line){
  int ix = 0;
  int i = 0;
  int len = strlen(line);
  while(i < len){
    while(buffer[ix] != line[i] && buffer[ix] != '\0'){
      if(buffer[ix] == '\"'){
	ix++;
	while(buffer[ix] != '\"' && buffer[ix] != '\0'){
	  ix++;
	}
	if(buffer[ix] == '\"')
	  ix++;
      }else
	ix++;
    }
    if(buffer[ix] == '#'){
      if(buffer[ix-1] != '$' && (buffer[ix-1] != '\"' && buffer[ix+1] != '\"')){
	buffer[ix] = '\0';
	return 0;
      }else{
	ix++;
	continue;
      }
    }
    else if(buffer[ix] == line[i]){
      return ix;
    }else{
      i++;
      ix = 0;
      continue;
    }
  }
  return -1;
}

//helper function to wait on child process and get exit status
void waiter(){
  int status;
  pid_t pid;
  /* Have the parent wait for child to complete */

  while ((pid = waitpid(-1, &status, WNOHANG)) >= 0)
  
  /*check to see how the child exited*/
  if(WIFEXITED(status)){
    extVal = WEXITSTATUS(status);
  }else
    extVal = 127;
}

//signal handler to catch signals
//just ignores signals right now
void signalHandler(int signum){
}
