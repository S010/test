all: test7.exe

test7.exe: test7.obj ui_win32.obj
	cl test7.obj ui_win32.obj /link User32.lib Gdi32.lib Comctl32.lib /subsystem:windows

test7.obj: test7.c
	cl /c test7.c /W3

ui_win32.obj: ui.h ui_win32.c
	cl /c ui_win32.c /W3

clean:
	del *.exe *.obj
