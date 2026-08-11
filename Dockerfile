FROM klee/klee:3.1

# fix of kitware key
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
RUN sudo apt-get update

RUN sudo apt update --allow-unauthenticated && sudo apt install time libglib2.0-dev libboost-all-dev wget libssl-dev python3-reportlab nano less python3-tabulate build-essential git autoconf flex bison -y

RUN echo '/tmp/libc++-install-130/lib' | sudo tee /etc/ld.so.conf.d/libc++.conf
RUN sudo ldconfig

RUN sudo pip3 install wllvm flask

RUN sudo ln -sT /tmp/libc++-install-130/lib /usr/local/lib/libc++

WORKDIR /home/klee
RUN mkdir build build_native && echo "cd build && cmake -DBUILD=bytecode ../source && make -j$(nproc) && cd ../build_native && cmake ../source && make -j$(nproc)" > make.sh && chmod +x make.sh
ENTRYPOINT ["bash", "--init-file", "~/.bashrc"]
