.SUFFIXES:
.SUFFIXES: .htm .gut

all:
	make *.htm

DIR     = /omi/httpd/www-dev/root
TOOLS   = ${DIR}/Tools
GUTSCSH = ${TOOLS}/fcgi-pa.csh

.gut.htm:
	${GUTSCSH} $*.gut > $*.htm

clean:
	rm -f *~
