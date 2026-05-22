#include "util/BufferBoundsValidator.hpp"
#include <sstream>

void BufferBoundsValidator::ensure_readable(size_t current_pos, size_t bytes_needed, 
                                            size_t buffer_len, const std::string& field_name) {
    if (current_pos + bytes_needed > buffer_len) {
        std::ostringstream oss;
        oss << "Buffer bounds exceeded while reading '" << field_name << "': "
            << "position=" << current_pos << ", need=" << bytes_needed 
            << ", buffer_len=" << buffer_len;
        throw std::invalid_argument(oss.str());
    }
}
