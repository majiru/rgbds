</$objtype/mkfile

DIRS=\
	asm\
	link\
	fix\
	gfx\

all:V:
	for(i in $DIRS)@{
		cd src/^$i && mk
	}

install:V:
	for(i in $DIRS)@{
		cd src/^$i && mk install
	}

clean:V:
	for(i in $DIRS)@{
		cd src/^$i && mk clean
	}
