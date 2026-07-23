cmake_minimum_required(VERSION 3.16)
project(aboutdialogClass)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

set(GUIS_SRCS testaboutdialog.cpp
        testaddeditautoprofiledialog.cpp
        testadvancebuttondialog.cpp
        testcalibration.cpp
        testjoycontrolstickeditdialog.cpp
        testbuttoneditdialog.cpp
        testquicksetdialog.cpp
        testqkeydisplaydialog.cpp
        )
add_executable(GuiTests ${GUIS_SRCS})
#target_link_libraries( GuiTests antilib Qt5::Test )
ADD_TEST(NAME GuiTests COMMAND GuiTests)

add_executable(MouseInputSelectionTests
        testmouseinputselection.cpp
        ../src/mouseinputselection.cpp
        )
target_include_directories(MouseInputSelectionTests PRIVATE ../src)
add_test(NAME MouseInputSelectionTests COMMAND MouseInputSelectionTests)
