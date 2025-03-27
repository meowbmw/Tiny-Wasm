#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to align pointer for specific architecture requirements
static inline void *align_ptr(void *ptr, size_t alignment) {
  // Calculate how many bytes we need to add to make the pointer aligned
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
  return (void *)aligned;
}

// The main printf function that takes a format string and raw memory buffer of arguments
int my_printf(const char *format, void *args_ptr) {
  // Start with the raw memory pointer
  char *args_mem = (char *)args_ptr;
  int chars_printed = 0;

  // Disable stdout buffering to ensure immediate output
  setvbuf(stdout, NULL, _IONBF, 0);

  const char *p = format;

  while (*p) {
    // If not a format specifier, just print the character
    if (*p != '%') {
      putchar(*p++);
      chars_printed++;
      continue;
    }
    // Skip the '%'
    p++;
    // Check for length modifiers
    int is_long = 0;
    int is_long_long = 0;
    if (*p == 'l') {
      p++;
      is_long = 1;
      if (*p == 'l') {
        p++;
        is_long = 0;
        is_long_long = 1;
      }
    }
    // Process based on format specifier
    switch (*p) {
    case 'd':
    case 'i': {
      if (is_long_long) {
        // For long long, ensure 8-byte alignment
        args_mem = (char *)align_ptr(args_mem, 8);
        // Read 8 bytes with proper alignment
        long long value;
        memcpy(&value, args_mem, sizeof(long long));
        chars_printed += printf("%lld", value);
        args_mem += sizeof(long long); // Move pointer 8 bytes forward
      } else if (is_long) {
        // For long, ensure proper alignment (4 or 8 bytes based on architecture)
        args_mem = (char *)align_ptr(args_mem, sizeof(long));
        long value;
        memcpy(&value, args_mem, sizeof(long));
        chars_printed += printf("%ld", value);
        args_mem += sizeof(long); // Move pointer forward
      } else {
        // For regular int, ensure 4-byte alignment
        args_mem = (char *)align_ptr(args_mem, 4);
        int value;
        memcpy(&value, args_mem, sizeof(int));
        chars_printed += printf("%d", value);
        args_mem += sizeof(int); // Move pointer 4 bytes forward
      }
      break;
    }
    case 'u': {
      if (is_long_long) {
        // For unsigned long long, ensure 8-byte alignment
        args_mem = (char *)align_ptr(args_mem, 8);
        unsigned long long value;
        memcpy(&value, args_mem, sizeof(unsigned long long));
        chars_printed += printf("%llu", value);
        args_mem += sizeof(unsigned long long);
      } else if (is_long) {
        // For unsigned long, ensure proper alignment
        args_mem = (char *)align_ptr(args_mem, sizeof(unsigned long));
        unsigned long value;
        memcpy(&value, args_mem, sizeof(unsigned long));
        chars_printed += printf("%lu", value);
        args_mem += sizeof(unsigned long);
      } else {
        // For unsigned int, ensure 4-byte alignment
        args_mem = (char *)align_ptr(args_mem, 4);
        unsigned int value;
        memcpy(&value, args_mem, sizeof(unsigned int));
        chars_printed += printf("%u", value);
        args_mem += sizeof(unsigned int);
      }
      break;
    }
    case 'c': {
      // Character is passed as an integer, ensure 4-byte alignment
      args_mem = (char *)align_ptr(args_mem, 4);
      // Read as int but print as char
      int value;
      memcpy(&value, args_mem, sizeof(int));
      chars_printed += printf("%c", (char)value);
      args_mem += sizeof(int); // Move 4 bytes forward
      break;
    }
    case 's': {
      args_mem = (char *)align_ptr(args_mem, sizeof(void *));
      int offset_in_wasm_stack;
      /**
       * e.g.
       * mem[9042]=1024
       * mem[1024]='abc'
       */
      memcpy(&offset_in_wasm_stack, args_mem, sizeof(int));
      // we need to to calculate base memory address, this could be done by understanding how args_ptr is made and subtract it to get base address
      char *base_memory = (char *)args_ptr - (9248 - 16);
      // Read the string pointer
      chars_printed += printf("%s", base_memory + offset_in_wasm_stack);
      args_mem += sizeof(char *); // Move pointer-size bytes forward
      break;
    }
    case 'p': {
      // TODO: this part isn't fully verified yet!
      chars_printed += printf("%p", args_mem - (9248 - 4));
      args_mem += sizeof(void *);
      break;
    }
    case '%': {
      // Literal %
      putchar('%');
      chars_printed++;
      break;
    }
    default: {
      // Unknown format specifier, just print it
      putchar('%');
      putchar(*p);
      chars_printed += 2;
      break;
    }
    }

    p++; // Move to next character in format string
  }
  return chars_printed;
}

// Function to get address of my_printf for JIT purposes
void *get_my_printf_address() {
  return (void *)&my_printf;
}