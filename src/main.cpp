#include "DiscoveryEngine.hpp"

/**
 * @brief Program entry point: constructs and runs the discovery engine.
 */
int main() {
    DiscoveryEngine engine;

    if (engine.start()) {
        engine.broadcast_query();
        engine.listen_and_parse();
    }

    return 0;
}