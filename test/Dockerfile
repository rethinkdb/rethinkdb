FROM ubuntu:18.04
RUN apt-get update && apt-get install -y \
	  locales git-core build-essential protobuf-compiler \
          libprotobuf-dev libboost-all-dev python nodejs npm python \
          libncurses5-dev libjemalloc-dev wget m4 clang libssl1.0-dev \
          debhelper curl python3.7 python3.7-dev python3.7-distutils && \
	  rm -rf /var/lib/apt/lists/* && \
	  localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8



