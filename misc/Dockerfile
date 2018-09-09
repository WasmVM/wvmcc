FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive
ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/usr/local/lib"

RUN apt-get update
RUN apt-get install -y git cmake automake libtool build-essential

# SkyPat
RUN git clone http://github.com/skymizer/SkyPat.git
RUN cd SkyPat && git checkout v3.1.1
RUN cd SkyPat && ./autogen.sh && ./configure && make -j8 install
RUN rm -rf SkyPat

WORKDIR /root