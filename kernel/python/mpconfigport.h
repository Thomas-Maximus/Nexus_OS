#include <stdint.h>
#define alloca(size) __builtin_alloca(size)

// Python internal features
#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_HELPER_REPL                     (1)
#define MICROPY_ERROR_REPORTING                 (MICROPY_ERROR_REPORTING_TERSE)
#define MICROPY_ENABLE_COMPILER                 (1)

// Type definitions for the target (i686)
typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
typedef intptr_t mp_off_t;

// Minimal features to avoid dependencies
#define MICROPY_PY_BUILTINS_FLOAT               (0)
#define MICROPY_PY_SYS                          (0)
#define MICROPY_READER_VFS                      (0)
#define MICROPY_VFS                             (0)

// Stack and Heap
#define MICROPY_STACK_CHECK                     (0)
#define MICROPY_ENABLE_REPL_HELPERS             (1)
#define MICROPY_READLINE_HISTORY_SIZE           (0)

// Root pointers
#define MICROPY_PORT_ROOT_POINTERS
