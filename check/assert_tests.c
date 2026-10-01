/* NDEBUG is defined on purpose: ASSERT must stay active, unlike assert(). */
#define NDEBUG
#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define data_t int
#define prefix int
#include <array.h>
#undef data_t
#undef prefix

#define data_t int
#define prefix int
#include <pqueue.h>
#undef data_t
#undef prefix

/* Runs f in a child process and returns the signal that ended it (0 if it
   exited normally); what the child wrote to stderr is copied to msg. */
static int run_child(void (*f)(void), char *msg, size_t msg_size) {
  int fds[2];
  if (pipe(fds) != 0) {
    return -1;
  }
  fflush(NULL);
  pid_t pid = fork();
  if (pid < 0) {
    return -1;
  }
  if (pid == 0) {
    close(fds[0]);
    dup2(fds[1], STDERR_FILENO);
    f();
    _exit(0);
  }
  close(fds[1]);
  size_t used = 0;
  ssize_t n;
  while (used + 1 < msg_size && (n = read(fds[0], msg + used, msg_size - 1 - used)) > 0) {
    used += (size_t) n;
  }
  msg[used] = 0;
  close(fds[0]);
  int status = 0;
  waitpid(pid, &status, 0);
  return WIFSIGNALED(status) ? WTERMSIG(status) : 0;
}

static void child_true(void) {
  ASSERT(1 + 1 == 2);
}

static void child_false(void) {
  int zero = 0;
  ASSERT(zero != 0);
}

static void child_array_get(void) {
  array_int_t *a = array_int_init();
  array_int_append(a, 1);
  array_int_get(a, 5);
}

static void child_array_pop(void) {
  array_int_t *a = array_int_init();
  array_int_pop(a);
}

static void child_pqueue_null(void) {
  pqueue_int_size(NULL);
}

START_TEST(assert_test1)

char msg[512];

CHECK(run_child(child_true, msg, sizeof(msg)) == 0);
CHECK(msg[0] == 0);

CHECK(run_child(child_false, msg, sizeof(msg)) == SIGABRT);
CHECK(strncmp(msg, "ASSERT failure: ", 16) == 0);
CHECK(strstr(msg, "assert_tests.c") != NULL);
CHECK(strstr(msg, "child_false") != NULL);
CHECK(strstr(msg, "zero != 0") != NULL);

END_TEST

// the documented misuse of the containers is caught even with NDEBUG
START_TEST(assert_test2)

char msg[512];

CHECK(run_child(child_array_get, msg, sizeof(msg)) == SIGABRT);
CHECK(strstr(msg, "array_int_get") != NULL);
CHECK(strstr(msg, "idx < a->size") != NULL);

CHECK(run_child(child_array_pop, msg, sizeof(msg)) == SIGABRT);
CHECK(strstr(msg, "array_int_pop") != NULL);

CHECK(run_child(child_pqueue_null, msg, sizeof(msg)) == SIGABRT);
CHECK(strstr(msg, "q != NULL") != NULL);

END_TEST
