#pragma once

#include <cstdint>
#include <cstdio>

namespace fuzz {

/**
 * @namespace fuzz::debug
 * @brief Centralized debug and logging utilities for AFL++ ISA mutator
 * 
 * @details
 * Provides a unified, thread-safe logging interface that respects the DEBUG 
 * environment variable. This system consolidates all debug output from both
 * the ISA mutator and AFL harness into a single log file.
 * 
 * **Key Features:**
 * - Environment-controlled: All logging gated by DEBUG environment variable
 * - Thread-safe: Mutex-protected file I/O prevents interleaved output
 * - Zero overhead when disabled: No-op functions when DEBUG=0
 * - Single log file: All output to workdir/logs/mutator_debug.log
 * - RAII function tracing: Automatic entry/exit logging via FunctionTracer
 * - AFL++-friendly: No stderr output that could interfere with fuzzer UI
 * 
 * **Environment Variable:**
 * - `DEBUG=1`: Enables all logging to file
 * - `DEBUG=0` or unset: All logging disabled (no file created, zero overhead)
 * 
 * **Log File Location:**
 * - If PROJECT_ROOT set: `$PROJECT_ROOT/workdir/logs/mutator_debug.log`
 * - Otherwise: `./workdir/logs/mutator_debug.log`
 * 
 * **Initialization:**
 * The debug system initializes automatically on first use. No manual setup required.
 * The log directory is created automatically if it doesn't exist.
 * 
 * @example Basic Logging
 * @code
 * #include <fuzz/Debug.hpp>
 * 
 * void loadConfig(const char* path) {
 *     fuzz::debug::logInfo("Loading config from: %s\n", path);
 *     
 *     if (!fileExists(path)) {
 *         fuzz::debug::logError("Config file not found: %s\n", path);
 *         return;
 *     }
 *     
 *     fuzz::debug::logDebug("Parsing YAML structure...\n");
 *     // ... config loading code ...
 *     fuzz::debug::logInfo("Config loaded successfully\n");
 * }
 * @endcode
 * 
 * @example Function Tracing
 * @code
 * void ISAMutator::applyMutations(uint8_t* buf, size_t size) {
 *     // Automatically logs entry on construction, exit on destruction
 *     fuzz::debug::FunctionTracer tracer(__FILE__, __FUNCTION__);
 *     
 *     fuzz::debug::logDebug("Mutating %zu bytes\n", size);
 *     // ... mutation logic ...
 *     // Exit logged automatically even if exception thrown
 * }
 * @endcode
 * 
 * @example Conditional Expensive Operations
 * @code
 * void dumpState() {
 *     if (fuzz::debug::isDebugEnabled()) {
 *         // Only compute expensive debug info when actually logging
 *         std::string trace = generateInstructionTrace();
 *         fuzz::debug::logDebug("Full trace:\n%s\n", trace.c_str());
 *     }
 * }
 * @endcode
 * 
 * @note All functions are thread-safe and can be called concurrently from multiple threads.
 * @note When DEBUG=0, all logging functions become no-ops with minimal overhead.
 * @see DEBUG_API.md for complete usage guide and migration examples
 */
namespace debug {

/**
 * @brief Check if debug mode is enabled via DEBUG environment variable
 * 
 * @details
 * Checks whether the DEBUG environment variable is set to "1". This function
 * caches the result after first call to avoid repeated getenv() calls.
 * 
 * Use this when you need to conditionally execute expensive debug operations
 * that shouldn't run in production/fuzzing mode.
 * 
 * @return true if DEBUG is set to "1", false otherwise
 * 
 * @note The debug flag is cached on first access for performance.
 * @note Changing DEBUG after process start has no effect.
 * 
 * @example
 * @code
 * void processInstruction(uint32_t instr) {
 *     // Only disassemble and log if debug is enabled
 *     if (fuzz::debug::isDebugEnabled()) {
 *         std::string disasm = disassemble(instr);
 *         fuzz::debug::logDebug("Decoded: %s\n", disasm.c_str());
 *     }
 *     
 *     // Normal processing always happens
 *     execute(instr);
 * }
 * @endcode
 */
bool isDebugEnabled();

/**
 * @brief Get the log file handle for debug output
 * 
 * @details
 * Returns the FILE* handle for the debug log file. This is primarily for
 * advanced use cases where you need direct file access (e.g., custom formatting,
 * binary dumps, etc.).
 * 
 * For most logging, use the provided logInfo(), logWarn(), etc. functions instead.
 * 
 * @return FILE* pointing to debug log file (workdir/logs/mutator_debug.log)
 *         Returns nullptr if debug is disabled or file cannot be opened
 * 
 * @warning Do not close or modify the file handle. It's managed internally.
 * @warning Remember to fflush() if you write to it directly.
 * 
 * @example Custom Formatted Output
 * @code
 * void dumpInstructionBytes(const uint8_t* buf, size_t len) {
 *     FILE* log = fuzz::debug::getDebugLog();
 *     if (!log) return;  // Debug disabled
 *     
 *     std::fprintf(log, "Instruction bytes (%zu):\n", len);
 *     for (size_t i = 0; i < len; i++) {
 *         std::fprintf(log, " %02x", buf[i]);
 *         if ((i + 1) % 16 == 0) std::fprintf(log, "\n");
 *     }
 *     std::fprintf(log, "\n");
 *     std::fflush(log);
 * }
 * @endcode
 */
FILE* getDebugLog();

/**
 * @brief Log an informational message
 * 
 * @details
 * Logs informational messages to the debug log file. Use this for:
 * - Configuration loading status
 * - Initialization progress
 * - General operational milestones
 * - Non-error state changes
 * 
 * Output format: `[INFO] <your message>`
 * 
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 * 
 * @note Only logs if DEBUG=1. No-op otherwise with zero overhead.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * @note Automatically flushed to disk for immediate visibility.
 * 
 * @example
 * @code
 * void ISAMutator::initFromEnv() {
 *     fuzz::debug::logInfo("Initializing ISA mutator\n");
 *     
 *     const char* isa_name = std::getenv("ISA_NAME");
 *     fuzz::debug::logInfo("Target ISA: %s\n", isa_name ? isa_name : "rv32imc");
 *     
 *     loadSchemas();
 *     fuzz::debug::logInfo("Loaded %zu instructions from schema\n", 
 *                          schema_.instructions.size());
 * }
 * @endcode
 */
void logInfo(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Log a warning message
 * 
 * @details
 * Logs warning messages to the debug log file. Use this for:
 * - Non-fatal errors (system can continue)
 * - Fallback behavior activation
 * - Invalid but recoverable input
 * - Unusual but not incorrect conditions
 * 
 * Output format: `[WARN] <your message>`
 * 
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 * 
 * @note Only logs if DEBUG=1. No-op otherwise with zero overhead.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * 
 * @example
 * @code
 * uint32_t clampProbability(int value) {
 *     if (value < 0 || value > 100) {
 *         fuzz::debug::logWarn("Probability %d out of range [0,100], clamping\n", 
 *                              value);
 *         return std::clamp(value, 0, 100);
 *     }
 *     return value;
 * }
 * 
 * void loadConfig(const char* path) {
 *     if (!fileExists(path)) {
 *         fuzz::debug::logWarn("Config not found: %s, using defaults\n", path);
 *         return loadDefaultConfig();
 *     }
 *     return parseConfig(path);
 * }
 * @endcode
 */
void logWarn(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Log an error message
 * 
 * @details
 * Logs error messages to the debug log file. Use this for:
 * - Fatal errors (before exit/abort)
 * - Failed operations that prevent continuation
 * - Invalid input that cannot be recovered
 * - Resource allocation failures
 * 
 * Output format: `[ERROR] <your message>`
 * 
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 * 
 * @note Only logs if DEBUG=1. No-op otherwise with zero overhead.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * @note Does NOT terminate the program - call exit() separately if needed.
 * 
 * @example
 * @code
 * void ISAMutator::loadSchema(const char* path) {
 *     try {
 *         schema_ = parseYaml(path);
 *     } catch (const std::exception& e) {
 *         fuzz::debug::logError("Failed to parse schema '%s': %s\n", 
 *                               path, e.what());
 *         std::exit(1);
 *     }
 *     
 *     if (schema_.instructions.empty()) {
 *         fuzz::debug::logError("Schema '%s' contains no instructions\n", path);
 *         std::exit(1);
 *     }
 * }
 * @endcode
 */
void logError(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Log a debug trace message
 * 
 * @details
 * Logs detailed debugging information to the debug log file. Use this for:
 * - Verbose algorithm traces
 * - Intermediate computation results
 * - Loop iteration details
 * - State dumps
 * 
 * This is the most verbose logging level. Use liberally for debugging,
 * but remember it only activates when DEBUG=1.
 * 
 * Output format: `[DEBUG] <your message>`
 * 
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 * 
 * @note Only logs if DEBUG=1. No-op otherwise with zero overhead.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * 
 * @example
 * @code
 * void ISAMutator::encodeInstruction(const InstrSpec& spec, uint32_t* out) {
 *     fuzz::debug::logDebug("Encoding instruction: %s\n", spec.name.c_str());
 *     
 *     uint32_t encoded = spec.base_pattern;
 *     fuzz::debug::logDebug("  Base pattern: 0x%08x\n", encoded);
 *     
 *     for (const auto& field : spec.fields) {
 *         uint32_t value = randomFieldValue(field);
 *         fuzz::debug::logDebug("  Field %s: bits [%d:%d] = 0x%x\n",
 *                               field.name.c_str(), field.lsb, field.msb, value);
 *         encoded |= (value << field.lsb);
 *     }
 *     
 *     fuzz::debug::logDebug("  Final encoding: 0x%08x\n", encoded);
 *     *out = encoded;
 * }
 * @endcode
 */
void logDebug(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Log an illegal mutation attempt
 * 
 * @details
 * Specialized logging function for tracking mutation attempts that produce
 * illegal or invalid instructions. This helps debug mutation logic and
 * identify patterns that frequently generate invalid code.
 * 
 * Output format:
 * ```
 * [ILLEGAL] <source_function>()
 *   before = 0x<before_value>
 *   after  = 0x<after_value>
 * ```
 * 
 * @param src Source function name (use __FUNCTION__ or manual string)
 * @param before Original value before mutation (typically instruction word)
 * @param after Mutated value that was illegal (typically instruction word)
 * 
 * @note Only logs if DEBUG=1. No-op otherwise with zero overhead.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * 
 * @example Basic Usage
 * @code
 * void ISAMutator::mutateImmediate(uint32_t* instr) {
 *     uint32_t original = *instr;
 *     
 *     // Attempt mutation
 *     uint32_t mutated = applyImmediateMutation(original);
 *     
 *     // Validate the result
 *     if (!isLegalInstruction(mutated)) {
 *         fuzz::debug::logIllegal(__FUNCTION__, original, mutated);
 *         return;  // Don't apply illegal mutation
 *     }
 *     
 *     *instr = mutated;
 * }
 * @endcode
 * 
 * @example With Context
 * @code
 * bool tryCompressInstruction(uint32_t instr32, uint16_t* out16) {
 *     if (!canCompress(instr32)) {
 *         return false;
 *     }
 *     
 *     uint16_t compressed = compress(instr32);
 *     
 *     if (!isValidRVC(compressed)) {
 *         // Log with 32-bit context even though result is 16-bit
 *         fuzz::debug::logIllegal("tryCompressInstruction", 
 *                                 instr32, 
 *                                 static_cast<uint32_t>(compressed));
 *         return false;
 *     }
 *     
 *     *out16 = compressed;
 *     return true;
 * }
 * @endcode
 */
void logIllegal(const char* src, uint32_t before, uint32_t after);

/**
 * @brief Extract basename from file path
 * 
 * @details
 * Utility function to extract the filename from a full path. Handles both
 * Unix-style (/) and Windows-style (\) path separators. This is useful
 * for cleaning up __FILE__ macros in log output.
 * 
 * This function does not allocate memory and returns a pointer into the
 * original string, so the input must remain valid.
 * 
 * @param path Full file path (e.g., "/home/user/project/src/file.cpp")
 * @return Pointer to basename within the path (e.g., "file.cpp")
 *         Returns empty string if path is nullptr
 * 
 * @note Does not allocate memory - returns pointer into original string
 * @note Handles both / and \ path separators
 * @note Input pointer must remain valid while using returned pointer
 * 
 * @example
 * @code
 * void logCurrentLocation() {
 *     const char* file = fuzz::debug::basename(__FILE__);
 *     fuzz::debug::logInfo("Current file: %s, line: %d\n", file, __LINE__);
 *     // Instead of: /home/user/project/src/ISAMutator.cpp
 *     // Logs: ISAMutator.cpp
 * }
 * 
 * // Used internally by FunctionTracer
 * FunctionTracer tracer(__FILE__, __FUNCTION__);
 * // Logs: [Fn Start] ISAMutator.cpp::applyMutations
 * // Not:  [Fn Start] /long/path/to/ISAMutator.cpp::applyMutations
 * @endcode
 */
const char* basename(const char* path);

/**
 * @brief RAII-based function tracer for automatic entry/exit logging
 * 
 * @details
 * FunctionTracer provides automatic function entry/exit logging using RAII
 * (Resource Acquisition Is Initialization). Simply construct an instance at
 * the start of any function you want to trace, and it will automatically log:
 * - Function entry on construction
 * - Function exit on destruction (even if exception thrown)
 * 
 * This is particularly useful for understanding call flow, timing, and
 * debugging exception handling paths.
 * 
 * Output format:
 * ```
 * [Fn Start  ] <filename>::<function>
 * [Fn End    ] <filename>::<function>
 * ```
 * 
 * @note Only logs if DEBUG=1. Zero overhead when DEBUG=0.
 * @note Thread-safe: Multiple threads can use tracers concurrently.
 * @note Exception-safe: Exit is logged even if function throws.
 * @note Uses basename() to show only filename, not full path.
 * 
 * @example Basic Function Tracing
 * @code
 * void ISAMutator::applyMutations(uint8_t* buf, size_t size) {
 *     fuzz::debug::FunctionTracer tracer(__FILE__, __FUNCTION__);
 *     
 *     // Function body here...
 *     pickInstruction();
 *     encodeInstruction();
 *     
 *     // Exit logged automatically when tracer goes out of scope
 * }
 * 
 * // Log output:
 * // [Fn Start  ] ISAMutator.cpp::applyMutations
 * // [Fn Start  ] ISAMutator.cpp::pickInstruction
 * // [Fn End    ] ISAMutator.cpp::pickInstruction
 * // [Fn Start  ] ISAMutator.cpp::encodeInstruction
 * // [Fn End    ] ISAMutator.cpp::encodeInstruction
 * // [Fn End    ] ISAMutator.cpp::applyMutations
 * @endcode
 * 
 * @example Exception-Safe Tracing
 * @code
 * void parseSchema(const char* path) {
 *     fuzz::debug::FunctionTracer tracer(__FILE__, __FUNCTION__);
 *     
 *     // Even if this throws, tracer destructor still runs
 *     YAML::Node root = YAML::LoadFile(path);
 *     processNode(root);
 *     
 *     // [Fn End] logged even if exception thrown above
 * }
 * @endcode
 * 
 * @example Early Return Paths
 * @code
 * bool validateInput(const uint8_t* buf, size_t size) {
 *     fuzz::debug::FunctionTracer tracer(__FILE__, __FUNCTION__);
 *     
 *     if (!buf) {
 *         fuzz::debug::logError("Null buffer\n");
 *         return false;  // [Fn End] logged here
 *     }
 *     
 *     if (size < MIN_SIZE) {
 *         fuzz::debug::logError("Buffer too small: %zu\n", size);
 *         return false;  // [Fn End] logged here too
 *     }
 *     
 *     return true;  // And here
 * }
 * 
 * // All three return paths automatically log function exit
 * @endcode
 * 
 * @example Conditional Tracing
 * @code
 * void hotPath() {
 *     // Only trace when debugging is enabled
 *     // No overhead in production (DEBUG=0)
 *     fuzz::debug::FunctionTracer tracer(__FILE__, __FUNCTION__);
 *     
 *     // Performance-critical code here...
 *     // When DEBUG=0: tracer construction/destruction are no-ops
 * }
 * @endcode
 * 
 * @see logInfo() for manual logging
 * @see isDebugEnabled() to check if tracing is active
 */
class FunctionTracer {
public:
  /**
   * @brief Construct tracer and log function entry
   * 
   * @details
   * Logs function entry if DEBUG=1. Stores filename and function name
   * for later use by destructor. The filename is automatically cleaned
   * to show only the basename (not full path).
   * 
   * @param file Source file path (use __FILE__ macro)
   * @param func Function name (use __FUNCTION__ or __func__ macro)
   * 
   * @note Filename is cleaned via basename() to show only filename
   * @note If DEBUG=0, this is a no-op with minimal overhead
   */
  FunctionTracer(const char* file, const char* func);
  
  /**
   * @brief Destructor logs function exit
   * 
   * @details
   * Automatically logs function exit when the tracer goes out of scope.
   * This happens even if an exception is thrown, making it reliable for
   * tracking all exit paths including error cases.
   * 
   * @note Guaranteed to run even during stack unwinding (exceptions)
   * @note If DEBUG=0, this is a no-op with minimal overhead
   */
  ~FunctionTracer();

private:
  const char* base_;  ///< Basename of source file (e.g., "ISAMutator.cpp")
  const char* func_;  ///< Function name (e.g., "applyMutations")
};

} // namespace debug
} // namespace fuzz
