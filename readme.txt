========================================================================
   C64 RAYTRACER BY MARIUS IRGENS
========================================================================

So far it is only doing sphere/ray intersection and lambertian shading. In the future I will try other ray tracing algorithms.

Its written in C (with lots of inline assembly) and uses the cc65 compiler. Especially the floating point aritmetic functions is written in assembly.

For the floating point arithmetic I am using the FAC1 and FAC2 in the zero-pages, together with Kernel and Basic ROM subroutine calls that performs the calculations.

