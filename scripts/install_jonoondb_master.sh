set -x
set -e
wget --output-document=jonoondb.tar.gz https://github.com/zarianw/jonoondb/archive/master.tar.gz >> output.txt
tar xzf jonoondb.tar.gz > output.txt
$(cd jonoondb && cmake . -G "Unix Makefiles" -DBOOST_ROOT=../boost_1_60_0/64bit && sudo make install)