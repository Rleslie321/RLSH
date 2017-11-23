/*   $Id: proto.h,v 1.6 2017/06/01 22:37:44 leslier4 Exp $   */

/* Proto.h
 *
 * Robert Leslie
 * 6/1/2017
 * CSCI 352
 * Assignment 6
 *
 */

char** arg_parse(char *line, int *argcp);
void builtin(char **argv, int *argcp, int infd, int outfd, int errfd);
int expand(char *orig, char *new, int newsize);
