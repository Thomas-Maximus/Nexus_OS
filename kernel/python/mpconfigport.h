#include <stdint.h>
#ifndef alloca
#define alloca(size) __builtin_alloca(size)
#endif

// Python internal features
#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)
#define MICROPY_ENABLE_GC                       (1)

#define MICROPY_HELPER_REPL                     (1)
#define MICROPY_ERROR_REPORTING                 (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_ENABLE_SOURCE_LINE              (1)
#define MICROPY_ENABLE_COMPILER                 (1)


// Type definitions for the target (i686)
typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
typedef intptr_t mp_off_t;

// Minimal features and builtins
#define MICROPY_PY_BUILTINS_FLOAT               (0)
#define MICROPY_PY_BUILTINS_INPUT               (1)
#define MICROPY_PY_BUILTINS_HELP                (1)
#define MICROPY_PY_BUILTINS_SLICE               (1)
#define MICROPY_PY_BUILTINS_STR_COUNT           (1)
#define MICROPY_PY_BUILTINS_STR_OP_MODULO       (0)

#define MICROPY_PY_SYS                          (0)
#define MICROPY_READER_VFS                      (0)
#define MICROPY_VFS                             (0)

// Stack and Heap
#define MICROPY_STACK_CHECK                     (1)
#define MICROPY_ENABLE_REPL_HELPERS             (1)
#define MICROPY_READLINE_HISTORY_SIZE           (0)

// Root pointers
#define MICROPY_PORT_ROOT_POINTERS

