#ifndef HMAP_H
#define HMAP_H

#include <string.h>

typedef struct hmap hmap;

/** Adds the the KV-pair (KEY, VALUE) to MAP, or updates the value associated
 *  with KEY to VALUE if KEY already exists in MAP. */
void hmap_put(hmap *map, char *key, void *value);

/** Returns the value associated with KEY in MAP, or -1 if it is not found. */
int hmap_get(hmap *map, char *key);

/** As hmap_get(), but updates the value pointed to by SUCCESS to indicate
 *  whether or not  there is a value associated with KEY in MAP. */
int hmap_get_extended(hmap *map, char *key, int *success);

/** Increases the value at KEY in MAP by AMT. If KEY does not yet exist in MAP
 *  then acts as hmap_put(). */
void hmap_increment(hmap *map, char *key, int amt);

/** Allocates a new hmap on the heap and returns a pointer to it.*/
hmap *hmap_new();

/** Deallocates all of the memory allocated to MAP. */
void hmap_del(hmap *map);

/** Deallocates all of the memory allocated to the strings in MAP. */
void hmap_del_contents(hmap *map);
#endif
