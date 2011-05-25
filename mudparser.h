/** mudparser.h
 *
 * Convert lines coming in from mu*s to HTML.
 */

#ifndef _MY_MUDPARSER_H_
#define _MY_MUDPARSER_H_

#define ANSI_UNDERLINE  0x01 /* <u> */
#define ANSI_FLASH      0x02 /* <em> */
#define ANSI_HILITE     0x04 /* <strong> */
#define ANSI_INVERT     0x08 /* Switch bg and fg */

#define ANSI_RGB        0x80 /* not supported yet: Uses style="<color>" instead
                              * of span classes */
#define ANSI_SIZE 12

// Every world has a markupdata structure named mdata, this is used to
// carry any semi-persistant markup information. (nested/bleeding ansi, etc).
struct markupdata {
  char f[ANSI_SIZE]; /* fg_r, fg_25. */
  char b[ANSI_SIZE]; /* bg_r */
  int flags;
};
struct world;

// WARNING: remove_markup destructively modifies <str>.
char *remove_markup(char *str);

// Convert a line of ansi to JSON-escaped html
char *ansi2html(struct world *w, char *str);

// Convert a string from one charset to another. This returns
// a malloc'd string that needs to be free()'d.
char *convert_charset( char *from_charset, char *to_charset, char *input );

// Run some text through conversion relevant for a world. For now,
// this is only convert_charset and ansi2html, but later it may include
// handling for, e.g: Pueblo, MXP, etc.
char *convert_to_json(struct world *w, char *str);
#endif
