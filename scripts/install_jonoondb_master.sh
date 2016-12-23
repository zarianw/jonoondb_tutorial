set -x
set -e
wget --output-document=jonoondb-master.tar.gz https://github.com/zarianw/jonoondb/archive/master.tar.gz >> output.txt
tar xzf jonoondb-master.tar.gz > output.txt
$(cd jonoondb-master && cmake . -G "Unix Makefiles" -DBOOST_ROOT=../boost_1_60_0/64bit >> output.txt && sudo make install >> output.txt)