# nhi

**:warning: This project is in pre-alpha stage :warning:**

`nhi` automatically saves every potentially useful information about each executed command and everything around
and delivers powerful querying mechanism

`nhi` keeps records of
- output of command
- working directory at the end of command execution
- start time of command
- finish time of command
- shell prompt at the time of execution
- (and much more in the future :smile:)

`nhi` also keeps record of infomations about shell session in general

These features allow retriving of past executed commands and whole shell sessions
as well as every other useful information in convenient way

You can think of `nhi` as a greatly enhanced `bash history`

`nhi` currently supports only `linux x86_64` and `bash`, but support for other architectures and `zsh` is planned as well

### Requirements

`bash 4.4+`, `systemd` and non-archaic `linux` kernel

### Installation

#### From source

Install [sqlite-queue](https://github.com/strang1ato/sqlite-queue) following [this guide](https://github.com/strang1ato/sqlite-queue#installation)

Clone repository:
```bash
  git clone https://github.com/strang1ato/nhi.git
```

Install [Go](https://golang.org/) 1.16

Make sure that you have `gcc`, `make`, `sqlite3` and `libsqlite3-dev` installed (example for debian/ubuntu):
```bash
  sudo apt-get install build-essential sqlite3 libsqlite3-dev
```

`cd` to cloned directory and run:
```bash
  make build-lib && make build-cli && make copy-.nhi && make create-db && make install-sqlite-queue-service && make start-sqlite-queue-service
```

Next run:
```bash
  sudo make install
```

If you didn't get any errors, set `bash-nhi` to your shell in your terminal and restart terminal.


#### Binaries

Download latest files and source code from [releases](https://github.com/strang1ato/nhi/releases)

But first of all install [sqlite-queue](https://github.com/strang1ato/sqlite-queue) following [this guide](https://github.com/strang1ato/sqlite-queue#installation)

Make files executable:
```bash
  sudo chmod +x <path-to-bash-nhi>
  sudo chmod +x <path-to-nhi.so>
  sudo chmod +x <path-to-nhi>
```

Move files to proper directories:
```bash
  sudo mv <path-to-bash-nhi> /usr/local/bin
  sudo mv <path-to-nhi.so> /usr/lib
  sudo mv <path-to-nhi> /usr/local/bin
```

Make sure that you have `make`, `sqlite3` and `libsqlite3-dev` installed (example for debian/ubuntu):
```bash
  sudo apt-get install build-essential sqlite3 libsqlite3-dev
```

Unzip downloaded source code

`cd` to source code and run:
```bash
  make copy-.nhi && make create-db && make install-sqlite-queue-service && make start-sqlite-queue-service
```

If you didn't get any errors, set `bash-nhi` to your shell in your terminal and restart terminal.

#### Testing

That's it, you can test if `nhi` installed correctly by running any command in your terminal, and later, in the same session running:
```bash
  nhi fetch {-1}
```
If your shell session "duplicated" then you have installed `nhi` correctly
