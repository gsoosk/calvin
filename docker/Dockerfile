#Dockerfile for ICNODES
FROM ubuntu:20.04
RUN apt update && apt-get -y install tzdata
RUN apt update && apt -y install build-essential git jq libc6 python3-pip \
    iperf wget iputils-ping net-tools libcurl4-openssl-dev libtool m4 automake openssh-server iproute2 psmisc \
    vim python2 dstat unzip nodejs npm curl autoconf make g++  protobuf-compiler redis \
    && pip3 install solc-select \
    && solc-select install 0.4.24 \
    && solc-select use 0.4.24


RUN apt update && apt-get -y install sudo 


ENV HOME /root



RUN ssh-keygen -t rsa -N "" -f /root/.ssh/id_rsa && cd /root/.ssh && cp id_rsa.pub authorized_keys
ADD id_rsa.pub /
RUN cat /id_rsa.pub >> ~/.ssh/authorized_keys
RUN echo "StrictHostKeyChecking no" > /root/.ssh/config
RUN echo "PermitUserEnvironment yes" >> /etc/ssh/sshd_config
RUN echo "root:newpass" | chpasswd
RUN service ssh start
CMD ["/usr/sbin/sshd","-D"]

# RUN wget https://golang.org/dl/go1.20.8.linux-amd64.tar.gz
# RUN rm -rf /usr/local/go && tar -C /usr/local -xzf go1.20.8.linux-amd64.tar.gz
# 
COPY ./dic3 ${HOME}/ShardDB/

### Calvin
# Copy the local calvin project and run install-ext inside it
COPY ./bin ${HOME}/calvin/bin
COPY ./ext ${HOME}/calvin/ext
RUN ls -la ${HOME}/calvin
# RUN cd ${HOME}/calvin && \
#     chmod +x install-ext && \
#     ./install-ext && \
#     cd ${HOME}

# RUN cd ${HOME}/calvin/src && make -j && cd ${HOME}

# Option A: persistent loader config (recommended)
RUN printf "%s\n" \
  "/root/calvin/ext/protobuf/src/.libs" \
  "/root/calvin/ext/zookeeper/.libs" \
  "/root/calvin/ext/zeromq/src/.libs" \
  > /etc/ld.so.conf.d/calvin.conf && ldconfig
####



WORKDIR ${HOME}/ShardDB
# RUN /usr/local/go/bin/go mod tidy