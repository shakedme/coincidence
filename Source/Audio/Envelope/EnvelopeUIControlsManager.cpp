void EnvelopeUIControlsManager::setSnapToGridEnabled(bool enabled) {
    snapToGridEnabled = enabled;
    
    if (snapToGridButton != nullptr) {
        snapToGridButton->setToggleState(enabled, juce::dontSendNotification);
    }
    
    if (onSnapToGridChanged) {
        onSnapToGridChanged(enabled);
    }
} 