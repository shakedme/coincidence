# Coincidence


<img width="805" alt="Screenshot 2025-06-16 at 9 37 37" src="https://github.com/user-attachments/assets/60dd2113-b575-46ed-ba93-958e03a28b8e" />
<img width="807" alt="Screenshot 2025-06-16 at 9 38 29" src="https://github.com/user-attachments/assets/f6bc4c9e-a368-4e87-9860-e4834fbd5544" />


Coincidence is an audio plugin that combines sample manipulation, MIDI generation, and effects processing, designed to create complex, glitchy, and psychedelic soundscapes. It allows users to trigger samples with probability-based controls, manipulate MIDI notes, and apply a variety of audio effects in real-time using graphical controls. The plugin is built with the JUCE framework.

It was built mostly for the fun of it and the learning process, trying to encapsulate my workflow for creating glitchy-psychedelic sequences, which usually requires several plugins. I heavily relied on LLMs for the more complex parts of it.

It's also not really complete (: 

I kind of lost interest in it, but I still think it's a fun project and I might come back to it in the future.


## Features

### Sample Management
- Multi-sample support with sample grouping
- Flexible sample playback with start/end markers
- Probability-based sample triggering
- Multiple playback rates (1/1, 1/2, 1/4, 1/8, 1/16, 1/32)
- Sample direction control
- Group-based organization with probability control

### MIDI Capabilities
- MIDI input and output support
- Dual-mode operation:
  - Sample playback mode
  - MIDI effect mode
- Note generation and manipulation
- Scale management for musical control

### Effects Processing
Comprehensive effects chain including:
- Reverb
- Delay
- Stutter
- Flanger
- Phaser
- Compression
- Gain control
- Panning

### Modulation
- Advanced modulation matrix
- Parameter modulation capabilities
- Real-time control over multiple parameters

### User Interface
- Waveform visualization
- Envelope editor with preset generation
- Sample management interface
- Real-time parameter control
- Direction control buttons
- Group and sample probability controls

## Requirements
- VST3 compatible host or standalone application
- Modern operating system (Windows/macOS)

## Development
Built with:
- C++20
- JUCE framework
- CMake build system
