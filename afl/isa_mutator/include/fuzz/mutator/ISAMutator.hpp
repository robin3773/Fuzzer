#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <fuzz/isa/IsaLoader.hpp>
#include <fuzz/mutator/MutatorConfig.hpp>
#include <fuzz/mutator/MutatorInterface.hpp>

namespace fuzz::mutator {

class ISAMutator : public MutatorInterface {
public:
  ISAMutator();
  void initFromEnv() override;
  void setConfigPath(const std::string &path) override { cli_config_path_ = path; }

  unsigned char *mutateStream(unsigned char *in, size_t in_len,
                              unsigned char *out_buf,
                              size_t max_size) override;

  size_t last_out_len() const override { return last_len_; }

  bool using_schema() const { return use_schema_; }
  const std::string &isa_name() const { return isa_.isa_name; }
  Strategy strategy() const { return cfg_.strategy; }

private:
  Config cfg_;
  isa::ISAConfig isa_;
  std::string cli_config_path_;
  bool use_schema_ = false;
  size_t last_len_ = 0;
  uint32_t word_bytes_ = 4;

  unsigned char *mutateWithSchema(unsigned char *in, size_t in_len,
                                   unsigned char *out_buf,
                                   size_t max_size);
  unsigned char *mutateFallback(unsigned char *in, size_t in_len,
                                 unsigned char *out_buf,
                                 size_t max_size);

  const isa::InstructionSpec &pickInstruction() const;
  uint32_t encodeInstruction(const isa::InstructionSpec &) const;
  uint32_t randomFieldValue(const std::string &field_name,
                            const isa::FieldEncoding &enc) const;
  void applyField(uint32_t &word, const isa::FieldEncoding &enc,
                  uint32_t value) const;
  void writeWord(unsigned char *buf, size_t offset, uint32_t word) const;

  struct Rule {
    std::string type;
    uint32_t weight = 10;
    uint32_t min = 1;
    uint32_t max = 1;
    std::vector<uint8_t> pattern;
  };

  std::vector<Rule> rules_;
  bool loadFallbackConfig(const std::string &path);
  static std::string trim(const std::string &s);
  void applyRule(const Rule &r, unsigned char *buf, size_t &len, size_t cap);
};

} // namespace fuzz::mutator
