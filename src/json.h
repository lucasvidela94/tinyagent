/* Small read-only JSON tokenizer for API responses. Portable C.
 *
 * Tokens keep offsets into the source text, which must outlive the Json value.
 * Structure is validated (brackets, key/value pairs, exactly one top-level value,
 * MAX_DEPTH and MAX_TOKENS); primitive spelling is not, since only strings and
 * structure are consumed. Every lookup accepts -1 and returns -1, so chains like
 * json_key(j, json_index(j, json_key(j, 0, "choices"), 0), "message") are safe. */
#ifndef TINYSHELL_JSON_H
#define TINYSHELL_JSON_H

typedef struct { int start, end, size; } Tok; /* size = number of direct children */
typedef struct { const char *src; Tok *tok; int n, cap; } Json;

int json_parse(Json *j, const char *s); /* 1 if well-formed (token 0 is the root); 0 frees */
void json_free(Json *j);

char json_type(const Json *j, int i); /* '{' '[' '"' or first char of a primitive; 0 if none */
int json_size(const Json *j, int i);  /* direct children of an object/array, 0 otherwise */
int json_key(const Json *j, int obj, const char *name);
int json_index(const Json *j, int arr, int idx);

/* Decoded string as UTF-8 (\uXXXX and surrogate pairs included). Missing or null gives "";
 * numbers, booleans, objects and arrays give their raw JSON text. Caller frees. */
char *json_str(const Json *j, int i);
char *json_raw(const Json *j, int i); /* raw source text; "null" if missing. Caller frees. */

#endif
