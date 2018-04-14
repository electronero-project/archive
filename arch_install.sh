#!/usr/bin/env bash
yum install base-devel cmake pkg-config boost openssl zeromq unbound miniupnpc libunwind xz readline ldns expat gtest yum --enablerepo=epel install libsodium libsodium-devel doxygen graphviz
echo "Dependencies installation complete"
make
echo "Electronero Linux Build process complete"
