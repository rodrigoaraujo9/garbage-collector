int main(int argc, char* argv[] ) {
    int VM_threshold;
    int VM_loop_counter;
    int VM_roots_size;
    int VM_stack_size;
    int VM_stack_top;
    int VM_stack[];
    char VM_heap[];
    char VM_program[];
    char VM_roots[];
    /* initialize threshold */
    VM_threshold = atoi(argv[1]);
    /* initialize roots */
    VM_roots_size = atoi(argv[2]);
    VM_roots = (tree*)malloc(VM_roots_size * sizeof(tree));
    for(int i = 0; i < roots_size; i++)
    VM_roots[i] = new_empty_tree();
    /* initialize stack */
    VM_stack_size = atoi(argv[3]);
    VM_stack = (int*)malloc(VM_stack_size * sizeof(int));
    VM_stack_top = 0;
    /* initialize heap */
    VM_heap = ...;
    /* initialize program */
    #define PAD 0x00
    char VM_program[] = {
        0x01, 0xc4,
        0x05, 0x14,
        0x06, PAD,
        0x05, 0xc4,
        0x04, 0x0e, /* __add = 14 = 0x0e */
        0x08, PAD,
        0x03, 0x10, /* __end = 16 = 0x10 */
        0x07, PAD,
        0x01, PAD,
        0x02, 0x02, /* __loop = 2 = 0x02 */
        0x09, 0x00
    };

    /* run program */
    char* pc = VM_program;
    for( ; ; ) {
        char opcode = pc[0];
        switch (opcode) {
            case 0x01:
                /* llp */
                VM_loop_counter = pc[1];
                pc = pc + 2;
                break;
            case 0x02:
                /* jlp */
                VM_loop_counter--;
                if ( VM_loop_counter != 0 )
                    pc = &VM_program[pc[1]];
                else
                    pc = pc + 2;
                break;
            case 0x03:
                /* j */
                pc = &VM_program[pc[1]];
                break;
            case 0x04:
                /* blt */
                if ( VM_stack[--VM_stack_top] < VM_threshold )
                    pc = &VM_program[pc[1]];
                else
                    pc = pc + 2;
                break;
            case 0x05:
                /* rnd */
                VM_stack[VM_stack_top++] = rand() % pc[1];
                pc = pc + 2;
                break;
            case 0x06:
                /* sel */
                value_stack[VM_stack_top++] = rand() % roots_size;
                pc = pc + 2;
                break;
            case 0x07:
                /* add */
                int number = VM_stack[VM_stack_top - 1];
                tree* root = &VM_roots[VM_stack[VM_stack_top - 2]];
                bstree_insert(number, root);
                VM_stack_top = VM_stack_top - 2;
                pc = pc + 2;
                break;
            case 0x08:
                /* del */
                int number = VM_stack[VM_stack_top - 1];
                tree* root = &VM_roots[VM_stack[VM_stack_top - 2]];
                bstree_delete(number, root);
                VM_stack_top = VM_stack_top - 2;
                pc = pc + 2;
                break;
            case 0x09:
                /* quit */
                exit(0);
            default:
                /* error, exit */
                printf("%x: unkown opcode\n");
                exit(1);
        }
    }
}
