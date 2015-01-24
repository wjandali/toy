#include "parser.h"
#include "lexer.h"
#include <string.h>
#include <stdlib.h>

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
/** A hashmap used for more efficient lookups of (keyword, enum) pairs. */
smap *keyword_str_to_enum;


/** Initializes keyword_str_to_enum so that it contains a map
 *  from the string value of a keyword to its corresponding enum. */
void initialize_keyword_to_enum_mapping();


void parse_init() {
    decls = smap_new();
    stack_sizes = smap_new();
    num_args = smap_new();
    strings = smap_new();
    keyword_str_to_enum = smap_new();
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
    /* TODO: Implement me. */
    /* Hint: switch statements are pretty cool, and they work 
     *       brilliantly with enums. */
    token_type curr_char = peek_type(lex);
    AST *tree = (AST *) malloc(sizeof(AST));
    switch(curr_char) {
      case token_OPEN_PAREN:
        read_token(lex);
        tree->type = lookup_keyword_enum(lex->buffer);// not right, we could have a defined function rather than keyword
        tree->val = malloc(lex->buff_len);
        strcpy(tree->val, lex->val);

        AST_lst *children = (AST_lst *) malloc(sizeof(AST_lst));
        tree->children = children;
        tree->last_child = children;
        read_token(lex);
        AST *child = build_ast(lex);
        tree->children->val = child; //does this deref right?
        tree->children->next = NULL;
        while (peek_type(lex) != token_CLOSE_PAREN) {
          child = build_ast(lex);
          tree->last_child->next = (AST_lst *) malloc(sizeof(AST_lst));
          tree->last_child->next->val = build_ast(lex);
          tree->last_child->next->next = NULL;
          tree->last_child = tree->last_child->next;
        }
      case token_INT:
        return;
    }
}

void free_ast (AST *ptr) {
    /* TODO: Implement me. */
}

void check_tree_shape(AST *ptr) {
    /* TODO: Implement me. */
    /* Hint: This function is just asking to be table-driven */
}

void gather_decls(AST *ast, char *env, int is_top_level) {
    /* TODO: Implement me. */
    /* Hint: switch statements are pretty cool, and they work 
     *       brilliantly with enums. */
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
