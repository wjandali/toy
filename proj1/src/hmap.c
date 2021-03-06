#include "hmap.h"
#include "util.h"
#include <stdlib.h>

/* More sophisticated hashmaps would allow you to specify the load factor at
 * runtime, but a constant load factor of 0.75 is sufficient for our purposes. */
#define LOAD_FACTOR (0.75)
/* Again, more sophisticated hashmaps would be more customizable, but that
 * added flexibility doesn't buy us much of anything. */
#define INIT_CAPACITY (16)
/* A simple macro for taking maxima. */
#define MAX(X, Y) (X > Y ? X : Y)

typedef struct pair {
  char *key;
  int val;
} pair;

typedef struct bucket {
  pair *pairs;
  size_t capacity;
  size_t num_pairs;
} bucket;

struct hmap {
  bucket *buckets;
  size_t num_buckets;
  size_t num_pairs;
};

/** Computes a hashcode for STR in the same way that java would. */
size_t hash_string(char *str);

/** Inserts a new KV-pair (STR, VAL) into the bucket BUCK. */
void bucket_insert(bucket *buck, char *str, int val);

/** Doubles the capacity of MAP, and rehashes its contents so that nothing is lost. */
void expand(hmap *map);

void hmap_put(hmap *map, char *key, int value) {
  if (!map) {
    return;
  }
  size_t hash = hash_string(key) % map->num_buckets;
  bucket buck = map->buckets[hash];
  for (size_t i = 0; i < buck.num_pairs; i += 1) {
    if (!strcmp(key, buck.pairs[i].key)) {
      buck.pairs[i].val = value;
      return;
    }
  }

  if ((map->num_pairs + 1) < map->num_buckets * LOAD_FACTOR) {
    bucket_insert(map->buckets + hash, key, value);
    map->num_pairs += 1;
  } else {
    expand(map);
    hmap_put(map, key, value);
  }
}

int hmap_get(hmap *map, char *key) {
  int tmp;
  return hmap_get_extended(map, key, &tmp);
}

int hmap_get_extended(hmap *map, char *key, int *success) {
  if (!map) {
    *success = 0;
    return -1;
  }
  size_t hash = hash_string(key) % map->num_buckets;
  bucket buck = map->buckets[hash];
  for (size_t i = 0; i < buck.num_pairs; i += 1) {
    if (!strcmp(key, buck.pairs[i].key)) {
      return buck.pairs[i].val;
    }
  }
  *success = 0;
  return -1;
}

hmap *hmap_new() {
  hmap *ret = safe_malloc(sizeof(hmap));
  ret->num_buckets = INIT_CAPACITY;
  ret->num_pairs = 0;
  ret->buckets = safe_calloc(ret->num_buckets * sizeof(bucket));
  return ret;
}

void hmap_del(hmap *map) {
  if (!map) {
    return;
  }

  for (size_t i = 0; i < map->num_buckets; i += 1) {
    free(map->buckets[i].pairs);
  }
  free(map->buckets);
  free(map);
}

size_t hash_string(char *str) {
  size_t ret = 0;
  while (*str) {
    ret *= 31;
    ret += *str;
    str++;
  }
  return ret;
}

void bucket_insert(bucket *buck, char *str, int val) {
  if (buck->capacity < buck->num_pairs + 1) {
    size_t new_size = MAX(1, 2 * buck->capacity);
    buck->capacity = new_size;
    buck->pairs = safe_realloc(buck->pairs, new_size * sizeof(pair));
  }
  buck->pairs[buck->num_pairs].key = str;
  buck->pairs[buck->num_pairs].val = val;
  buck->num_pairs += 1;
}

void expand(hmap *map) {
  bucket* old_buckets = map->buckets;
  size_t old_num_buckets = map->num_buckets;
  map->buckets = safe_calloc(map->num_buckets * 2 * sizeof(bucket));
  map->num_buckets *= 2;
  map->num_pairs = 0;
  for (size_t i = 0; i < old_num_buckets; i += 1) {
    bucket *cur_bucket = old_buckets + i;
    for (size_t j = 0; j < cur_bucket->num_pairs; j += 1)  {
      hmap_put(map, 
          cur_bucket->pairs[j].key, 
          cur_bucket->pairs[j].val);
    }
    if (cur_bucket->pairs)  {
      free(cur_bucket->pairs);
    }
  }
  free(old_buckets);
}

void hmap_increment(hmap *map, char *key, int amt) {
  int already_contains = 1;
  int val = hmap_get_extended(map, key, &already_contains);
  if (already_contains) {
    hmap_put(map, key, val + amt);
  } else {
    hmap_put(map, key, amt);
  }
}

void hmap_del_contents(hmap *map) {
  for (size_t i = 0; i < map->num_buckets; i += 1) {
    for (size_t j = 0; j < map->buckets[i].num_pairs; j += 1) {
      free(map->buckets[i].pairs[j].key);
    }
  }
}
