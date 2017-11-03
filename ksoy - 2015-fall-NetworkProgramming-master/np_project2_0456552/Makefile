all:common/noop.cpp common/number.cpp common/removetag.cpp common/removetag0.cpp
	g++ common/noop.cpp -o single/ras/bin/noop
	g++ common/number.cpp -o single/ras/bin/number
	g++ common/removetag.cpp -o single/ras/bin/removetag
	g++ common/removetag0.cpp -o single/ras/bin/removetag0
	cp /bin/ls single/ras/bin/ls
	cp /bin/cat single/ras/bin/cat
	g++ common/noop.cpp -o concurrent/ras/bin/noop
	g++ common/number.cpp -o concurrent/ras/bin/number
	g++ common/removetag.cpp -o concurrent/ras/bin/removetag
	g++ common/removetag0.cpp -o concurrent/ras/bin/removetag0
	cp /bin/ls concurrent/ras/bin/ls
	cp /bin/cat concurrent/ras/bin/cat
