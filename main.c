#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <Python.h>

#define LISTEN_PORT 8123
static const char reply[] = "HTTP/1.1 200 OK\r\n" \
			    "Content-type: text/html\r\n" \
			    "Connection: close\r\n" \
			    "\r\n" \
			    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" \
			    "<html>" \
			    "<head><title>It works!</title></head>" \
			    "<body><h1>It works!</h1><p>This is only a test.</p></body>" \
			    "</html>\n";

#define BUFLEN 2048
static char recvbuf[BUFLEN];

int main(int argc, char *argv[])
{
	int rc = 0;
	int srv, client;
	ssize_t n;
	struct sockaddr_in srv_addr;

	struct timespec start={0,0};
	struct timespec end={0,0};
    
	srv = socket(AF_INET, SOCK_STREAM, 0);
	if (srv < 0) {
		fprintf(stderr, "Failed to create socket");
		goto out;
	}

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = INADDR_ANY;
	srv_addr.sin_port = htons(LISTEN_PORT);

	rc = bind(srv, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
	if (rc < 0) {
		fprintf(stderr, "Failed to bind socket");
		goto out;
	}

	/* Accept one simultaneous connection */
	rc = listen(srv, 1);
	if (rc < 0) {
		fprintf(stderr, "Failed to listen on socket");
		goto out;
	}

	char filename[] = "wrapper.py";
	FILE* fp;

	Py_Initialize();

	printf("Listening on port %d...\n", LISTEN_PORT);
	while (1) {
		client = accept(srv, NULL, 0);
		if (client < 0) {
			fprintf(stderr,
				"Failed to accept incoming connection");
			goto out;
		}

		/* Receive some bytes (ignore errors) */
		read(client, recvbuf, BUFLEN);

		clock_gettime(CLOCK_MONOTONIC, &start);

		fp = _Py_fopen(filename, "r");
		PyRun_SimpleFile(fp, filename);

		clock_gettime(CLOCK_MONOTONIC, &end);

		printf("Function execution: %.5f\n", ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec));

		/* Send reply */
		n = write(client, reply, sizeof(reply));
		if (n < 0)
			fprintf(stderr, "Failed to send a reply\n");
		else
			printf("Sent a reply\n");

		/* Close connection */
		close(client);
	}
	Py_Finalize();

out:
	return rc;
}