FROM ubuntu:latest

RUN apt update && apt upgrade -y && \
    apt install git cmake emacs vim nano flex bison bash-completion \
    g++ gcc -y && mkdir srcs/ && \
    git clone https://github.com/llvm/llvm-project \
    -b llvmorg-19.1.3 --depth=1 && cd llvm-project/llvm && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) install && cd ../../../ && rm -rf llvm-project && \
    git clone https://github.com/allenh1/slc && cd slc && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && \
    make -j$(nproc) install
