package main

import (
	"archive/tar"
	"compress/gzip"
	"path/filepath"
	"fmt"
	"io"
	"io/ioutil"
	"os"
)

func tarball(path string, t *tar.Writer) {
	path = filepath.Clean(path)

	info, err := os.Stat(path)
	if err != nil {
		return
	}

	if t == nil {
		r, w := io.Pipe()

		t = tar.NewWriter(w)

		f, err := os.Create(fmt.Sprintf("%s.tgz", path))
		if err != nil {
			panic(err)
		}
		z := gzip.NewWriter(f)

		defer f.Close()
		defer z.Close()
		defer r.Close()
		defer w.Close()
		defer t.Close()

		go io.Copy(z, r)
	}

	h, err := tar.FileInfoHeader(info, path) // FIXME what is link arg supposed to be?
	if err != nil {
		return
	}
	h.Name = path
	t.WriteHeader(h)
	if info.IsDir() {
		infos, err := ioutil.ReadDir(path)
		if err != nil {
			return
		}
		for _, info := range infos {
			tgz(fmt.Sprintf("%v/%v", path, info.Name()), t)
		}
	} else {
		f, err := os.Open(path)
		if err != nil {
			panic(err)
		}
		defer f.Close()

		if _, err := io.Copy(t, f); err != nil {
			panic(err)
		}
	}
}

func main() {
	for _, arg := range os.Args[1:] {
		tarball(arg, nil)
	}
}
