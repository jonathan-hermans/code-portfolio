
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "lib.h"

/*
 *  Read count bytes from fd if at all possible.
 *  The only potential problem is an end of file
 *  which suggests a problem with the server.
 */
int readn(int fd, char *buffer, int count) {
	char *ptr;
	int n;
	int left;

	ptr = buffer;
	left = count;
	while(left > 0) {
		n = read(fd, ptr, left);
		if(n == 0)
			break;
		if(n < 0) {
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				n = 0;
			} else {
				return(-1);
			}
		}
		left -= n;
		ptr += n;
	}
	return(count-left);
}


/*
 *  Write count bytes on fd if at all possible.
 *  The only potential problem is an error that
 *  can't be recovered from.
 */
int writen(int fd, char *buffer, int count) {
	char *ptr;
	int n;
	int left;

	ptr = buffer;
	left = count;
	while(left > 0) {
		n = write(fd, ptr, left);
		if(n < 0) {
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				n = 0;
			} else {
				return(-1);
			}
		}
		left -= n;
		ptr += n;
	}
	return(count-left);
}

 /*
  *  Reliably read a text string from fd.
  *  Returns NULL if it can't read the string
  *  otherwise a pointer to the string.
  */
char *readString(int fd) {
	short len;
	char *buffer;
	int ret;

	ret = readn(fd, (char*)&len, sizeof(len));
	if(ret <= 0)
		return(NULL);
	len = ntohs(len);
	buffer = (char*) malloc(len);
	ret = readn(fd, buffer, len);
	if(ret != len)
		return(NULL);
	else
		return(buffer);
}


/*
 *  Reliably write the string on fd.
 *  Returns -1 on failure, 0 on success.
 */
int writeString(int fd, char *string) {
	short len;
	int ret;
	short buffer;

	len = strlen(string)+1;
	buffer = htons(len);
	ret = writen(fd, (char*)&buffer, sizeof(len));
	if(ret != sizeof(len))
		return(-1);
	ret = writen(fd, string, len);
	if(ret != len)
		return(-1);
	else
		return(0);
}
