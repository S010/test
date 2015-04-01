package main

import (
	"os"
	"fmt"
	"elf"
)

func isRegularFile(path string) (err error) {
	if fi, err := os.Stat(path); err == nil {
		return !fi.IsDir()
	} else {
		return err
	}
}

func elfinfo(path string) {
	fmt.Println(path)

	if !isRegularFile(path) {
		return
	}

	var f *File
	var err error

	f, err := elf.Open(path)
	f.Close()
}

func main() {
	for _, path := range os.Args[1:] {
		elfinfo(path)
	}
}
