# Simple-Raytracer
A simple c++ Raytracer made as part of University assignment

## Overview

This project is a simple raytracer developed as part of the Computer Graphics Lab. It builds on classes that were provided by the Professor (Geometry.h and Math.h) which were further expanded upon in previous assignments.
The raytracer is a basic raytracing engine with recursive reflections and Lambertian shading.


## Features

- Raytracing engine with recursive reflections up to a configurable depth
- Lambertian shading for realistic diffuse lighting
- Supports multiple point light sources 
- Basic geometric objects: triangles and spheres
- Custom materials with ambient, diffuse, specular and reflectivity parameters
- Camera system with perspective projections and pixel-to-ray mapping
- Shadow computation via shadow rays
- Output of rendered scenes as PPM image files


## Tech Stack
- **Programming Language:** C++
- **Build System:** CMake
- **Libraries:** Standard C++ Librariess (<vector>,<iostream>,<fstream>,<memory>, <algorithm>)
- **Version Control:** Git

> **Note:** Usage of Language Models such as ChatGPT was permitted and encouraged by the Professor.
> The implementation was developed by me based on lecture material and pseudocode provided by the Professor. ChatGPT-5 was used to troubleshoot, debug and to generate the "Cornell Box".
