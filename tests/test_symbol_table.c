#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "symbol_tables.h"
#include "cputime.h"

static char buffer[1000];
static char **words;
static uint32_t n_words;
static uint32_t size_words;
static stbl_t sym_table;


static void words_from_file(char *filename) {
  FILE *f;
  char *str, *tmp;
  uint32_t i, n, len;

  size_words = 0;
  n_words = 0;

  f = fopen(filename, "r");
  if (f == NULL) {
    perror(filename);
    return;
  }

  i = 0;
  n = 100;
  words = (char **) malloc(n * sizeof(char *));
  size_words = n;

  buffer[99] = '\0';
  buffer[100] = '\0';
  str = fgets(buffer, 99, f);
  while (str != NULL) {
    len = strlen(str);
    // remove \n if present
    if (str[len - 1] == '\n') {
      len --;
      str[len] = '\0';
    }
    tmp = (char *) malloc(len + 1);
    strcpy(tmp, str);
    if (i == n) {
      n += 100;
      words = (char **) realloc(words, n * sizeof(char *));
      size_words = n;
    }
    words[i] = tmp;
    i ++;

    str = fgets(buffer, 99, f);
  }
  fclose(f);

  n_words = i;
}

static void clear_words() {
  uint32_t j;

  for (j=0; j<n_words; j++) free(words[j]);
  free(words);  
}


/*
 * to test the iterator: print each record
 */
static void print_stbl_record(void *aux, stbl_rec_t *r) {
  printf("record %p: [hash = %08"PRIx32", val = %"PRId32", string = %s, next = %p]\n",
	 r, r->hash, r->value, r->string, r->next);
}

static void print_stbl_records(stbl_t *table) {
  printf("--- Table %p ---\n", table);
  stbl_iterate(table, NULL, print_stbl_record);
  printf("---\n\n");
}


int main() {
  uint32_t i, n;
  double runtime;
  int32_t x, *val;

  init_stbl(&sym_table, 0);
  
  printf("\n*** Initial table ***\n");
  print_stbl_records(&sym_table);  

  words_from_file("data2.txt");
  val = (int32_t *) malloc(n_words * sizeof(int32_t));

  for (i=0; i<n_words; i++) {
    x = stbl_find(&sym_table, words[i]);
    if (x < 0) {
      stbl_add(&sym_table, words[i], i);
      val[i] = i;
    } else {
      val[i] = x;
    }
  }

  //  printf("\n--- checking ---\n");
  for (i=0; i<n_words; i++) {
    x = stbl_find(&sym_table, words[i]);
    if (x != val[i]) {
      printf("*** Error: %s, val = %"PRId32", should be %"PRId32" ***\n", words[i], x, val[i]);
    }
  }

  printf("\n*** Added data2.txt ***\n");
  print_stbl_records(&sym_table);  


  // test reset
  reset_stbl(&sym_table);

  printf("\n*** After reset ***\n");
  print_stbl_records(&sym_table);  

  //  printf("\n--- checking ---\n");
  for (i=0; i<n_words; i++) {
    x = stbl_find(&sym_table, words[i]);
    if (x >= 0) {
      printf("*** Error: %s, val = %"PRId32", should be -1 ***\n", words[i], x);
    }
  }


  // rebuild 
  for (i=0; i<n_words; i++) {
    x = stbl_find(&sym_table, words[i]);
    if (x < 0) {
      stbl_add(&sym_table, words[i], i);
      val[i] = i;
    } else {
      val[i] = x;
    }
  }
  
  x = stbl_find(&sym_table, "");
  if (x >= 0) {
    printf("*** Error: <empty string>, val = %"PRId32", should be -1\n", x);
  }

  x = stbl_find(&sym_table, "####61723####");
  if (x >= 0) {
    printf("*** Error: ####61723####, val = %"PRId32", should be -1\n", x);
  }

  x = stbl_find(&sym_table, "bbbbbbbbb");
  if (x >= 0) {
    printf("*** Error: bbbbbbbbb, val = %"PRId32", should be -1\n", x);
  }

  printf("\n*** After rebuild ***\n");
  print_stbl_records(&sym_table);  
    

  printf("\n--- overwriting ---\n");
  for (i=10; i<n_words/5; i ++) {
  printf("adding %s: new val = %"PRIu32"\n", words[i], 999999 - i);
    stbl_add(&sym_table, words[i], 999999 - i);
  }

  printf("\n*** After overwriting ***\n");
  print_stbl_records(&sym_table);  
    
  printf("\n--- checking ---\n");
  for (i=0; i<n_words; i++) {
    printf("checking %s: val = %"PRId32"\n", words[i], stbl_find(&sym_table, words[i]));
  }  

  printf("\n--- removing ---\n");
  for (i=0; i<n_words; i++) {
    stbl_remove(&sym_table, words[i]);
  }

  printf("\n*** After removing ***\n");
  print_stbl_records(&sym_table);  
    
  printf("\n--- checking ---\n");
  for (i=0; i<n_words; i++) {
    printf("checking %s: val = %"PRId32"\n", words[i], stbl_find(&sym_table, words[i]));
  }  

  printf("\n--- removing all ---\n");
  for (i=0; i<n_words; i++) {
    stbl_remove(&sym_table, words[i]);
  }

  printf("\n*** After removing all ***\n");
  print_stbl_records(&sym_table);  
    
  printf("\n--- checking ---\n");
  for (i=0; i<n_words; i++) {
    printf("checking %s: val = %"PRId32"\n", words[i], stbl_find(&sym_table, words[i]));
  }  

  for (i=0; i<n_words; i++) {
    printf("adding %s: val = %"PRIu32"\n", words[i], i);
    stbl_add(&sym_table, words[i], i);
  }

  printf("\n--- checking ---\n");
  for (i=0; i<n_words; i++) {
    printf("checking %s: val = %"PRId32"\n", words[i], stbl_find(&sym_table, words[i]));
  }



  // speed test
  runtime = get_cpu_time();
  for (n=0; n<100000; n++) {
    for (i=0; i<n_words; i++) {
      x = stbl_find(&sym_table, words[i]);
    }
  }
  runtime = get_cpu_time() - runtime;
  printf("Reading %"PRIu32" words\n", (uint32_t) 100000 * n_words);
  printf("Runtime: %.4f s\n", runtime);

  clear_words();
  free(val);
  delete_stbl(&sym_table);

  return 0;
}
