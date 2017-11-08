#define CLIENT_NUMBERS 30

typedef struct client_t{
	int  id;
	char name;

	char ip[15];
	int  port;

	int  fd;//socket fd

	char path;
}client_t;
