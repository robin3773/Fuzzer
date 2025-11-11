#pragma once

#include <cstdint>
#include <cstdio>

namespace hwfuzz {

/**
 * @namespace hwfuzz::debug
 * @brief Centralized runtime logging for AFL++ ISA mutator and harness
 * 
 * @details
 * Provides a unified, thread-safe logging interface for all custom fuzzer components.
 * This system is shared between the ISA mutator and AFL harness, consolidating all
 * runtime output into a single log file while keeping stdout/stderr clean for AFL++.
 * 
 * **Key Features:**
 * - Always-on logging: All output captured regardless of DEBUG flag
 * - Thread-safe: Mutex-protected file I/O prevents interleaved output
 * - Single log file: All output to workdir/logs/runtime.log
 * - RAII function tracing: Automatic entry/exit logging via FunctionTracer
 * - AFL++-friendly: No stdout/stderr output that could interfere with fuzzer UI
 * - Shared across components: Both mutator and harness use the same log
 * 
 * **Log File Location:**
 * - If PROJECT_ROOT set: `$PROJECT_ROOT/workdir/logs/runtime.log`
 * - Otherwise: `./workdir/logs/runtime.log`
 * 
 * **Initialization:**
 * The logging system initializes automatically on first use. No manual setup required.
 * The log directory is created automatically if it doesn't exist.
 * 
 * **Component Prefixes:**
 * - Mutator logs: `[MUTATOR]` prefix
 * - Harness logs: `[HARNESS]` prefix
 * - This helps identify the source component in shared logs
 * 
 * @example Basic Logging from Mutator
 * @code
 * #include <hwfuzz/Debug.hpp>
 * 
 * void ISAMutator::loadConfig(const char* path) {
 *     hwfuzz::debug::logInfo("[MUTATOR] Loading config from: %s\n", path);
 *     
 *     if (!fileExists(path)) {
 *         hwfuzz::debug::logError("[MUTATOR] Config file not found: %s\n", path);
 *         return;
 *     }
 *     
 *     hwfuzz::debug::logDebug("[MUTATOR] Parsing YAML structure...\n");
 *     // ... config loading code ...
 *     hwfuzz::debug::logInfo("[MUTATOR] Config loaded successfully\n");
 * }
 * @endcode
 * 
 * @example Basic Logging from Harness
 * @code
 * #include <hwfuzz/Debug.hpp>
 * 
 * void CpuIface::executeProgram(const uint8_t* buf, size_t len) {
 *     hwfuzz::debug::logInfo("[HARNESS] Executing program (%zu bytes)\n", len);
 *     hwfuzz::debug::FunctionTracer tracer(__FILE__, __FUNCTION__);
 *     
 *     // ... execution logic ...
 *     
 *     hwfuzz::debug::logDebug("[HARNESS] Execution complete, cycles: %llu\n", cycles);
 * }
 * @endcode
 * 
 * @example Shared Log Output
 * Both components log to the same file:
 * @code
 * // workdir/logs/runtime.log:
 * === Runtime session started (pid=12345) ===
 * [INFO] [MUTATOR] Initializing ISA mutator
 * [INFO] [MUTATOR] Target ISA: rv32imc
 * [INFO] [MUTATOR] Loaded 47 instructions from schema
 * [Fn Start  ] ISAMutator.cpp::applyMutations
 * [DEBUG] [MUTATOR] Mutating 128 bytes
 * [Fn End    ] ISAMutator.cpp::applyMutations
 * [INFO] [HARNESS] Executing program (128 bytes)
 * [Fn Start  ] CpuPicorv32.cpp::executeProgram
 * [DEBUG] [HARNESS] Execution complete, cycles: 542
 * [Fn End    ] CpuPicorv32.cpp::executeProgram
 * @endcode
 * 
 * @note All functions are thread-safe and can be called concurrently from multiple threads.
 * @note All logging is always enabled to capture runtime behavior.
 * @see For complete usage guide and migration examples, see afl/isa_mutator/DEBUG_API.md
 */
namespace debug {

/**
 * @brief Check if runtime logging is enabled
 * 
 * @details
 * Returns true if the runtime logging system is initialized and ready.
 * This function caches the result after first call.
 * 
 * Use this when you need to conditionally execute expensive logging operations.
 * 
 * @return true if logging is initialized, false otherwise
 * 
 * @note The logging system is always enabled to capture runtime behavior.
 * @note This function mainly exists for backward compatibility.
 * 
 * @example
 * @code
 * void processInstruction(uint32_t instr) {
 *     // Only disassemble and log if logging is available
 *     if (hwfuzz::debug::isDebugEnabled()) {
 *         std::string disasm = disassemble(instr);
 *         hwfuzz::debug::logDebug("[HARNESS] Decoded: %s\n", disasm.c_str());
 *     }
 *     
 *     // Normal processing always happens
 *     execute(instr);
 * }
 * @endcode
 */
bool isDebugEnabled();

/**
 * @brief Get the log file handle for runtime output
 * 
 * @details
 * Returns the FILE* handle for the runtime log file. This is primarily for
 * advanced use cases where you need direct file access (e.g., custom formatting,
 * binary dumps, etc.).
 * 
 * For most logging, use the provided logInfo(), logWarn(), etc. functions instead.
 * 
 * @return FILE* pointing to runtime log file (workdir/logs/runtime.log)
 *         Returns nullptr if logging cannot be initialized
 * 
 * @warning Do not close or modify the file handle. It's managed internally.
 * @warning Remember to fflush() if you write to it directly.
 * 
 * @example Custom Formatted Output
 * @code
 * void dumpInstructionBytes(const uint8_t* buf, size_t len) {
 *     FILE* log = hwfuzz::debug::getDebugLog();
 *     if (!log) return;  // Logging unavailable
 *     
 *     std::fprintf(log, "[HARNESS] Instruction bytes (%zu):\n", len);
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
 * Logs informational messages to the runtime log file. Use this for:
 * - Configuration loading status
 * - Initialization progress
 * - General operational milestones
 * - Non-error state changes
 * 
 * Output format: `[INFO] <your message>`
 * 
 * **Recommended component prefixes:**
 * - Mutator: `[MUTATOR]` prefix
 * - Harness: `[HARNESS]` prefix
 * 
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 * 
 * @note Always logs to runtime.log to capture all behavior.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * @note Automatically flushed to disk for immediate visibility.
 * 
 * @example
 * @code
 * void CpuIface::initialize() {
 *     hwfuzz::debug::logInfo("[HARNESS] Initializing CPU interface\n");
 *     hwfuzz::debug::logInfo("[HARNESS] Verilator version: %s\n", version);
 *     hwfuzz::debug::logInfo("[HARNESS] Ready for execution\n");
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
 * @note Always logs to runtime.log to capture all behavior.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * 
 * @example
 * @code
 * void CpuIface::executeInstruction(uint32_t instr) {
 *     if (!isValidOpcode(instr)) {
 *         hwfuzz::debug::logWarn("[HARNESS] Invalid opcode 0x%08x, treating as NOP\n", instr);
 *         return;
 *     }
 *     execute(instr);
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
 * @note Always logs to runtime.log to capture all behavior.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * @note Does NOT terminate the program - call exit() separately if needed.
 * 
 * @example
 * @code
 * void CpuIface::loadProgram(const uint8_t* buf, size_t size) {
 *     if (size > MAX_PROGRAM_SIZE) {
 *         hwfuzz::debug::logError("[HARNESS] Program too large: %zu > %zu\n", 
 *                                 size, MAX_PROGRAM_SIZE);
 *         std::exit(1);
 *     }
 *     memcpy(memory_, buf, size);
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
 * @note Always logs to runtime.log to capture all behavior.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * 
 * @example
 * @code
 * void CpuIface::step() {
 *     hwfuzz::debug::logDebug("[HARNESS] Cycle %llu: PC=0x%08x\n", cycles_, pc_);
 *     hwfuzz::debug::logDebug("[HARNESS] Fetching instruction...\n");
 *     uint32_t instr = fetchInstruction(pc_);
 *     hwfuzz::debug::logDebug("[HARNESS] Decoded: %s\n", disassemble(instr).c_str());
 *     execute(instr);
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
 * @note Always logs to runtime.log to capture all behavior.
 * @note Thread-safe: Can be called from multiple threads concurrently.
 * @note Primarily used by ISA mutator, but available to harness if needed.
 * 
 * @example
 * @code
 * void ISAMutator::mutateImmediate(uint32_t* instr) {
 *     uint32_t original = *instr;
 *     uint32_t mutated = applyImmediateMutation(original);
 *     
 *     if (!isLegalInstruction(mutated)) {
 *         hwfuzz::debug::logIllegal(__FUNCTION__, original, mutated);
 *         return;  // Don't apply illegal mutation
 *     }
 *     
 *     *instr = mutated;
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
 *     const char* file = hwfuzz::debug::basename(__FILE__);
 *     hwfuzz::debug::logInfo("[HARNESS] Current file: %s, line: %d\n", file, __LINE__);
 * }
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
 * debugging exception handling paths across both mutator and harness.
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
 * @example Harness Function Tracing
 * @code
 * void CpuIface::executeProgram(const uint8_t* buf, size_t size) {
 *     hwfuzz::debug::FunctionTracer tracer(__FILE__, __FUNCTION__);
 *     
 *     loadMemory(buf, size);
 *     runSimulation();
 *     checkResults();
 *     
 *     // Exit logged automatically when tracer goes out of scope
 * }
 * @endcode
 * 
 * @example Mutator Function Tracing
 * @code
 * void ISAMutator::applyMutations(uint8_t* buf, size_t size) {
 *     hwfuzz::debug::FunctionTracer tracer(__FILE__, __FUNCTION__);
 *     
 *     pickInstruction();
 *     encodeInstruction();
 *     
 *     // Exit logged automatically
 * }
 * @endcode
 */
class FunctionTracer {
public:
  /**
   * @brief Construct tracer and log function entry
   * 
   * @param file Source file path (use __FILE__ macro)
   * @param func Function name (use __FUNCTION__ or __func__ macro)
   */
  FunctionTracer(const char* file, const char* func);
  
  /**
   * @brief Destructor logs function exit
   * 
   * @note Guaranteed to run even during stack unwinding (exceptions)
   */
  ~FunctionTracer();

private:
  const char* base_;  ///< Basename of source file
  const char* func_;  ///< Function name
};

} // namespace debug
} // namespace hwfuzz
