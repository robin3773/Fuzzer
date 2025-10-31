#pragma once

#include <cstdint>
#include <filesystem>
#include <utility>
#include <string>
#include <unordered_set>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace fuzz::isa {
namespace yaml_utils {

/**
 * @brief Parse a YAML scalar node as an integer, supporting decimal, hex (0x), and binary (0b) formats.
 *
 * @param node YAML node containing the integer value
 * @return uint64_t Parsed integer value (0 if node is null/empty)
 * @throws std::runtime_error if the literal format is invalid
 */
uint64_t parse_integer(const YAML::Node &node);

/**
 * @brief Merge two YAML nodes recursively, with overlay taking precedence.
 *
 * For maps, merges keys recursively. For other types, overlay replaces base.
 * Keys starting with "__" are skipped during merge.
 *
 * @param base Base YAML node (modified in-place)
 * @param overlay Overlay YAML node to merge into base
 */
void merge_nodes(YAML::Node &base, const YAML::Node &overlay);

/**
 * @brief Remove surrounding single or double quotes from a string.
 *
 * @param s Input string
 * @return std::string String with quotes removed
 */
std::string strip_quotes(std::string s);

/**
 * @brief Split a multi-line string into individual lines.
 *
 * @param text Input text
 * @return std::vector<std::string> Vector of lines
 */
std::vector<std::string> split_lines(const std::string &text);

/**
 * @brief Extract file paths associated with a YAML key from raw text.
 *
 * Handles both inline array syntax and multi-line list syntax.
 * Example:
 *   extends: [file1.yaml, file2.yaml]
 *   include:
 *     - file3.yaml
 *     - file4.yaml
 *
 * @param text Raw YAML text content
 * @param key Key name to search for (e.g., "extends", "include")
 * @return std::vector<std::string> List of file paths found
 */
std::vector<std::string> extract_paths_for_key(const std::string &text, const std::string &key);

/**
 * @brief Read entire file contents into a string.
 *
 * @param path Filesystem path to read
 * @return std::string File contents
 * @throws std::runtime_error if file cannot be opened
 */
std::string read_file_to_string(const std::filesystem::path &path);

/**
 * @brief Extract YAML anchor definitions from raw text.
 *
 * Scans for anchor definitions (e.g., &anchor_name) and captures the anchor
 * block including all indented content following the anchor line.
 *
 * @param text Raw YAML text content
 * @return std::map<std::string, std::string> Map from anchor name to anchor block text
 */
std::vector<std::pair<std::string, std::string>> extract_anchor_blocks(const std::string &text);

/**
 * @brief Build a YAML __anchors section from a collection of anchor blocks.
 *
 * Creates a synthetic YAML section that can be prepended to files to make
 * anchors from other files available for alias resolution.
 *
 * @param anchors Map of anchor names to anchor block text
 * @return std::string YAML-formatted __anchors section
 */
std::string build_anchor_context(const std::vector<std::pair<std::string, std::string>> &anchors);

/**
 * @brief Recursively collect schema file dependencies following extends/include directives.
 *
 * Performs topological ordering: dependencies appear before dependents in the result.
 * Handles cycles by tracking visited files.
 *
 * @param path Schema file to process
 * @param ordered Output vector receiving files in dependency order
 * @param visited Set of already-processed file paths (for cycle detection)
 */
void collect_dependencies(const std::filesystem::path &path,
                          std::vector<std::filesystem::path> &ordered,
                          std::unordered_set<std::string> &visited);

/**
 * @brief Load ISA schema includes from an ISA map file.
 *
 * Searches for the given ISA name in the map file structure, which may contain
 * either flat entries or nested isa_families. Returns the list of schema files
 * to include for the specified ISA.
 *
 * @param map_path Path to isa_map.yaml file
 * @param isa_name ISA identifier to look up
 * @return std::vector<std::string> List of schema file paths to include
 * @throws std::runtime_error if map file parsing fails
 */
std::vector<std::string> includes_from_map(const std::filesystem::path &map_path,
                                           const std::string &isa_name);

} // namespace yaml_utils
} // namespace fuzz::isa
