#include "parser.h"
#include "lexer.h"
#include "hmap.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/** An array of the different string values of keywords. */
char *keywords[] = {"and", "or", "+", "-", "*", "/", "lt", "eq", 
  "function", "struct", "arrow", "assign", "if", 
  "while", "for", "sequence", "intprint", "stringprint",
  "readint"};
/** Sister array of keywords. Keeps track of the corresponding enum. */
int enums[] = {node_AND, node_OR, node_PLUS, node_MINUS, node_MUL, node_DIV,
  node_LT, node_EQ, node_FUNCTION, node_STRUCT, 
  node_ARROW, node_ASSIGN, node_IF, node_WHILE, node_FOR, 
  node_SEQ, node_I_PRINT, node_S_PRINT, node_READ_INT};

int args_length[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, -2, 2, 2, 3, 2, 4, -2, 1, 1, 0};
/** A hashmap used for more efficient lookups of (keyword, enum) pairs. */
smap *keyword_str_to_enum;
smap *keyword_str_to_args_length;
smap *func_decls;
int struct_count = 1;

/** Initializes keyword_str_to_enum so that it contains a map
 *  from the string value of a keyword to its corresponding enum. */

/** A hash of one-level-deep assignments **/

void initialize_keyword_to_enum_mapping();

void init_extras() {
  if (!(smap_get(keyword_str_to_args_length, keywords[0]) == -1)) {
    return;
  }
  keyword_str_to_args_length = smap_new();
  size_t num_keywords = sizeof(enums) / sizeof(int);
  for (size_t i = 0; i < num_keywords; i += 1) {
    smap_put(keyword_str_to_args_length, keywords[i], args_length[i]);
  }
}

void check_sub_tree(AST *ptr) {
  if (ptr == NULL) {
    return;
  }
  int arg_l;
  AST_lst *child = ptr->children;
  int length = 0; // number of children
  while (child != NULL) {
    check_sub_tree(child->val);
    length++;
    child = child->next;
  }

  switch(ptr->type) {
    case node_AND:
    case node_OR:
    case node_PLUS:
    case node_MINUS:
    case node_MUL:
    case node_DIV:
    case node_LT:
    case node_EQ:
    case node_FUNCTION:
    case node_STRUCT:
    case node_ASSIGN:
    case node_IF:
    case node_ARROW:
    case node_WHILE:
    case node_FOR:
    case node_SEQ:
    case node_I_PRINT:
    case node_S_PRINT:
    case node_READ_INT:
      arg_l = smap_get(keyword_str_to_args_length, ptr->val);
      if (arg_l >= 0 && arg_l != length) {
        break;
      }
      return;
    case node_VAR:
    case node_INT:
    case node_STRING:
      if (length) { // if the length is not zero
        break;
      }
      return;
    case node_CALL:
      return;
  }
  fatal_error("wrong number of args");
}

void parse_init() {
  decls = smap_new();
  stack_sizes = smap_new();
  num_args = smap_new();
  strings = smap_new();
  keyword_str_to_enum = smap_new();
  func_decls = smap_new();
}

void parse_close() {
  smap_del_contents(decls);
  smap_del(decls);
  smap_del(stack_sizes);
  smap_del(num_args);
  smap_del(strings);
  smap_del(keyword_str_to_enum);
}

AST *build_ast (lexer *lex) {
  /* build a tree of tokens */
  token_type curr_char = peek_type(lex);
  AST *tree = (AST *) safe_calloc(sizeof(AST));
  tree->val = safe_calloc((lex->buff_len)*sizeof(char));
  AST *child;
  switch(curr_char) {
    /* begin by inspecting the current character */
    /* we have the following token types: */
    /* int, string, name, keyword, open paren, closed parent, end, sentinel */
    /* all trees must begin with an open paren, recursing may see a string, a name, or an int */
    case token_OPEN_PAREN:
      read_token(lex);
      if (lex->type == token_NAME ) {
        tree->type = node_CALL;
      } else {
        tree->type = lookup_keyword_enum(lex->buffer);
        if (tree->type == -1) {
          fatal_error("undefined type in parenthesis");
        }
      }
      strcpy(tree->val, lex->buffer);
      read_token(lex);

      while(peek_type(lex) != token_CLOSE_PAREN && peek_type(lex) != token_END) { 
        child = build_ast(lex);
        if (!tree->last_child) {
          tree->last_child = (AST_lst *) safe_calloc(sizeof(AST_lst));
          tree->last_child->val = child;
          tree->children = tree->last_child;
        } else {
          tree->last_child->next = (AST_lst *) safe_calloc(sizeof(AST_lst));
          tree->last_child->next->val = child;
          tree->last_child = tree->last_child->next;
        }
      }
      if (peek_type(lex) == token_END) {
        fatal_error("unexpected token end");
      }
      break;
    case token_INT:
      tree->type = node_INT;
      strcpy(tree->val, lex->buffer);
      break;
    case token_STRING:
      tree->type = node_STRING;
      strcpy(tree->val, lex->buffer);
      break;
    case token_NAME:
      tree->type = node_VAR;
      strcpy(tree->val, lex->buffer);
      break;
    case token_KEYWORD:
      fatal_error("token must be wrapped in parentheses");
    case token_END:
      free(tree->val);
      free(tree);
      return NULL;
    case token_CLOSE_PAREN:
      fatal_error("cannot parse closed parenthesis");
    default: 
      fatal_error("error");
  }
  read_token(lex); // get the next token
  return tree;
}

void free_ast (AST *ptr) {
  /* TODO: Implement me. */
  AST_lst *child = ptr->children;
  while (child != NULL) {
    free_ast(child->val);
    child = child->next;
  }
  if (ptr->last_child) {
    free(ptr->last_child);
  }
  if (ptr->children && ptr->children != ptr->last_child) {
    free(ptr->children);
  }
  free(ptr->val);
  free(ptr);
}

void check_tree_shape(AST *ptr) {
  /* TODO: Implement me. */
  /* Hint: This function is just asking to be table-driven */
  /* go down the tree (left to right) and for each node check the children length */
  init_extras();
  check_sub_tree(ptr);
}

void gather_decls(AST *ast, char *env, int is_top_level) {
  /* our first iteration only allows functions to be defined at the top level */
  /* this means an assignment can occur at the top level or in a function, but not deeper */
  /* typedef enum node_type { */
  /*   node_INT * integer literal *, */
  /*   node_STRING * string literal*, */
  /*   node_VAR * Name of a variable or a function. *, */
  /*   node_CALL * A call to a function *, */
  /*   * The different built-in utilitites. * */
  /*   node_AND, node_OR, node_PLUS, node_MINUS, node_MUL, */
  /*   node_LT, node_EQ, node_DIV, node_FUNCTION,  */
  /*   node_STRUCT, node_ARROW, node_ASSIGN, node_IF,  */
  /*   node_WHILE, node_FOR, node_SEQ, node_I_PRINT,  */
  /*   node_S_PRINT, node_READ_INT} node_type; */
  AST_lst *child;
  AST_lst *last_child;
  AST_lst *args;
  char *struct_p;
  switch(ast->type) {
    /* no checks needed in the case of constants */
    case node_INT:
    case node_STRING:
      // a string outside an assignment
      return;
    case node_STRUCT:
      if (is_top_level) {
        struct_p = (char *) safe_calloc(((int) log10(struct_count) + 9)*sizeof(char));
        sprintf(struct_p, "$struct%d", struct_count);
        sprintf(ast->val, "$struct%d", struct_count);
        struct_count++;
        smap_put(decls, struct_p, AST_lst_len(ast->children)); 
      }
      return;
    case node_ASSIGN:
      last_child = ast->last_child;
      gather_decls(last_child->val, env, is_top_level);
      if (is_top_level) {
        smap_put(decls, ast->children->val->val, 1);
      } else {
        smap_put(func_decls, ast->children->val->val, 1);
      }
      return;
    case node_CALL:
      struct_p = (char *) safe_calloc((strlen(ast->val) + 7)*sizeof(char));
      sprintf(struct_p, "$func_%s", ast->val);
      if (smap_get(decls, struct_p) == -1) {
        fatal_error("function undefined");
      }
      free(struct_p);
      child = ast->children;
      while (child != NULL) {
        gather_decls(child->val, env, 1);
        child = child->next;
      }
      return;
    case node_VAR:
      if (is_top_level) {
        if (smap_get(decls, ast->val) == -1) {
          fatal_error("unassigned variable");
        }
      } else {
        if (smap_get(func_decls, ast->val) == -1 && smap_get(decls, ast->val) == -1) {
          fatal_error("unassigned variable");
        }
      }
      return;
    /* function definition */
    case node_FUNCTION:
      if (!is_top_level) {
        fatal_error("no higher order functions");
      }
      /* (function (func arg1 arg2..) (* arg1 arg2)) */
      args = ast->children->val->children; //
      /* set func at the top level */
      struct_p = (char *) safe_calloc((strlen(ast->children->val->val) + 7)*sizeof(char));
      sprintf(struct_p, "$func_%s", ast->children->val->val);
      smap_put(decls, struct_p, 1); // set func name
      while (args != NULL) { 
        smap_put(func_decls, args->val->val, 1); // set function level assignment for each arg
        args = args->next;
      }
      /* recurse on the body */
      gather_decls(ast->last_child->val, env, 0);
      /* clear function-level assignments */
      smap_del_contents(func_decls);
      return;
    /* simple functions */
    default:
      child = ast->children;
      while (child != NULL) {
        gather_decls(child->val, env, is_top_level);
        child = child->next;
      }
      return;
  }
}

node_type lookup_keyword_enum(char *str) {
  if (smap_get(keyword_str_to_enum, keywords[0]) == -1) {
    initialize_keyword_to_enum_mapping();
  }
  return smap_get(keyword_str_to_enum, str);
}

void initialize_keyword_to_enum_mapping() {
  /* Note that enums is an *array*, not a pointer, so this
   * sizeof business is reasonable. */
  size_t num_keywords = sizeof(enums) / sizeof(int);
  for (size_t i = 0; i < num_keywords; i += 1) {
    smap_put(keyword_str_to_enum, keywords[i], enums[i]);
  }
}

size_t AST_lst_len(AST_lst *lst) {
  int num_fields = 0;
  while (lst) {
    num_fields += 1;
    lst = lst->next;
  }
  return num_fields;
}


smap *decls;
smap *stack_sizes;
smap *num_args;
smap *strings;
