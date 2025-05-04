#include <arpa/inet.h>  // sockaddr_in

#include "server.h"  // echo_server, related functions
#include "util.h"    // socket_address, PORT

int main(void) {
  struct sockaddr_in server_addr = socket_address(INADDR_ANY, PORT);
  echo_server* server = make_echo_server(server_addr, BACKLOG_SIZE);
  listen_for_connections(server);
  int accept_status = 0;
  while (accept_status != -1) {
    accept_status = accept_client(server);
  }
  free_echo_server(server);
  return 0;
}
