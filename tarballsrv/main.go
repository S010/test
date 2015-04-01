package main

import (
	"archive/tar"
	"compress/gzip"
	"path/filepath"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"net/http"
)

const (
	LISTEN_ADDR = ":8080"
	ROOT_DIR = "."
)

func tarball(path string, t *tar.Writer, out io.Writer) {
	path = filepath.Clean(path)

	fmt.Printf("taring and gzipping %v\n", path)

	info, err := os.Stat(path)
	if err != nil {
		fmt.Printf("%v: stat failed", path)
		return
	}

	if t == nil {
		r, w := io.Pipe()

		t = tar.NewWriter(w)

		z := gzip.NewWriter(out)

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
			tarball(fmt.Sprintf("%v/%v", path, info.Name()), t, out)
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

type HTTPServer int

func (s HTTPServer) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/gzip")
	p := fmt.Sprintf("%s%s", ROOT_DIR, r.URL.Path)
	fmt.Printf("serving %v\n", p)
	tarball(p, nil, w)
}

func main() {
	var s HTTPServer
	err := http.ListenAndServe(LISTEN_ADDR, s)
	panic(err)
}

