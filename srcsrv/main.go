package main

import (
	"fmt"
	"os"
	"path"
)

const (
	NONE = iota
	SVN
	GIT
)

func getScm(p string) int {
	if p == "/" {
		return NONE
	}
	
	d := p
	for i, name := range []string{".svn", ".git"} {
		p = fmt.Sprintf("%v/%v", d, name)
		if _, err := os.Stat(p); err == nil {
			return i + 1
		}
	}

	d, _ = path.Split(d)
	d = path.Clean(d)

	return getScm(d)
}

func srcsrv(path string) {
	switch getScm(path) {
	case NONE:
		fmt.Printf("%v is not under any SCM\n", path)
	case SVN:
		fmt.Printf("%v is under Subversion\n", path)
	case GIT:
		fmt.Printf("%v is under Git\n", path)
	}
}

func fatal(msg string) {
	fmt.Fprintf(os.Stderr, "%s", msg)
	os.Exit(1)
}

func main() {
	wd, err := os.Getwd()
	if err != nil {
		fatal("failed to get work dir")
	}

	for _, p := range os.Args[1:] {
		if len(p) == 0 || p[0] != '/' {
			p = fmt.Sprintf("%s/%s", wd, p)
		}
		p = path.Clean(p)
		srcsrv(p)
	}
}
