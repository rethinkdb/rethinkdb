FROM ubuntu:18.04
LABEL author="Shashank Singh"
LABEL version="0.0.1"
COPY . /rethinkdb
# RUN apt-get install build-essential 
RUN apt-get update -q
RUN apt-get upgrade -y
RUN apt-get install -y protobuf-compiler python \
        libprotobuf-dev libcurl4-openssl-dev libboost-all-dev \
        libncurses5-dev libjemalloc-dev wget m4 g++ libssl-dev

RUN ./configure --allow-fetch CXX=clang++ --fetch protoc --fetch npm 
RUN make -j4
CMD ["rethinkdb"]