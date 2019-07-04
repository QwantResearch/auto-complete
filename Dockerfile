# Copyright 2019 Qwant Research. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

FROM debian:9-slim

LABEL AUTHORS="Christophe Servan"

ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

ENV TZ=Europe/Paris
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN echo deb http://deb.debian.org/debian stretch-backports main contrib non-free >> /etc/apt/sources.list


RUN apt-get -y update && \
    apt-get -y install \
        cmake \
        g++ \
        libboost-locale-dev \
        libboost-regex-dev \
        libyaml-cpp-dev \
        git \
        cmake

RUN apt-get -y -t stretch-backports install cmake
COPY . /opt/auto-complete

WORKDIR /opt/auto-complete

RUN ./install.sh

#RUN apt-get -y remove \
#      libboost-locale1.65-dev \
#      libboost-regex1.65-dev \
#      libyaml-cpp-dev

RUN groupadd -r qnlp && useradd --system -s /bin/bash -g qnlp qnlp

USER qnlp 

ENTRYPOINT ["/usr/local/bin/url-segmenter"]
