/** api.c
 *
 * Handler stuff for api_* functions.
 */
#include "banana.h"

void write_ajax_header(struct mg_connection *conn) {
  static const char *header =
      "HTTP/1.1 200 OK\r\n"
      HEADER_NOCACHE
      "Content-Type: application/x-javascript\r\n"
      "\r\n";
  mg_write(conn, header, strlen(header));
}
