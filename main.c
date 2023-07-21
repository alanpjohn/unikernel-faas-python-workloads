#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <Python.h>

#define LISTEN_PORT 8123

#define BUFLEN 104857600
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

	Py_Initialize();

	PyObject *module = PyImport_ImportModule("app.wrapper");
	if(!module) {
        // The import failed.
        // Double check spelling in import call and confirm
        // you can import platform in Python on system.
        printf("\nCould not import wrapper module.\n");
        goto out;
    }

	PyObject *wrapper_function = PyObject_GetAttrString(module,"wrapper");
	if(!wrapper_function || !PyCallable_Check(wrapper_function)){
		printf("\nCould not import handler function from module\n");
        goto out;
	}

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


		PyObject *args = Py_BuildValue("(s)",recvbuf);

		PyObject *output = PyObject_CallObject(wrapper_function,args);
		Py_XDECREF(args);
		PyObject* str = PyUnicode_AsEncodedString(output, "utf-8", "~E~");
        const char * system_str = PyBytes_AS_STRING(str);
        // printf("Function output:\n%s\n\n", system_str);

		clock_gettime(CLOCK_MONOTONIC, &end);

		printf("Function execution: %.5f\n", ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec));

		/* Send reply */
		n = write(client, system_str, strlen(system_str));
		if (n < 0)
			fprintf(stderr, "Failed to send a reply\n");
		else
			printf("Sent a reply\n");

		/* Close connection */
		close(client);
	}
	Py_XDECREF(wrapper_function);
	Py_Finalize();

out:
	close(srv);
	return rc;
}