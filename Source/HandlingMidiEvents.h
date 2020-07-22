/*
  ==============================================================================
   This file is part of the JUCE tutorials.
   Copyright (c) 2017 - ROLI Ltd.
   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.
   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.
  ==============================================================================
*/
/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.
 BEGIN_JUCE_PIP_METADATA
 name:             HandlingMidiEventsTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Handles incoming midi events.
 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make
 type:             Component
 mainClass:        MainContentComponent
 useLocalCopy:     1
 END_JUCE_PIP_METADATA
*******************************************************************************/
#pragma once
//==============================================================================
class MainContentComponent : public juce::Component,
    private juce::MidiInputCallback,
    private juce::MidiKeyboardStateListener
{
    struct TunerValues
    {
        String noteName;
        double noteHex;
    };
public:
    MainContentComponent()
        :startTime(juce::Time::getMillisecondCounterHiRes() * 0.001)
    {
        setOpaque(true);
        addAndMakeVisible(midiInputListLabel);
        midiInputListLabel.setText("MIDI Input:", juce::dontSendNotification);
        midiInputListLabel.attachToComponent(&midiInputList, true);
        addAndMakeVisible(midiInputList);
        midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs Enabled");
        auto midiInputs = juce::MidiInput::getAvailableDevices();
        juce::StringArray midiInputNames;
        for (auto input : midiInputs)
            midiInputNames.add(input.name);
        midiInputList.addItemList(midiInputNames, 1);
        midiInputList.onChange = [this] { setMidiInput(midiInputList.getSelectedItemIndex()); };
        // find the first enabled device and use that by default
        for (auto input : midiInputs)
        {
            if (deviceManager.isMidiInputDeviceEnabled(input.identifier))
            {
                setMidiInput(midiInputs.indexOf(input));
                break;
            }
        }
        // if no enabled devices were found just use the first one in the list
        if (midiInputList.getSelectedId() == 0)
            setMidiInput(0);
        //        addAndMakeVisible (keyboardComponent);
        //        keyboardState.addListener (this);
                //addAndMakeVisible (midiMessagesBox);
        midiMessagesBox.setMultiLine(true);
        midiMessagesBox.setReturnKeyStartsNewLine(true);
        midiMessagesBox.setReadOnly(true);
        midiMessagesBox.setScrollbarsShown(true);
        midiMessagesBox.setCaretVisible(false);
        midiMessagesBox.setPopupMenuEnabled(true);
        midiMessagesBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x32ffffff));
        midiMessagesBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x1c000000));
        midiMessagesBox.setColour(juce::TextEditor::shadowColourId, juce::Colour(0x16000000));
        addAndMakeVisible(lblTunerNote);
        lblTunerNote.setText("---", juce::dontSendNotification);
        lblTunerNote.setColour(juce::Label::textColourId, juce::Colours::orange);
        lblTunerNote.setJustificationType(juce::Justification::centred);
        lblTunerNote.setFont(juce::Font(136.0f, juce::Font::bold));
        addAndMakeVisible(lblTunerHrtz);
        lblTunerHrtz.setText("0 HZ", juce::dontSendNotification);
        lblTunerHrtz.setColour(juce::Label::textColourId, juce::Colours::grey);
        lblTunerHrtz.setJustificationType(juce::Justification::centred);
        lblTunerHrtz.setFont(juce::Font(30.0f, juce::Font::plain));
        setSize(600, 400);
    }
    ~MainContentComponent() override
    {
        deviceManager.removeMidiInputDeviceCallback(juce::MidiInput::getAvailableDevices()[midiInputList.getSelectedItemIndex()].identifier, this);
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(Colour::fromRGB(30, 30, 30));
    }
    void resized() override
    {
        midiInputList.setBounds(10, 10, getWidth() - 20, 30);
        lblTunerNote.setBounds(11, 80, getWidth(), 100);
        lblTunerHrtz.setBounds(11, 180, getWidth(), 50);
    }
private:
    static TunerValues getMidiMessageDescription(const juce::MidiMessage& m)
    {
        if (m.isNoteOn()) {
            TunerValues tuneVals = { juce::MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3), m.getMidiNoteInHertz(m.getNoteNumber()-12) };
            return tuneVals;
        }
        if (m.isNoteOff())
            return { "",0 };
        return { "",0 };
    }
    /** Starts listening to a MIDI input device, enabling it if necessary. */
    void setMidiInput(int index)
    {
        auto list = juce::MidiInput::getAvailableDevices();
        deviceManager.removeMidiInputDeviceCallback(list[lastInputIndex].identifier, this);
        auto newInput = list[index];
        if (!deviceManager.isMidiInputDeviceEnabled(newInput.identifier))
            deviceManager.setMidiInputDeviceEnabled(newInput.identifier, true);
        deviceManager.addMidiInputDeviceCallback(newInput.identifier, this);
        midiInputList.setSelectedId(index + 1, juce::dontSendNotification);
        lastInputIndex = index;
    }
    // These methods handle callbacks from the midi device + on-screen keyboard..
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override
    {
        const juce::ScopedValueSetter<bool> scopedInputFlag(isAddingFromMidiInput, true);
        //        keyboardState.processNextMidiEvent (message);
        postMessageToList(message, source->getName());
    }
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (!isAddingFromMidiInput)
        {
            auto m = juce::MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);
            m.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            postMessageToList(m, "On-Screen Keyboard");
        }
    }
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override
    {
        if (!isAddingFromMidiInput)
        {
            auto m = juce::MidiMessage::noteOff(midiChannel, midiNoteNumber);
            m.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
            postMessageToList(m, "On-Screen Keyboard");
        }
    }
    // This is used to dispach an incoming message to the message thread
    class IncomingMessageCallback : public juce::CallbackMessage
    {
    public:
        IncomingMessageCallback(MainContentComponent* o, const juce::MidiMessage& m, const juce::String& s)
            : owner(o), message(m), source(s)
        {}
        void messageCallback() override
        {
            if (owner != nullptr)
                owner->displayMessage(message, source);
        }
        Component::SafePointer<MainContentComponent> owner;
        juce::MidiMessage message;
        juce::String source;
    };
    void postMessageToList(const juce::MidiMessage& message, const juce::String& source)
    {
        (new IncomingMessageCallback(this, message, source))->post();
    }
    void displayMessage(const juce::MidiMessage& message, const juce::String& source)
    {
        TunerValues tunerValues = getMidiMessageDescription(message);
        String note = "";
        double hrtz = 0.0;
        if (tunerValues.noteHex != 0) {
            note = tunerValues.noteName;
            hrtz = tunerValues.noteHex;
        }
        lblTunerNote.setText(note, juce::dontSendNotification);
        lblTunerHrtz.setText((String)hrtz + " Hz", juce::dontSendNotification);
    }
    //==============================================================================
    juce::AudioDeviceManager deviceManager;           
    juce::ComboBox midiInputList;                     
    juce::Label midiInputListLabel;
    int lastInputIndex = 0;                           
    bool isAddingFromMidiInput = false;               
    juce::TextEditor midiMessagesBox;
    juce::Label lblTunerNote;
    juce::Label lblTunerHrtz;
    double startTime;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};