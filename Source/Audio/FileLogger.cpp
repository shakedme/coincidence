#include "PluginProcessor.h"

FileLogger::FileLogger()
{
    logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                  .getChildFile("plugin_debug.log");

    if (!logFile.existsAsFile())
        logFile.create();
}

void FileLogger::logMessage(const juce::String& message)
{
    juce::Time currentTime = juce::Time::getCurrentTime();
    juce::String timestamp = currentTime.toString(true, true);

    logFile.appendText("[" + timestamp + "] " + message + "\n", false, false);
}