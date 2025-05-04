#include "util.h"

#include <stdint.h>  // uint16_t
#include <stdio.h>   // perror
#include <stdlib.h>  // exit, EXIT_FAILURE
#include <unistd.h>  // close

const uint16_t PORT = 4242;

void error_and_exit(const char* error_msg) {
  perror(error_msg);
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  exit(EXIT_FAILURE);
}

int open_tcp_socket(void) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    error_and_exit("Error opening TCP socket");
  }
  return sock;
}

void close_tcp_socket(int socket_) {
  if (close(socket_) == -1) {
    error_and_exit("Error closing TCP socket");
  }
}

struct sockaddr_in socket_address(in_addr_t addr, in_port_t port) {
  struct sockaddr_in sock = {.sin_addr.s_addr = htonl(addr),
                             .sin_family = AF_INET,
                             .sin_port = (in_port_t)htons(port)};
  return sock;
}
