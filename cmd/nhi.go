package cmd

import (
	"errors"

	"github.com/alecthomas/kong"
	"github.com/strang1ato/nhi/pkg/fetch"
	"github.com/strang1ato/nhi/pkg/log"
	"github.com/strang1ato/nhi/pkg/logSession"
	"github.com/strang1ato/nhi/pkg/rename"
	"github.com/strang1ato/nhi/pkg/utils"
)

// Declare cli variable used by kong
var cli struct {
	Log struct {
		Session   string `arg optional name:"session"`
		Directory string `short:"d" help:"Only show commands that were executed in specified directory"`
		Long      bool   `short:"l" help:"Use a long listing format"`
	} `cmd help:"Show logs"`

	Fetch struct {
		Session       string `arg required name:"session"`
		StartEndRange string `arg optional name:"start:end"`
		Directory     string `short:"d" help:"Only fetch commands that were executed in specified directory"`
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
		err = log.Log(db, cli.Log.Directory, cli.Log.Long)
	case "log <session>":
		err = logSession.LogSession(db, cli.Log.Session, cli.Log.Directory, cli.Log.Long)
	case "fetch <session>":
		var indicator string
		indicator, err = utils.GetSessionIndicator(db, cli.Fetch.Session)
		if err == nil {
			err = fetch.Fetch(db, indicator, ":", cli.Fetch.Directory)
		}
	case "fetch <session> <start:end>":
		var indicator string
		indicator, err = utils.GetSessionIndicator(db, cli.Fetch.Session)
		if err == nil {
			err = fetch.Fetch(db, indicator, cli.Fetch.StartEndRange, cli.Fetch.Directory)
		}
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
