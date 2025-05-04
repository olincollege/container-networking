#ifdef __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#else
#define _POSIX_C_SOURCE 200809L
#endif
#define _GNU_SOURCE

#include "server.h"

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

echo_server* make_echo_server(struct sockaddr_in ip_addr, int max_backlog) {
  echo_server* server = malloc(sizeof(echo_server));
  server->listener = open_tcp_socket();
  server->addr = ip_addr;
  server->max_backlog = max_backlog;
  return server;
}

void free_echo_server(echo_server* server) {
  close_tcp_socket(server->listener);
  free(server);
}

void listen_for_connections(echo_server* server) {
  if (bind(server->listener, (struct sockaddr*)&server->addr,
           sizeof(server->addr)) < 0) {
    error_and_exit("Error binding to listener socket");
  }
  if (listen(server->listener, server->max_backlog) == -1) {
    error_and_exit("Error listening on listener socket");
  }
}

int accept_client(echo_server* server) {
  int addrlen = sizeof(server->addr);
  int connect_d = accept4(server->listener, (struct sockaddr*)&server->addr,
                          (socklen_t*)&addrlen, SOCK_CLOEXEC);
  if (connect_d < 0) {
    error_and_exit("Error opening connection");
  }
  pid_t proc = fork();
  if (proc == -1) {
    error_and_exit("Error forking process");
  } else if (proc == 0) {
    echo(connect_d);
    return -1;
  } else {
    return 0;
  }
}

void echo(int socket_descriptor) {
  FILE* client = fdopen(socket_descriptor, "r+");
  char* line = NULL;
  size_t len = 0;

  if (client == NULL) {
    error_and_exit("Error opening network stream");
  }

  while (getline(&line, &len, client) != -1) {
    if (fputs(line, client) < 0) {
      error_and_exit("Error writing to network stream");
    }
  }

  if (ferror(client)) {
    error_and_exit("Error reading from network stream");
  } else {
    if (fclose(client) < 0) {
      error_and_exit("Error closing network stream");
    }
    close_tcp_socket(socket_descriptor);
  }
}
