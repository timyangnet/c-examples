/*
You need to have Libev installed:
http://software.schmorp.de/pkg/libev.html
*/


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h> 
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <stddef.h>
#include <ev.h>
#include <leveldb/c.h>

#define SERVER_PORT 8002
struct client {
  int fd;
  ev_io ev_write;
  ev_io ev_read;
  int cmd;
  void *data;
};

ev_io ev_accept;

int
setnonblock(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
            return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) 
            return -1;

    return 0;
}


static char param[255];
char* get_param(char* req)
{
        char* lp = 0;
        if (*req == 0 || req[0] != 'G')
                return lp;
        int i, start_pos = 4;
        int has_param = 0;
        for (i = start_pos; i < 100; i++) {
                if (req[i] == ' ')
                        break;
                else if (req[i] == '?') {
                        has_param = 1;
                        start_pos = i;
                }
        
        }
        if (i == 4) return lp;
        
        strncpy(param, req + start_pos + 1, i - start_pos - 1);
        return param;
}

char* readdb(char*);
void writedb(char*,char*);
static void write_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{ 
	struct client *cli= ((struct client*) (((char*)w) - offsetof(struct client,ev_write)));
	static char buf[512];
	char *p = "set";

	if (cli->cmd == 0) {
		char* read = readdb((char*)cli->data);
		p = read;
		if (p == 0) p = "(null)";
	} else {
		writedb((char*)cli->data, (char*)cli->data);
	}

	sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\
Connection: close\r\nContent-Type: text/html\r\nDate: Sat, 26 Apr 2008 01:13:35 GMT\r\n\
Server: leveldb-http-server/0.1\r\n\r\n\
Hello %s", 6 + strlen(p), p);

 	if (revents & EV_WRITE){
		write(cli->fd,buf,strlen(buf));
		ev_io_stop(EV_A_ w);
	}
 	close(cli->fd);
	free(cli);
	
}

static void read_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{ 
	struct client *cli= ((struct client*) (((char*)w) - offsetof(struct client,ev_read)));
	int r=0;
	char rbuff[1024];
	if (revents & EV_READ) {
		r=read(cli->fd,&rbuff,1024);
	}
	int is_set = 0;
	if (rbuff[5] == 's' || rbuff[5] == 'S')
		is_set = 1;
	char* p_value = get_param(rbuff);
	cli->data = p_value;
	cli->cmd = is_set;
	ev_io_stop(EV_A_ w);
        ev_io_init(&cli->ev_write, write_cb, cli->fd, EV_WRITE);
	ev_io_start(loop,&cli->ev_write);
	
}
static void accept_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	int client_fd;
	struct client *client;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
	client_fd = accept(w->fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
                return;
        }
   	 
	client = calloc(1,sizeof(*client));
	client->fd=client_fd;
	if (setnonblock(client->fd) < 0)
                err(1, "failed to set client socket to non-blocking");
	ev_io_init(&client->ev_read,read_cb,client->fd,EV_READ);
	ev_io_start(loop,&client->ev_read);
}

static leveldb_t *db;
void opendb() {
    leveldb_options_t *options;
    char *err = NULL;

    /******************************************/
    /* OPEN */

    options = leveldb_options_create();
    leveldb_options_set_create_if_missing(options, 1);
    db = leveldb_open(options, "testdb", &err);

    if (err != NULL) {
      fprintf(stderr, "Open fail.\n");
	return;
    }

    /* reset error var */
    leveldb_free(err); err = NULL;
}

static int key_offset = 0;
static char full_key[32];

void writedb(char* key, char* value) {
    char *err = NULL;
    leveldb_writeoptions_t *woptions;
    /******************************************/
    /* WRITE */

    woptions = leveldb_writeoptions_create();
    sprintf(full_key, "%s%d", key, key_offset++);
    leveldb_put(db, woptions, full_key, strlen(full_key), value, strlen(value), &err);

    if (err != NULL) {
      fprintf(stderr, "Write fail.\n");
	return;
    }

    leveldb_free(err); err = NULL;
}

char* readdb(char* key) {
    char *err = NULL;
    leveldb_readoptions_t *roptions;
    char *read;
    size_t read_len;
    /******************************************/
    /* READ */

    roptions = leveldb_readoptions_create();
    sprintf(full_key, "%s%d", key, key_offset++);
    read = leveldb_get(db, roptions, full_key, strlen(key), &read_len, &err);

    if (err != NULL) {
      fprintf(stderr, "Read fail.\n");
      return 0;
    }

    if (read_len <= 0)
	return 0;
    read[read_len] = '\0';

    leveldb_free(err); err = NULL;

    return read;
}


    /******************************************/
    /* DELETE */
/*
    leveldb_delete(db, woptions, "key", 3, &err);

    if (err != NULL) {
      fprintf(stderr, "Delete fail.\n");
      return;
    }

    leveldb_free(err); err = NULL;
*/

void closedb() {
    /******************************************/
    /* CLOSE */
    leveldb_close(db);
}


int main()
{
    struct ev_loop *loop = ev_default_loop (0);
    int listen_fd;
    struct sockaddr_in listen_addr;
    int reuseaddr_on = 1;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
            err(1, "listen failed");
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,
            sizeof(reuseaddr_on)) == -1)
            err(1, "setsockopt failed");
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(SERVER_PORT);
    if (bind(listen_fd, (struct sockaddr *)&listen_addr,
            sizeof(listen_addr)) < 0)
            err(1, "bind failed");
    if (listen(listen_fd,5) < 0)
            err(1, "listen failed");
    if (setnonblock(listen_fd) < 0)
            err(1, "failed to set server socket to non-blocking");

        opendb();

        ev_io_init(&ev_accept,accept_cb,listen_fd,EV_READ);
        ev_io_start(loop,&ev_accept);
        ev_loop (loop, 0);

        closedb();

        return 0;
}

