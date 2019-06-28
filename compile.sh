gppflags='-Wunused-result -fpermissive -Wall -shared -m32 -fPIC -O -s -static -std=c++0x'
time(
	echo "Compiling"
	g++ $gppflags main.cpp -o nolimits.so
	echo "Done")