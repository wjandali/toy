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
/** stack space increases that happen inside the body of a function **/
int in_function = NULL;
char *space = "     ";  
char **vars;
int func_arg_count = 0;
int sp_to_ra;
int used_stack_to_ra;

void get_keys(smap *map);
void increment_func_args();
void store_v0();
void get_from_stack(char *, int);
void restore_stack();
smap *stack_map;

void emit_strings(AST *ast) {
    /* TODO: Implement me. */
  if (!data_seg_opened) {
    printf(".data\n");
    printf("$space: .asciiz \"\\n\"\n");
    data_seg_opened = 1;
  }

  AST_lst *child;
  switch(ast->type) {
    case node_STRING:
      printf("$string%d: .asciiz %s\n", label_count, ast->val);
      char *word_ptr = (char *) safe_calloc((int) log10(label_count) + 8);
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
      printf("    la   $v0, %s\n", ast->val);
      printf("\n");
      return;
    case node_VAR:
      // all global variables have an address associated with them
      printf("    la   $v0, %s\n", ast->val);
      printf("    lw   $v0, 0($v0)\n");
      printf("\n");
      return;
    case node_CALL:
      // count the number of arguments
      printf("    addi $sp, $sp, -4\n");
      printf("    sw   $ra, 0($sp)\n");
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
        printf("    sw   $v0, %d($sp)\n", arg_count*4);
        // store each argument on the stack
        arg_count++;
        ast_marker = ast_marker->next;
      }
      // store a pointer to the original stack in a0 and call the function
      printf("    jal %s\n", ast->val);
      printf("    lw   $ra, %d($sp)\n", arg_count*4);
      printf("    addi $sp, $sp, %d\n", (arg_count + 1)*4);
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
      printf("    beq  $v0, $0, $short_to_false%d\n", branch_counter);
      emit_main(ast->last_child->val);
      printf("    beq  $v0, $0, $short_to_false%d\n", branch_counter);
      printf("    j $short_to_true%d\n", branch_counter); // $v0 contains the second value
      printf("\n");
      printf("$short_to_false%d:\n", branch_counter);
      printf("    move $v0, $0\n");
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
      printf("    bne  $v0, $0, $short_to_true%d\n", branch_counter);
      emit_main(ast->last_child->val);
      printf("    beq  $v0, $0, $short_to_false%d\n", branch_counter);
      printf("$short_to_true%d:\n", branch_counter);
      printf("    addi $v0, $0, 1\n");
      printf("    j $branch_end%d\n", branch_counter);
      printf("$short_to_false%d:\n", branch_counter);
      printf("    move $v0, $0\n");
      printf("$branch_end%d:\n", branch_counter);
      branch_counter++;
      return;
    case node_PLUS:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0", 0);
      restore_stack();
      printf("    add  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MINUS:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0", 0);
      restore_stack();
      printf("    sub  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MUL:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0", 0);
      restore_stack();
      printf("    mult $t0, $v0\n");
      printf("    mflo $v0\n");
      printf("\n");
      return;
    case node_LT:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0", 0);
      restore_stack();
      printf("    slt  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_EQ:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0", 0);
      restore_stack();
      printf("    beq  $t0, $v0, $short_to_true%d\n", branch_counter);
      printf("\n");
      printf("    move $v0, $0\n");
      printf("    j $branch_end%d\n", branch_counter);
      printf("\n");
      printf("    $short_to_true%d:\n", branch_counter);
      printf("    addi $v0, $0, 1\n");
      printf("\n");
      printf("    $branch_end%d:\n", branch_counter);
      branch_counter++;
      return;
    case node_DIV:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0", 0);
      restore_stack();
      printf("    div  $t0, $v0\n");
      printf("    mflo $v0\n");
      printf("\n");
      return;
    case node_FUNCTION:
      return;
    case node_STRUCT:
      // any main struct has allocated address space associated with it
      printf("    la   $v1, %s\n", ast->val);
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        emit_main(ast_marker->val);
        printf("    sw   $v0, %d($v1)\n", arg_count*4);
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    move $v0, $v1\n");
      return;
    case node_ARROW:
      // (arrow (struct 3 2) 2) -> 2
      emit_main(ast->children->val);
      printf("    addi $t0, $0, %s\n", ast->last_child->val->val);
      printf("    addi $t1, $0, 4\n");
      printf("    mult $t0, $t1\n");
      printf("    mflo $t0\n"); // $t0 is the offset
      printf("    add  $v0, $t0, $v0\n"); // $v0 now points to the value
      printf("    lw   $v0, 0($v0)\n");
      return;
    case node_ASSIGN:
      // (assign n (* 2 3))
      // recurse on the last child, that should result in a value
      // in the return register
      // look up n in our global hash to get the string reference
      // move the result into the reference
      emit_main(ast->last_child->val);
      printf("    la   $a0, %s\n", ast->children->val->val);
      printf("    sw   $v0, 0($a0)\n");
      printf("\n");
      return;
    case node_IF:
      // (IF (predicate) (true body) (false body))
      emit_main(ast->children->val);
      printf("    beq  $v0, $0, false_body%d\n", branch_counter);
      // print the true body
      printf("true_body%d:\n", branch_counter);
      emit_main(ast->children->next->val);
      printf("    j if_end%d\n", branch_counter);
      printf("\n");
      printf("false_body%d:\n", branch_counter);
      emit_main(ast->last_child->val);
      printf("\n");
      printf("if_end%d:\n", branch_counter);
      printf("\n");
      branch_counter++;
      return;
    case node_WHILE:
      // (while (predicate) (body))
      printf("while%d:\n", branch_counter);
      printf("\n");
      emit_main(ast->children->val);
      printf("    beq  $v0, $0, loop_end%d\n", branch_counter);
      // print loop body
      printf("loop_body%d:\n", branch_counter);
      printf("\n");
      emit_main(ast->last_child->val);
      printf("j while%d\n", branch_counter);
      printf("loop_end%d:\n", branch_counter);
      printf("\n");
      branch_counter++;
      return;
    case node_FOR:
      // (for (init) (predicate) (increment) (body))
      // evaluate the init, then check the predicate
      // if predicate, jump to end
      printf("for_loop_init%d:\n", branch_counter);
      emit_main(ast->children->val);
      printf("for_loop_predicate%d:\n", branch_counter);
      printf("\n");
      emit_main(ast->children->next->val);
      printf("\n");
      printf("for_loop_check%d:\n", branch_counter);
      printf("    beq  $v0, $0, for_loop_end%d\n", branch_counter);
      printf("for_loop_eval%d:\n", branch_counter);
      emit_main(ast->last_child->val);
      printf("for_loop_inc%d:\n", branch_counter);
      emit_main(ast->children->next->next->val);
      printf("    j for_loop_predicate%d\n", branch_counter);
      printf("for_loop_end%d:\n", branch_counter);
      printf("\n");
      branch_counter++;
      return;
    case node_SEQ:
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        printf("seq_%d_%d:\n", branch_counter, arg_count);
        emit_main(ast_marker->val);
        arg_count++;
        ast_marker = ast_marker->next;
      }
      branch_counter++;
      return;
    case node_I_PRINT:
      emit_main(ast->children->val);
      printf("    move $a0, $v0\n");
      printf("    addi $v0, $0, 1\n");
      printf("    syscall\n");
      printf("    addi $v0, $0, 4\n");
      printf("    la   $a0, $space\n");
      printf("    syscall\n");
      printf("    move $v0, $0\n");
      printf("\n");
      return;
    case node_S_PRINT:
      emit_main(ast->children->val);
      printf("    move $a0, $v0\n");
      printf("    addi $v0, $0, 4\n");
      printf("    syscall\n");
      printf("    la   $a0, $space\n");
      printf("    syscall\n");
      printf("    move $v0, $0\n");
      printf("\n");
      return;
    case node_READ_INT:
      printf("    addi $v0, $0, 5\n");
      printf("    syscall\n");
      printf("\n");
      return;
    default:
      return;
  }
}

void increment_func_args(int val) {
  // ** for each argument we increment its value by val
  int t;
  for (int i = 0; i < func_arg_count; i++) {
    t = smap_get(stack_map, *(vars + i));
    smap_put(stack_map, *(vars + i), t + val);
  }
  sp_to_ra += val;
}

void add_to_func_args(char *string, int val) {
  smap_put(stack_map, string, val);
  char **t = (char **) safe_calloc((func_arg_count + 1)*sizeof(char *));
  for (int i = 0; i < func_arg_count; i++) {
    *(t + i) = *(vars + i);
  }
  *(t + func_arg_count) = string;
  func_arg_count++;
  free(vars);
  vars = t;
}

void store_v0() {
  /** CHECK THIS ONE OUT **/
  increment_func_args(1);
  printf("    addi $sp, $sp, -4\n");
  printf("    sw   $v0, 0($sp)\n");
}

void get_from_stack(char *s, int i) {
  printf("    lw   %s, %d($sp)\n", s, i*4);
}

void restore_stack() {
  increment_func_args(-1);
  printf("    addi $sp, $sp, 4\n");
}

void emit_exit() {
    printf("    li $v0 10\n");
    printf("    syscall\n");
}

void emit_functions(AST *ast) {
    /* TODO: Implement me. */
  int arg_count;
  int sp0;
  char *str;
  AST_lst *ast_marker;

  if (!in_function && ast->type != node_FUNCTION) {
    return;
  }

  switch(ast->type) {
    case node_INT:
      // vetted
      printf("    addi $v0, $0, %s\n", ast->val);
      return;
    case node_STRING:
      // vetted
      // all strings are global
      printf("    la   $v0, %s\n", ast->val);
      printf("\n");
      return;
    case node_VAR:
      printf("    lw $v0, %d($sp)\n", smap_get(stack_map, ast->val)*4);
      printf("\n");
      return;
    case node_CALL:
      //FLAG
      // count the number of arguments
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        arg_count++;
        ast_marker = ast_marker->next;
      }
      // {x, y, z}
      // (func a b c)
      ast_marker = ast->children;
      increment_func_args(arg_count + 1);
      printf("    addi $sp, $sp, -%d\n", (arg_count + 1)*4);
      arg_count = 0;
      while (ast_marker != NULL) {
        // this very well could alter the distance from the var
        emit_functions(ast_marker->val);
        printf("    sw   $v0, %d($sp)\n", (sp_to_ra - used_stack_to_ra - 1)*4);
        printf("    sw   $v0, %d($sp)\n", arg_count*4);
        used_stack_to_ra++;
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    sw   $ra, %d($sp)\n", arg_count*4);
      printf("    jal %s\n", ast->val);
      printf("    lw   $ra, %d($sp)\n", arg_count*4);
      printf("    addi $sp, $sp, %d\n", (arg_count + 1)*4);
      increment_func_args(- arg_count - 1);
      printf("\n");
      return;
    case node_AND:
      // we check if the first argument is false
      // if so, jump down to return false
      // the next line checks if the next argument is false
      // if so, jump down to return false
      //
      // branch to return true
      emit_functions(ast->children->val);
      printf("    beq  $v0, $0, $short_to_false%d\n", branch_counter);
      emit_functions(ast->last_child->val);
      printf("    beq  $v0, $0, $short_to_false%d\n", branch_counter);
      printf("    j $short_to_true%d\n", branch_counter); // $v0 contains the second value
      printf("\n");
      printf("$short_to_false%d:\n", branch_counter);
      printf("    move $v0, $0\n");
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
      emit_functions(ast->children->val);
      printf("    bne  $v0, $0, $short_to_true%d\n", branch_counter);
      emit_functions(ast->last_child->val);
      printf("    beq  $v0, $0, $short_to_false%d\n", branch_counter);
      printf("$short_to_true%d:\n", branch_counter);
      printf("    addi $v0, $0, 1\n");
      printf("    j $branch_end%d\n", branch_counter);
      printf("$short_to_false%d:\n", branch_counter);
      printf("    move $v0, $0\n");
      printf("$branch_end%d:\n", branch_counter);
      branch_counter++;
      return;
    case node_PLUS:
      emit_functions(ast->children->val);
      store_v0();
      sp0 = sp_to_ra;
      emit_functions(ast->last_child->val);
      get_from_stack("$t0", sp_to_ra - sp0);
      restore_stack();
      printf("    add  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MINUS:
      emit_functions(ast->children->val);
      store_v0();
      sp0 = sp_to_ra;
      emit_functions(ast->last_child->val);
      get_from_stack("$t0", sp_to_ra - sp0);
      restore_stack();
      printf("    sub  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MUL:
      emit_functions(ast->children->val);
      store_v0();
      sp0 = sp_to_ra;
      emit_functions(ast->last_child->val);
      get_from_stack("$t0", sp_to_ra - sp0);
      restore_stack();
      printf("    mult $t0, $v0\n");
      printf("    mflo $v0\n");
      printf("\n");
      return;
    case node_LT:
      emit_functions(ast->children->val);
      store_v0();
      sp0 = sp_to_ra;
      emit_functions(ast->last_child->val);
      get_from_stack("$t0", sp_to_ra - sp0);
      restore_stack();
      printf("    slt  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_EQ:
      emit_functions(ast->children->val);
      store_v0();
      sp0 = sp_to_ra;
      emit_functions(ast->last_child->val);
      get_from_stack("$t0", sp_to_ra - sp0);
      restore_stack();
      printf("    beq  $t0, $v0, $short_to_true%d\n", branch_counter);
      printf("\n");
      printf("    move $v0, $0\n");
      printf("    j $branch_end%d\n", branch_counter);
      printf("\n");
      printf("    $short_to_true%d:\n", branch_counter);
      printf("    addi $v0, $0, 1\n");
      printf("\n");
      printf("    $branch_end%d:\n", branch_counter);
      branch_counter++;
      return;
    case node_DIV:
      emit_functions(ast->children->val);
      store_v0();
      sp0 = sp_to_ra;
      emit_functions(ast->last_child->val);
      get_from_stack("$t0", sp_to_ra - sp0);
      restore_stack();
      printf("    div  $t0, $v0\n");
      printf("    mflo $v0\n");
      printf("\n");
      return;
    case node_FUNCTION:
      /*
       * we will have a pointer pointing to the closest empty stack space available for non-dynamic space
       * we will also have a pointer pointing to the end of the stack spce available that is non dynamic
       * we will have a counter of sp from 0 period
       *
       * the caller allocates space for ra and arguments
       * */
      stack_map = smap_new();
      in_function = 1;
      sp_to_ra = 0;
      used_stack_to_ra = 0;
      // (function (t a b c) (body))
      printf("%s:\n", ast->children->val->val);
      arg_count = 0;
      ast_marker = ast->children->val->children;
      // a function (func f (a b c) ())
      // will result in func args of count 3
      // and a: -2, b: -1, c: 0
      str = (char *) safe_calloc(4*sizeof(char));
      strcpy(str, "$ra");
      add_to_func_args(str, 0);
      while (ast_marker != NULL) {
        str = (char *) safe_calloc((strlen(ast_marker->val->val) + 1)*sizeof(char));
        strcpy(str, ast_marker->val->val);
        increment_func_args(-1);
        /* sp_to_ra = -1 */
        // n -> 0
        add_to_func_args(str, 0);
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    addi $sp, $sp, -%d\n", smap_get(stack_sizes, ast->children->val->val)*4);
      // sp_to_ra = 1
      increment_func_args(smap_get(stack_sizes, ast->children->val->val) + arg_count - 1);
      sp_to_ra = arg_count + smap_get(stack_sizes, ast->children->val->val);
      used_stack_to_ra = arg_count;
      emit_functions(ast->last_child->val);
      // so a0 points to the args i.e. is $sp to begin
      printf("    addi $sp, $sp, %d\n", smap_get(stack_sizes, ast->children->val->val)*4);
      printf("    jr $ra\n");
      in_function = 0;
      func_arg_count = 0;
      free(vars);
      vars = 0;
      smap_del_contents(stack_map);
      smap_del(stack_map);
      return;
    case node_STRUCT:
      /* CHECK THIS ONE OUT */
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        arg_count++;
        ast_marker = ast_marker->next;
      }
      // ** increment stack_map by arg_count
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        emit_functions(ast_marker->val);
        printf("    sw   $v0, %d($sp)\n", (sp_to_ra - used_stack_to_ra - 1)*4);
        used_stack_to_ra++;
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    addi   $v0, $sp, %d\n", (sp_to_ra - used_stack_to_ra - 1)*4);
      used_stack_to_ra++;
      return;
    case node_ARROW:
      // (arrow (struct 3 2) 2) -> 2
      emit_functions(ast->children->val);
      printf("    addi $t0, $0, %s\n", ast->last_child->val->val);
      printf("    addi $t1, $0, 4\n");
      printf("    mult $t0, $t1\n");
      printf("    mflo $t0\n"); // $t0 is the offset
      printf("    add  $v0, $t0, $v0\n"); // $v0 now points to the value
      printf("    lw $v0, 0($v0)\n");
      return;
    case node_ASSIGN:
      // (assign n (* 2 3))
      // recurse on the last child, that should result in a value
      // in the return register
      // look up n in our global hash to get the string reference
      // move the result into the reference
      emit_functions(ast->last_child->val);
      if (smap_get(stack_map, ast->children->val->val) != -1) {
        // this is defined variable
        // move our pointer up the total number of existing vars scoped to the function that are not arguments
        printf("    addi $a0, $sp, %d\n", smap_get(stack_map, ast->children->val->val)*4);
      } else {
        // this is a new variable
        str = (char *) safe_calloc((strlen(ast->children->val->val) + 1)*sizeof(char));
        strcpy(str, ast->children->val->val);
        add_to_func_args(str, (sp_to_ra - used_stack_to_ra - 1));
        printf("    addi $a0, $sp, %d\n", (sp_to_ra - used_stack_to_ra - 1)*4);
        used_stack_to_ra++;
      }
      printf("    sw   $v0, 0($a0)\n");
      printf("\n");
      return;
    case node_IF:
      // (IF (predicate) (true body) (false body))
      emit_functions(ast->children->val);
      printf("    beq  $v0, $0, false_body%d\n", branch_counter);
      // print the true body
      printf("true_body%d:\n", branch_counter);
      emit_functions(ast->children->next->val);
      printf("    j if_end%d\n", branch_counter);
      printf("\n");
      printf("false_body%d:\n", branch_counter);
      emit_functions(ast->last_child->val);
      printf("\n");
      printf("if_end%d:\n", branch_counter);
      printf("\n");
      branch_counter++;
      return;
    case node_WHILE:
      // (while (predicate) (body))
      printf("while%d:\n", branch_counter);
      printf("\n");
      emit_functions(ast->children->val);
      printf("    beq  $v0, $0, loop_end%d\n", branch_counter);
      // print loop body
      printf("loop_body%d:\n", branch_counter);
      printf("\n");
      emit_functions(ast->last_child->val);
      printf("j while%d\n", branch_counter);
      printf("loop_end%d:\n", branch_counter);
      printf("\n");
      branch_counter++;
      return;
    case node_FOR:
      // (for (init) (predicate) (increment) (body))
      // evaluate the init, then check the predicate
      // if predicate, jump to end
      printf("for_loop_init%d:\n", branch_counter);
      emit_functions(ast->children->val);
      printf("for_loop_predicate%d:\n", branch_counter);
      printf("\n");
      emit_functions(ast->children->next->val);
      printf("\n");
      printf("for_loop_check%d:\n", branch_counter);
      printf("    beq  $v0, $0, for_loop_end%d\n", branch_counter);
      printf("for_loop_eval%d:\n", branch_counter);
      emit_functions(ast->last_child->val);
      printf("for_loop_inc%d:\n", branch_counter);
      emit_functions(ast->children->next->next->val);
      printf("    j for_loop_predicate%d\n", branch_counter);
      printf("for_loop_end%d:\n", branch_counter);
      printf("\n");
      branch_counter++;
      return;
    case node_SEQ:
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        printf("seq_%d_%d:\n", branch_counter, arg_count);
        emit_functions(ast_marker->val);
        arg_count++;
        ast_marker = ast_marker->next;
      }
      branch_counter++;
      return;
    case node_I_PRINT:
      emit_functions(ast->children->val);
      printf("    move $a0, $v0\n");
      printf("    addi $v0, $0, 1\n");
      printf("    syscall\n");
      printf("    addi $v0, $0, 4\n");
      printf("    la   $a0, $space\n");
      printf("    syscall\n");
      printf("    move $v0, $0\n");
      printf("\n");
      return;
    case node_S_PRINT:
      emit_functions(ast->children->val);
      printf("    move $a0, $v0\n");
      printf("    addi $v0, $0, 4\n");
      printf("    syscall\n");
      printf("    la   $a0, $space\n");
      printf("    syscall\n");
      printf("    move $v0, $0\n");
      printf("\n");
      return;
    case node_READ_INT:
      printf("    addi $v0, $0, 5\n");
      printf("    syscall\n");
      printf("\n");
      return;
    default:
      return;
  }
}
