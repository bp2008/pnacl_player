#pragma once

// Use assert as a poor-man's CHECK, even in non-debug mode.
// Since <assert.h> redefines assert on every inclusion (it doesn't use include-guards), make sure this is the last file #include'd in this file.
#undef NDEBUG
#include <assert.h>