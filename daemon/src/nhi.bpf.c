#include "common.h"

#include "vmlinux.h"

#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "GPL";

struct bpf_map_def SEC("maps") shells = {
  .type = BPF_MAP_TYPE_ARRAY,
  .key_size = 4,
  .value_size = sizeof(struct shell),
  .max_entries = SHELLS_MAX_ENTRIES,
};

struct bpf_map_def SEC("maps") children = {
  .type = BPF_MAP_TYPE_ARRAY,
  .key_size = 4,
  .value_size = sizeof(struct child),
  .max_entries = CHILDREN_MAX_ENTRIES,
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

  pid_t parent_shell_pid = 0;
  long parent_shell_indicator = 0;
  if (sig == SIGUSR1) {
    // remove element where shell_pid from children
    int i = 0;
    struct child zero = {0};
    int pid_index = -1;
    for (int j = 0; j<CHILDREN_MAX_ENTRIES; j++) {
      struct child *child;
      child = bpf_map_lookup_elem(&children, &i);
      if (child) {
        if (child->child_pid == shell_pid) {
          pid_index = i;

          parent_shell_pid = child->shell_pid;
        } else if (!child->child_pid) {
          if (pid_index != -1) {
            i--;
            child = bpf_map_lookup_elem(&children, &i);
            if (child) {
              bpf_map_update_elem(&children, &pid_index, child, BPF_ANY);
            }
            bpf_map_update_elem(&children, &i, &zero, BPF_ANY);
          }
          break;
        }
      }
      i++;
    }

    // find indicator of the parent shell
    i = 0;
    for (int j = 0; j<SHELLS_MAX_ENTRIES; j++) {
      struct shell *shell;
      shell = bpf_map_lookup_elem(&shells, &i);
      if (shell) {
        if (!shell->shell_pid) {
          break;
        }

        if (shell->shell_pid == parent_shell_pid) {
          parent_shell_indicator = shell->indicator;
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
  kill_event->parent_shell_indicator = parent_shell_indicator;
  bpf_ringbuf_submit(kill_event, 0);
  return 0;
}

SEC("fexit/kernel_clone")
int BPF_PROG(kernel_clone_ret, struct kernel_clone_args *args, pid_t new_child_pid)
{
  pid_t parent_pid = bpf_get_current_pid_tgid() >> 32;
  // add new_child_pid to children if parent_pid in shells
  {
    struct shell *shell;
    int i = 0;
    for (int j = 0; j<SHELLS_MAX_ENTRIES; j++) {
      shell = bpf_map_lookup_elem(&shells, &i);
      if (shell) {
        if (!shell->shell_pid) {
          break;
        } else if (shell->shell_pid == parent_pid) {
          goto add_new_child_pid;
        }
      }
      i++;
    }
  }

  // add new_child_pid to children if parent_pid in children
  {
    struct child *child;
    int i = 0;
    for (int j = 0; j<CHILDREN_MAX_ENTRIES; j++) {
      child = bpf_map_lookup_elem(&children, &i);
      if (child) {
        if (!child->child_pid) {
          return 0;
        } else if (child->child_pid == parent_pid) {
          parent_pid = child->shell_pid;
          goto add_new_child_pid;
        }
      }
      i++;
    }
  }

add_new_child_pid: {
    int i = 0;
    for (int j = 0; j<CHILDREN_MAX_ENTRIES; j++) {
      struct child *child = bpf_map_lookup_elem(&children, &i);
      if (child) {
        if (!child->child_pid) {
          struct child helper;
          helper.child_pid = new_child_pid;
          helper.shell_pid = parent_pid;
          bpf_map_update_elem(&children, &i, &helper, BPF_ANY);

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
  // remove element where current_pid, from shells
  {
    int i = 0;
    int pid_index = -1;

    long indicator;
    for (int j = 0; j<SHELLS_MAX_ENTRIES; j++) {
      struct shell *shell;
      shell = bpf_map_lookup_elem(&shells, &i);
      if (shell) {
        if (!i && !shell->shell_pid) {
          return 0;
        }

        if (shell->shell_pid == current_pid) {
          pid_index = i;

          indicator = shell->indicator;
        } else if (!shell->shell_pid) {
          if (pid_index != -1) {
            i--;
            shell = bpf_map_lookup_elem(&shells, &i);
            if (shell) {
              bpf_map_update_elem(&shells, &pid_index, shell, BPF_ANY);
            }
            struct shell zero = {0};
            bpf_map_update_elem(&shells, &i, &zero, BPF_ANY);

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

  // remove element where current_pid, from children
  {
    int i = 0;
    int pid_index = -1;
    for (int j = 0; j<CHILDREN_MAX_ENTRIES; j++) {
      struct child *child;
      child = bpf_map_lookup_elem(&children, &i);
      if (child) {
        if (child->child_pid == current_pid) {
          pid_index = i;
        } else if (!child->child_pid) {
          if (pid_index != -1) {
            i--;
            child = bpf_map_lookup_elem(&children, &i);
            if (child) {
              bpf_map_update_elem(&children, &pid_index, child, BPF_ANY);
            }
            struct child zero = {0};
            bpf_map_update_elem(&children, &i, &zero, BPF_ANY);
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
    for (int j = 0; j<CHILDREN_MAX_ENTRIES; j++) {
      struct child *child;
      child = bpf_map_lookup_elem(&children, &i);
      if (child) {
        if (!child->child_pid) {
          return 0;
        }

        if (child->child_pid == current_pid) {
          shell_pid = child->shell_pid;
          break;
        }
      }
      i++;
    }
  }

  long indicator = 0;
  int shell_index = 0;
  // find indicator of the shell
  for (int j = 0; j<SHELLS_MAX_ENTRIES; j++) {
    struct shell *shell;
    shell = bpf_map_lookup_elem(&shells, &shell_index);
    if (shell) {
      if (!shell->shell_pid) {
        return 0;
      }

      if (shell->shell_pid == shell_pid) {
        indicator = shell->indicator;
        break;
      }
    }
    shell_index++;
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

  char specificity;
  if (fd == 2) {
    specificity = -2;
  } else {
    specificity = -1;
  }

  if (count <= KSYS_WRITE_EVENT_SIZE-sizeof(long)-1) {
    int zero = 0;
    struct write_event *write_event;
    write_event = bpf_map_lookup_elem(&ksys_write_event, &zero);
    if (!write_event) {
      return 0;
    }

    write_event->indicator = indicator;
    write_event->output[0] = specificity;
    if (buf) {
      bpf_probe_read_user(write_event->output+1, count, buf);
    }

    // look for and handle alternate screen escape sequences if any
    for (int i=1; i<512; i++) {
      if (i == count) {
        break;
      }

      if (write_event->output[i] == 27 && write_event->output[i+1] == '[' && write_event->output[i+2] == '?' && write_event->output[i+3] == '1' && write_event->output[i+4] == '0' && write_event->output[i+5] == '4' && write_event->output[i+6] == '9') {
        struct shell *helper;
        helper = bpf_map_lookup_elem(&shells, &shell_index);
        if (!helper) {
          return 0;
        }
        if (write_event->output[i+7] == 'h') {
          helper->omit_write = 1;
          bpf_map_update_elem(&shells, &shell_index, helper, BPF_ANY);
          return 0;
        } else if (write_event->output[i+7] == 'l') {
          helper->omit_write = 0;
          bpf_map_update_elem(&shells, &shell_index, helper, BPF_ANY);
          write_event->output[i] = 0;
          write_event->output[i+1] = 0;
          write_event->output[i+2] = 0;
          write_event->output[i+3] = 0;
          write_event->output[i+4] = 0;
          write_event->output[i+5] = 0;
          write_event->output[i+6] = 0;
          write_event->output[i+7] = 0;
        }
      }
    }

    struct shell *shell;
    shell = bpf_map_lookup_elem(&shells, &shell_index);
    if (shell && shell->omit_write == 1) {
      return 0;
    }

    if (count == 7) {
      count++;
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
