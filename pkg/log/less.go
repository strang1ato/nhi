package log

import (
	"fmt"
	"os"
	"os/exec"

	"golang.org/x/sys/unix"
)

// setupMemoryFile setups temporary RAM file which exists within program lifetime
func setupMemoryFile(content string, contentLen int) (string, error) {
	fd, err := unix.MemfdCreate("nhiMemoryFile", 0)
	if err != nil {
		return "", err
	}

	if err := unix.Ftruncate(fd, int64(contentLen)); err != nil {
		return "", err
	}

	data, err := unix.Mmap(fd, 0, contentLen, unix.PROT_READ|unix.PROT_WRITE, unix.MAP_SHARED)
	if err != nil {
		return "", err
	}

	copy(data, content)

	if err := unix.Munmap(data); err != nil {
		return "", err
	}

	fp := fmt.Sprintf("/proc/%d/fd/%d", os.Getpid(), fd)
	return fp, nil
}

// runLess open file of given path in less with suitable flags
func runLess(fp string) error {
	cmd := exec.Command("less", "-R", fp)
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout

	if err := cmd.Run(); err != nil {
		return err
	}
	return nil
}
