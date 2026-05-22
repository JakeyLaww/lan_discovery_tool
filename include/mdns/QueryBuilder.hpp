#pragma once

#include <vector>
#include <string>
#include <cstdint>

/**
 * @brief Builder for constructing DNS query messages.
 *
 * Encapsulates the low-level byte manipulation needed to construct
 * valid DNS query messages. Provides a simple fluent interface for
 * adding questions to a query.
 */
class QueryBuilder {
private:
    uint16_t transaction_id;
    uint16_t question_count_{0};
    std::vector<uint8_t> buffer;

    /**
     * @brief Append a domain name label to the buffer using DNS name encoding.
     *
     * @param label Label (e.g., "_http", "_tcp", "local")
     */
    void append_label(const std::string& label);

public:
    /**
     * @brief Construct a new QueryBuilder with a given transaction ID.
     *
     * @param tid Transaction ID (default 0 for multicast discovery).
     */
    explicit QueryBuilder(uint16_t tid = 0);

    /**
     * @brief Add a question to the query.
     *
     * @param name Fully qualified domain name (e.g., "_http._tcp.local")
     * @param type DNS record type (1=A, 28=AAAA, 12=PTR, 33=SRV, 16=TXT, etc.)
     * @param class_code DNS class (typically 1 for IN)
     * @return Reference to this builder for method chaining.
     */
    QueryBuilder& add_question(const std::string& name, uint16_t type, uint16_t class_code = 1);

    /**
     * @brief Build and return the completed DNS query message.
     *
     * @return Vector of bytes representing the DNS query.
     */
    std::vector<uint8_t> build() const;
};
