#include "common.h"

#include "vmlinux.h"

#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "GPL";

struct bpf_map_def SEC("maps") shell_pids_and_indicators = {
  .type = BPF_MAP_TYPE_ARRAY,
  .key_size = 4,
  .value_size = sizeof(struct shell_pid_and_indicator),
  .max_entries = SHELL_PIDS_AND_INDICATORS_MAX_ENTRIES,
};

struct bpf_map_def SEC("maps") child_pids_and_shell_pids = {
  .type = BPF_MAP_TYPE_ARRAY,
  .key_size = 4,
  .value_size = sizeof(struct child_pid_and_shell_pid),
  .max_entries = CHILD_PIDS_AND_SHELL_PIDS_MAX_ENTRIES,
};

struct bpf_map_def SEC("maps") ring_buffer = {
  .type = BPF_MAP_TYPE_RINGBUF,
  .max_entries = RING_BUFFER_MAX_ENTRIES,
};

struct bpf_map_def SEC("maps") __fdget_pos_pos = {
  .type = BPF_MAP_TYPE_PERCPU_ARRAY,
  .key_size = 4,
  .value_size = sizeof(unsigned long),
  .max_entries = 1,
};

struct bpf_map_def SEC("maps") ksys_write_event = {
  .type = BPF_MAP_TYPE_PERCPU_ARRAY,
  .key_size = 4,
  .value_size = KSYS_WRITE_EVENT_SIZE,
  .max_entries = 1,
};

struct bpf_map_def SEC("maps") ksys_write_d_inode = {
  .type = BPF_MAP_TYPE_PERCPU_ARRAY,
  .key_size = 4,
  .value_size = sizeof(struct inode),
  .max_entries = 1,
};

SEC("fentry/kill_something_info")
int BPF_PROG(kill_something_info, int sig, struct kernel_siginfo *info, pid_t pid)
{
  if (pid != NHID_HASH) {
    return 0;
  }

  pid_t shell_pid = bpf_get_current_pid_tgid() >> 32;
  if (sig == SIGUSR1) {
    // remove element where shell_pid from child_pids_and_shell_pids
    int i = 0;
    struct child_pid_and_shell_pid zero = {0};
    int pid_index = -1;
    for (int j = 0; j<CHILD_PIDS_AND_SHELL_PIDS_MAX_ENTRIES; j++) {
      struct child_pid_and_shell_pid *child_pid_and_shell_pid;
      child_pid_and_shell_pid = bpf_map_lookup_elem(&child_pids_and_shell_pids, &i);
      if (child_pid_and_shell_pid) {
        if (child_pid_and_shell_pid->child_pid == shell_pid) {
          pid_index = i;
        } else if (!child_pid_and_shell_pid->child_pid) {
          if (pid_index != -1) {
            i--;
            child_pid_and_shell_pid = bpf_map_lookup_elem(&child_pids_and_shell_pids, &i);
            if (child_pid_and_shell_pid) {
              bpf_map_update_elem(&child_pids_and_shell_pids, &pid_index, child_pid_and_shell_pid, BPF_ANY);
            }
            bpf_map_update_elem(&child_pids_and_shell_pids, &i, &zero, BPF_ANY);
          }
          break;
        }
      }
      i++;
    }
  }

  struct kill_event *kill_event = bpf_ringbuf_reserve(&ring_buffer, sizeof(struct kill_event), 0);
  if (!kill_event) {
    return 0;
  }

  kill_event->shell_pid = shell_pid;
  kill_event->sig = sig;
  bpf_ringbuf_submit(kill_event, 0);
  return 0;
}

SEC("fexit/kernel_clone")
int BPF_PROG(kernel_clone_ret, struct kernel_clone_args *args, pid_t new_child_pid)
{
  pid_t parent_pid = bpf_get_current_pid_tgid() >> 32;
  // add new_child_pid to child_pids_and_shell_pids if parent_pid in shell_pids_and_indicators
  {
    struct shell_pid_and_indicator *shell_pid_and_indicator;
    int i = 0;
    for (int j = 0; j<SHELL_PIDS_AND_INDICATORS_MAX_ENTRIES; j++) {
      shell_pid_and_indicator = bpf_map_lookup_elem(&shell_pids_and_indicators, &i);
      if (shell_pid_and_indicator) {
        if (!shell_pid_and_indicator->shell_pid) {
          break;
        } else if (shell_pid_and_indicator->shell_pid == parent_pid) {
          goto add_new_child_pid;
        }
      }
      i++;
    }
  }

  // add new_child_pid to child_pids_and_shell_pids if parent_pid in child_pids_and_shell_pids
  {
    struct child_pid_and_shell_pid *child_pid_and_shell_pid;
    int i = 0;
    for (int j = 0; j<CHILD_PIDS_AND_SHELL_PIDS_MAX_ENTRIES; j++) {
      child_pid_and_shell_pid = bpf_map_lookup_elem(&child_pids_and_shell_pids, &i);
      if (child_pid_and_shell_pid) {
        if (!child_pid_and_shell_pid->child_pid) {
          return 0;
        } else if (child_pid_and_shell_pid->child_pid == parent_pid) {
          parent_pid = child_pid_and_shell_pid->shell_pid;
          goto add_new_child_pid;
        }
      }
      i++;
    }
  }

add_new_child_pid: {
    int i = 0;
    for (int j = 0; j<CHILD_PIDS_AND_SHELL_PIDS_MAX_ENTRIES; j++) {
      struct child_pid_and_shell_pid *child_pid_and_shell_pid = bpf_map_lookup_elem(&child_pids_and_shell_pids, &i);
      if (child_pid_and_shell_pid) {
        if (!child_pid_and_shell_pid->child_pid) {
          struct child_pid_and_shell_pid helper;
          helper.child_pid = new_child_pid;
          helper.shell_pid = parent_pid;
          bpf_map_update_elem(&child_pids_and_shell_pids, &i, &helper, BPF_ANY);

          pid_t *shell_pid = bpf_ringbuf_reserve(&ring_buffer, sizeof(pid_t), 0);
          if (!shell_pid) {
            return 0;
          }
          *shell_pid = parent_pid;
          bpf_ringbuf_submit(shell_pid, 0);
          break;
        }
      }
      i++;
    }
  }
  return 0;
}

SEC("fentry/do_exit")
int BPF_PROG(do_exit, long code)
{
  pid_t current_pid = bpf_get_current_pid_tgid() >> 32;
  // remove element where current_pid, from shell_pids_and_indicators
  {
    int i = 0;
    int pid_index = -1;

    long indicator;
    for (int j = 0; j<SHELL_PIDS_AND_INDICATORS_MAX_ENTRIES; j++) {
      struct shell_pid_and_indicator *shell_pid_and_indicator;
      shell_pid_and_indicator = bpf_map_lookup_elem(&shell_pids_and_indicators, &i);
      if (shell_pid_and_indicator) {
        if (!i && !shell_pid_and_indicator->shell_pid) {
          return 0;
        }

        if (shell_pid_and_indicator->shell_pid == current_pid) {
          pid_index = i;

          indicator = shell_pid_and_indicator->indicator;
        } else if (!shell_pid_and_indicator->shell_pid) {
          if (pid_index != -1) {
            i--;
            shell_pid_and_indicator = bpf_map_lookup_elem(&shell_pids_and_indicators, &i);
            if (shell_pid_and_indicator) {
              bpf_map_update_elem(&shell_pids_and_indicators, &pid_index, shell_pid_and_indicator, BPF_ANY);
            }
            struct shell_pid_and_indicator zero = {0};
            bpf_map_update_elem(&shell_pids_and_indicators, &i, &zero, BPF_ANY);

            struct exit_shell_indicator_event *exit_shell_indicator_event = bpf_ringbuf_reserve(&ring_buffer, sizeof(struct exit_shell_indicator_event), 0);
            if (!exit_shell_indicator_event) {
              return 0;
            }
            exit_shell_indicator_event->indicator = indicator;
            bpf_ringbuf_submit(exit_shell_indicator_event, 0);
            return 0;
          }
          break;
        }
      }
      i++;
    }
  }

  // remove element where current_pid, from child_pids_and_shell_pids
  {
    int i = 0;
    int pid_index = -1;
    for (int j = 0; j<CHILD_PIDS_AND_SHELL_PIDS_MAX_ENTRIES; j++) {
      struct child_pid_and_shell_pid *child_pid_and_shell_pid;
      child_pid_and_shell_pid = bpf_map_lookup_elem(&child_pids_and_shell_pids, &i);
      if (child_pid_and_shell_pid) {
        if (child_pid_and_shell_pid->child_pid == current_pid) {
          pid_index = i;
        } else if (!child_pid_and_shell_pid->child_pid) {
          if (pid_index != -1) {
            i--;
            child_pid_and_shell_pid = bpf_map_lookup_elem(&child_pids_and_shell_pids, &i);
            if (child_pid_and_shell_pid) {
              bpf_map_update_elem(&child_pids_and_shell_pids, &pid_index, child_pid_and_shell_pid, BPF_ANY);
            }
            struct child_pid_and_shell_pid zero = {0};
            bpf_map_update_elem(&child_pids_and_shell_pids, &i, &zero, BPF_ANY);
          }
          return 0;
        }
      }
      i++;
    }
  }
  return 0;
}

SEC("fexit/ksys_write")
int BPF_PROG(ksys_write, int fd, char *buf, size_t count)
{
  if (count == 0) {
    return 0;
  }

  pid_t current_pid = bpf_get_current_pid_tgid() >> 32;
  // find shell pid of the process
  pid_t shell_pid;
  {
    int i = 0;
    for (int j = 0; j<CHILD_PIDS_AND_SHELL_PIDS_MAX_ENTRIES; j++) {
      struct child_pid_and_shell_pid *child_pid_and_shell_pid;
      child_pid_and_shell_pid = bpf_map_lookup_elem(&child_pids_and_shell_pids, &i);
      if (child_pid_and_shell_pid) {
        if (!child_pid_and_shell_pid->child_pid) {
          return 0;
        }

        if (child_pid_and_shell_pid->child_pid == current_pid) {
          shell_pid = child_pid_and_shell_pid->shell_pid;
          break;
        }
      }
      i++;
    }
  }

  // find indicator of the shell
  long indicator = 0;
  {
    int i = 0;
    for (int j = 0; j<SHELL_PIDS_AND_INDICATORS_MAX_ENTRIES; j++) {
      struct shell_pid_and_indicator *shell_pid_and_indicator;
      shell_pid_and_indicator = bpf_map_lookup_elem(&shell_pids_and_indicators, &i);
      if (shell_pid_and_indicator) {
        if (!shell_pid_and_indicator->shell_pid) {
          return 0;
        }

        if (shell_pid_and_indicator->shell_pid == shell_pid) {
          indicator = shell_pid_and_indicator->indicator;
          break;
        }
      }
      i++;
    }
  }

  int zero = 0;
  unsigned long *fd_pos = bpf_map_lookup_elem(&__fdget_pos_pos, &zero);
  if (!fd_pos) {
    return 0;
  }

  struct file file;
  bpf_core_read(&file, sizeof(struct file), (*fd_pos & ~3));

  struct dentry dentry;
  bpf_core_read(&dentry, sizeof(struct dentry), file.f_path.dentry);

  struct inode *d_inode;
  d_inode = bpf_map_lookup_elem(&ksys_write_d_inode, &zero);
  if (!d_inode) {
    return 0;
  }

  bpf_core_read(d_inode, sizeof(struct inode), dentry.d_inode);

  if (d_inode->i_gid.val != 5) {  // tty group id is 5
    return 0;
  }

  if (count <= KSYS_WRITE_EVENT_SIZE-sizeof(long)-1) {
    int zero = 0;
    struct write_event *write_event;
    write_event = bpf_map_lookup_elem(&ksys_write_event, &zero);
    if (!write_event) {
      return 0;
    }

    write_event->indicator = indicator;
    if (buf) {
      bpf_probe_read_user(write_event->output, count, buf);
    }

    bpf_ringbuf_output(&ring_buffer, write_event, count+sizeof(long)+1, 0);
  }
  return 0;
}

SEC("fexit/__fdget_pos")
int BPF_PROG(__fdget_pos, unsigned int fd, unsigned long fd_pos)
{
  int zero = 0;
  bpf_map_update_elem(&__fdget_pos_pos, &zero, &fd_pos, BPF_ANY);
  return 0;
}
