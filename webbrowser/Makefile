.PHONY: all clean

all: .qmakefile
	gmake -f .qmakefile

clean: .qmakefile
	gmake -f .qmakefile clean

distclean: .qmakefile
	gmake -f .qmakefile distclean

.qmakefile: webbrowser.pro
	qmake4 -o .qmakefile webbrowser.pro

