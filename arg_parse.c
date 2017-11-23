/*   $Id: arg_parse.c,v 1.3 2017/05/17 18:20:32 leslier4 Exp $   */

/*  Arg_Parse.c
 *
 *  Robert Leslie
 *  4/22/2017
 *  CSCI 352
 *  Assignment 3
 *
 */

#include "proto.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

char** arg_parse(char *line, int *argcp){
  /*initialize count and indices*/
  int count = 0;
  int ix = 0;
  int ixp = 0;
  /*initialize quote_count and string_length*/
  int qte_cnt = 0;
  int str_len = 0;
  
  /*count how many args in the line*/
  while(1){
    while(line[ix] == ' '){
      ix++;
    }
    if(line[ix] == '\0'){
      break;
    }
    
    /*check to see if there are any quotes and count them*/
    if(line[ix] == '\"'){
      count++;
      qte_cnt++;
      ix++;
      while(1){
	while(line[ix] != '\"'){
	  if(line[ix] == '\0'){
	    break;
	  }
	  ix++;
	}
	if(line[ix] == '\0'){
	  break;
	}
	qte_cnt++;
	ix++;
	while(line[ix] != ' '){
	  if(line[ix] == '\"'){
	    qte_cnt++;
	    break;
	  }
	  if(line[ix] == '\0'){
	    break;
	  }
	  ix++;
	}
	if(line[ix] == ' '){
	  break;
	}
	if(line[ix] == '\0'){
	  break;
	}
	ix++;
      }
    }
    else{
      count++;
      while((line[ix] != ' ') && (line[ix] != '\0')){
	/*count number of quotes and get to the end of quotes*/
	if(line[ix] == '\"'){
	  qte_cnt++;
	  ix++;
	  while(line[ix] != '\"'){
	    if(line[ix] == '\0'){
	      break;
	    }
	    ix++;
	  }
	  if(line[ix] == '\0'){
	    break;
	  }
	  qte_cnt++;
	}
	ix++;
      }
    }
  }
  /*if odd number of quotes, return to ask user for more input*/
  if((qte_cnt % 2) != 0){
    fprintf(stderr, "odd number of quotes %d\n", qte_cnt);
    return NULL;
  }
  
  /*set argcp to be count and set str_len*/
  *argcp = count;
  str_len = strlen(line);

  /*if there are no arguments don't do any more work*/
  if(count == 0){
    return NULL;
  }
  
  /*create array of pointers by calling malloc*/
  count++;
  char **ptr = (char**)malloc(sizeof(char*)*count);
  if(ptr == NULL){
    return NULL;
  }
  
  /*create pointers, add them to array of pointers and add EOS char*/
  ix = 0;
  while(1){
    while(line[ix] == ' '){
      ix++;
    }
    if(line[ix] == '\0'){
      break;
    }
    /*if we hit a quote, remove them and find the matching quote, then finish current arg*/
    if(line[ix] == '\"'){
      ptr[ixp] = &line[ix];
      ixp++;
      while(1){
	memmove(&line[ix], &line[ix+1], str_len - ix);
	while(line[ix] != '\"'){
	  if(line[ix] == '\0'){
	    break;
	  }
	  ix++;
	}
	if(line[ix] == '\0'){
	  break;
	}
	if(line[ix] == '\"'){
	  memmove(&line[ix], &line[ix+1], str_len - ix);
	}
	while(line[ix] != ' ' && line[ix] != '\0'){
	  if(line[ix] == '\"'){
	    break;
	  }
	  ix++;
	}
	if(line[ix] == '\0'){
	  break;
	}
	if(line[ix] == ' '){
	  line[ix] = 0;
	  ix++;
	  break;
	}
      }
    }
    else{
      ptr[ixp] = &line[ix];
      ixp++;
      while(line[ix] != ' ' && line[ix] != '\0'){
	/*if quote is found remove them, then continue to end of arg*/
	if(line[ix] == '\"'){
	  memmove(&line[ix], &line[ix+1], str_len - ix);
	  while(line[ix] != '\"'){
	    if(line[ix] == '\0'){
	      break;
	    }
	    ix++;
	  }
	  if(line[ix] == '\"'){
	    memmove(&line[ix], &line[ix+1], str_len - ix);
	    continue;
	  }
	}
	ix++;
      }
      if(line[ix] == '\0'){
	break;
      }
      line[ix] = 0;
      ix++;
    }
  }
  ptr[ixp] = NULL;

  /*return array of pointers*/
  return ptr;
}

  
  
