#!/usr/bin/env bash

export PREFIX=/usr/local/

echo "Prefix set to $PREFIX"

export CMAKE_PREFIX_PATH=$PREFIX

git submodule update --init --recursive

echo "Installing dependencies"

# pushd third_party && rm -rf openfst-1.6.1 && tar xvfz openfst-1.6.1.tar.gz && cd openfst-1.6.1 &&  ./configure --enable-ngram-fsts &&  make -j 4  && make install && popd

for dep in json pistache SymSpellPlusPlus
do
pushd third_party/$dep
	rm -rf build
	mkdir -p build
	pushd build
		cmake .. -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DCMAKE_BUILD_TYPE=Release
		make -j 4 && make install
	popd
popd
done


echo "Installing auto-complete"
mkdir -p $PREFIX
rm -rf build
mkdir -p build
pushd build
	cmake .. -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DCMAKE_BUILD_TYPE=Release 
	make -j 4 && make install
popd
