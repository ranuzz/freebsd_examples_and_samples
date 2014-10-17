#include <stdio.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <sys/un.h>
#include <stdlib.h>
#include <errno.h>


#define MAX_KEVENT (10)
int kevent_queue;
struct kevent *active_evlist;
struct kevent *add_evlist;
int evlist_curlen;
char buff[1024];
int bufflen = 1024;

main ()
{

	/* intialize socket */
	struct sockaddr_un s_addr;
	int len, flag, server_sock_fd, client_sock_fd;

	if ((server_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Socket creation failed : %s \n", strerror(errno));
		return;
	}

	bzero(&s_addr, sizeof(struct sockaddr_un));
	s_addr.sun_family = AF_UNIX;
	strcpy(s_addr.sun_path, "/var/untest.sock");
	unlink(s_addr.sun_path);
	len = strlen(s_addr.sun_path) + strlen((char *)(&s_addr.sun_family));

	if (bind(server_sock_fd, (struct sockaddr *)&s_addr, len) < 0) {
		fprintf(stderr, "bind failed : %s \n", strerror(errno));
		return;
	}

	if (listen(server_sock_fd, MAX_KEVENT) < 0) {
		fprintf(stderr, "listen failed : %s \n", strerror(errno));
		return;
	}

	/* initialize kqueue */
	if ((kevent_queue = kqueue()) < 0) {
		fprintf(stderr, "kqueue failed : %s \n", strerror(errno));
		return;
	}

	active_evlist = (struct kevent *) calloc(MAX_KEVENT, sizeof(struct kevent));
	add_evlist = (struct kevent *) calloc(MAX_KEVENT, sizeof(struct kevent));

	if (!active_evlist || !add_evlist) {
		fprintf(stderr, "event list allocation failed\n");
	}

	/* setting event for reading data on listening socket */
	EV_SET(&add_evlist[evlist_curlen++], server_sock_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		    
	struct kevent *kev;
	int inx = 0, num_events;
	struct timespec tm = {0, 1000000};

	/* main LOOP */
	while(1) {

		/* registering/adding kevents (if any) and getting active event list */
		num_events = kevent(kevent_queue, add_evlist, evlist_curlen, active_evlist, MAX_KEVENT, &tm);
		if (num_events < 0) {
			fprintf(stderr, "kevent dailed : %s\n", strerror(errno));
			continue;
		}

		/* rewind add_evlist to avoid multiple insertion in next iteration */
		evlist_curlen = 0;

		for(inx = 0; inx < num_events; inx++) {

			/* event handling */

			kev = &active_evlist[inx];

			if (kev->flags & EV_ERROR) {
				fprintf(stderr , "kevent error Filter [%u] Ident [%u] : %s \n", kev->filter, kev->ident, strerror(kev->data));
				close(kev->ident);
				continue;
			}

			switch (kev->filter) {
				case EVFILT_READ:
				{
					if (kev->flags & EV_EOF) {
						if (kev->ident != server_sock_fd) close(kev->ident);
					}

					if (kev->ident == server_sock_fd) {
						/* accepting new connection */
						if ((client_sock_fd = accept(server_sock_fd, NULL, NULL)) < 0) {
							fprintf(stderr, "Accept failed %s\n", strerror(errno));
							break;
						}

						if (evlist_curlen == MAX_KEVENT) {
							evlist_curlen = 1;
						}
						fprintf(stdout, "recieved new connection request FD : %d\n", client_sock_fd);

						/* registering ONESHOT event for reading incoming data */
					    EV_SET(&add_evlist[evlist_curlen++], client_sock_fd, EVFILT_READ,
								(EV_ADD|EV_ONESHOT), 0, 0, NULL);

					} else {
						int r_bytes, w_bytes;
						if ((r_bytes = read(kev->ident, &buff, bufflen)) < 0) {
							fprintf(stderr, "read failed : %s \n", strerror(errno));
							break;
						}
						fprintf(stdout, "FD %d : read %s\n", kev->ident, buff);
						if ((w_bytes = write(kev->ident, &buff, r_bytes)) < 0) {
							fprintf(stderr, "write failed : %s \n", strerror(errno));
							break;
						}
						fprintf(stdout, "FD %d : wrote %s\n", kev->ident, buff);

						close(kev->ident);
					}
				}
				break;

				default:
				break;
			}
		}
	}
}
