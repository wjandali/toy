#include "lexer.h"
#include <stdio.h>
int strcmp(char *, char *);

char * tokens_to_string[] = { "INT",
  "STRING", "NAME", "KEYWORD", "OPEN", "CLOSE", "END", "SENTINEL"};


int TOKEN_COUNT = 8;

int main(int argc, char * argv[]) {
  int i;
  lexer lex;
  init_lex(&lex);
  open_file(&lex, argv[1]);
  while ((i = peek_type(&lex)) != token_END) {
    read_token(&lex);
    printf("%s\n", tokens_to_string[i]);
  }
}
