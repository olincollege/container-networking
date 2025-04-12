// function 1: choose host name and initialize socketpair and fcntl
// function 2: allocate stack and clone child process
// function 3: handle child uid map
// function 4: user namespace
// function 5: mount namespace




char hostname[256] = {0};
  if (choose_hostname(hostname, sizeof(hostname))) goto error;
  config.hostname = hostname;

  if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets)) {
    fprintf(stderr, "socketpair failed: %m\n");
    goto error;
  }
  if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC)) {
    fprintf(stderr, "fcntl failed: %m\n");
    goto error;
  }
  config.fd = sockets[1];
#define STACK_SIZE (1024 * 1024)

  char* stack = 0;
  if (!(stack = malloc(STACK_SIZE))) {
    fprintf(stderr, "=> malloc failed, out of memory?\n");
    goto error;
  }
  if (resources(&config)) {
    err = 1;
    goto clear_resources;
  }
  int flags = CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWPID | CLONE_NEWIPC |
              CLONE_NEWNET | CLONE_NEWUTS;
  if ((child_pid =
           clone(child, stack + STACK_SIZE, flags | SIGCHLD, &config)) == -1) {
    fprintf(stderr, "=> clone failed! %m\n");
    err = 1;
    goto clear_resources;
  }
  close(sockets[1]);
  sockets[1] = 0;
  close(sockets[1]);
  sockets[1] = 0;




// -------------------------------------------------------------------------------
  int choose_hostname(char* buff, size_t len) {
  static const char* suits[] = {"swords", "wands", "pentacles", "cups"};
  static const char* minor[] = {"ace",  "two",    "three", "four", "five",
                                "six",  "seven",  "eight", "nine", "ten",
                                "page", "knight", "queen", "king"};
  static const char* major[] = {
      "fool",       "magician", "high-priestess", "empress",  "emperor",
      "hierophant", "lovers",   "chariot",        "strength", "hermit",
      "wheel",      "justice",  "hanged-man",     "death",    "temperance",
      "devil",      "tower",    "star",           "moon",     "sun",
      "judgment",   "world"};
  struct timespec now = {0};
  clock_gettime(CLOCK_MONOTONIC, &now);
  size_t ix = now.tv_nsec % 78;
  if (ix < sizeof(major) / sizeof(*major)) {
    snprintf(buff, len, "%05lx-%s", now.tv_sec, major[ix]);
  } else {
    ix -= sizeof(major) / sizeof(*major);
    snprintf(buff, len, "%05lxc-%s-of-%s", now.tv_sec,
             minor[ix % (sizeof(minor) / sizeof(*minor))],
             suits[ix / (sizeof(minor) / sizeof(*minor))]);
  }
  return 0;
}
