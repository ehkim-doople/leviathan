#ifndef	__HASH_H__
#define	__HASH_H__



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASHSIZE 30

struct hash {
  struct nlist **hashtab;
  int size;
};



struct nlist {
   struct nlist *next;
   char *key;
   char *value;
};



#ifdef	__cplusplus
extern "C" {
#endif



struct hash* hash_new(int size);
unsigned int hashfunc(struct hash *h, char *s);
struct nlist *hash_lookup(struct hash *h, char *s);
struct nlist *hash_add(struct hash *h, char *key, char *val);
char * hash_string(struct hash *h);
void hash_empty();
void hash_destroy(struct hash *h);

#ifdef	__cplusplus
}
#endif


#endif	//__HASH_H__

