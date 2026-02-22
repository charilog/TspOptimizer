TspOptimizerQt (C++/Qt6)

A small Qt6 Widgets application that loads a TSPLIB .tsp instance, visualizes the route, and optimizes it.
This is a C++ rewrite of the provided Java/Swing project, with an extra feature:
the user selects which optimization method to run (2 methods included).

Build (Linux/macOS)
  mkdir build
  cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/<compiler>/lib/cmake
  cmake --build build -j

Build (Windows / Visual Studio)
  cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DQt6_DIR="C:/Qt/6.x.x/msvc2022_64/lib/cmake/Qt6"
  cmake --build build --config Release

Usage
  File -> Open (.tsp)
  Choose method (Genetic Algorithm or Simulated Annealing) from the bottom combo box
  Start to begin optimizing, Stop to stop

Export
  File -> Export (.tour) exports the best route as "index x y" lines (same format as the Java app did).
