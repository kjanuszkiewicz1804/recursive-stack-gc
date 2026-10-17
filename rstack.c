// Kamil Januszkiewicz
// This program is my implementation of the recursive stacks problem
// It implements a library that :
//-allows user to create stacks
//-allows to place numbers of type uint64 on stacks
//-allows to place stacks on another stacks
//-manages the memory using garbage collector
//
//(I used mns as the short term version of mark&sweep throughout the code)

#include "rstack.h"

#include <assert.h>
#include <inttypes.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    TYPE_VALUE,
    TYPE_RSTACK
} elem_type_t;

// Struct for elements that are stored on stacks.
typedef struct node
{
    elem_type_t type;

    union
    {
        uint64_t value;
        rstack_t *rs;
    } data;

    struct node *next;

} node_t;

struct rstack
{
    node_t *top;

    bool is_root;          // Flag that says if the user still has a direct reference to the stack.
    bool marked;           // Flag used by mark phase of garbage collector.
    bool visited;          // Flag used for cycle detection during dfs algorithms used in empty write and front.
    rstack_t *next_global; // Pointer to the next stack on the global list.
};

// A pointer to the global list of all allocated stacks used by the garbage collector.
static rstack_t *global_head = nullptr;

// AUXILARY FUNCTIONS OF THE LIBRARY:

// Mark phase- the algorithm marks given stack
// and calls itself recursively on all stacks, which were placed on top of the given stack.
static void mns_mark(rstack_t *rs)
{
    if (rs == nullptr || rs->marked)
    {
        return;
    }

    rs->marked = true;

    node_t *curr = rs->top;
    while (curr != nullptr)
    {
        if (curr->type == TYPE_RSTACK)
        {
            mns_mark(curr->data.rs);
        }
        curr = curr->next;
    }
}

// Sweep phase- the algorithm unmarks all of the marked stacks for the next mns cycle, as they are still reachable.
// For the unmarked stacks, it proceeds with unlinking them from the list, iterates through their content,
// freeing their nodes, and then it frees memory after them.
static void mns_sweep()
{
    rstack_t *curr = global_head;
    rstack_t *prev = nullptr;

    while (curr != nullptr)
    {
        if (curr->marked)
        {
            curr->marked = false;
            prev = curr;
            curr = curr->next_global;
        }
        else
        {
            rstack_t *to_sweep = curr;

            if (prev != nullptr)
            {
                prev->next_global = curr->next_global;
            }
            else
            {
                global_head = curr->next_global;
            }

            curr = curr->next_global;

            node_t *curr_node = to_sweep->top;
            while (curr_node != nullptr)
            {
                node_t *next_node = curr_node->next;
                free(curr_node);
                curr_node = next_node;
            }

            free(to_sweep);
        }
    }
}

// Dfs traversal auxiliary function for the front function,
// that searches for the first number value it can find and aborts.
// There is also implemented safety measure against cycles to avoid infinite loops.
// It relies on the rstack's flag- visited which is later restored by cleanup function.
static bool front_dfs(rstack_t *rs, uint64_t *val)
{
    if (rs == nullptr || rs->visited)
        return false;

    rs->visited = true;

    node_t *curr = rs->top;
    while (curr != nullptr)
    {
        if (curr->type == TYPE_VALUE)
        {
            *val = curr->data.value;
            return true;
        }
        else
        {
            if (front_dfs(curr->data.rs, val))
            {
                return true;
            }
        }
        curr = curr->next;
    }

    return false;
}

// Auxiliary function used to restore visited flags back to base (false) setting,
// after the front_dfs function has been called.
static void cleanup_dfs(rstack_t *rs)
{
    if (rs == nullptr || !rs->visited)
    {
        return;
    }

    rs->visited = false;

    node_t *curr = rs->top;
    while (curr != nullptr)
    {
        if (curr->type == TYPE_RSTACK)
        {
            cleanup_dfs(curr->data.rs);
        }
        curr = curr->next;
    }
}

// Dfs traversal auxiliary function for the write function, that uses post-order traversal to ensure that output is in correct order.
// According to the rules specified in task if the cycle is detected the traversal is aborted (hence cycle flag).
static void write_dfs(node_t *curr, FILE *f, bool *cycle)
{
    if (curr == nullptr || *cycle)
    {
        return;
    }

    write_dfs(curr->next, f, cycle);

    if (*cycle)
    {
        return;
    }

    if (curr->type == TYPE_VALUE)
    {
        fprintf(f, "%lu\n", curr->data.value);
    }
    else
    {
        rstack_t *rs = curr->data.rs;

        if (rs->visited)
        {
            *cycle = true;
            return;
        }

        rs->visited = true;
        write_dfs(rs->top, f, cycle);
        rs->visited = false;
    }
}

// MAIN FUNCTIONS OF THE LIBRARY:

rstack_t *rstack_new()
{
    rstack_t *rs = malloc(sizeof(rstack_t));

    if (rs == nullptr)
    {
        errno = ENOMEM;
        return nullptr;
    }

    rs->is_root = true;
    rs->marked = false;
    rs->visited = false;

    rstack_t *prev = global_head;
    rs->next_global = prev;
    rs->top = nullptr;
    global_head = rs;

    return rs;
}

void rstack_delete(rstack_t *rs)
{
    if (rs == nullptr)
    {
        return;
    }

    rs->is_root = false;

    rstack_t *curr = global_head;
    while (curr != nullptr)
    {
        if (curr->is_root)
        {
            mns_mark(curr);
        }
        curr = curr->next_global;
    }

    mns_sweep();
}

int rstack_push_value(rstack_t *rs, uint64_t value)
{

    if (rs == nullptr)
    {
        errno = EINVAL;
        return -1;
    }

    node_t *new_node = malloc(sizeof(node_t));

    if (new_node == nullptr)
    {
        errno = ENOMEM;
        return -1;
    }

    new_node->type = TYPE_VALUE;
    new_node->data.value = value;
    new_node->next = rs->top;

    rs->top = new_node;

    return 0;
}

int rstack_push_rstack(rstack_t *rs1, rstack_t *rs2)
{

    if (rs1 == nullptr || rs2 == nullptr)
    {
        errno = EINVAL;
        return -1;
    }

    node_t *new_node = malloc(sizeof(node_t));

    if (new_node == nullptr)
    {
        errno = ENOMEM;
        return -1;
    }

    new_node->type = TYPE_RSTACK;
    new_node->data.rs = rs2;
    new_node->next = rs1->top;

    rs1->top = new_node;

    return 0;
}

void rstack_pop(rstack_t *rs)
{
    if (rs == nullptr || rs->top == nullptr)
    {
        return;
    }

    node_t *to_pop = rs->top;
    rs->top = (rs->top)->next;
    free(to_pop);
}

result_t rstack_front(rstack_t *rs)
{
    result_t res = {.flag = false, .value = 0};

    if (rs == nullptr)
    {
        return res;
    }

    uint64_t val = 0;

    if (front_dfs(rs, &val))
    {
        res.flag = true;
        res.value = val;
    }

    cleanup_dfs(rs);

    return res;
}

// Function that implements the same method as the rstack_front,
// because the front function already checks if the stack contains a number,
// setting result_t.flag accordingly.
bool rstack_empty(rstack_t *rs)
{
    if (rs == nullptr)
    {
        return true;
    }

    return !rstack_front(rs).flag;
}

// Parsing function that iterates through given file,
// character by character building the stack.
// The function returns a pointer to the created stack.
// The file must contain only numbers seperated by white spaces.
// If the file content is in wrong format the function sets errno accordingly
// and returns nullptr.
rstack_t *rstack_read(char const *path)
{
    if (path == nullptr)
    {
        errno = EINVAL;
        return nullptr;
    }

    FILE *f = fopen(path, "r");
    if (f == nullptr)
    {
        return nullptr;
    }

    rstack_t *rs = rstack_new();

    if (rs == nullptr)
    {
        fclose(f);
        return nullptr;
    }

    int c;
    bool in_number = false;
    uint64_t val = 0;

    while ((c = fgetc(f)) != EOF)
    {
        if (isspace(c))
        {
            if (in_number)
            {
                if (rstack_push_value(rs, val) != 0)
                {
                    goto error;
                }
                in_number = false;
                val = 0;
            }
        }
        else if (isdigit(c))
        {
            in_number = true;
            uint64_t digit = c - '0';

            if (val > (UINT64_MAX - digit) / 10)
            {
                errno = ERANGE;
                goto error;
            }

            val = val * 10 + digit;
        }
        else
        {
            errno = EINVAL;
            goto error;
        }
    }
    if (in_number)
    {
        if (rstack_push_value(rs, val) != 0)
        {
            goto error;
        }
    }

    fclose(f);
    return rs;

error:
    {
        int saved_errno = errno; // By doing this, the program prevents more important errno to be overwritten by errno from fclose or rstack_delete.

        rstack_delete(rs);
        fclose(f);

        errno = saved_errno;
        return nullptr;
    }
}

int rstack_write(char const *path, rstack_t *rs)
{
    if (path == nullptr || rs == nullptr)
    {
        errno = EINVAL;
        return -1;
    }

    FILE *f = fopen(path, "w");

    if (f == nullptr)
    {
        return -1;
    }

    bool cycle = false;

    rs->visited = true;

    write_dfs(rs->top, f, &cycle);

    rs->visited = false;

    if (fclose(f) != 0)
    {
        return -1;
    }

    return 0;
}
