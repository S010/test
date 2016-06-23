package main

import (
	"bytes"
	"crypto/rand"
	"encoding/hex"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"math"
	"math/big"
	"net"
	"os"
	"strings"
	"time"
)

var queryID uint16

//
// Message
//

const MAX_MESSAGE_SIZE = 512

const (
	TYPE_A = 1
	CLASS_IN = 1
)

type Message struct {
	Header
	Questions []Question
	Resources []Resource
}

type Header struct {
	ID uint16
	QR bool
	OpCode uint8
	AA bool
	TC bool
	RD bool
	RA bool
	Z uint8
	RCode uint8
	QDCount uint16
	ANCount uint16
	NSCount uint16
	ARCount uint16
}

type Question struct {
	Name string
	Type uint16
	Class uint16
}

type Resource struct {
	Name string
	Type uint16
	Class uint16
	TTL uint32
	Length uint16
	Data []byte
}

func (m Message) String() string {
	s := fmt.Sprintf("ID: %v\nQR: %v\nOpCode: %v\nAA: %v\nTC: %v\nRD: %v\nRA: %v\nZ: %v\nRCode: %v\nQDCount: %v\nANCount: %v\nNSCount: %v\nARCount: %v\n",
		m.ID, m.QR, m.OpCode, m.AA, m.TC, m.RD, m.RA, m.Z, m.RCode, m.QDCount, m.ANCount, m.NSCount, m.ARCount)
	if len(m.Questions) > 0 {
		s += "Questions:\n"
		for _, q := range m.Questions {
			s += fmt.Sprintf("%v\n", q)
		}
	}
	if len(m.Resources) > 0 {
		s += "Resources:\n"
		for _, r := range m.Resources {
			s += fmt.Sprintf("%v\n", r)
		}
	}

	return s
}

func (m *Message) Serialize() MessageData {
	data := make(MessageData, 0, MAX_MESSAGE_SIZE)

	data.AppendUint16(m.ID)
	data.AppendByte(boolToByte(m.QR, 7) | (m.OpCode << 3) | boolToByte(m.AA, 2) | boolToByte(m.TC, 1) | boolToByte(m.RD, 0))
	data.AppendByte(boolToByte(m.RA, 7) | ((m.Z & 0xf) << 4) | (m.RCode & 0xf))
	data.AppendUint16(m.QDCount)
	data.AppendUint16(m.ANCount)
	data.AppendUint16(m.NSCount)
	data.AppendUint16(m.ARCount)

	for _, q := range m.Questions {
		data.AppendName(q.Name)
		data.AppendUint16(q.Type)
		data.AppendUint16(q.Class)
	}

	for _, r := range m.Resources {
		data.AppendName(r.Name)
		data.AppendUint16(r.Type)
		data.AppendUint16(r.Class)
		data.AppendUint32(r.TTL)
		data.AppendUint16(r.Length)
		data.AppendData(r.Data)
	}

	return data[:len(data)]
}

//
// MessageData
//

type MessageData []byte

func (data *MessageData) Byte(pos int) (byte) {
	return []byte(*data)[pos]
}

func (data *MessageData) Uint16(pos int) (uint16, int) {
	return (uint16(data.Byte(pos)) << 8) | uint16(data.Byte(pos + 1)), pos + 2
}

func (data *MessageData) Uint32(pos int) (uint32, int) {
	return (uint32(data.Byte(pos)) << 24) | (uint32(data.Byte(pos + 1)) << 16) | (uint32(data.Byte(pos + 2)) << 8) | uint32(data.Byte(pos + 3)), pos + 4
}

func (data *MessageData) Bit(byteOffset int, bitOffset uint) bool {
	return (data.Byte(byteOffset) & (1 << bitOffset)) != 0
}

func (data *MessageData) Slice(left, right int) []byte {
	return ([]byte(*data))[left:right]
}

func (data *MessageData) Name(pos int) (string, int) {
	//logPrintf("dbg: pos=%v", pos)

	b := bytes.Buffer{}
	for {
		l := data.Byte(pos)
		pos++
		if l == 0 {
			break
		}
		/* FIXME
		if l > len(data) - pos {
			log.Printf("Error: malformed data in resource")
			return "", 0
		}
		*/
		if b.Len() > 0 {
			b.WriteString(".")
		}
		if (l & 0xc0) == 0xc0 {
			var ptr uint16
			ptr = uint16(l ^ 0xc0) << 8
			ptr |= uint16(data.Byte(pos))
			pos++
			nextNamePart, _ := data.Name(int(ptr))
			return b.String() + nextNamePart, pos
		}
		b.Write(data.Slice(pos, pos + int(l)))
		pos += int(l)
	}

	return b.String(), pos
}

func (data *MessageData) Question(pos int) (Question, int) {
	qName, pos := data.Name(pos)
	qType, pos := data.Uint16(pos)
	qClass, pos := data.Uint16(pos)

	return Question{qName, qType, qClass}, pos
}

func (data *MessageData) Resource(pos int) (Resource, int) {
	rrName, pos := data.Name(pos)
	rrType, pos := data.Uint16(pos)
	rrClass, pos := data.Uint16(pos)
	rrTTL, pos := data.Uint32(pos)
	rrRDLength, pos := data.Uint16(pos)
	rrData := data.Slice(pos, pos + int(rrRDLength))
	pos += int(rrRDLength)

	return Resource{rrName, rrType, rrClass, rrTTL, rrRDLength, rrData}, pos
}

func (data MessageData) String() string {
	return hex.Dump(data)
}

func (data *MessageData) AppendByte(val byte) {
	*data = append(*data, val)
}

func (data *MessageData) AppendUint16(val uint16) {
	*data = append(*data, byte((val >> 8) & 0xff), byte(val & 0xff))
}

func (data *MessageData) AppendUint32(val uint32) {
	*data = append(*data, byte((val >> 24) & 0xff), byte((val >> 16) & 0xff), byte((val >> 8) & 0xff), byte(val & 0xff))
}

func (data *MessageData) AppendData(appendData []byte) {
	*data = append(*data, appendData...)
}

func (data *MessageData) AppendName(name string) {
	tokens := strings.Split(name, ".")

	for _, t := range tokens {
		// FIXME assert length <= 63
		data.AppendByte(byte(len(t)))
		data.AppendData([]byte(t))
	}
	data.AppendByte(0)
}

func (data *MessageData) Parse() *Message {
	m := &Message{}
	pos := 0
	m.ID, pos = data.Uint16(pos)
	m.QR = data.Bit(2, 7)
	m.OpCode = (data.Byte(2) >> 3) & 0xf
	m.AA = data.Bit(2, 2)
	m.TC = data.Bit(2, 1)
	m.RD = data.Bit(2, 0)

	m.RA = data.Bit(3, 7)
	m.Z = (data.Byte(3) >> 4) & 0x7
	m.RCode = data.Byte(3) & 0xf

	pos = 4

	m.QDCount, pos = data.Uint16(pos)
	m.ANCount, pos = data.Uint16(pos)
	m.NSCount, pos = data.Uint16(pos)
	m.ARCount, pos = data.Uint16(pos)

	var question Question
	var resource Resource

	for i := 0; i < int(m.QDCount); i++ {
		question, pos = data.Question(pos)
		m.Questions = append(m.Questions, question)
	}

	count := m.ANCount + m.NSCount + m.ARCount
	for i := 0; i < int(count); i++ {
		resource, pos = data.Resource(pos)
		m.Resources = append(m.Resources, resource)
	}

	return m
}

func (q Question) String() string {
	return fmt.Sprintf("{ %v; %v; %v }", q.Name, q.Type, q.Class)
}

func (r Resource) String() string {
	return fmt.Sprintf("{ %v; %v; %v; %v; %v; %v }", r.Name, r.Type, r.Class, r.TTL, r.Length, r.Data)
}

//
// misc
//

func boolToByte(val bool, bitNo uint) byte {
	if val {
		return 1 << bitNo
	} else {
		return 0
	}
}

func NewQuery(name string) *Message {
	var m Message

	m.ID = queryID
	m.RD = true
	m.QDCount = 1
	m.Questions = append(m.Questions, Question{Name: name, Type: TYPE_A, Class: CLASS_IN})

	queryID++

	return &m
}

var debugMode bool

func query(name string) {
	addr, err := net.ResolveUDPAddr("udp", *serverFlag)
	if err != nil {
		log.Fatalf("%v: %v", *serverFlag, err)
	}

	c, err := net.DialUDP("udp", nil, addr)
	if err != nil {
		logFatal(err)
	}
	defer c.Close()

	msg := NewQuery(name)

	data := msg.Serialize()
	logPrintf("Query:\n%v%v", data, msg)
	c.Write(data)

	c.SetReadDeadline(time.Now().Add(time.Duration(*timeoutFlag) * time.Millisecond))
	data = make(MessageData, MAX_MESSAGE_SIZE)
	n, err := c.Read([]byte(data))
	if err != nil {
		logPrint(err)
		return
	}
	data = data[:n]

	logPrintf("Reply:\n%v%v", data, data.Parse())
}

func logPrintf(fmt string, args ...interface{}) {
	if !*quietFlag {
		log.Printf(fmt, args...)
	}
}

func logPrint(args ...interface{}) {
	if !*quietFlag {
		log.Print(args...)
	}
}

func logFatal(args ...interface{}) {
	log.Fatal(args...)
}

var quietFlag = flag.Bool("quiet", false, "disable debug messages")
var serverFlag = flag.String("server", "8.8.8.8", "IP address of DNS server to query")
var waitFlag = flag.Int("wait", -1, "interval in milliseconds at which to make queries in round robing fashion")
var nameListFlag = flag.String("names", "", "File with a list of names to query")
var countFlag = flag.Int("count", 1, "Number of iterations, negative or zero value means unlimited")
var timeoutFlag = flag.Int("timeout", 500, "Number of milliseconds after which to give up and stop waiting for reply")
var listenFlag = flag.String("listen", "", "Act as a DNS server and listen for connections on specified address")

func appendNamesFromFile(names []string, path string) []string {
	contents, err := ioutil.ReadFile(path)
	if err != nil {
		log.Fatal(err)
	}
	a := strings.Split(string(contents), "\n")
	for _, line := range a {
		line = strings.TrimSpace(line)
		if line != "" {
			names = append(names, line)
		}
	}

	return names
}

func initQueryID() {
	n, err := rand.Int(rand.Reader, big.NewInt(int64(math.MaxUint16)))

	if err != nil {
		log.Printf("Failed to initialize DNS message ID from random source: %v", err)
	} else {
		queryID = uint16(n.Int64())
	}
}

type NameDB map[string]net.IP

func loadDB(dbPath string) (NameDB, error) {
	log.Printf("Loading name database from %v", dbPath)

	fin, err := os.Open(dbPath)
	if err != nil {
		return nil, err
	}

	data, err := ioutil.ReadAll(fin)
	if err != nil {
		return nil, err
	}

	db := make(NameDB)

	lines := strings.Split(string(data), "\n")
	for _, line := range lines {
		line := strings.Trim(line, " \t")
		line = strings.Split(line, "#")[0]
		if len(line) == 0 {
			continue
		}

		fields := strings.Fields(line)
		if len(fields) < 2 {
			continue
		}

		ip := net.ParseIP(fields[0])
		ip4 := ip.To4()
		ip6 := ip.To16()
		if ip4 != nil {
			ip = ip4
		} else {
			ip = ip6
		}
		for _, name := range fields[1:] {
			db[name] = ip
			log.Printf("Adding record: %v -> %v", name, ip)
		}
	}

	return db, nil
}

func runServer(listenAddr string, dbPath string) {
	db, err := loadDB(dbPath)
	if err != nil {
		log.Fatal(err)
	}

	myAddr, err := net.ResolveUDPAddr("udp", listenAddr)
	if err != nil {
		log.Fatal(err)
	}

	conn, err := net.ListenUDP("udp", myAddr)
	if err != nil {
		log.Fatal(err)
	}

	for {
		data := make(MessageData, MAX_MESSAGE_SIZE)
		n, clientAddr, err := conn.ReadFromUDP([]byte(data))
		if err != nil {
			log.Print(err)
			continue
		}

		data = data[:n]
		msg := data.Parse()

		log.Printf("Query:\n%v%v", hex.Dump(data), msg)

		if msg.QDCount < 1 {
			log.Print("Skipping query without questions")
			continue
		}

		reply := msg
		reply.ANCount = 0
		reply.Resources = make([]Resource, 0)

		for _, q := range msg.Questions {
			if q.Type != TYPE_A || q.Class != CLASS_IN {
				continue
			}
			ip, present := db[q.Name]
			if !present {
				continue
			}
			reply.ANCount++
			reply.Resources = append(reply.Resources, Resource{
				q.Name,
				TYPE_A,
				CLASS_IN,
				10,
				uint16(len([]byte(ip))),
				[]byte(ip),
			})
		}

		if reply.ANCount > 0 {
			replyData := reply.Serialize()
			conn.WriteToUDP(replyData, clientAddr)
			log.Printf("Reply:\n%v%v", hex.Dump(replyData), reply)
		} else {
			log.Printf("Reply: no-reply")
		}
	}
}

func runClient() {
	initQueryID()

	names := flag.Args()

	if *nameListFlag != "" {
		appendNamesFromFile(names, *nameListFlag)
	}

	for {
		for _, name := range names {
			query(name)
			if *waitFlag < 0 {
				break
			} else if *waitFlag > 0 {
				time.Sleep(time.Duration(*waitFlag) * time.Millisecond)
			}
		}
		if *countFlag > 0 {
			*countFlag--
			if *countFlag <= 0 {
				break
			}
		}
	}
}

func main() {
	flag.Parse()

	if *listenFlag != "" {
		dbPath := flag.Arg(0)
		if dbPath == "" {
			fmt.Fprintf(os.Stderr, "Please specify the path to hosts file as a free argument.")
			os.Exit(1)
		}
		runServer(*listenFlag, dbPath)
	} else {
		runClient()
	}
}
