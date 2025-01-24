here=$(pwd)
cd ../cpp_fixed && make all -j8
cd ../cpp_orderbook && make all -j8
cd ../cpp_fix_codec && make all -j8
cd ../cpp_fix_engine && make all -j8
cd $here
make all -j8