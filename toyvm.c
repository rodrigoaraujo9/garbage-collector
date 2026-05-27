#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "heap.h"
#include "bistree.h"
#include "globals.h"
#include "collector.h"

#if \
   (defined(MARK_SWEEP) && defined(MARK_COMPACT)) || \
   (defined(MARK_SWEEP) && defined(COPY_COLLECT)) || \
   (defined(MARK_COMPACT) && defined(COPY_COLLECT))
#error "*error* Define only one GC strategy: MARK_SWEEP, MARK_COMPACT, or COPY_COLLECT"
#endif

#define HEAP_SIZE (32 * 1024)  /* 2 KByte */

typedef enum {
    LLP  = 0x01,
    JLP  = 0x02,
    J    = 0x03,
    BLT  = 0x04,
    RND  = 0x05,
    SEL  = 0x06,
    ADD  = 0x07,
    DEL  = 0x08,
    QUIT = 0x09,
    PAD  = 0x00
} Opcode;

int main(int argc, char* argv[] ) {
    int VM_threshold;
    int VM_loop_counter;
    int VM_roots_size;
    int VM_stack_size;
    int VM_stack_top;
    int *VM_stack;
    BisTree *VM_roots;

    if (argc < 5) {
        fprintf(stderr, "*error* %s <threshold> <roots_size> <unused> <stack_size>\n", argv[0]);
        return 1;
    }

    /* initialize threshold */
    VM_threshold = atoi(argv[1]);

    /* initialize roots */
    VM_roots_size = atoi(argv[2]);
    VM_roots = (BisTree*)malloc(VM_roots_size * sizeof(BisTree));

    if (VM_roots == NULL) {
        fprintf(stderr, "*error* could not allocate VM_roots\n");
        return 1;
    }

    for (int i = 0; i < VM_roots_size; i++)
         bistree_init(&VM_roots[i]);

    /*
     * expose VM roots to the garbage collector
     * these names must match globals.h
     */
    roots = VM_roots;
    max_roots = VM_roots_size;

    /* initialize stack */
    VM_stack_size = atoi(argv[4]);
    VM_stack = (int*)malloc(VM_stack_size * sizeof(int));

    if (VM_stack == NULL) {
        fprintf(stderr, "*error* could not allocate VM_stack\n");
        return 1;
    }

    VM_stack_top = 0;

    /* initialize heap */
    heap = (Heap*)malloc(sizeof(Heap));

    if (heap == NULL) {
        fprintf(stderr, "*error* could not allocate heap\n");
        return 1;
    }

    #if defined(MARK_SWEEP)
       heap_init(heap, HEAP_SIZE, mark_sweep_gc);
    #elif defined(MARK_COMPACT)
       heap_init(heap, HEAP_SIZE, mark_compact_gc);
    #elif defined(COPY_COLLECT)
       heap_init(heap, HEAP_SIZE, copy_collection_gc);
    #else
       #error "*error*     you must define one GC strategy: MARK_SWEEP, MARK_COMPACT, or COPY_COLLECT"
    #endif

    /* initialize program */
    char VM_program[] = {
        LLP,  0xc4,
        RND,  0x14,
        SEL,  PAD,
        RND,  0xc4,
        BLT,  0x0e, /* __add = 14 = 0x0e */
        DEL,  PAD,
        J,    0x10, /* __end = 16 = 0x10 */
        ADD,  PAD,
        JLP,  0x02, /* __loop = 2 = 0x02 */
        QUIT, 0x00
    };

    /* run program */
    char* pc = VM_program;

    for( ; ; ) {
        unsigned char opcode = (unsigned char)pc[0];

        switch ((Opcode)opcode) {
            case LLP:
                VM_loop_counter = (unsigned char)pc[1];
                printf("*debug* llp: loop counter set to %d\n", VM_loop_counter);
                pc = pc + 2;
                break;

            case JLP:
                VM_loop_counter--;
                printf("*debug* jlp: loop counter now %d\n", VM_loop_counter);

                if (VM_loop_counter != 0) {
                    printf("*debug* jlp: jumping back to %d\n", (unsigned char)pc[1]);
                    pc = &VM_program[(unsigned char)pc[1]];
                } else {
                    printf("*debug* jlp: loop finished\n");
                    pc = pc + 2;
                }
                break;

            case J:
                pc = &VM_program[(unsigned char)pc[1]];
                break;

                case BLT: {
                    if (VM_stack_top <= 0) {
                        fprintf(stderr, "*error* stack underflow in blt\n");
                        exit(1);
                    }

                    int value = VM_stack[--VM_stack_top];

                    printf("*debug* blt: comparing %d < %d\n", value, VM_threshold);

                    if (value < VM_threshold) {
                        printf("*debug* blt: true, jumping to %d\n", (unsigned char)pc[1]);
                        pc = &VM_program[(unsigned char)pc[1]];
                    } else {
                        printf("*debug* blt: false, continuing\n");
                        pc = pc + 2;
                    }

                    break;
                }

            case RND: {
                if (VM_stack_top >= VM_stack_size) {
                    fprintf(stderr, "*error* stack overflow in rnd\n");
                    exit(1);
                }

                int value = rand() % (unsigned char)pc[1];
                VM_stack[VM_stack_top++] = value;

                printf("*debug* rnd: pushed random value %d\n", value);

                pc = pc + 2;
                break;
            }

            case SEL: {
                if (VM_stack_top >= VM_stack_size) {
                    fprintf(stderr, "*error* stack overflow in sel\n");
                    exit(1);
                }

                int root_index = rand() % VM_roots_size;
                VM_stack[VM_stack_top++] = root_index;

                printf("*debug* sel: selected root %d\n", root_index);

                pc = pc + 2;
                break;
            }

            case ADD: {
                if (VM_stack_top < 2) {
                    fprintf(stderr, "*error* stack underflow in add\n");
                    exit(1);
                }

                int number1 = VM_stack[VM_stack_top - 2];
                int root_index1 = VM_stack[VM_stack_top - 1];

                if (root_index1 < 0 || root_index1 >= VM_roots_size) {
                    fprintf(stderr, "*error* invalid root index in add: %d\n", root_index1);
                    exit(1);
                }

                BisTree* root1 = &VM_roots[root_index1];
                bistree_insert(root1, number1);

                printf("*debug* add: inserted %d into root %d\n", number1, root_index1);

                VM_stack_top = VM_stack_top - 2;
                pc = pc + 2;
                break;
            }

            case DEL: {
                if (VM_stack_top < 2) {
                    fprintf(stderr, "*error* stack underflow in del\n");
                    exit(1);
                }

                int number2 = VM_stack[VM_stack_top - 2];
                int root_index2 = VM_stack[VM_stack_top - 1];

                if (root_index2 < 0 || root_index2 >= VM_roots_size) {
                    fprintf(stderr, "*error* invalid root index in del: %d\n", root_index2);
                    exit(1);
                }

                BisTree* root2 = &VM_roots[root_index2];
                bistree_remove(root2, number2);

                printf("*debug* del: removed %d from root %d\n", number2, root_index2);

                VM_stack_top = VM_stack_top - 2;
                pc = pc + 2;
                break;
            }

            case QUIT:
                printf("*debug* quit: program reached end instruction\n");
                printf("*success* toyvm finished\n");
                exit(0);

            default:
                printf("0x%02x: unknown opcode\n", opcode);
                exit(1);
        }
    }
}
