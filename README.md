# ImageManager

This is responsible for managing images and compatibility files.

This repository is part of project [ARIEL](https://github.com/TCC-PES-2022).

To clone this repository, run:

    git clone https://github.com/TCC-PES-2022/ImageManager.git
    cd ImageManager

Before building your project, you may need to install some dependencies. To do so, run:

    sudo apt update
    sudo apt install -y --allow-downgrades build-essential=12.9ubuntu3 libgcrypt20-dev=1.9.4-3ubuntu3 libtinyxml2-dev=9.0.0+dfsg-3
    
For tests, you'll also need
    
    sudo apt install -y libgtest-dev=1.11.0-3 cmake-data=3.22.1-1ubuntu1 cmake=3.22.1-1ubuntu1 lcov=1.15-1

Export the instalation path to the environment:
    export DESTDIR=<path_to_install>

You can also define this variable in your `.bashrc` file. or when calling any make rule. The default installation path is `/tmp`.

To build, run:

    make deps && make

To install, run:

    make install


To test, first build gtest:

    cd /usr/src/gtest
    sudo cmake CMakeLists.txt
    sudo make
    sudo cp lib/*.a /usr/lib
    sudo mkdir -p /usr/local/lib/gtest/
    sudo ln -s /usr/lib/libgtest.a /usr/local/lib/gtest/libgtest.a
    sudo ln -s /usr/lib/libgtest_main.a /usr/local/lib/gtest/libgtest_main.a

To test and generate test coverage, run:

    cd test
    make report