FROM ubuntu:latest

EXPOSE 1234

WORKDIR /root

RUN apt update && apt install -y cmake build-essential git udev gdb python

ADD VimbaUSBTL VimbaUSBTL
RUN cd VimbaUSBTL && bash Install.sh

ADD inc inc
ADD lib lib

RUN cp inc/* /usr/local/include && cp lib/x86_64bit/* /usr/local/lib && ldconfig
ADD run_time.py .
ADD build_time.sh .

VOLUME /root/build

ARG CMAKE_DURING_BUILD
RUN /bin/bash build_time.sh

ENV CMAKE_DURING_BUILD=${CMAKE_DURING_BUILD}

ADD CMakeLists.txt CMakeLists.txt
ADD src src

#CMD /bin/bash
CMD python run_time.py
