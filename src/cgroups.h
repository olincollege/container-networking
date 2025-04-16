

#define MEMORY "1073741824"
#define SHARES "256"
#define PIDS "64"
#define WEIGHT "10"
#define FD_COUNT 64

struct cgrp_control {
  char control[256];
  struct cgrp_setting {
    char name[256];
    char value[256];
  } * *settings;
};
