FROM ubuntu:18.04
LABEL author="Shashank Singh"
LABEL version="0.0.1"


# https://superuser.com/questions/1059346/apt-get-update-not-working-signing-verification-errors
RUN rm -rf /tmp/* 
RUN apt-get update -q
RUN apt-get upgrade -y
RUN apt-get install -y  build-essential  protobuf-compiler python \
        libprotobuf-dev libcurl4-openssl-dev libboost-all-dev \
        libncurses5-dev libjemalloc-dev wget m4 g++ libssl-dev \
        clang llvm

COPY . /rethinkdb
WORKDIR /rethinkdb
RUN ./configure --allow-fetch CXX=clang++ --fetch protoc --fetch npm 
RUN make -j4
CMD ["rethinkdb"]