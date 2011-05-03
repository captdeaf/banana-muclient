/* mudparser.c
 *
 * Convert incoming lines as needed into HTML.
 */

#include "banana.h"
#include <iconv.h>

// Remove HTML markup from a line of text. */
char *
remove_markup(char *str) {
  char *r = str;
  char *s = str;
  while (s && *s) {
    switch (*s) {
    case '&':
      if (!strncmp(s, "&nbsp;", 6)) {
        s += 6;
        *(r++) = ' ';
      } else if (!strncmp(s, "&amp;", 5)) {
        s += 5;
        *(r++) = '&';
      } else if (!strncmp(s, "&gt;", 4)) {
        s += 4;
        *(r++) = '>';
      } else if (!strncmp(s, "&lt;", 4)) {
        s += 4;
        *(r++) = '<';
      }
      break;
    case '<':
      while (*s && *s != '>') s++;
      s++;
      break;
    case '\\':
      s++;
    default:
      *(r++) = *s++;
    }
  }
  *(r) = '\0';
  return str;
}

char *
convert_charset( char *from_charset, char *to_charset, char *input ) {
  size_t input_size, output_size, bytes_converted;
  char * output;
  char * ret;
  iconv_t cd;

  cd = iconv_open( to_charset, from_charset);
  if ( cd == (iconv_t) -1 ) {
    //Something went wrong with iconv_open
    if (errno == EINVAL)
    {
      slog("Conversion from %s to %s not available", from_charset, to_charset);
    }
    perror("iconv_open");
    return NULL;
  }

  input_size = strlen(input);
  output_size = 4 * input_size;
  ret = output = malloc(output_size + 1);
  memset(output, 0, output_size + 1);

  bytes_converted = iconv(cd, &input, &input_size, &output, &output_size);
  if ( iconv_close (cd) != 0) {
    perror("iconv_open");
    slog("Error closing iconv port?");
  }

  return ret;
}

// start_span and end_span must write out text that is JSON-safe.
int
start_span(char **r, struct a2h *ah) {
  int ret = 0;
  if (ah->flags) {
    ret = 1;
    if (ah->flags & ANSI_UNDERLINE) { *r += sprintf(*r, "<u>"); }
    if (ah->flags & ANSI_FLASH) { *r += sprintf(*r, "<em>"); }
    if (ah->flags & ANSI_HILITE) { *r += sprintf(*r, "<strong>"); }
    if (ah->flags & ANSI_INVERT) { *r += sprintf(*r, "<span class=\\\"invert\\\">"); }
  }
  if (ah->f[0] || ah->b[0]) {
    ret = 1;
    (*r) += sprintf(*r, "<span class=\\\"");
    if (ah->b[0]) {
      (*r) += sprintf(*r, "%s", ah->b);
    }
    if (ah->f[0]) {
      (*r) += sprintf(*r, "%s%s", ah->b[0] ? " " : "", ah->f);
    }
    (*r) += sprintf(*r, "\\\">");
  }
  return ret;
}

/** We need to spit out The bits we printed, in reverse */
void
end_span(char **r, struct a2h *ah) {
  if (ah->f[0] || ah->b[0]) {
    (*r) += sprintf(*r, "</span>");
  }
  if (ah->flags) {
    if (ah->flags & ANSI_INVERT) { *r += sprintf(*r, "</span>"); }
    if (ah->flags & ANSI_HILITE) { *r += sprintf(*r, "</strong>"); }
    if (ah->flags & ANSI_FLASH) { *r += sprintf(*r, "</em>"); }
    if (ah->flags & ANSI_UNDERLINE) { *r += sprintf(*r, "</u>"); }
  }
}

#define _ESC 0x1B
char *
ansi2html(World *w, char *str) {
  int inspan;
  char *r, *s;
  struct a2h *ah = &w->a2h;
  int code;
  int donbsp;
  // With BUFFER_LEN of 8192, this takes 10kb, but it's called for every
  // line and the other solution is to malloc 10*stren(), which isn't wise.
  // We'll probably almost never fill this buffer, but who cares.
  static char ret[BUFFER_LEN * 10];
  // I think this should be enough for every case. If not, shoot me.
  // It's short lived anyway, so who cares?
  r = ret;

  inspan = start_span(&r, ah);
  donbsp = 1;
  while (*str) {
    while (*str && *str != _ESC) {
      switch (*str) {
      case '\t':
        r += sprintf(r, "&nbsp; ");
        donbsp = 1;
        break;
      case ' ':
        if (donbsp) {
          r += sprintf(r, "&nbsp;");
        } else {
          *(r++) = ' ';
        }
        donbsp = !donbsp;
        break;
      case '<':
        r += sprintf(r, "&lt;");
        break;
      case '>':
        r += sprintf(r, "&gt;");
        break;
      case '&':
        r += sprintf(r, "&amp;");
        break;
      case '\n':
        // We should never get \\r or \\n, but just in case . . .
        r += sprintf(r, "\\r");
        break;
      case '\r':
        // We should never get \\r or \\n, but just in case . . .
        r += sprintf(r, "\\r");
        break;
      case '"':
      case '\'':
      case '\\':
        r += sprintf(r, "\\%c", (*str) & 0xFF);
        break;
      default:
        /*
        if (isprint(*str)) {
          *(r++) = *str;
          donbsp = 0;
        } else {
          r += sprintf(r, "\\x%.2x", (*str) & 0xFF);
        }
        */
        *(r++) = *str;
      }
      str++;
    }
    if (inspan) {
      if (r > ret && *(r-1) == ' ') {
        r -= 1;
        r += sprintf(r, "&nbsp;");
      }
      end_span(&r, ah); donbsp = 1;
    }
    inspan = 0;
    if (*str == _ESC) {
      while (*str == _ESC && *(str+1) == '[') {
        str ++;
        do {
          str++;
          s = str;
          while (*str && isdigit(*str)) str++;
          code = atoi(s);
          switch (code) {
          case 30: strcpy(ah->f,"fg_x"); break;
          case 31: strcpy(ah->f,"fg_r"); break;
          case 32: strcpy(ah->f,"fg_g"); break;
          case 33: strcpy(ah->f,"fg_y"); break;
          case 34: strcpy(ah->f,"fg_b"); break;
          case 35: strcpy(ah->f,"fg_m"); break;
          case 36: strcpy(ah->f,"fg_c"); break;
          case 37: strcpy(ah->f,"fg_w"); break;
          case 40: strcpy(ah->b,"bg_x"); break;
          case 41: strcpy(ah->b,"bg_r"); break;
          case 42: strcpy(ah->b,"bg_g"); break;
          case 43: strcpy(ah->b,"bg_y"); break;
          case 44: strcpy(ah->b,"bg_b"); break;
          case 45: strcpy(ah->b,"bg_m"); break;
          case 46: strcpy(ah->b,"bg_c"); break;
          case 47: strcpy(ah->b,"bg_w"); break;
          case 1: ah->flags |= ANSI_HILITE; break;
          case 3: ah->flags |= ANSI_FLASH; break;
          case 4: ah->flags |= ANSI_UNDERLINE; break;
          case 7: ah->flags |= ANSI_INVERT; break;
          case 5: ah->flags |= ANSI_FLASH; break;
          case 6: ah->flags |= ANSI_FLASH; break;
          case 0:
            // Unset everything.
            ah->f[0] = '\0';
            ah->b[0] = '\0';
            ah->flags = 0;
            break;
          case 38: 
            // 256 color foreground
            if ((*str == ';') && (*(str+1) == '5') && *(str+2) == ';') {
              str += 3;
              code = atoi(str);
              while (*str && isdigit(*str)) str++;
              snprintf(ah->f, ANSI_SIZE, "fg_%d", code);
            }
            break;
          case 48: 
            // 256 color background
            if ((*str == ';') && (*(str+1) == '5') && *(str+2) == ';') {
              str += 3;
              code = atoi(str);
              while (*str && isdigit(*str)) str++;
              snprintf(ah->b, ANSI_SIZE, "bg_%d", code);
            }
            break;
          }
        } while (*str == ';');
        if (*str == 'm') str++;
      }
    }
    if (*str) {
      if (r > ret && *(r-1) == ' ') {
        r -= 1;
        r += sprintf(r, "&nbsp;");
      }
      inspan = start_span(&r, ah);
      donbsp = 1;
    }
  }
  *(r) = '\0';
  return strdup(ret);
}

char *
convert_to_json(World *w, char *str) {
  char *post_convert;
  char *post_ansi2html;

  if (strcasecmp(w->charset, "UTF-8")) {
    post_convert = convert_charset(w->charset, "UTF-8", str);
    if (post_convert) {
      post_ansi2html = ansi2html(w, post_convert);
      free(post_convert);
    } else {
      post_ansi2html = ansi2html(w, str);
    }
  } else {
    post_ansi2html = ansi2html(w, str);
  }
  return post_ansi2html;
}
