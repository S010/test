package main

import (
	"bytes"
	"fmt"
	"log"
	"net"
	"strings"
)

const (
	RECV_BUF_SIZE = 8192
)

type Rule struct {
	ListenAddr string
	PathToDst map[string]string
}

var rules = []Rule{
	Rule{
		":8080",
		map[string]string{
			"/one" : "localhost:9000",
			"/two" : "localhost:9001",
			"/" : "localhost:9009",
		},
	},
}

type Error string

func (e Error) String() string {
	return string(e)
}

func getPath(s string) string {
	pos := strings.Index(s, " ") + 1
	s = s[pos:]
	pos = strings.Index(s, " ")
	return s[:pos]
}

const (
	STATE_START = iota
	STATE_REDIRECTING
)

func matchPath(path string, m map[string]string) string {
	for k, v := range m {
		if strings.HasPrefix(path, k) {
			return v
		}
	}
	return ""
}

type ConnectionPool map[string]net.Conn

func (cp *ConnectionPool) Get(addr string) (net.Conn, error) {
	for k, v := range *cp {
		if k == addr {
			return v, nil
		}
	}

	conn, err := net.Dial("tcp", addr)
	if err != nil {
		log.Printf("failed to connect to %v: %v", addr, err)
		return nil, err
	}

	cp.Add(addr, conn)

	return conn, nil
}

func (cp *ConnectionPool) Add(addr string, conn net.Conn) {
	map[string]net.Conn(*cp)[addr] = conn
}

func (cp *ConnectionPool) CloseAll() {
	for _, v := range *cp {
		v.Close()
	}
}

func NewConnectionPool() *ConnectionPool {
	cp := ConnectionPool(make(map[string]net.Conn))
	return &cp
}

func pump(src net.Conn, dst net.Conn) {
	for {
		buf := make([]byte, RECV_BUF_SIZE)
		nread, _ := src.Read(buf)
		if nread < 1 {
			return
		}
		nwrite, _ := dst.Write(buf)
		if nwrite < 1 {
			return
		}
	}
}

func demux(ch chan<- Error, r Rule, srcConn net.Conn) {
	connPool := NewConnectionPool()
	// FIXME so hacky, it's not even funny...
	hasPump := make(map[string]bool)

	defer srcConn.Close()
	defer connPool.CloseAll()

	var path string
	var dstAddr string
	var dstConn net.Conn
	var buf []byte
	var chunk []byte

	for {
		if buf == nil {
			buf = make([]byte, RECV_BUF_SIZE)
			n, err := srcConn.Read(buf)
			if err != nil {
				log.Printf("failed to read on %v: %v", r.ListenAddr, err)
				return
			}
			if n == 0 {
				log.Printf("closed a connection on %v", r.ListenAddr)
				return
			}

			path = getPath(string(buf))
			dstAddr = matchPath(path, r.PathToDst)
			dstConn, err = connPool.Get(dstAddr)
			if dstConn != nil {
				tmp := hasPump[dstAddr]
				if tmp == false {
					hasPump[dstAddr] = true
					go pump(dstConn, srcConn)
				}
			}
			// FIXME check error
		}

		pos := bytes.Index(buf, []byte("\r\n\r\n"))
		if pos == -1 {
			chunk = buf
			buf = nil
		} else {
			chunk = buf[:pos + 4]
			buf = buf[pos + 4:]
		}
		if dstConn != nil {
			dstConn.Write(chunk)
		}
	}
}

func listen(ch chan<- Error, r Rule) {
	ln, err := net.Listen("tcp", r.ListenAddr)
	if err != nil {
		ch <- Error(fmt.Sprintf("failed to listen on %v: %v", r.ListenAddr, err))
	}
	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Printf("failed to accept connection on %v: %v", r.ListenAddr, err)
			continue
		}
		go demux(ch, r, conn)
	}
}

func main() {
	ch := make(chan Error)
	for _, r := range rules {
		go listen(ch, r)
	}
	err := <-ch
	log.Fatal("%v", err)
}
