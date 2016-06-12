package main

import (
	"log"
	"fmt"
	"encoding/hex"
	"net"
)

const MAX_MESSAGE_SIZE = 512

type Message struct {
	Hdr Header
	Questions []Question
	Resources []Resource
}

type Header struct {
	ID uint16
	QR bool
	Opcode uint8
	AA bool
	TC bool
	RD bool
	RA bool
	Z uint8
	Rcode uint8
	QDCount uint16
	ANCount uint16
	NSCount uint16
	ARCount uint16
}

type Question struct {
	Name []byte
	Type uint16
	Class uint16
}

type Resource struct {
	Name []byte
	Type uint16
	Class uint16
	TTL uint32
	Lenght uint16
	Data []byte
}

func (m Message) String() string {
	return fmt.Sprintf("%#v", m)
}

type DataTooShortError error

type MessageData []byte

func (data *MessageData) Byte(offset uint) byte {
	return []byte(*data)[offset]
}

func (data *MessageData) Uint16(byteOffset uint) uint16 {
	return (uint16(data.Byte(byteOffset)) << 8) | uint16(data.Byte(byteOffset + 1))
}

func (data *MessageData) Bit(byteOffset, bitOffset uint) bool {
	return (data.Byte(byteOffset) & (1 << bitOffset)) != 0
}

func (data MessageData) String() string {
	return hex.Dump(data)
}

func (data *MessageData) Parse() *Message {
	m := &Message{}
	m.Hdr.ID = data.Uint16(0)
	m.Hdr.QR = data.Bit(2, 7)
	m.Hdr.Opcode = (data.Byte(2) >> 3) & 0xf
	m.Hdr.AA = data.Bit(2, 2)
	m.Hdr.TC = data.Bit(2, 1)
	m.Hdr.RD = data.Bit(2, 0)

	m.Hdr.RA = data.Bit(3, 7)
	m.Hdr.Z = (data.Byte(3) >> 4) & 0x7
	m.Hdr.Rcode = data.Byte(3) & 0xf

	m.Hdr.QDCount = data.Uint16(4)
	m.Hdr.ANCount = data.Uint16(6)
	m.Hdr.NSCount = data.Uint16(8)
	m.Hdr.ARCount = data.Uint16(10)

	return m
}

func main() {
	log.Printf("dnsd started\n")

	c, err := net.ListenUDP("udp", &net.UDPAddr{[]byte{127, 0, 0, 1}, 5353, ""})
	if err != nil {
		log.Fatal(err)
	}

	data := make(MessageData, MAX_MESSAGE_SIZE)
	for {
		log.Printf("Waiting for a message")
		_, err := c.Read([]byte(data))
		if err != nil {
			log.Print(err)
			continue
		}
		log.Printf("Message data:\n%v", data)
		m := data.Parse()
		log.Printf("Parsed message:\n%v", m)
	}
}



