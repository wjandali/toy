#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "lexer.h"
#include "util/util.h"
#include "parser.h"

#define INIT_BUFFER_SIZE 256
#define TOTAL_TOKENS 23
#define OP_TOKENS 19

  /* node_INT */
  /* node_STRING */
  /* node_VAR */
  /* node_CALL */
  /* node_AND */
  /* node_OR */
  /* node_PLUS */
  /* node_MINUS */
  /* node_MUL */
  /* node_LT */
  /* node_EQ */
  /* node_DIV */
  /* node_FUNCTION */
  /* node_STRUCT */
  /* node_ARROW */
  /* node_ASSIGN */
  /* node_IF */
  /* node_WHILE */
  /* node_FOR */
  /* node_SEQ */
  /* node_I_PRINT */
  /* node_S_PRINT */
  /* node_READ_INT */


void init_lex(lexer *luthor) {
  luthor->file = NULL;
  luthor->buffer = NULL;
  luthor->type = token_SENTINEL;
  luthor->buff_len = 0;
}

void open_file(lexer *lex, char *filename) {
  if (lex) {
    lex->file = fopen(filename, "r");
    if (!lex->file) {
      fatal_error("Could not read input file.\n");
    }
    lex->buff_len = INIT_BUFFER_SIZE;
    /* set buffer to memory space */
    lex->buffer = safe_calloc(INIT_BUFFER_SIZE * sizeof(char));
  }
}

void close_file(lexer *lex) {
  if (lex) {
    fclose(lex->file);
    free(lex->buffer);
    lex->buff_len = 0;
    lex->buffer = NULL;
  }
}

int white(char curr_char) {
  return (curr_char == '\r' || curr_char == '\n' || curr_char == ' ' || curr_char == '\t');
}

/* function to read the next token from lc */
void read_token(lexer *lex) {
  char *keywords[] = {"and", "or", "+", "-", "*", "/", "lt", "eq", 
    "function", "struct", "arrow", "assign", "if", 
    "while", "for", "sequence", "intprint", "stringprint",
    "readint"};
  char curr_char = fgetc(lex->file);
  int i = 1;
  int keywords_left = OP_TOKENS;
  if (white(curr_char)) {
    do {
      curr_char = fgetc(lex->file);
    } while (white(curr_char) && i++);
    ungetc(curr_char, lex->file);
    read_token(lex);
    return;
  } else if (curr_char == EOF) { // skip white space
    lex->type = token_END;
  } else if (curr_char == '(') {
    lex->type = token_OPEN_PAREN;
    *lex->buffer = curr_char;
  } else if (curr_char == ')') {
    lex->type = token_CLOSE_PAREN;
    *lex->buffer = curr_char;
  } else if (curr_char == '"') {
    lex->type = token_STRING;
    do {
      *(lex->buffer + i - 1) = curr_char;
      curr_char = fgetc(lex->file);
    } while (i++ && curr_char != '"');
    *(lex->buffer + i - 1) = curr_char;
  } else if (curr_char >= '0' && curr_char <= '9') {
    lex->type = token_INT;
    do {
      *(lex->buffer + i - 1) = curr_char;
      curr_char = fgetc(lex->file);
    } while (curr_char >= '0' && curr_char <= '9' && i++);
    ungetc(curr_char, lex->file);
  } else {
    int keyword = 0;
    do {
      *(lex->buffer + i - 1) = curr_char;
      curr_char = fgetc(lex->file);
    } while (!white(curr_char) && curr_char != '(' && curr_char != ')' && i++);
    ungetc(curr_char, lex->file);
    *(lex->buffer + i) = '\0';
    for (; keywords_left >= 0; keywords_left--) {
      if (strcmp(lex->buffer, keywords[keywords_left]) == 0) {
        lex->type = token_KEYWORD;
        keyword = 1;
        break;
      }
    }
    if (!keyword) {
      if (strcmp(lex->buffer, "None") == 0) {
        *lex->buffer = '0';
        i = 1;
        lex->type = token_INT;
      } else {
        lex->type = token_NAME;
      }
    }
  }
  *(lex->buffer + i) = '\0';
  lex->buff_len = i;
  return;
}

token_type peek_type(lexer *lex) {
  if (!lex) {
    return token_SENTINEL;
  }
  if (lex->type == token_SENTINEL) {
    read_token(lex);
  }
  return lex->type;
}

char *peek_value(lexer *lex) {
  if (!lex) {
    return NULL;
  }
  if (lex->type == token_SENTINEL) {
    read_token(lex);
  }
  return lex->buffer;
}
