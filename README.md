# TspOptimizer (Qt6)

TspOptimizer is a lightweight desktop application for loading, visualizing, and optimizing **Traveling Salesman Problem (TSP)** instances (TSPLIB-style `.tsp`). It provides an interactive map view and lets you select an optimization method to improve a tour.

## Purpose

- Open a `.tsp` instance and visualize the cities on a 2D plane.
- Display the **original** tour and the **best-found** tour.
- Run an optimizer and monitor improvement (best distance / improvement %).
- Export the best tour to a file for external use or benchmarking.

## Key features

- **TSPLIB `.tsp` import** (2D Euclidean coordinates).
- **Tour visualization** with zoom/rotation and optional edge drawing.
- **Pan the map**: hold **left mouse button** and drag to move the view.
- **Method selection** from a drop-down (e.g., Genetic Algorithm, Simulated Annealing, 2-opt, Iterated Local Search — depending on your build).
- **Export** the best tour (`.tour`).
- Runs optimization in a worker thread so the UI stays responsive.

## Requirements

- **CMake** 3.20+ (recommended 3.24+)
- **Qt 6.x** (Qt6 Widgets)
- A C++17-capable compiler:
  - Windows: **Visual Studio 2022** (MSVC)
  - Linux: GCC 10+ or Clang 12+

## Build & run (Windows)

1. Install:
   - Qt 6.x (MSVC 2022 64-bit kit)
   - Visual Studio 2022 (Desktop development with C++)
   - CMake

2. Configure (Developer PowerShell for VS), from the project root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DQt6_DIR="C:\Qt\6.x.x\msvc2022_64\lib\cmake\Qt6"
```

3. Build:

```powershell
cmake --build build --config Release
```

4. Run:
- `build\Release\TspOptimizerQt.exe`

5. If you get missing Qt DLL errors, deploy Qt runtime next to the `.exe`:

```powershell
C:\Qt\6.x.x\msvc2022_64\bin\windeployqt.exe build\Release\TspOptimizerQt.exe
```

## Build & run (Linux)

Install Qt6 dev packages (distribution dependent). Example for Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y cmake g++ qt6-base-dev qt6-base-dev-tools
```

Then build and run:

```bash
cmake -S . -B build
cmake --build build -j
./build/TspOptimizerQt
```

If your Qt was installed via the Qt Online Installer, you may need:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
```

## Usage

1. **File → Open…** and choose a `.tsp` file.
2. Select an optimization method in the **Method** drop-down.
3. **Optimize → Run Optimizer** to start.
4. **Optimize → Stop Optimizer** to stop early.
5. **File → Export Tour…** to save the current best tour.

## File formats

- Input: `.tsp` (TSPLIB-like 2D coordinates)
- Output: `.tour` (an ordered list of node indices in visiting order)

## Notes

- Performance depends on instance size and method parameters.
- Some methods are stochastic: different runs can produce different results.

