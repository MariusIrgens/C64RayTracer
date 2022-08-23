========================================================================
   C64 SDF RAYTRACER BY MARIUS IRGENS
========================================================================

OBS! I am working on the code at the moment, so the program might not work.

This is an implementation of the Signed Distance Function style raymarching for the Commodore 64. 

Its written in C (with lots of inline assembly) and uses the cc65 compiler. Especially the floating point aritmetic subroutine call functions is written in assembly.
I did a little dithering trick to get 7 shades of color instead of 4. Check out the render loop at the bottom of the main function.

For the floating point arithmetic I am using the FAC1 and FAC2 in the zero-pages, together with Kernel and Basic ROM subroutine calls that performs the calculations.

PS: The rendering takes a long time, even when running VINWICE on 100x speed (10000%). For 3 shapes it can take over an hour.
I did not try to optimize it yet, but I am not sure how optimized I could make it since it is mostly using the internal floating point subroutines anyway.
I guess I could try to make my own fixed point interpretation, but I am not sure I could beat (or even match) the internal floating point one.

If you want to build the code you need to set up cc65 and VINWICE. If you have it set up already you could probably just make a new project and copy the main.c file into your project.
The lib file includes the cc65, VINWICE and make that I have used during testing. Remember to set VINWICE speed to full blast (10000%).


If you have any optimization suggestions I would love to hear them! 

