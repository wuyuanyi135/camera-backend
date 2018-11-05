FROM ubuntu:latest

WORKDIR /root

RUN apt update && apt install -y cmake build-essential git udev

ADD VimbaUSBTL VimbaUSBTL
RUN cd VimbaUSBTL && bash Install.sh

ADD inc inc
ADD lib lib

RUN cp inc/* /usr/local/include && cp lib/x86_64bit/* /usr/local/lib && ldconfig

ADD CMakeLists.txt CMakeLists.txt
ADD src src

RUN mkdir build && cd build && cmake .. && make

CMD build/src/vimba_backend