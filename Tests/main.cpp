#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>
#include <juce_events/juce_events.h>

int main(int argc, char* argv[])
{
    int result = Catch::Session().run(argc, argv);
    juce::DeletedAtShutdown::deleteAll();  // clean up JUCE singletons
    return result;
}
