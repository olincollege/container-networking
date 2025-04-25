
#include "syscalls.h"

int capabilities() {
  (void)fprintf(stderr, "=> dropping capabilities...");
  int drop_caps[] = {
      CAP_AUDIT_CONTROL,   CAP_AUDIT_READ,   CAP_AUDIT_WRITE, CAP_BLOCK_SUSPEND,
      CAP_DAC_READ_SEARCH, CAP_FSETID,       CAP_IPC_LOCK,    CAP_MAC_ADMIN,
      CAP_MAC_OVERRIDE,    CAP_MKNOD,        CAP_SETFCAP,     CAP_SYSLOG,
      CAP_SYS_ADMIN,       CAP_SYS_BOOT,     CAP_SYS_MODULE,  CAP_SYS_NICE,
      CAP_SYS_RAWIO,       CAP_SYS_RESOURCE, CAP_SYS_TIME,    CAP_WAKE_ALARM};
  size_t num_caps = sizeof(drop_caps) / sizeof(*drop_caps);
  (void)fprintf(stderr, "bounding...");
  for (size_t i = 0; i < num_caps; i++) {
    if (prctl(PR_CAPBSET_DROP, drop_caps[i], 0, 0, 0)) {
      (void)fprintf(stderr, "prctl failed: %m\n");
      return 1;
    }
  }
  (void)fprintf(stderr, "inheritable...");
  cap_t caps = NULL;
  if (!(caps = cap_get_proc()) ||
      cap_set_flag(caps, CAP_INHERITABLE, num_caps, drop_caps, CAP_CLEAR) ||
      cap_set_proc(caps)) {
    (void)fprintf(stderr, "failed: %m\n");
    if (caps) cap_free(caps);
    return 1;
  }
  cap_free(caps);
  (void)fprintf(stderr, "done.\n");
  return 0;
}


#define SCMP_FAIL SCMP_ACT_ERRNO(EPERM)

int syscalls() {
  scmp_filter_ctx ctx = NULL;
  (void)fprintf(stderr, "=> filtering syscalls...");
  if (!(ctx = seccomp_init(SCMP_ACT_ALLOW)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
                       SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
                       SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
      seccomp_rule_add(
          ctx, SCMP_FAIL, SCMP_SYS(unshare), 1,
          SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER)) ||
      seccomp_rule_add(
          ctx, SCMP_FAIL, SCMP_SYS(clone), 1,
          SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ioctl), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, TIOCSTI, TIOCSTI)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(keyctl), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(add_key), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(request_key), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ptrace), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(mbind), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(migrate_pages), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(move_pages), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(set_mempolicy), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(userfaultfd), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(perf_event_open), 0) ||
      seccomp_attr_set(ctx, SCMP_FLTATR_CTL_NNP, 0) || seccomp_load(ctx)) {
    if (ctx) seccomp_release(ctx);
    (void)fprintf(stderr, "failed: %m\n");
    return 1;
  }
  seccomp_release(ctx);
  (void)fprintf(stderr, "done.\n");
  return 0;
}
