// Queen Mary University of London
// ECS7012 - Music and Audio Programming
// Spring 2021
//
// Assignment 1: Synth Filter
// The code was written for ECS7012 Assignment 1. A resonant lowpass filter was 
// implemented which models the classic Moog VCF based on a simplified digital model 
// by Vesa Välimäki and Antti Huovilainen. The resonant filter has
// parameters adjustable in the Bela GUI, and waveform visible in the scope

#include <Bela.h>
#include <libraries/Gui/Gui.h>
#include <libraries/GuiController/GuiController.h>
#include <libraries/Scope/Scope.h>
#include <libraries/math_neon/math_neon.h>
#include <cmath>
#include "Wavetable.h"

// Browser-based GUI to adjust parameters
Gui gGui;
GuiController gGuiController;

// Browser-based oscilloscope to visualise signal
Scope gScope;

// Oscillator objects
Wavetable gSineOscillator, gSawtoothOscillator;

// ****************************************************************
// Declare global variables for coefficients and filter state
// ****************************************************************
	
	float gLastX1 = 0, gLastX2 = 0, gLastX3 = 0, gLastX4 = 0;
	float gLastY1 = 0, gLastY2 = 0, gLastY3 = 0, gLastY4 = 0;
	float gA1 = 0;
    float gB0 = 0, gB1 = 0;
    float gGres = 0;
    float gGcomp = 0.5;
    float out4 = 0;
    float feedback = 0;    	
        
// Calculate filter coefficients given specifications
// frequencyHz -- filter frequency in Hertz (needs to be converted to discrete time frequency)
// resonance -- normalised parameter 0-1 which is related to filter Q
void calculate_coefficients(float sampleRate, float frequencyHz, float resonance)
{
	// Calculate filter coefficients
	float gWc = 2.0 * M_PI * frequencyHz / sampleRate;
	float g = 0.9892*gWc - 0.4342*powf(gWc, 2) + 0.1381*powf(gWc, 3) - 0.0202*powf(gWc, 4);

	gGres = resonance*(1.0029 + 0.0526*gWc - 0.0926*powf(gWc, 2) + 0.0218*powf(gWc, 3));

	gB0 = g/1.3;
	gB1 = g*0.3/1.3;
	gA1 = 1-g;
	
}

bool setup(BelaContext *context, void *userData)
{
	std::vector<float> wavetable;
	const unsigned int wavetableSize = 1024;
		
	// Populate a buffer with the first 64 harmonics of a sawtooth wave
	wavetable.resize(wavetableSize);
	
	// Generate a sawtooth wavetable (a ramp from -1 to 1)
	for(unsigned int n = 0; n < wavetableSize; n++) {
		wavetable[n] = -1.0 + 2.0 * (float)n / (float)(wavetableSize - 1);
	}
	
	// Initialise the sawtooth wavetable, passing the sample rate and the buffer
	gSawtoothOscillator.setup(context->audioSampleRate, wavetable);

	// Recalculate the wavetable for a sine
	for(unsigned int n = 0; n < wavetableSize; n++) {
		wavetable[n] = sin(2.0 * M_PI * (float)n / (float)wavetableSize);
	}	
	
	// Initialise the sine oscillator
	gSineOscillator.setup(context->audioSampleRate, wavetable);
	
	// Set up the GUI
	gGui.setup(context->projectName);
	gGuiController.setup(&gGui, "Oscillator and Filter Controls");

	// Arguments: name, default value, minimum, maximum, increment
	// Create sliders for oscillator and filter settings
	gGuiController.addSlider("Oscillator Frequency", 440, 40, 8000, 0);
	gGuiController.addSlider("Oscillator Amplitude", 0.5, 0, 2.0, 0);
	gGuiController.addSlider("Cutoff Frequency", 1000, 100, 5000, 0);
	gGuiController.addSlider("Resonance", 0, 0, 1.0, 0);
	
	// Sliders for 	weighting coefficient values
	gGuiController.addSlider("A", 0, 0, 1, 1);
	gGuiController.addSlider("B", 0, -4, 2, 1);
	gGuiController.addSlider("C", 0, -2, 6, 1);
	gGuiController.addSlider("D", 0, -8, 0, 1);
	gGuiController.addSlider("E", 1, 0, 4, 1);
	
	// Slider for selecting the waveform
	gGuiController.addSlider("SINE/SAWTOOTH", 0, 0, 1, 1);

	// Set up the scope
	gScope.setup(2, context->audioSampleRate);

	return true;
}

void render(BelaContext *context, void *userData)
{
	// Read the slider values
	float oscFrequency = gGuiController.getSliderValue(0);	
	float oscAmplitude = gGuiController.getSliderValue(1);
	float cutoff = gGuiController.getSliderValue(2);
	float gCres = gGuiController.getSliderValue(3);
	
	// Weighting coefficient values for typical magnitude response types that can be obtained with this filter structure
	float A = gGuiController.getSliderValue(4);
	float B = gGuiController.getSliderValue(5);
	float C = gGuiController.getSliderValue(6);
	float D = gGuiController.getSliderValue(7);
	float E = gGuiController.getSliderValue(8);
	
	// Read the slider value 
	int waveformSelect = gGuiController.getSliderValue(9);

	// Set the oscillator frequency
	gSineOscillator.setFrequency(oscFrequency);
	gSawtoothOscillator.setFrequency(oscFrequency);

	// Calculate new filter coefficients
	calculate_coefficients(context->audioSampleRate, cutoff, gCres);

    for(unsigned int n = 0; n < context->audioFrames; n++) {
    	// Choose sine or sawtooth oscillator
		float sineOsc = oscAmplitude * gSineOscillator.process();
		float sawtoothOsc = oscAmplitude * gSawtoothOscillator.process();
		
		float in  = waveformSelect == 0 ? sineOsc : sawtoothOsc;

        // ****************************************************************
        // Implement the feedback loop
        // (1 + 4 * Gres * Gcomp) * x[n] - 4 * Gres * y[n-1]
        feedback = (1 + 4*gGres*gGcomp)*in - 4*gGres*out4;
        
        // Add nonlinearity
        float nonlinear = tanhf_neon(feedback);
		
		// Fourth-order filter
		// y[n] = g/1.3 x[n] + g*0.3/1.3 x[n-1] + (1-g)*y[n-1]
        float out1 = gB0*nonlinear + gB1*gLastX1 + gA1*gLastY1;		
        gLastX1 = nonlinear;  // update state
    	gLastY1 = out1;   
    	
    	float out2 = gB0*out1 + gB1*gLastX2 + gA1*gLastY2;		
        gLastX2 = out1; // update state
    	gLastY2 = out2; 
    	
    	float out3 = gB0*out2 + gB1*gLastX3 + gA1*gLastY3;		
        gLastX3 = out2; // update state
    	gLastY3 = out3; 
    	
    	out4 = gB0*out3 + gB1*gLastX4 + gA1*gLastY4;		
        gLastX4 = out3; // update state
    	gLastY4 = out4; 

		float out = nonlinear*A+out1*B+out2*C+out3*D+out4*E; // Output with weighting coefficients
		
        // ****************************************************************
            
        // Write the output to every audio channel
    	for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
    		audioWrite(context, n, channel, out);
    	}
    	
    	gScope.log(in, out);
    }
}

void cleanup(BelaContext *context, void *userData)
{

}
