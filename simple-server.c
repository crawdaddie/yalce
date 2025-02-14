#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8000
#define BUFFER_SIZE 100

// echo client
void *handle_client_echo(void *arg) {
  int client_fd = *((int *)arg);
  printf("client fd %d\n", client_fd);
  char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

  // receive request data from client and store into buffer
  ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
  if (bytes_received > 0) {
    send(client_fd, buffer, bytes_received, 0);
  }
  close(client_fd);
  free(arg);
  free(buffer);
  return NULL;
}

void *handle_client(void *arg) {
  int client_fd = *((int *)arg);
  printf("client fd %d:\n", client_fd);
  char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

  // receive request data from client and store into buffer
  ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
  printf("received %zd bytes: '%s'\n", bytes_received, buffer);
  if (bytes_received > 0) {
    send(client_fd, buffer, bytes_received, 0);
  }
  close(client_fd);
  free(arg);
  free(buffer);
  return NULL;
}

int main() {
  int server_fd;
  struct sockaddr_in server_addr;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("bind failed");
    exit(1);
  }

  if (listen(server_fd, 10) < 0) {
    perror("listen failed");
    exit(1);
  }

  while (true) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int *client_fd = malloc(sizeof(int));
    if ((*client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                             &client_addr_len)) < 0) {
      perror("accept failed");
      continue;
    }

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, handle_client, (void *)client_fd);
    pthread_detach(thread_id);
  }
  close(server_fd);
  return 0;
}
