========================================================================
   C64 RAYTRACER BY MARIUS IRGENS
========================================================================

So far it is only doing sphere/ray intersection and lambertian shading. In the future I will try other ray tracing algorithms (when I have time).

Its written in C (with lots of inline assembly) and uses the cc65 compiler. Especially the floating point aritmetic subroutine call functions is written in assembly.
I did a little dithering trick to get 7 shades of color instead of 4. Check out the render loop at the bottom of the main function.

For the floating point arithmetic I am using the FAC1 and FAC2 in the zero-pages, together with Kernel and Basic ROM subroutine calls that performs the calculations.

Check out the C64SphereRender.png file for an example render. 

PS: The rendering takes about 20 minutes when running VINWICE on 100x speed (10000%). So rendering this at normal speed would take around 40 hours.
I did not try to optimize it yet, but I am not sure how optimized I could make it since it is mostly using the internal floating point subroutines anyway.
I guess I could try to make my own fixed point interpretation, but I am not sure I could beat (or even match) the internal floating point one.

If you have any optimization suggestions I would love to hear them! Please fork and update the code, or send me a message.

