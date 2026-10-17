# Recursive Stack Shared Library (`librstack.so`)

## Overview
A lightweight, dynamically loaded shared library (`.so`) written in C (GNU2x/C23) implementing recursive, nested stack structures (`rstack_t`). Stacks can store both 64-bit unsigned integers (`uint64_t`) and references to other stacks, supporting arbitrary nesting and self-referential cyclic graphs[cite: 4].

Memory is managed automatically via a custom **Mark-and-Sweep Garbage Collector**, ensuring complete immunity to reference-cycle memory leaks while maintaining a **Strong Exception Safety Guarantee** (Out-Of-Memory safe).

---

## Architecture & Implementation Details

- **Mark-and-Sweep Garbage Collector (`mns_mark` & `mns_sweep`):**
  - All allocated stacks are tracked via an intrusive singly linked global list (`global_head`)[cite: 4].
  - Each stack maintains an `is_root` flag indicating whether an active external handle exists in user space[cite: 4].
  - Upon handle deletion (`rstack_delete`), the collector traverses all stacks reachable from active roots (`mns_mark`) and sweeps unreachable stacks along with their internal node chains (`mns_sweep`), effortlessly reclaiming isolated cyclic structures[cite: 4].
- **Tagged-Union Stack Nodes (`node_t`):**
  - Stack elements are represented as linked nodes containing an enum tag (`TYPE_VALUE` or `TYPE_RSTACK`) and a memory-efficient `union` holding either a `uint64_t` payload or an `rstack_t*` reference[cite: 4].
- **Cycle-Safe DFS Traversal & Post-Order Serialization:**
  - `rstack_front` and `rstack_empty` utilize Depth-First Search (`front_dfs`) with an on-stack `visited` flag and a dedicated restoration pass (`cleanup_dfs`) to inspect deeply nested or cyclic stacks without infinite recursion[cite: 4].
  - `rstack_write` employs recursive post-order DFS traversal (`write_dfs`) with active cycle detection on the recursion stack, halting gracefully if a cycle is encountered[cite: 4].
- **Overflow-Safe Stream Parser (`rstack_read`):**
  - Parses whitespace-delimited base-10 integers character-by-character with pre-multiplication bounds checking (`val > (UINT64_MAX - digit) / 10`), setting `errno = ERANGE` on 64-bit overflow and `EINVAL` on malformed tokens while preserving the primary `errno` across cleanup[cite: 4].

---

## Public API (`rstack.h`)

| Function | Description | Error / Return Semantics |
| :--- | :--- | :--- |
| `rstack_t* rstack_new(void)` | Allocates a new empty stack, registers it as a root on the global GC list[cite: 4]. | Returns pointer, or `nullptr` (`errno = ENOMEM`)[cite: 4]. |
| `void rstack_delete(rstack_t *rs)` | Unmarks `rs` as a root handle and triggers a Mark-and-Sweep GC cycle[cite: 4]. | Safe no-op if `rs == nullptr`[cite: 4]. |
| `int rstack_push_value(rstack_t *rs, uint64_t value)` | Pushes a 64-bit unsigned integer onto the top of `rs`[cite: 4]. | Returns `0` on success, `-1` on error (`EINVAL` / `ENOMEM`)[cite: 4]. |
| `int rstack_push_rstack(rstack_t *rs1, rstack_t *rs2)` | Pushes a reference to stack `rs2` onto the top of `rs1`[cite: 4]. | Returns `0` on success, `-1` on error (`EINVAL` / `ENOMEM`)[cite: 4]. |
| `void rstack_pop(rstack_t *rs)` | Non-recursively removes and frees the top node of `rs`[cite: 4]. | Safe no-op if `rs == nullptr` or empty[cite: 4]. |
| `result_t rstack_front(rstack_t *rs)` | Recursively searches for the `uint64_t` value closest to the top of `rs`[cite: 4]. | Returns `{.flag = true, .value = val}` or `{.flag = false}`[cite: 4]. |
| `bool rstack_empty(rstack_t *rs)` | Recursively checks whether `rs` contains any integer values[cite: 4]. | Returns `true` if `rs == nullptr` or contains no integers[cite: 4]. |
| `rstack_t* rstack_read(char const *path)` | Reads whitespace-separated base-10 integers from a file onto a new stack[cite: 4]. | Returns stack pointer, or `nullptr` with `errno` set (`EINVAL`, `ERANGE`, `ENOMEM`, or I/O `errno`)[cite: 4]. |
| `int rstack_write(char const *path, rstack_t *rs)` | Writes stack integers in bottom-to-top order to a file; aborts on cycles[cite: 4]. | Returns `0` on success, `-1` on error[cite: 4]. |

---

## Tech Stack & Build Instructions

- **Language:** C (`-std=gnu2x` / C23)
- **Compiler Flags:** `-Wall -Wextra -Wno-implicit-fallthrough -std=gnu2x -fPIC -O2`
- **Verification:** Verified for zero memory leaks and strict allocation-failure resilience using **Valgrind**.

### Building the Shared Library
Run `make` in the project root directory to compile `librstack.so`:
```bash
make
```
*(Or explicitly target `make librstack.so`)*.

### Cleaning Build Artifacts
To remove object files (`*.o`) and the compiled shared library (`librstack.so`):
```bash
make clean
```
