#include "rjava.h"
#include <unistd.h>

#ifdef _WIN64
typedef long long ptrlong;
#else
typedef long ptrlong;
#endif

int ipcout;
int resin;
int *rjctrl = 0;

typedef void(callbackfn)(void *);

int RJava_request_lock(void) {
  ptrlong buf[4];
  int n;
  if (rjctrl && *rjctrl) return 2;

  buf[0] = IPCC_LOCK_REQUEST;
  if (write(ipcout, buf, sizeof(ptrlong)) < sizeof(ptrlong)) return 0;
  n = read(resin, buf, sizeof(ptrlong));
  return (n == sizeof(ptrlong) && buf[0] == IPCC_LOCK_GRANTED) ? 1 : 0;
}

int RJava_clear_lock(void) {
  ptrlong buf[4];
  buf[0] = IPCC_CLEAR_LOCK;
  return (write(ipcout, buf, sizeof(ptrlong)) == sizeof(ptrlong)) ? 1 : 0;
}

int RJava_request_callback(callbackfn *fn, void *data) {
  ptrlong buf[4];
  buf[0] = IPCC_CALL_REQUEST;
  buf[1] = (ptrlong) fn;
  buf[2] = (ptrlong) data;
  return (write(ipcout, buf, sizeof(ptrlong) * 3) == sizeof(ptrlong) * 3) ? 1 : 0;
}

void RJava_setup(int _in, int _out) {
  /* ptrlong buf[4]; */
  ipcout = _out;
  resin = _in;
}

void RJava_init_ctrl(void) {
  ptrlong buf[4];
  buf[0] = IPCC_CONTROL_ADDR;
  if (write(ipcout, buf, sizeof(ptrlong)) == sizeof(ptrlong) &&
      read(resin, buf, sizeof(ptrlong) * 2) == sizeof(ptrlong) * 2 &&
      buf[0] == IPCC_CONTROL_ADDR) {
      rjctrl= (int*) buf[1];
  }
}
