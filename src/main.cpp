#include "DiscoveryEngine.hpp"
#include "core/DeviceStateStore.hpp"
#include "event_sink/ChangeFilteredEventSink.hpp"
#include "event_sink/StdoutEventSink.hpp"
#include "log/ThreadSafeLogger.hpp"
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>

static volatile std::sig_atomic_t g_shutdown_requested = 0;

static void handle_signal(int /*signum*/) {
    g_shutdown_requested = 1;
}

static bool has_flag(int argc, char* argv[], const char* flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

static std::string get_option_value(int argc, char* argv[], const char* flag) {
    for (int i = 1; i < argc - 1; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return {};
}

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [options]\n"
              << "  -d, --debug    Log level DEBUG (engine diagnostics)\n"
              << "  -q, --quiet    Log level WARN (errors and warnings only)\n"
              << "  -v, --verbose  Wire-level hex dumps and parse summaries\n"
              << "  -I, -i,--interface <name>  Bind multicast to interface (e.g. eth0)\n"
              << "  -h, --help     Show this help\n"
              << "\nDefault log level is INFO. Discovery lines print on new/changed records.\n";
}

static LogLevel parse_log_level(int argc, char* argv[]) {
    if (has_flag(argc, argv, "-d") || has_flag(argc, argv, "--debug")) {
        return LogLevel::Debug;
    }
    if (has_flag(argc, argv, "-q") || has_flag(argc, argv, "--quiet")) {
        return LogLevel::Warn;
    }
    return LogLevel::Info;
}

int main(int argc, char* argv[]) {
    if (has_flag(argc, argv, "-h") || has_flag(argc, argv, "--help")) {
        print_usage(argv[0]);
        return 0;
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    const LogLevel log_level = parse_log_level(argc, argv);
    const bool wire_verbose = has_flag(argc, argv, "-v") || has_flag(argc, argv, "--verbose");

    auto logger = std::make_shared<StdoutLogger>(log_level);
    auto device_store = std::make_shared<DeviceStateStore>();

    std::string iface = get_option_value(argc, argv, "-I");
    if (iface.empty()) iface = get_option_value(argc, argv, "-i");
    if (iface.empty()) iface = get_option_value(argc, argv, "--interface");

    DiscoveryEngine engine(logger);
    engine.set_verbose(wire_verbose);
    if (!iface.empty()) engine.set_interface(iface);
    engine.set_event_sink(make_change_filtered_event_sink(
        device_store,
        make_stdout_event_sink(logger)));

    bool ok = engine.run(5000, []() { return g_shutdown_requested != 0; });
    return ok ? 0 : 1;
}
