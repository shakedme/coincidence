include("/Users/shakedmelman/Projects/coincidence/CMake/CPM.cmake")
CPMAddPackage("GITHUB_REPOSITORY;juce-framework/JUCE;GIT_TAG;develop;EXCLUDE_FROM_ALL;YES;SYSTEM;YES;")
set(JUCE_FOUND TRUE)