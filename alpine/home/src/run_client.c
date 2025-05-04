#include <arpa/inet.h>  // INADDR_LOOPBACK, inet_ntop
#include <stdio.h>      // fprintf

#include "client.h"
#include "util.h"

const socklen_t MAX_IP_ADDR_LEN = 16;

// Inet 192.168.100.2
#define INADDR_CLIENT ((in_addr_t)0xc0a86402)

// Inet 192.168.100.3
#define INADDR_SERVER ((in_addr_t)0xc0a86403)

int main(void) {
  // Open a TCP socket to connect to the server.
  int socket_descriptor = open_tcp_socket();
  (void)fprintf(stderr, "Client socket descriptor: %d\n", socket_descriptor);

  // Connect to the server running in the other container (192.168.100.3).
  struct sockaddr_in server_addr = socket_address(INADDR_SERVER, PORT);
  try_connect(socket_descriptor, server_addr);
  // To print an IP address as a string, we need to allocate a buffer.
  char addr_string[MAX_IP_ADDR_LEN];
  (void)fprintf(
      stderr, "Connected to server at %s, port %d\n",
      inet_ntop(AF_INET, &server_addr.sin_addr, addr_string, MAX_IP_ADDR_LEN),
      PORT);

  // Use a file pointer to make it easier to deal with text lines.
  FILE* socket_file = get_socket_file(socket_descriptor);
  // Echo until either the client or the server closes its stream.
  int socket_file_status = 0;
  while (socket_file_status != -1) {
    socket_file_status = echo(socket_file);
  }

  // If we didn't hit the end of file for either stdin or the response from the
  // server, then something went wrong.
  if (!feof(stdin) && !feof(socket_file)) {
    error_and_exit("Error reading or writing line:");
  }

  // Clean up and exit.
  close_tcp_socket(socket_descriptor);
  return 0;
}
