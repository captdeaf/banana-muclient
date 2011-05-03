/** mudparser.h
 *
 * Convert lines coming in from mu*s to HTML.
 */

#ifndef _MY_MUDPARSER_H_
#define _MY_MUDPARSER_H_

// WARNING: remove_markup destructively modifies <str>.
char *remove_markup(char *str);

// Convert a line of ansi to JSON-escaped html
char *ansi2html(World *w, char *str);

// Convert a string from one charset to another. This returns
// a malloc'd string that needs to be free()'d.
char *convert_charset( char *from_charset, char *to_charset, char *input );

// Run some text through conversion relevant for a world. For now,
// this is only convert_charset and ansi2html, but later it may include
// handling for, e.g: Pueblo, MXP, etc.
char *convert_to_json(World *w, char *str);
#endif
