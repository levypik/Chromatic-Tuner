Created a working chromatic tuner with several features. 
Specifically, it displays the frequency being played as input, 
the nearest note and octave to that frequency and a graphical 
display showing the error estimate of the input frequency compared 
to the nearest frequency. The frequency being displayed is accurate within
10 geometric cents of the actual frequency being input. It has a menu option 
to switch into different modes, such as an FFT Histogram, Octave Selection, 
and Spectrogram mode. All of these features run intuitively so a user would 
be able to easily use this tool without instruction and run smoothly without 
noticeable delays or glitches. To achieve these goals we had to combine 
debouncing of interrupt devices, the HSM, and the FFT algorithm.

