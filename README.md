# Smoke Simulation

2D fluid simulation using Smoothed Particle Hydrodynamics (SPH) rendered with SDL3.

## Prerequisites

- C-11 compliant compiler
- CMake
- SDL3

## Build Instructions

### Generate CMake Files

```
cmake -B build
```

### Compile

```
cmake --build build
```

## How to Run

### Linux/MacOS

```
./build/PlayGame
```

### Windows

```
.\build\Debug\PlayGame.exe
```

## Credits & References

### Primary Reference(s)

- [Tutorial Presentation on Smoothed Particle Hydrodynamics](https://personal.ems.psu.edu/~fkd/courses/EGEE520/2017Deliverables/SPH_2017.pdf) - EGEE 520, Penn State University.

### Secondary Reference(s)

- [Fluid Simulation Course Notes](https://www.cs.cornell.edu/courses/cs5643/2015sp/stuff/BridsonFluidsCourseNotes_SPH_pp83-86.pdf) - Robert Bridson, CS5643 (Spring 2015), Cornell University.
- [Interactive Computer Graphics Course Texts (Smoothed Particle Hydrodynamics)](https://cs418.cs.illinois.edu/website/text/sph.html) - CS418, University of Illinois Urbana-Champaign (UIUC).
