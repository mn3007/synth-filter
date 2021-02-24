% Reading values from the Bela oscilloscope
frequency = [110 220 440 660 770 880 990 1000 1030 1050 1100 1210 1320];
amplitude = [0.503 0.484 0.459 0.423 0.404 0.384 0.365 0.362 0.356 0.354 0.348 0.329 0.309];

% normalise gain
gain = (amplitude/0.5);

% plot gain vs frequency on a logarithmic Y axis
plot(frequency, 20*log10(gain));

% label axes
xlabel('Frequency (Hz)');
ylabel('Gain(dB)');

% show figure
shg

