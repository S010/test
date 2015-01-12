package main

import (
	"os"
	"log"
	"net/http"
	"encoding/json"
	"database/sql"
	_ "github.com/mattn/go-sqlite3"
)

const MY_ADDRESS = ":8080"
const DB_PATH = "./moderator.sqlite3"

var db *sql.DB

type Item struct {
	Id int
	Score int
	Summary string
	Details string
}

func main() {
	var err error

	db, err = sql.Open("sqlite3", DB_PATH)
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	if !isDbInitialized() {
		initDb()
	}

	http.Handle("/moderator/files/",
	    http.StripPrefix("/moderator/files/", http.FileServer(http.Dir("./files"))))
	http.HandleFunc("/moderator", serveModerator)
	http.HandleFunc("/moderator/items", serveItems)
	http.ListenAndServe(MY_ADDRESS, nil)
}

func isDbInitialized() bool {
	_, err := os.Stat(DB_PATH)
	if err == nil {
		return true
	}
	return false
}

func initDb() {
	sql := `create table items (id integer not null primary key,
		score integer not null,
		summary text not null,
		details text not null)`
	_, err := db.Exec(sql)
	if err != nil {
		log.Fatal(err)
	}

	// XXX
	addItem("one", "one")
	addItem("two", "two")
	addItem("three", "three")
}

func addItem(summary string, details string) {
	_, err := db.Exec(`insert into items (score, summary, details) values (?, ?, ?)`,
	    0, summary, details)
	if err != nil {
		log.Fatal(err)
	}
}

func serveModerator(w http.ResponseWriter, r *http.Request) {
	http.Redirect(w, r, "moderator/files/main.html", 302)
}

func serveItems(w http.ResponseWriter, r *http.Request) {
	rows, err := db.Query(`select * from items order by score`)
	if err != nil {
		log.Fatal(err)
	}

	items := make([]Item, 0)
	for rows.Next() {
		item := Item{}
		if err := rows.Scan(&item.Id, &item.Score, &item.Summary, &item.Details); err != nil {
			log.Fatal(err)
		}
		items = append(items, item)
	}

	data, err := json.Marshal(items)
	if err != nil {
		log.Fatal(err)
	}
	w.Write(data)
}
