package main

import (
	"fmt"
	"github.com/SlyMarbo/rss"
	"html/template"
	"os"
)

const TEMPLATE = `
<html>
<head><title>Latest News</title></head>
<body>
<h2>{{.Title}}</h2>
{{range .Items}}
<div>
<h4>{{.Title}}</h4>
<div>{{.Content}}</div>
</div>
{{end}}
</body>
</html>
`

func main() {
	urls := []string{
		"http://rss.slashdot.org/Slashdot/slashdotMain",
		"http://www.osnews.com/files/recent.xml",
	}

	feeds := make([]*rss.Feed, 0)

	for _, url := range urls {
		feed, err := rss.Fetch(url)
		if err != nil {
			fmt.Printf("%v: %v\n", url, err)
		}

		feeds = append(feeds, feed)

		err = feed.Update()
		if err != nil {
			fmt.Printf("%v: %v\n", feed.Link, err)
		}
	}

	fmt.Println("LATEST NEWS\n")
	t := template.Must(template.New("webpage").Parse(TEMPLATE))
	for _, feed := range feeds {
		err := t.Execute(os.Stdout, feed)
		if err != nil {
			fmt.Printf("error executing template: %v\n", err)
		}
	}
}

func ellipsis(s string, l int) string {
	if len(s) > l {
		s = fmt.Sprintf("%v...", s[:l])
	}
	return s
}
