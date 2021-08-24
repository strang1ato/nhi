package cmd

import (
	"errors"

	"github.com/alecthomas/kong"
	"github.com/strang1ato/nhi/pkg/fetch"
	"github.com/strang1ato/nhi/pkg/log"
	"github.com/strang1ato/nhi/pkg/rename"
	"github.com/strang1ato/nhi/pkg/utils"
)

// Declare cli variable used by kong
var cli struct {
	Log struct {
		Session string `arg optional name:"session"`
	} `cmd help:"Show logs"`

	Fetch struct {
		Session       string `arg required name:"session"`
		StartEndRange string `arg optional name:"start:end"`
	} `cmd help:"Fetch shell session, optionally with given range of commands"`

	Rename struct {
		Session string `arg required name:"session"`
		NewName string `arg required name:"new-name"`
	} `cmd help:"Rename shell session"`
}

// Run runs nhi
func Run() error {
	db, err := utils.OpenDb()
	if err != nil {
		return err
	}

	ctx := kong.Parse(&cli)
	switch ctx.Command() {
	case "log":
		err = log.Log(db, "")
	case "log <session>":
		err = log.Log(db, cli.Log.Session)
	case "fetch <session>":
		err = fetch.Fetch(db, cli.Fetch.Session, ":")
	case "fetch <session> <start:end>":
		err = fetch.Fetch(db, cli.Fetch.Session, cli.Fetch.StartEndRange)
	case "rename <session> <new-name>":
		err = rename.Rename(db, cli.Rename.Session, cli.Rename.NewName)
	default:
		err = errors.New("Command not found")
	}

	if err == nil {
		err = db.Close()
	} else {
		db.Close()
	}
	return err
}
