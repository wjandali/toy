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
int dynamic_stackspace = 0;
int in_function = NULL;
char *space = "     ";  

void get_keys(smap *map);
void store_v0();
void get_from_stack(char *);
void restore_stack();
smap *func_var_map;
smap *func_arg_map;

void emit_strings(AST *ast) {
    /* TODO: Implement me. */
  func_arg_map = smap_new();
  func_var_map = smap_new();
  if (!data_seg_opened) {
    printf(".data\n");
    printf("$space: .asciiz \"\\n\"\n");
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
      printf("    add $a0, $sp, $0\n");
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
      get_from_stack("$t0");
      restore_stack();
      printf("    add  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MINUS:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0");
      restore_stack();
      printf("    sub  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MUL:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0");
      restore_stack();
      printf("    mult $t0, $v0\n");
      printf("    mflo $v0\n");
      printf("\n");
      return;
    case node_LT:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0");
      restore_stack();
      printf("    slt  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_EQ:
      emit_main(ast->children->val);
      store_v0();
      emit_main(ast->last_child->val);
      get_from_stack("$t0");
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
      get_from_stack("$t0");
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

void store_v0() {
  printf("    addi $sp, $sp, -4\n");
  printf("    sw   $v0, 0($sp)\n");
  dynamic_stackspace++;
}

void get_from_stack(char *s) {
  printf("    lw   %s, 0($sp)\n", s);
}

void restore_stack() {
  printf("    addi $sp, $sp, 4\n");
  dynamic_stackspace--;
}

void emit_exit() {
    printf("    li $v0 10\n");
    printf("    syscall\n");
}

void emit_functions(AST *ast) {
    /* TODO: Implement me. */
  int arg_count;
  int offset;
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
      if (smap_get(func_arg_map, ast->val) != -1) {
        // this is an argument
        // move our pointer up the total number of existing vars scoped to the function that are not arguments
        // add dynamic stack space and get the arg number
        offset = (key_count(func_var_map) + smap_get(func_arg_map, ast->val) + dynamic_stackspace)*4;
        printf("    lw $v0, %d($sp)\n", offset);
      } else if (smap_get(func_var_map, ast->val) != -1) {
        // this is a variable scoped to the function
        printf("prob");
        printf("    addi $a0, $sp, %d\n", 4*(key_count(func_var_map) - smap_get(func_var_map, ast->val) -1));
      }
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
      // the dynamic stack space needs to increase with the sp increase
      dynamic_stackspace = dynamic_stackspace + arg_count + 1;
      printf("    addi $sp, $sp, -%d\n", (arg_count + 1)*4);
      ast_marker = ast->children;
      arg_count = 0;
      while (ast_marker != NULL) {
        emit_functions(ast_marker->val);
        printf("    sw   $v0, %d($sp)\n", arg_count*4);
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    sw   $ra, %d($sp)\n", arg_count*4);
      printf("    add $a0, $sp, $0\n");
      printf("    jal %s\n", ast->val);
      printf("    lw   $ra, %d($sp)\n", arg_count*4);
      printf("    addi $sp, $sp, %d\n", (arg_count + 1)*4);
      printf("\n");
      dynamic_stackspace = dynamic_stackspace - arg_count - 1;
      return;
      // needs function def hash
    case node_AND:
      // we check if the first argument is false
      // if so, jump down to return false
      // the next line checks if the next argument is false
      // if so, jump down to return false
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
      emit_functions(ast->last_child->val);
      get_from_stack("$t0");
      restore_stack();
      printf("    add  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MINUS:
      emit_functions(ast->children->val);
      store_v0();
      emit_functions(ast->last_child->val);
      get_from_stack("$t0");
      restore_stack();
      printf("    sub  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_MUL:
      emit_functions(ast->children->val);
      store_v0();
      emit_functions(ast->last_child->val);
      get_from_stack("$t0");
      restore_stack();
      printf("    mult $t0, $v0\n");
      printf("    mflo $v0\n");
      printf("\n");
      return;
    case node_LT:
      emit_functions(ast->children->val);
      store_v0();
      emit_functions(ast->last_child->val);
      get_from_stack("$t0");
      restore_stack();
      printf("    slt  $v0, $t0, $v0\n");
      printf("\n");
      return;
    case node_EQ:
      emit_functions(ast->children->val);
      store_v0();
      emit_functions(ast->last_child->val);
      get_from_stack("$t0");
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
      emit_functions(ast->last_child->val);
      get_from_stack("$t0");
      restore_stack();
      printf("    div  $t0, $v0\n");
      printf("    mflo $v0\n");
      printf("\n");
      return;
    case node_FUNCTION:
      dynamic_stackspace = 0;
      in_function = 1;
      // (function (t a b c) (body))
      printf("%s:\n", ast->children->val->val);
      arg_count = 0;
      ast_marker = ast->children->val->children;
      // each var is stored in the hash beside its offset
      while (ast_marker != NULL) {
        str = (char *) safe_calloc((strlen(ast_marker->val->val))*sizeof(char));
        strcpy(str, ast_marker->val->val);
        smap_put(func_arg_map, str, arg_count);
        arg_count++;
        ast_marker = ast_marker->next;
      }
      emit_functions(ast->last_child->val);
      // so a0 points to the args i.e. is $sp to begin
      smap_del(func_var_map);
      smap_del(func_arg_map);
      func_arg_map = smap_new();
      func_var_map = smap_new();
      printf("    addi $sp, $sp, %d\n", dynamic_stackspace*4);
      printf("    jr $ra\n");
      in_function = 0;
      return;
    case node_STRUCT:
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    addi   $sp, $sp, -%d\n", arg_count*4);
      arg_count = 0;
      ast_marker = ast->children;
      while (ast_marker != NULL) {
        emit_functions(ast_marker->val);
        printf("    sw   $v0, %d($sp)\n", arg_count*4);
        arg_count++;
        ast_marker = ast_marker->next;
      }
      printf("    addi   $v0, $sp, %d\n", arg_count*4);
      dynamic_stackspace = dynamic_stackspace + arg_count;
      return;
    case node_ARROW:
      // (arrow (struct 3 2) 2) -> 2
      emit_main(ast->children->val);
      printf("    addi $t0, $0, %s\n", ast->last_child->val->val);
      printf("    addi $t1, $0, 4\n");
      printf("    mult $t0, $t1\n");
      printf("    mflo $t0\n"); // $t0 is the offset
      printf("    add  $v0, $t0, $v0\n"); // $v0 now points to the value
      printf("    $v0, 0($v0)\n");
      return;
    case node_ASSIGN:
      // (assign n (* 2 3))
      // recurse on the last child, that should result in a value
      // in the return register
      // look up n in our global hash to get the string reference
      // move the result into the reference
      emit_functions(ast->last_child->val);
      if (smap_get(func_arg_map, ast->children->val->val) != -1) {
        // this is an argument
        // move our pointer up the total number of existing vars scoped to the function that are not arguments
        printf("    addi $a0, $sp, %d\n", 4*key_count(func_var_map));
        printf("    addi $a0, $a0, %d\n", 4*(key_count(func_arg_map)- smap_get(func_arg_map, ast->children->val->val) - 1));
      } else if (smap_get(func_var_map, ast->children->val->val) != -1) {
        // this is a variable scoped to the function
        printf("    addi $a0, $sp, %d\n", 4*(key_count(func_var_map)- smap_get(func_var_map, ast->children->val->val) -1));
      } else {
        // this is a new variable
        dynamic_stackspace++;
        printf("    addi $sp, $sp, -4\n");
        printf("    add   $a0, $sp, $0\n");
        str = (char *) safe_calloc((strlen(ast_marker->val->val))*sizeof(char));
        strcpy(str, ast_marker->val->val);
        smap_put(func_var_map, str, key_count(func_var_map));
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
