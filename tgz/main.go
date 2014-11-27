package main

import (
	"fmt"
	"io"
	"os"
)

type ChanBuf struct {
	ch	chan []byte
	buf	[]byte
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

func (cb *ChanBuf) Read(buf []byte) (n int, err error) {
	var ok bool
	if cb.buf == nil {
		cb.buf, ok = <-cb.ch
		if !ok {
			return 0, io.EOF
		}
	}
	l := copy(buf, cb.buf)
	cb.buf = cb.buf[l:]
	if len(cb.buf) == 0 {
		cb.buf = nil
	}
	return l, nil
}

func (cb *ChanBuf) Write(buf []byte) (n int, err error) {
	cb.ch <- buf
	return len(buf), nil
}

func process(cb *ChanBuf) {
	buf := make([]byte, 2)
	for {
		n, err := cb.Read(buf)
		if n > 0 {
			fmt.Printf("n=%v, buf=%v\n", n, buf)
		}
		if err != nil {
			return
		}
	}
}

func tgz(path string, tw *io.Writer) {
	fi, err := os.Stat(path)
	if err != nil {
		return
	}

	if tw == nil {
		tw = ChanBuf{make(chan []byte), nil}
		/*
		start a go routine which reads from chanbuf and 
		dumps into specified file
		*/
	}

	if fi.IsDir() {
		fmt.Printf("%s является директорией\n", path)
	} else {
		fmt.Printf("%s не является директорией\n", path)
	}
}

func main() {
	/*
	fmt.Println("hello, world")

	cb := ChanBuf{make(chan []byte), nil}
	go process(&cb)
	cb.ch <- []byte("abc")
	close(cb.ch)
	*/

	for _, arg := range os.Args[1:] {
		tgz(arg, nil)
	}
}
