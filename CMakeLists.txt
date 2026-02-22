cmake_minimum_required(VERSION 3.20)
project(TspOptimizerQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(TspOptimizerQt
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/TspWidget.h
    src/TspWidget.cpp
    src/TspInstance.h
    src/TspInstance.cpp
    src/Tour.h
    src/Tour.cpp
    src/optim/IOptimizer.h
    src/optim/GeneticOptimizer.h
    src/optim/GeneticOptimizer.cpp
    src/optim/SimAnnealOptimizer.h
    src/optim/SimAnnealOptimizer.cpp
    src/optim/TwoOptOptimizer.h
    src/optim/TwoOptOptimizer.cpp
    src/optim/IlsOptimizer.h
    src/optim/IlsOptimizer.cpp
    src/optim/OptimizerWorker.h
    src/optim/OptimizerWorker.cpp
)

target_link_libraries(TspOptimizerQt PRIVATE Qt6::Widgets)

# Hide the console window on Windows (GUI subsystem)
if(WIN32)
    set_target_properties(TspOptimizerQt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
