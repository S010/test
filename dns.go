package main

import (
	"log"
	"fmt"
	"encoding/hex"
	"net"
	"strings"
	"bytes"
	"time"
)

//
// Message
//

const MAX_MESSAGE_SIZE = 512

const (
	TYPE_A = 1
	CLASS_IN = 1
)

type Message struct {
	Hdr Header
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
	Lenght uint16
	Data []byte
}

func (m Message) String() string {
	return fmt.Sprintf("%#v", m)
}

func (m Message) PrettyString() string {
	return fmt.Sprintf("ID: %v\nQR: %v\nOpCode: %v\nAA: %v\nTC: %v\nRD: %v\nRA: %v\nZ: %v\nRCode: %v\nQDCount: %v\nANCount: %v\nNSCount: %v\nARCount: %v\n",
		m.Hdr.ID, m.Hdr.QR, m.Hdr.OpCode, m.Hdr.AA, m.Hdr.TC, m.Hdr.RD, m.Hdr.RA, m.Hdr.Z, m.Hdr.RCode, m.Hdr.QDCount, m.Hdr.ANCount, m.Hdr.NSCount, m.Hdr.ARCount)
}

func (m *Message) Serialize() MessageData {
	data := make(MessageData, 0, MAX_MESSAGE_SIZE)

	data.AppendUint16(m.Hdr.ID)
	data.AppendByte(boolToByte(m.Hdr.QR, 7) | (m.Hdr.OpCode << 3) | boolToByte(m.Hdr.AA, 2) | boolToByte(m.Hdr.TC, 1) | boolToByte(m.Hdr.RD, 0))
	data.AppendByte(boolToByte(m.Hdr.RA, 7) | ((m.Hdr.Z & 0xf) << 4) | (m.Hdr.RCode & 0xf))
	data.AppendUint16(m.Hdr.QDCount)
	data.AppendUint16(m.Hdr.ANCount)
	data.AppendUint16(m.Hdr.NSCount)
	data.AppendUint16(m.Hdr.ARCount)

	for _, q := range m.Questions {
		data.AppendName(q.Name)
		data.AppendUint16(q.Type)
		data.AppendUint16(q.Class)
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
	//log.Printf("dbg: pos=%v", pos)

	b := bytes.Buffer{}
	for {
		l := data.Byte(pos)
		pos++
		if l == 0 {
			break
		}
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
	m.Hdr.ID, pos = data.Uint16(pos)
	m.Hdr.QR = data.Bit(2, 7)
	m.Hdr.OpCode = (data.Byte(2) >> 3) & 0xf
	m.Hdr.AA = data.Bit(2, 2)
	m.Hdr.TC = data.Bit(2, 1)
	m.Hdr.RD = data.Bit(2, 0)

	m.Hdr.RA = data.Bit(3, 7)
	m.Hdr.Z = (data.Byte(3) >> 4) & 0x7
	m.Hdr.RCode = data.Byte(3) & 0xf

	pos = 4

	m.Hdr.QDCount, pos = data.Uint16(pos)
	m.Hdr.ANCount, pos = data.Uint16(pos)
	m.Hdr.NSCount, pos = data.Uint16(pos)
	m.Hdr.ARCount, pos = data.Uint16(pos)

	var question Question
	var resource Resource

	for i := 0; i < int(m.Hdr.QDCount); i++ {
		question, pos = data.Question(pos)
		m.Questions = append(m.Questions, question)
	}

	count := m.Hdr.ANCount + m.Hdr.NSCount + m.Hdr.ARCount
	for i := 0; i < int(count); i++ {
		resource, pos = data.Resource(pos)
		m.Resources = append(m.Resources, resource)
	}

	return m
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

/*
// Server
func main() {
	log.Printf("dnsd started\n")

	c, err := net.ListenUDP("udp", &net.UDPAddr{[]byte{127, 0, 0, 1}, 5353, ""})
	if err != nil {
		log.Fatal(err)
	}

	go query("www.google.com")

	for {
		data := make(MessageData, MAX_MESSAGE_SIZE)
		log.Printf("Waiting for a message")
		n, err := c.Read([]byte(data))
		if err != nil {
			log.Print(err)
			continue
		}
		data = data[:n]
		log.Printf("Message data:\n%v", data)
		m := data.Parse()
		log.Printf("Parsed message:\n%v", m)
	}
}
*/

func NewQuery(name string) *Message {
	var m Message

	m.Hdr.ID = 0x1001
	m.Hdr.RD = true
	m.Hdr.QDCount = 1
	m.Questions = append(m.Questions, Question{Name: name, Type: TYPE_A, Class: CLASS_IN})

	return &m
}

// Client
func main() {
	c, err := net.DialUDP("udp", nil, &net.UDPAddr{[]byte{8, 8, 8, 8}, 53, ""})
	if err != nil {
		log.Fatal(err)
	}

	msg := NewQuery("www.google.com")

	data := msg.Serialize()
	log.Printf("Query data:\n%v", data)
	log.Printf("Parsed query:\n%v", msg.PrettyString())
	c.Write(data)

	c.SetReadDeadline(time.Now().Add(time.Second))
	data = make(MessageData, MAX_MESSAGE_SIZE)
	n, err := c.Read([]byte(data))
	if err != nil {
		log.Fatal(err)
	}
	data = data[:n]

	log.Printf("Reply data:\n%v", data)
	
	log.Printf("Parsed reply:\n%v", data.Parse().PrettyString())
}
