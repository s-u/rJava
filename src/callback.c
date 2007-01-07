#include "rJava.h"
#include <R_ext/eventloop.h>
#include <unistd.h>

#define RJavaActivity 16

int RJava_has_control = 0;

static int ipcin, ipcout, resin, resout;

/* all IPC messages are long-alligned */
#define IPCC_LOCK_REQUEST 1
#define IPCC_LOCK_GRANTED 2 /* reponse on IPCC_LOCK_REQUEST */
#define IPCC_CLEAR_LOCK   3
#define IPCC_CALL_REQUEST 4 /* pars: <fn-ptr> <data-ptr> */

typedef void(callbackfn)(void*);

static void RJava_ProcessEvents(void *data) {
  long buf[4];
  int n = read(ipcin, buf, sizeof(long));
  if (buf[0] == IPCC_LOCK_REQUEST) {
    RJava_has_control = 1;
    buf[0] = IPCC_LOCK_GRANTED;
    write(resout, buf, sizeof(long));
    n = read(ipcin, buf, sizeof(long));
  }
  if (buf[0] == IPCC_CLEAR_LOCK) {
    RJava_has_control = 0;
  }
  if (buf[0] == IPCC_CALL_REQUEST) {
    read(ipcin, buf+1, sizeof(long)*2);
    callbackfn *fn = (callbackfn*) buf[1];
    RJava_has_control = 1;
    fn((void*) buf[2]);
    RJava_has_control = 0;
  }
}

int RJava_init_loop() {
  int pfd[2];
  pipe(pfd);
  ipcin = pfd[0];
  ipcout = pfd[1];
  pipe(pfd);
  resin = pfd[0];
  resout = pfd[1];
  addInputHandler(R_InputHandlers, ipcin, RJava_ProcessEvents, RJavaActivity);
}

int RJava_request_lock() {
  long buf[4];
  int n;
  if (RJava_has_control) return 1;
  
  buf[0] = IPCC_LOCK_REQUEST;
  write(ipcout, buf, sizeof(long));
  n = read(resin, buf, sizeof(long));
  return (buf[0] == IPCC_LOCK_GRANTED);
}

int RJava_clear_lock() {
  long buf[4];
  buf[0] = IPCC_CLEAR_LOCK;
  write(ipcout, buf, sizeof(long));
  return 1;
}

void RJava_request_callback(callbackfn *fn, void *data) {
  long buf[4];
  buf[0] = IPCC_CALL_REQUEST;
  buf[1] = (long) fn;
  buf[2] = (long) data;
  write(ipcout, buf, sizeof(long)*3);
}

