/*   $Id: expand.c,v 1.30 2017/06/01 22:37:44 leslier4 Exp $   */

/*  expand.c  
*
*   Robert Leslie
*   6/1/2017
*   CSCI 352
*   Assignment 6
*/


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include "proto.h"

/*global variables*/
extern int glArgc;
extern char ** glArgv;
extern int shift;
extern int extVal;
extern int DONTWAIT;
extern int EXPAND;

/*use to call other sources functions*/
extern int processline(char *line, int infd, int outfd, int flags);
extern void waiter();

/* prototypes */
static int expandBrac(char *orig, char *new, int newsize, int *ix_O, int *ix_N);
static void expand$$(char *orig, char *new, int newsize, int *ix_O, int *ix_N);
static void expand$dig(char *orig, char *new, int newsize, int *ix_O, int *ix_N);
static void expand$pnd(char *orig, char *new, int newsize, int *ix_O, int *ix_N);
static void expand$Q(char *orig, char *new, int newsize, int *ix_O, int *ix_N);
static int expandAst(char *orig, char *new, int newsize, int *ix_O, int *ix_N);
static int commandOutputExpand(char *orig, char *new, int newsize, int *ix_O, int *ix_N);

/* check to see if any characters need to be changed, call proper function if necessary*/
int expand(char *orig, char *new, int newsize){
  /*find the length of string and set indices for each string, success for proper syntax*/
  int str_len = strlen(orig);
  int ix_O = 0;
  int ix_N = 0;
  int success;

  /*transfer chars to new string, call helper if something needs to be overwritten*/
  while(ix_O < str_len && ix_N < newsize){
    if(orig[ix_O] == '$'){
      if(orig[ix_O+1] == '$'){
	expand$$(orig, new, newsize, &ix_O, &ix_N);
      }
      else if(orig[ix_O+1] == '{'){
	ix_O = ix_O + 2;
	success = expandBrac(orig, new, newsize, &ix_O, &ix_N);
	if(success < 0){
	  return -1;
	}
      }
      else if(isdigit((int) orig[ix_O+1])){
	expand$dig(orig, new, newsize, &ix_O, &ix_N);
      }
      else if(orig[ix_O+1] == '#'){
	expand$pnd(orig, new, newsize, &ix_O, &ix_N);
      }
      else if(orig[ix_O+1] == '?'){
	expand$Q(orig, new, newsize, &ix_O, &ix_N);
      }
      else if(orig[ix_O+1] == '('){
	success = commandOutputExpand(orig, new, newsize, &ix_O, &ix_N);
	if(success < 0){
	  return -1;
	}
      }else{
	new[ix_N] = orig[ix_O];
	ix_N++;
      }
    }
    else if(orig[ix_O] == '*'){
      success = expandAst(orig, new, newsize, &ix_O, &ix_N);
      if(success < 0){
	return -1;
      }
    }
    else if(orig[ix_O] == '\'' && orig[ix_O+1] == '\\' && orig[ix_O+2] == '*' && orig[ix_O+3] == '\''){
      new[ix_N] = '*';
      ix_N++;
      ix_O = ix_O + 3;
    }else if(orig[ix_O] == '\\' && orig[ix_O+1] == '*'){
      new[ix_N] = '*';
      ix_N++;
      ix_O++;
    }else{
      new[ix_N] = orig[ix_O];
      ix_N++;
    }
    ix_O++;
  }
  new[ix_N] = '\0';
  return 1;
}

/*helper function to replace ${...}, if proper input it returns 1, else -1*/
int expandBrac(char *orig, char *new, int newsize, int *ix_O, int *ix_N){
  /* string to contain the environment var, index, and string to contain value for overwriting*/
  char name[30];
  int ix = 0;
  char *value;

  /* transfer env var to string name, exit if odd num of braces*/
  while(orig[*ix_O] != '}'){
    if(orig[*ix_O] == '\0'){
        fprintf(stderr, "odd number of braces\n");
	return -1;
    }else{
      name[ix] = orig[*ix_O];
    }
    *ix_O = *ix_O + 1;
    ix++;
    if(ix == 30){
      return -1;
    }
  }
  /*null terminate the string name, and retrieve value using name*/
  name[ix] = '\0';
  ix = 0;
  value = getenv(name);

  /*if valuse is null set add null char*/
  if(value != NULL){
    int val_len = strlen(value);
    /* while less than newsize, add value to new string*/
    while(*ix_N < newsize && value[ix] != '\n' && value[ix] != '\0' && ix < val_len){
      new[*ix_N] = value[ix];
      ix++;
      *ix_N = *ix_N + 1;
    }
  }
  return 1;
}

/*helper function to expand $$ to ascii representation of pid*/
void expand$$(char *orig, char *new, int newsize, int *ix_O, int *ix_N){
  int pid = getpid();
  char pidChars[20];
  int numChars = snprintf(pidChars, 20, "%d", pid);
  int ix;
  for(ix = 0; ix < numChars; ix++){
    new[*ix_N] = pidChars[ix];
    *ix_N = *ix_N + 1;
  }
  
}

/*helper function to expand $n to argv[n]*/
void expand$dig(char *orig, char *new, int newsize, int *ix_O, int *ix_N){
  *ix_O = *ix_O + 1;
  char digit[20];
  int ix = 0;
  
  /* get the full number and convert to integer*/
  while(isdigit(orig[*ix_O])){
    digit[ix] = orig[*ix_O];
    *ix_O = *ix_O + 1;
    ix++;
  }
  *ix_O = *ix_O - 1;
  digit[ix] = '\0';
  int n = atoi(digit);
  
  char *value;
  ix = 0;

  /*decide what value needs to be entered based on what was given*/
  if(n == 0 && glArgc == 1){
    value = glArgv[0];
  }
  else if(n == 0){
    value = glArgv[1];
  }
  else if(n > glArgc){
    //*ix_N = *ix_N + 2;
    return;
  }else{
    if((n+shift) <= (glArgc-1)){  //check to make sure the shift is still within the args
      value = glArgv[n+shift];
    }else{
      return;
    }
  }

  /*add value to new line*/
  while(value[ix] != '\0' && *ix_N < newsize){
    new[*ix_N] = value[ix];
    ix++;
    *ix_N = *ix_N + 1;
  }
}

/*helper function to expand $# to number of arguments entered in ascii*/
void expand$pnd(char *orig, char *new, int newsize, int *ix_O, int *ix_N){
  *ix_O = *ix_O + 1;
  char value[20];
  int numChars;
  if(glArgc == 1){
    numChars = snprintf(value, 20, "%d", glArgc);
  }else{
    numChars = snprintf(value, 20, "%d", glArgc-shift);
  }
  int ix;
  
  for(ix = 0; ix < numChars; ix++){
    new[*ix_N] = value[ix];
    *ix_N = *ix_N + 1;
  }
}

/*helper function to expand $? into the most recent exit value given*/
void expand$Q(char *orig, char *new, int newsize, int *ix_O, int *ix_N){
  *ix_O = *ix_O + 1;
  char value[20];
  int numChars = snprintf(value, 20, "%d", extVal);
  int ix;
  
  for(ix = 0; ix < numChars; ix++){
    new[*ix_N] = value[ix];
    *ix_N = *ix_N + 1;
  }

}

/*helper function to expand * to be any file that matches user input in current dir*/
int expandAst(char *orig, char *new, int newsize, int *ix_O, int *ix_N){
  /*if there isn't a space before, just add the * char*/
  /*else open the directory and decide if all files or specific files should be added*/
  if(orig[*ix_O-1] != ' ' && orig[*ix_O-1] != '"'){
    new[*ix_N] = orig[*ix_O];
    *ix_N = *ix_N + 1;
  }else{
    DIR *dir;
    struct dirent *dp;
    char *file;
    dir = opendir(".");
    int ix = 0;
    int addspc = 0;
    /*add all files except those beginning with .*/
    if(orig[*ix_O+1] == ' ' || orig[*ix_O+1] == '"' || orig[*ix_O+1] == '\0'){
      while((dp = readdir(dir)) != NULL){
	if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")){
	}else{
	  if(addspc == 0){
	    addspc++;
	  }else{
	    new[*ix_N] = ' ';
	    *ix_N = *ix_N + 1;
	  }
	  file = dp->d_name;
	  while(file[ix] != '\0' && *ix_N < newsize){
	    new[*ix_N] = file[ix];
	    ix++;
	    *ix_N = *ix_N + 1;
	  }
	  ix = 0;
	}
      }
    }else{
      int count = 0;
      char check[20];
      *ix_O = *ix_O + 1;
      int added = 0;
      int fileLen;
      /*transfer chars that need to be checked, count how many are checked*/
      while(orig[*ix_O] != ' ' && orig[*ix_O] != '\0' && orig[*ix_O] != '/' && orig[*ix_O] != '"'){
	check[ix] = orig[*ix_O];
	ix++;
	*ix_O = *ix_O + 1;
	count++;
      }
      if(orig[*ix_O] == '/'){
	return -1;
      }
      *ix_O = *ix_O - 1;
      check[ix] = '\0';
      ix = 0;

      /*add files only if they have the same ending as what was given*/
      while((dp = readdir(dir)) != NULL){
	file = dp->d_name;
        fileLen = strlen(file);
	if(fileLen - count < 0){
	  continue;
	}
	if(strcmp(&check[ix], &file[fileLen-count]) == 0){
	  if(addspc == 0){
	    addspc++;
	  }else{
	    new[*ix_N] = ' ';
	    *ix_N = *ix_N + 1;
	  }
	  while(file[ix] != '\0' && *ix_N < newsize){
	    new[*ix_N] = file[ix];
	    ix++;
	    *ix_N = *ix_N + 1;
	  }
	  ix = 0;
	  added = 1;
	}
      }
      /*if none were added then add the pattern*/
      if(added == 0){
	new[*ix_N] = '*';
	*ix_N = *ix_N + 1;
	while(ix < count && *ix_N < newsize){
	  new[*ix_N] = check[ix];
	  ix++;
	  *ix_N = *ix_N + 1;
	}
	ix = 0;
      }
    }
    closedir(dir);
  }
  return 1;

}

//helper function to provide output expansion
int commandOutputExpand(char *orig, char *new, int newsize, int *ix_O, int *ix_N){
  *ix_O = *ix_O + 2;
  int qte_cnt = 1;
  int qte_itr = *ix_O;

  //find the end paren to be able to send string to processline
  while(orig[qte_itr] != '\0'){
    if(orig[qte_itr] == '('){
      qte_cnt++;
    }
    else if(orig[qte_itr] == ')'){
      qte_cnt--;
      if(qte_cnt == 0){
	break;
      }
    }
    qte_itr++;
  }
  if(qte_cnt != 0){
    fprintf(stderr, "odd number of parens\n");
    return -1;
  }

  //change the end paren to null, initialize pipes and call processline
  orig[qte_itr] = '\0';

  int fd[2];
  char c[1];
  pipe(fd);
  int x = processline(&orig[*ix_O], 0, fd[1], (DONTWAIT | EXPAND));
  close(fd[1]);
  if(x == -1){
    close(fd[0]);
    return -1;
  }
  
  //read loop to get all characters or until overflow
  while(*ix_N < newsize && read(fd[0], c, 1) > 0){
    new[*ix_N] = c[0];
    *ix_N = *ix_N + 1;
  }
  //handle any characters left after overflow
  if(*ix_N == newsize){
    new[(*ix_N)-1] = '\0';
    fprintf(stderr, "Too many characters entered\n");
    while(read(fd[0], c, 1) > 0 ){}
    close(fd[0]);
    return -1;
  }
  while(read(fd[0], c, 1) > 0 ){}
  close(fd[0]);

  //post processing, remove newline characters
  orig[qte_itr] = ')';
  *ix_O = qte_itr;
  int ix;
  for(ix = 0; ix < *ix_N; ix++){
    if(new[ix] == '\n' && ix+1 < *ix_N){
      new[ix] = ' ';
    }
    else if(new[ix] == '\n' && *ix_N != newsize){
      *ix_N = *ix_N - 1;
    }
  }
  //wait on child if processline forked
  if(x > 0){
    waiter();
  }
  return 1;
}
