#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <iostream>

struct ConsoleLogger : public juce::Logger {
    void logMessage(const juce::String& msg) override {
        std::cout << msg << std::endl;
    }
};

// Attach at program start
static ConsoleLogger juceLogger;
struct LoggerAttach {
    LoggerAttach()  { juce::Logger::setCurrentLogger(&juceLogger); }
    ~LoggerAttach() { juce::Logger::setCurrentLogger(nullptr); }
} attachLogger;

int main(int argc, char* argv[])
{
    int result = Catch::Session().run(argc, argv);
    juce::DeletedAtShutdown::deleteAll();
    return result;
}
