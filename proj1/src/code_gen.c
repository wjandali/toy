#include "code_gen.h"
#include "parser.h"
#include <stdlib.h>
#include <math.h>

/** A counter to prevent the issuance of duplicate labels. */
unsigned label_count = 1;
/** True iff the data segment has already been partially printed. */
int data_seg_opened = 0;
/** True iff the text segment has already been partially printed. */
int text_seg_opened = 0;
char *space = "   ";  

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
  AST_lst *child;
  switch(ast->type) {
    case node_ASSIGN:
      child = ast->children;
      printf("$%s: .word 0\n", child->next->val->val); // print the label for the var
      child = child->next;
      while (child != NULL) {
        emit_static_memory(child->val);
        child = child->next;
      }
      label_count++;
      return;
    case node_STRUCT:
      child = ast->children;
      printf("$struct%d: .word ", label_count); // print the label for the var
      label_count++;
      while (child != NULL) {
        emit_static_memory(child->val);
        child = child->next;
        printf("0 ");
      }
      printf("\n");
      return;
    case node_CALL:
    case node_FUNCTION:
      return;
    default:
      return;
  }
}

void emit_main(AST *ast) {
    /* TODO: Implement me. */
}

void emit_exit() {
    printf("    li $v0 10\n");
    printf("    syscall\n");
}

void emit_functions(AST *ast) {
    /* TODO: Implement me. */
}
