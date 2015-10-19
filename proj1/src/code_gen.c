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
int branch_counter = 0;
char *space = "     ";  

void get_keys(smap *map);

void emit_strings(AST *ast) {
    /* TODO: Implement me. */
  if (!data_seg_opened) {
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
  int arg_count;
  AST_lst *ast_marker;
  if (!main_seg_opened) {
    printf("\n.text\n");
    printf("\n.global main\n");
    printf("\nmain:\n");
    main_seg_opened = 1;
  }
  switch(ast->type) {
    case node_INT:
      printf("    addi $v0, $0, %s\n", ast->val);
      return;
    case node_STRING:
      printf("    la $v0, %s\n", ast->val);
      printf("\n");
      return;
    case node_VAR:
      // all global variables have an address associated with them
      printf("    la $v0, %s\n", ast->val);
      printf("    lw $v0, 0($v0)\n");
      printf("\n");
      return;
    case node_CALL:
      // count the number of arguments
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    addi $sp, $sp, -%d\n", arg_count*4);
      ast_marker = ast->children;
      arg_count = 0;
      while (ast_marker != NULL) {
        emit_main(ast_marker->val);
        printf("    sw $v0, %d($sp)\n", arg_count*4);
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    jal %s\n", ast->val);
      printf("    addi $sp, $sp, %d\n", arg_count*4);
      printf("\n");
      return;
      // needs function def hash
    case node_AND:
      // we check if the first argument is false
      // if so, jump down to return false
      // the next line checks if the next argument is false
      // if so, jump down to return false
      // branch to return true
      emit_main(ast->children->val);
      printf("    beq $v0, $0, $short_to_false%d\n", branch_counter);
      emit_main(ast->last_child->val);
      printf("    beq $v0, $0, $short_to_false%d\n", branch_counter);
      printf("    j $short_to_true%d\n", branch_counter); // $v0 contains the second value
      printf("\n");
      printf("$short_to_false%d:\n", branch_counter);
      printf("    add $v0, $0, $0\n");
      printf("    j $branch_end%d\n", branch_counter);
      printf("\n");
      printf("$short_to_true%d:\n", branch_counter);
      printf("    addi $v0, $0, 1\n");
      printf("\n");
      printf("$branch_end%d:\n", branch_counter);
      branch_counter++;
      return;
    case node_OR:
      // check if the first argument is true, and if so jump to the end
      emit_main(ast->children->val);
      printf("    bne $v0, $0, $short_to_true%d\n", branch_counter);
      emit_main(ast->last_child->val);
      printf("    beq $v0, $0, $short_to_false%d\n", branch_counter);
      printf("$short_to_true%d:\n", branch_counter);
      printf("    addi $v0, $0, 1\n");
      printf("    j $branch_end%d\n", branch_counter);
      printf("$short_to_false%d:\n", branch_counter);
      printf("    add $v0, $0, $0\n");
      printf("$branch_end%d:\n", branch_counter);
      branch_counter++;
      return;
    case node_PLUS:
      emit_main(ast->children->val);
      printf("    add $t0, $v0, $0\n");
      emit_main(ast->last_child->val);
      printf("    add $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MINUS:
      emit_main(ast->children->val);
      printf("    add $t0, $v0, $0\n");
      emit_main(ast->last_child->val);
      printf("    sub $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MUL:
      emit_main(ast->children->val);
      printf("    add $t0, $v0, $0\n");
      emit_main(ast->last_child->val);
      printf("    mul $t0, $v0\n");
      printf("    mflo $v0\n");
      printf("\n");
      return;
    case node_LT:
      emit_main(ast->children->val);
      printf("    add $t0, $v0, $0\n");
      emit_main(ast->last_child->val);
      printf("    slt $v0, $t0, $v0");
      return;
    case node_EQ:
      emit_main(ast->children->val);
      printf("    add $t0, $v0, $0\n");
      emit_main(ast->last_child->val);
      printf("    beq $t0, $v0, $short_to_true%d\n", branch_counter);
      printf("    add $v0, $0, $0\n");
      printf("    j $branch_end%d\n", branch_counter);
      printf("    $short_to_true%d:\n", branch_counter);
      printf("    addi $v0, $0, 1\n");
      printf("    $branch_end%d:\n", branch_counter);
      branch_counter++;
      return;
    case node_DIV:
      emit_main(ast->children->val);
      printf("    add $t0, $v0, $0\n");
      emit_main(ast->last_child->val);
      printf("    div $t0, $v0\n");
      printf("    mflo $v0\n");
      printf("\n");
      return;
    case node_FUNCTION:
      return;
    case node_STRUCT:
      // any main struct has allocated address space associated with it
      printf("    la $v1, %s\n", ast->val);
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        emit_main(ast->children->val);
        printf("    sw $v0, %d($v1)\n", arg_count*4);
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    mv $v0, $v1\n");
      return;
    case node_ARROW:
      return;
    case node_ASSIGN:
      // (assign n (* 2 3))
      // recurse on the last child, that should result in a value
      // in the return register
      // look up n in our global hash to get the string reference
      // move the result into the reference
      emit_main(ast->last_child->val);
      printf("    la $a0, %s\n", ast->children->val->val);
      printf("    sw $v0, 0($a0)\n");
      printf("\n");
      return;
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
