[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/-Acvnhrq)

# Final Project

**Team Number: 13**

**Team Name: ModBox**

| Team Member Name  | Email Address           |
| ----------------- | ----------------------- |
| Bhavya Surapaneni | bhavyasp@seas.upenn.edu |
| Katya Mazurenko   | kmazu@seas.upenn.edu    |
| Sarah Liao        | sarahl28@seas.upenn.edu |
| Mary Mbuzi        | mmbuzi28@seas.upenn.edu |

**GitHub Repository URL: [https://github.com/upenn-embedded/final-project-s26-t13.git]([https://github.com/upenn-embedded/final-project-s26-t13.git]())**

**GitHub Pages Website URL:** [for final submission]*

## Final Project Proposal

### 1. Abstract

For our final project, we are building a completely embedded modular synthesizer. Modular synthesizers use different modules to produce signals that are usually analog, and you are able to modulate the signal in different ways by altering the connections between modules such as amplifiers, filters, oscillators, shapers. We want to take this concept and make it embedded, using both firmware and hardware to create an embedded modular synth box. The actual modules will be analogue, built with components like capacitors, resistors, and transistors to get a more authentic, imperfect sound, but the operational features like buttons and knobs for each module will operate as per our firmware, defining the order that modules operate and their relative intensity. We will have two input mechanisms that can be toggled between with a button - the first is inputting a control voltage that can then be modulated into a sound wave, and the second is inputting live audio that will be discretized into a voltage that can be similarly modulated. All of these components will be in a clean form factor that will operate as an instrument that can be easily used for electronic music creation.

### 2. Motivation

Our motivation for this project is a general interest in synthesizers and modular synthesis, and curiosity to see how they translate to the embedded world. It’s pretty normal to create basic modular synthesizers with MCUs, but we want to take it to the next level and add more features to make it a nice combination of the analog and digital worlds.

### 3. System Block Diagram

![block diagram](images/blockdiagram.png)

### 4. Design Sketches

![drawing](images/drawing.png)

### 5. Software Requirements Specification (SRS)

**5.1 Definitions, Abbreviations**

Audio sampling and processing:

* Taking input from a microphone, it will amplify and shift the signal to fit the ADC. The audio MCU samples the microphone signal so that a number represents the amplitude of the voice waveform at that moment. We can then monitor input amplitude for control and diagnostic purposes.

Module routing:

* Using buttons and knobs, the system will allow users to enable, disable, or adjust the modules parameters in real time without interrupting audio playback.
* The software should detect button presses using interrupts which will enable or disable audio processing modules. It should also include button debouncing.
* It will also read analog knob positions using the MCU ADC so the software can change the strength or intensity of a given module
* Validation methods could include adjusting modules while the audio is playing continuously and ensuring they are shaping the audio in the way they are intended and not causing audio dropouts.

MCU communication:

* The two MCUs will communicate with each other via I2C. MCU 1 will read the knobs (potentiometers) using ADC, detect button presses using interrupts, determine which module is active or bypassed, and send control values to the second MCU 2. MCU 2 will denerate control voltages for modules, control analog switches that enable/bypass modules, adjust effect intensity based on knob values, monitor audio levels using ADC if needed, and control any timing-related effects (like echo).

Output audio:

* We will then output the audio signal through a speaker. We can monitor the output signal level using the MCU ADC to ensure the signal remains within the allowable operating range. This will prevent clipping or excessive signal levels.

**5.2 Functionality**

| ID     | Description                                                                                                                            |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------- |
| SRS-01 | The buttons that control the input voltage and inputing live audio will use interrupts and require software debouncing.                |
| SRS-02 | The two MCUs must communicate using I2C where MCU1 sends updates to MCU2 when values change and MCU2 receives data using an interrupt. |
| SRS-03 | The microphone should intake audio when a button is pressed and the input signal should be monitored using an ADC.                     |
| SRS-04 | The ouput audio will also be monitred using the ADC to detect distortion or clipping.                                                  |

### 6. Hardware Requirements Specification (HRS)

**6.1 Definitions, Abbreviations**

Microphone:

* Since the audio input from the microphone won’t provide the best signal quality, we will need to amplify and filter the signal through hardware.

Modular signal processing:

* The modules such as ring modulator, voltage controlled filter, envelope follower, echo, and wave folder will all be hardware which are made of inductors, capacitors, resistors, transistors and op amps.
* The ring modulator: multiples the input audio signal with a carrier oscillator signal to produce sum and difference frequency components, creating a modulation effect
* Voltage controlled filter: attenuate frequencies above a configurable cutoff frequency while allowing lower frequencies to pass
* Envelope follower: detect the amplitude envelope of the incoming audio signal and produce a corresponding control voltage proportional to the signal amplitude
* Echo: reproduce delayed versions of the input signal to create a repeating echo effect
* Wave folder: "reflects" the waveform back on itself to introduce additional harmonic sound

Module routing (oscillator, filter, amplifier):

* Some module routing requires a specific sequence of events: the oscillator produces the periodic waveform, the filter shapes the tone of sound, and the amplifier amplifies the sound. So, hardware requirements might include physically routing some modules to create a desired output.

Audio output:

* We will likely require an audio buffer to stabilize and amplify the signal and to drive it so it can play through a speaker.

**

**6.2 Functionality**

| ID     | Description                                                                                                                                    |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| HRS-01 | A microphone will intake live audio and play through the modules.                                                                              |
| HRS-02 | The modules will be hardware based and made up of passive components.                                                                          |
| HRS-03 | Some manual routing will probably be required, so connecting certain modules through hardware will be needed.                                  |
| HRS-04 | The buttons and knobs will turn a module or adjust the input voltage which changed the intensity of the module.                                |
| HRS-05 | The audio will output threough a speaker which will require an audio buffer/amplifer and audio amplifer to drive the signal through a speaker. |

### 7. Bill of Materials (BOM)

Link: [https://docs.google.com/spreadsheets/d/1FfpUyTM7GOHpUId-kbmIrWoPR1Q2QyKV87Wz2lGQ6IE/edit?usp=sharing](https://docs.google.com/spreadsheets/d/1FfpUyTM7GOHpUId-kbmIrWoPR1Q2QyKV87Wz2lGQ6IE/edit?usp=sharing)

### 8. Final Demo Goals

For our final demo of the project, we plan on showing the different modules that the synthesizer does and how it affects the output of sound. We also plan to do a live demo of inputting audio with someone’s voice and playing around with the synthesizer to create and shape cool new sounds. There shouldn’t be too many constraints here since it doesn’t actually attach to anyone, but we need to make sure the MCUs are supplied with power so the device operates, likely with batteries.

### 9. Sprint Planning

| Milestone  | Functionality Achieved                                                                                             | Distribution of Work |
| ---------- | ------------------------------------------------------------------------------------------------------------------ | -------------------- |
| Sprint #1  | build an input module for the voice input and for the control voltage input, successfully demo on the oscilloscope |                      |
| Sprint #2  | have 2-3 successful modules (on top of input modules), demo on oscilloscope                                        |                      |
| MVP Demo   | have all modules complete, demo with oscilloscope and speaker.                                                     |                      |
| Final Demo | have a completed cad model of the design box, along with fully functioning device.                                 |                      |

**This is the end of the Project Proposal section. The remaining sections will be filled out based on the milestone schedule.**

## Sprint Review #1

### Last week's progress

So far, we have met with Andrea, our project manager, and figured out our situation in terms of parts. We found that despite needing a microphone and preamplifier, Detkin actually had the parts that we needed so we do not need to order anything a la carte for this project. We also decided to shift our microcontrollers to the STM32s instead of the ATMega328-PB, since the STM32 is much better suited for ADC applications and our project is largely using ADC. We also decided to shift our communication protocol between our microcontrollers from I2C to I2S, as per Andrea's guidance, since I2S is much better suited for high-quality audio.

As of the beginning of our first lab period designated for the final project, we do not have any code or hardware written. Our plan is to create our CircuitLab schematic for all of the connections between our physical hardware and module creation. We will also confirm all of our parts. We plan to start on at least some portions of the hardware connections (input, output, 1 module (VCO)), and get a decent start on our firmware. At the very least, figure out a structure for our code, learn the STM registers better so we can code more efficiently, and get some files started with pseudocode for what they need to do. At the end of the lab section, we will include our schematic and indicate which portions we started for hardware.

TLDR; GOALS:

* Circuitlab schematic
* Confirm all parts
* Hardware: Input, Output, VCO Module
* Firmware: Learn registers, file structure w/ pseudocode

### Current state of project

As of the end of the lab period, we've started looking into the registers of the STM, we built the VCO module and the speaker, confirmed all of our parts, and got started on a circuit schematic. We decided that since the modules are relatively simple and there are existing schematics for oscillators, amplifiers, and envelope generators, for now we will black box the modules on our schematic and focus more on the connections between the circuits and the STMs, which is more complex. As we finalize our designs after iteration, we will develop a full, accurate circuit schematic that we can reference as we make changes to connections and code, as well as the final form factor.

We demonstrated to Andrea that our oscillator works and is able to take a DC control voltage from the Keysight power supply and turn it into an AC triangle signal or square wave. We also showed her the beginnings of our circuit schematic, as well as our document verbally walking through descriptions of the schematic, which is in this repository at **schematic_description.txt**.  We also finalized all parts, sourced them from Detkin's storage, and began looking into how to connect them. Part list is here: finalized_parts.txt.

Photo of oscillator:
![vco photo](images/sprint1circuit.png)

Speaker driver pinout:
![speakerdriver](images/speakerdriver.png)

### Next week's plan

By the beginning of next week, we plan to have a finished schematic done and know exactly what pins to connect our microcontrollers to inputs and outputs of our modules and other hardware. We plan to have the beginnings of our code, as well as have a working input module for the microphone.

By the end of next week, we want a decent amount of code done, as well as a near-done MVP that takes in input from a control voltage. is able to oscillate the control voltage and adapt the pitch and shape using potentiometers, and output the adapted signal.

Though we plan to have the microphone module and logic done by next week, we plan to focus more on the microphone pipeline after sprint review #2 and ensure it's done by the MVP demo. This will require having the speech input module done, which will discretize the speech input and output it, to be put into the amplifier or other modules.

**Schematic and Design**: Bhavya

- will be done by beginning of next lab session
- 3-4 hours (especially for STM pin planning)

**Code Start**: Primarily Mary, but everyone focusing on different sections of the code.

- 3-4 hours

**Amplifier:** Katya

- 2-3 hours

**Other Modules**: Sarah

- 2-3 hours

## Sprint Review #2

### Last week's progress

### Current state of project

### Next week's plan

## MVP Demo

## Final Report

Don't forget to make the GitHub pages public website!
If you’ve never made a GitHub pages website before, you can follow this webpage (though, substitute your final project repository for the GitHub username one in the quickstart guide):  [https://docs.github.com/en/pages/quickstart](https://docs.github.com/en/pages/quickstart)

### 1. Video

### 2. Images

### 3. Results

#### 3.1 Software Requirements Specification (SRS) Results

| ID     | Description                                                                                               | Validation Outcome                                                                          |
| ------ | --------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| SRS-01 | The IMU 3-axis acceleration will be measured with 16-bit depth every 100 milliseconds +/-10 milliseconds. | Confirmed, logged output from the MCU is saved to "validation" folder in GitHub repository. |

#### 3.2 Hardware Requirements Specification (HRS) Results

| ID     | Description                                                                                                                        | Validation Outcome                                                                                                      |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| HRS-01 | A distance sensor shall be used for obstacle detection. The sensor shall detect obstacles at a maximum distance of at least 10 cm. | Confirmed, sensed obstacles up to 15cm. Video in "validation" folder, shows tape measure and logged output to terminal. |
|        |                                                                                                                                    |                                                                                                                         |

### 4. Conclusion

## References
