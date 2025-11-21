#include "code_hash.h"
#include "code_hash_generated.h"
#include <string>

namespace code_hash {
std::string compute_shared_code_hash() { return std::string(SHARED_CODE_HASH); }
} // namespace code_hash
