// Just the options go here.
#include "banana.h"

const char *options[] = {
  "document_root", DOCUMENT_ROOT,
  // "listening_ports", "8081,8082s",
  "listening_ports", HTTP_PORTS,
#ifdef USE_SSL
  "ssl_certificate", SSL_PEM_FILE,
#endif
  "i", "index.html",
  "num_threads", NUM_THREADS,
  NULL
};
