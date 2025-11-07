#pragma once

#include <cstdio>

namespace fuzz {

/**
 * @brief Centralized debug and logging utilities
 * 
 * Provides unified logging interface that respects DEBUG environment variable.
 * When DEBUG=1, all messages are logged to debug log file.
 * All messages go to file only, not to stderr.
 */
namespace debug {

/**
 * @brief Check if debug mode is enabled via DEBUG environment variable
 * @return true if DEBUG is set to "1", false otherwise
 */
bool isDebugEnabled();

/**
 * @brief Get the log file handle for debug output
 * @return FILE* pointing to debug log file (afl/isa_mutator/logs/mutator_debug.log)
 *         Returns nullptr if debug is disabled or file cannot be opened
 */
FILE* getDebugLog();

/**
 * @brief Log an informational message
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 * 
 * Only logs if DEBUG=1. Output goes to debug log file only.
 */
void logInfo(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Log a warning message
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 * 
 * Only logs if DEBUG=1. Output goes to debug log file only.
 */
void logWarn(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Log an error message
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 * 
 * Only logs if DEBUG=1. Output goes to debug log file only.
 */
void logError(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Log a debug trace message
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 * 
 * Only logs if DEBUG=1. Output goes to debug log file only.
 * Use for detailed debugging output.
 */
void logDebug(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

} // namespace debug
} // namespace fuzz
