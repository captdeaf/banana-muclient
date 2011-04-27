// Just the options go here.
#include "banana.h"

const char *options[] = {
  "document_root", "html",
  // "listening_ports", "8081,8082s",
  "listening_ports", "8088,9099s",
  "ssl_certificate", "ssl_cert.pem",
  "i", "index.html",
  "num_threads", "5",
  NULL
};

// Configuration options for banana.
// char *ssl_url   = "http://client.pennmush.org:8082/login";

char *login_url = "/login.html";

char *session_salt = "bananawhams";
