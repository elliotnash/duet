#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>

#include "git_version.h"

const char *argp_program_version = "duet " GIT_COMMIT " (" GIT_BRANCH ")";
const char *argp_program_bug_address =
    "https://github.com/elliotnash/duet/issues";

/* Program documentation with mode details */
static char doc[] = "Duet -- A utility for managing dual-screen setups on the "
                    "Asus Zenbook Duo (UX8406) under Hyprland.\n"
                    "\n"
                    "Valid modes:\n"
                    "  auto         (0) - Automatic configuration\n"
                    "  mirror       (1) - Mirror displays\n"
                    "  landscape    (2) - Landscape orientation\n"
                    "  portrait-90  (3) - Portrait 90° rotation\n"
                    "  portrait-270 (4) - Portrait 270° rotation";

/* Argument description with mode options */
static char args_doc[] =
    "MODE (auto|mirror|landscape|portrait-90|portrait-270|0-4)";

/* Options (none) */
static struct argp_option options[] = {{0}};

struct mode_mapping {
  const char *name;
  int value;
};

static const struct mode_mapping modes[] = {
    {"auto", 0},        {"mirror", 1},       {"landscape", 2},
    {"portrait-90", 3}, {"portrait-270", 4}, {NULL, 0}};

struct arguments {
  int mode;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
  case ARGP_KEY_ARG:
    if (state->arg_num >= 1)
      argp_usage(state);

    /* Try to match mode names */
    for (int i = 0; modes[i].name != NULL; i++) {
      if (strcmp(arg, modes[i].name) == 0) {
        arguments->mode = modes[i].value;
        return 0;
      }
    }

    /* Try to parse numeric value */
    char *endptr;
    long num = strtol(arg, &endptr, 10);
    if (*endptr == '\0' && num >= 0 && num <= 4) {
      arguments->mode = (int)num;
      return 0;
    }

    argp_error(state,
               "Invalid MODE '%s'. Valid modes are: auto, mirror, landscape, "
               "portrait-90, portrait-270, or 0-4",
               arg);
    break;

  case ARGP_KEY_END:
    if (state->arg_num < 1)
      argp_usage(state);
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

void send_event(char *event, char *payload) {
  int sockfd = -1;

  /* Validate input parameters */
  if (!event || !payload) {
    fprintf(stderr, "Error: event or payload cannot be NULL\n");
    return;
  }

  /* Get runtime directory from environment */
  char *runtime_dir = getenv("XDG_RUNTIME_DIR");
  if (!runtime_dir) {
    argp_failure(
        NULL, // No argp_state available
        EXIT_FAILURE, 0,
        "XDG_RUNTIME_DIR not set. This typically means:\n"
        "  1. The program is running as root instead of a user session\n"
        "  2. The environment is not properly configured\n"
        "Try running as your regular user account (not with sudo)");
    return;
  }

  /* Create socket path */
  char socket_path[256];
  int path_len = snprintf(socket_path, sizeof(socket_path),
                          "%s/duet/cmd.socket", runtime_dir);
  if (path_len < 0 || (size_t)path_len >= sizeof(socket_path)) {
    fprintf(stderr, "Error: Socket path too long\n");
    return;
  }

  /* Create UNIX socket */
  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    return;
  }

  /* Configure socket address */
  struct sockaddr_un addr = {
      .sun_family = AF_UNIX,
  };
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
  addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

  /* Connect to socket */
  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("connect");
    argp_failure(NULL, EXIT_FAILURE, 0,
                 "Failed to connect to duetd socket. This typically means:\n"
                 "  1. The duetd daemon is not running\n"
                 "  2. The user doesn't have permission to access the socket\n"
                 "Make sure duetd is started in your Hyprland config.");
    goto cleanup;
  }

  /* Format message with newline */
  size_t msg_len = strlen(event) + strlen(payload) + 3;
  char *message = malloc(msg_len);
  snprintf(message, msg_len, "%s:%s\n", event, payload);

  /* Full write with flush behavior */
  ssize_t total_sent = 0;
  while (total_sent < (ssize_t)strlen(message)) {
      ssize_t sent = send(sockfd, message + total_sent, 
                         strlen(message) - total_sent, 0);
      if (sent == -1) {
          perror("send");
          argp_failure(NULL, EXIT_FAILURE, 0,
              "Send failed after %zd bytes. Possible causes:\n"
              "1. Server stopped reading mid-message\n"
              "2. Connection reset by peer\n"
              "3. Kernel buffer overflow", total_sent);
      }
      total_sent += sent;
  }

  /* Crucial shutdown sequence */
  if (shutdown(sockfd, SHUT_WR) == -1) {  // Tell server we're done writing
      perror("shutdown");
      argp_failure(NULL, EXIT_FAILURE, 0,
          "Socket shutdown failed. Server might not have received EOF");
  }

  /* Non-blocking read to drain response (if any) */
  struct timeval tv = {.tv_sec = 0, .tv_usec = 1000 };  // 1 ms timeout
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  char buf[128];
  while (recv(sockfd, buf, sizeof(buf), 0) > 0) {
      // Drain any unexpected response
  }

cleanup:
  free(message);
  if (sockfd != -1)
    close(sockfd);
}

int main(int argc, char **argv) {
  struct arguments arguments = {0};
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  char mode = arguments.mode + '0';
  send_event("mode", &mode);

  return 0;
}
