.PHONY: all clean

all: hello.efi

clean:
	rm -f *.o *.so hello.efi

hello.efi: hello.so
	objcopy -j .text                   \
		  -j .sdata                \
		  -j .data                 \
		  -j .dynamic              \
		  -j .dynsym               \
		  -j .rel                  \
		  -j .rela                 \
		  -j .reloc                \
		  --target=efi-app-x86_64  \
		  hello.so                 \
		  hello.efi

hello.so: hello.o
	ld hello.o                                   \
	     /usr/lib64/gnuefi/crt0-efi-x86_64.o     \
	     -nostdlib                               \
	     -znocombreloc                           \
	     -T /usr/lib64/gnuefi/elf_x86_64_efi.lds \
	     -shared                                 \
	     -Bsymbolic                              \
	     -L /usr/lib64                           \
	     -l:libgnuefi.a                          \
	     -l:libefi.a                             \
	     -o hello.so

hello.o: hello.c
	gcc hello.c                              \
	      -c                                 \
	      -fno-stack-protector               \
	      -fpic                              \
	      -fshort-wchar                      \
	      -mno-red-zone                      \
	      -I /usr/include/efi                \
	      -I /usr/include/efi/x86_64         \
	      -DEFI_FUNCTION_WRAPPER             \
	      -o hello.o
