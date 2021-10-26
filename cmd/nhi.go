package cmd

import (
	"errors"

	"github.com/alecthomas/kong"
	"github.com/strang1ato/nhi/pkg/fetch"
	"github.com/strang1ato/nhi/pkg/log"
	"github.com/strang1ato/nhi/pkg/logSession"
	"github.com/strang1ato/nhi/pkg/remove"
	"github.com/strang1ato/nhi/pkg/rename"
	"github.com/strang1ato/nhi/pkg/utils"
)

// Declare cli variable used by kong
var cli struct {
	Log struct {
		ExitStatus string `short:"s" help:"Only show session(s) where command(s) have specified exit status."`
		Directory  string `short:"d" help:"Only show shell session(s) where command(s) were executed in specified directory."`
		Command    string `short:"c" help:"Only show shell session(s) where command(s) match given regex."`
		Before     string `short:"b" help:"Only show shell session(s) created before specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		After      string `short:"a" help:"Only show shell session(s) created after specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		Long       bool   `short:"l" help:"Use a long listing format."`
	} `cmd help:"Show logs of shell sessions."`

	LogSession struct {
		Session    string `arg required name:"session"`
		ExitStatus string `short:"s" help:"Only show command(s) with specified exit status."`
		Directory  string `short:"d" help:"Only show command(s) executed in specified directory."`
		Command    string `short:"c" help:"Only show command(s) that match given regex."`
		Before     string `short:"b" help:"Only show command(s) executed before specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		After      string `short:"a" help:"Only show command(s) executed after specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		Long       bool   `short:"l" help:"Use a long listing format."`
	} `cmd help:"Show logs of executed commands within <session>."`

	Fetch struct {
		Session          string `arg required name:"session"`
		StartEndRange    string `arg optional name:"start:end"`
		ExitStatus       string `short:"s" help:"Only fetch command(s) with specified exit status."`
		Directory        string `short:"d" help:"Only fetch command(s) that were executed in specified directory."`
		Command          string `short:"c" help:"Only fetch command(s) that match given regex."`
		Before           string `short:"b" help:"Only fetch command(s) that were executed before specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		After            string `short:"a" help:"Only fetch command(s) that were executed after specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		FetchChildShells bool   `short:"f" help:"Fetch content of shells executed within the session."`
		StderrInRed      bool   `short:"r" help:"Show error(s) in red color."`
	} `cmd help:"Fetch shell session, optionally with given range of command(s)."`

	Rename struct {
		Session string `arg required name:"session"`
		NewName string `arg required name:"new-name"`
	} `cmd help:"Rename shell session."`

	Remove struct {
		StartEndRange string `arg required name:"start:end"`
	} `cmd help:"Remove range of shell sessions."`
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
		err = log.Log(db, cli.Log.ExitStatus, cli.Log.Directory, cli.Log.Command, cli.Log.Before, cli.Log.After, cli.Log.Long)
	case "log-session <session>":
		err = logSession.LogSession(db, cli.LogSession.Session, cli.LogSession.ExitStatus, cli.LogSession.Directory, cli.LogSession.Command, cli.LogSession.Before, cli.LogSession.After, cli.LogSession.Long)
	case "fetch <session>":
		var indicator string
		indicator, err = utils.GetSessionIndicator(db, cli.Fetch.Session)
		if err == nil {
			err = fetch.Fetch(db, indicator, ":", cli.Fetch.ExitStatus, cli.Fetch.Directory, cli.Fetch.Command, cli.Fetch.Before, cli.Fetch.After, cli.Fetch.FetchChildShells, cli.Fetch.StderrInRed)
		}
	case "fetch <session> <start:end>":
		var indicator string
		indicator, err = utils.GetSessionIndicator(db, cli.Fetch.Session)
		if err == nil {
			err = fetch.Fetch(db, indicator, cli.Fetch.StartEndRange, cli.Fetch.ExitStatus, cli.Fetch.Directory, cli.Fetch.Command, cli.Fetch.Before, cli.Fetch.After, cli.Fetch.FetchChildShells, cli.Fetch.StderrInRed)
		}
	case "rename <session> <new-name>":
		err = rename.Rename(db, cli.Rename.Session, cli.Rename.NewName)
	case "remove <start:end>":
		err = remove.Remove(db, cli.Remove.StartEndRange)
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
