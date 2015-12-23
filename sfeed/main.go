package main

import (
	"bytes"
	"code.google.com/p/go-charset/charset"
	_ "code.google.com/p/go-charset/data"
	"encoding/xml"
	"fmt"
	"io/ioutil"
	"net/http"
	"reflect"
	"strings"
)

type Feed struct {
	XMLName xml.Name
	Title   string   `xml:"title"`
	Link    string   `xml:"link"`
	Items   []Item   `xml:"item"`
}

type Item struct {
	Title       string `xml:"title"`
	Link        string `xml:"link"`
	Description string `xml:"description"`
}

func main() {
	urls := []string{
		"http://rss.slashdot.org/Slashdot/slashdotMain",
		"http://www.osnews.com/files/recent.xml",
	}

	fmt.Println("LATEST NEWS")
	fmt.Println("")
	for _, url := range urls {
		fmt.Println(url)

		r, err := http.Get(url)
		if err != nil {
			fmt.Printf("%s: fetch error: %v\n", url, err)
			continue
		}

		body, err := ioutil.ReadAll(r.Body)

		dumpFeed(body)

		r.Body.Close()
	}
}

func ellipsis(s string, l int) string {
	if len(s) > l {
		s = fmt.Sprintf("%v...", s[:l])
	}
	return s
}

func dumpFeed(data []byte) {
	feed, err := decodeFeedXML(data)
	if err != nil {
		fmt.Printf("failed to decode XML: %v\n", err)
		return
	}

	fmt.Printf("Feed Title: %v\n", feed.Title)
	fmt.Printf("Feed Link: %v\n", feed.Link)
	fmt.Println("Feed items:")
	for _, item := range feed.Items {
		fmt.Printf("+ Title: %v\n", item.Title)
		fmt.Printf("   Link: %v\n", item.Link)
		fmt.Printf("   Desc: %v\n", ellipsis(item.Description, 60))
	}
	fmt.Println("")
}

func decodeFeedXML(xmlData []byte) (feed Feed, err error) {
	r := bytes.NewReader(xmlData)
	d := xml.NewDecoder(r)
	d.CharsetReader = charset.NewReader

	// Find the appropriate root element so we can use the same struct for
	// RSS and RDF.
	var t xml.Token
	var e xml.StartElement
	for {
		t, err = d.Token()
		if err != nil {
			return Feed{}, err
		}
		if reflect.TypeOf(t).ConvertibleTo(reflect.TypeOf(xml.StartElement{})) {
			e = t.(xml.StartElement)
			name := fmt.Sprintf("%v", e.Name)
			fmt.Printf("### examining element %v\n", name)
			if strings.Contains(name, "channel") || strings.Contains(name, "rdf") {
				break
			}
		}
	}

	fmt.Printf("### parsing element %v\n", e.Name)

	feed = Feed{}
	err = d.DecodeElement(&feed, &e)
	return
}
