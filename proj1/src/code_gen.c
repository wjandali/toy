#include "code_gen.h"
#include "parser.h"
#include <stdlib.h>

/** A counter to prevent the issuance of duplicate labels. */
unsigned label_count = 1;
/** True iff the data segment has already been partially printed. */
int data_seg_opened = 0;
/** True iff the text segment has already been partially printed. */
int text_seg_opened = 0;
int main_seg_opened = 0;
char *space = "   ";  

void get_keys(smap *map);

void emit_strings(AST *ast) {
    /* TODO: Implement me. */
  if (!data_seg_opened) {
    printf("%s", space);
    printf(".data\n");
    data_seg_opened = 1;
  }

  AST_lst *child;
  switch(ast->type) {
    case node_STRING:
      printf("$string%d: .asciiz %s\n", label_count, ast->val);
      char *word_ptr = (char *) safe_calloc((int) log10(label_count) + 7);
      sprintf(word_ptr, "$string%d", label_count);
      strcpy(ast->val, word_ptr);
      free(word_ptr);
      label_count++;
      return;
    default:
      child = ast->children;
      while (child != NULL) {
        emit_strings(child->val);
        child = child->next;
      }
      return;
  }
}

void emit_static_memory() {
    /* TODO: Implement me. */
  /* strat is to go down each tree, if a function call is reached we return (otherwise go down), if a var is detected we have to mark it, if a struct is created we mark it and allocate  the space there for it and fill it, ints as well  */
  /*  */
  /*   (printint 2) */
  /*   (assign n 4) */
  /*   (printint n) */
  /*   (assign n (struct 3 5 n "fa")) */
  /*   int1 2 */
  /*   int2 4 */
  /*   n 0 */
  /*   int3 3 */
  /*   int5 5 */
  /*   i have a hashmap of vars to check if they exist, if struct refers to one or one is encountered we can throw, */
  /*     and vars are labeled $VAR */
  /*   so we have struct1 int3 int5 $n string1 */
  get_keys(decls);
}


void emit_main(AST *ast) {
    /* TODO: Implement me. */
  /* for any ast that isn't  function we execute it */
  if (!main_seg_opened) {
    printf("\n%s", space);
    printf(".text\n");
    printf("%s", space);
    printf(".global main\n");
    printf("main:\n");
    main_seg_opened = 1;
  }
  switch(ast->type) {
    case node_INT:
    case node_STRING:
    case node_VAR:
    case node_CALL:
    case node_AND:
    case node_OR:
    case node_PLUS:
    case node_MINUS:
    case node_MUL:
    case node_LT:
    case node_EQ:
    case node_DIV:
    case node_FUNCTION:
    case node_STRUCT:
    case node_ARROW:
    case node_ASSIGN:
    case node_IF:
    case node_WHILE:
    case node_FOR:
    case node_SEQ:
    case node_I_PRINT:
    case node_S_PRINT:
    case node_READ_INT:
      return;
    default:
      return;
  }
}

void emit_exit() {
    printf("    li $v0 10\n");
    printf("    syscall\n");
}

void emit_functions(AST *ast) {
    /* TODO: Implement me. */
}
