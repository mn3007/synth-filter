% Reading values from the Bela oscilloscope
frequency = [220 440 880 1320 1500 1700 1800 1900 2000];
amplitude = [0.500 0.491 0.458 0.410 0.389 0.358 0.346 0.334 0.322];

% normalise gain
gain = (amplitude/0.5);

% plot gain vs frequency on a logarithmic Y axis
plot(frequency, 20*log10(gain));

% label axes
xlabel('Frequency (Hz)');
ylabel('Gain(dB)');

% show figure
shg





