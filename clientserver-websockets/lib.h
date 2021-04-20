/*************************************
 *
 *             lib.h
 *
 *  Declarations of the procedures in our
 *  library of useful procedures for
 *  reliable network programs.
 *************************************/

int readn(int fd, char *buffer, int count);
int writen(int fd, char *buffer, int count);
char *readString(int fd);
int writeString(int fd, char *string);
