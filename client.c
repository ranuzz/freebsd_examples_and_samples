#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>


char buff[1024];
int bufflen = 1024;

main ()
{
	int sock, len;
	struct sockaddr_un s_addr;
	char *msg = "## FreeBSD ##";

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "socket failed : %s\n", strerror(errno));
		return;
	}

	bzero(&s_addr, sizeof(s_addr));
	s_addr.sun_family = AF_UNIX;
	strcpy(s_addr.sun_path, "/var/untest.sock");
	len = strlen(s_addr.sun_path) + strlen((char *)(&s_addr.sun_family));

	if (connect(sock, (struct sockaddr *)&s_addr, len) == -1) {
		fprintf(stderr, "connect failed : %s\n", strerror(errno));
		return;
	}

	fprintf(stdout, "connection successful FD %d \n", sock);

	while(1) {
		sprintf(buff, "%s", msg);
		int msglen = strlen(buff), w_bytes, r_bytes;
		w_bytes = write(sock, buff, msglen);
		if (w_bytes < 0) {
			fprintf(stderr, "write failed : %s\n", strerror(errno));
			close(sock);
			return;
		}
		fprintf(stdout, "FD %d : Wrote [%s]\n", sock, buff);
		
		r_bytes = read(sock, buff, bufflen);
		if (r_bytes < 0) {
			fprintf(stderr, "write failed : %s\n", strerror(errno));
			close(sock);
		}

		fprintf(stdout, "FD %d : read [%s] \n", sock, buff);
	}
}
