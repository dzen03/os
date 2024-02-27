#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "../libs/message.h"
#include "../libs/util.h"

static volatile sig_atomic_t keep_running = 1;

static void sig_handler(int _)
{
  (void)_;
  keep_running = 0;
}

struct Directory {
  struct File files[FILES_SIZE];
  int file_descriptors[FILES_SIZE];
  struct Directory* directories[FILES_SIZE];

  uint16_t files_count;
};

int parse_filesystem(int parent_fd, int current_fd, struct Directory* directory, char* name) {
  if (current_fd == 0)
    return -1;

  struct stat st;
  if (fstat(current_fd, &st) < 0)
  {
    fprintf(stderr, "fstat error\n");
    return -2;
  }

  printf("now parsing %s: %lu; index=%d; fd=%d\n", name, (unsigned long)st.st_ino, directory->files_count, current_fd);

  if (directory->files_count == FILES_SIZE) {
    fprintf(stderr, "too many files in directory\n");
    return -3;
  }

  directory->files[directory->files_count++] = (struct File){.inode={st.st_ino},
                                               .type=(S_ISDIR(st.st_mode) ? FILE_TYPE_DIR : FILE_TYPE_FILE)};
  strcpy(directory->files[directory->files_count - 1].name, name);

  directory->file_descriptors[directory->files_count - 1] = current_fd;

  if (S_ISDIR(st.st_mode))
  {
    directory->directories[directory->files_count - 1] = (struct Directory*)malloc(sizeof(struct Directory));
    memset(directory->directories[directory->files_count - 1], 0, sizeof(struct Directory));

    if (fchdir(current_fd) < 0)
    {
      fprintf(stderr, "fchdir error\n");
      return -4;
    }

    DIR *dir = fdopendir(current_fd);
    rewinddir(dir);
    if (!dir)
    {
      fprintf(stderr, "fdopendir error\n");
      return -5;
    }


    struct dirent *ent;
    while ((ent = readdir(dir))) {
      if ((strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0))
        continue;

      int new_fd = open(ent->d_name, O_RDWR, 0);
      if (new_fd <= 0) {
        new_fd = open(ent->d_name, 0);
        if (new_fd <= 0) {
          fprintf(stderr, "open error %s\n", ent->d_name);
          return -6;
        }
      }

      int rec = parse_filesystem(parent_fd, new_fd, directory->directories[directory->files_count - 1], ent->d_name);
      if (rec < 0) {
        rewinddir(dir);
        return rec;
      }
    }
    rewinddir(dir);
  }

  if (fchdir(parent_fd) < 0)
  {
    printf("cant fchdir back\n");
    return -7;
  }

  return 0;
}

static struct Directory* root;

static int root_fd = -1;

int init_fileserver(void) {
  if (root_fd < 0) {
    fprintf(stderr, "Cannot open path: %s", strerror(errno));
    return -1;
  }

  root = malloc(sizeof(struct Directory));
  memset(root, 0, sizeof(struct Directory));

  return parse_filesystem(root_fd, root_fd, root, "root");
}

void stop_fileserver_rec(struct Directory* directory) {
  for (int ind = 0; ind < directory->files_count; ++ind) {
    if (directory->file_descriptors[ind] != root_fd) {
      close(directory->file_descriptors[ind]);
      directory->file_descriptors[ind] = -1;
    }
    if (directory->directories[ind] != NULL) {
      stop_fileserver_rec(directory->directories[ind]);
      free(directory->directories[ind]);
      directory->directories[ind] = NULL;
    }
  }
}

void stop_fileserver(void) {
  stop_fileserver_rec(root);
}

void reboot_fileserver(void) {
  stop_fileserver();
  init_fileserver();
}

static struct Directory* parent_directory_out;
static int parent_ind_out;

void find_by_inode(uint32_t inode, struct Directory* directory, struct Directory** directory_out, int* ind_out) {
  fflush(stdout);

  if (inode == ROOT_INODE) {
    *directory_out = root;
    *ind_out = 0;
    return;
  }

  for (int ind = 0; ind < directory->files_count; ++ind) {
    if (directory->files[ind].inode.inode == inode) {
      *directory_out = directory;
      *ind_out = ind;
      return;
    }

    if (directory->files[ind].type == FILE_TYPE_DIR && directory->directories[ind] != NULL) {
      struct Directory* rec_dir = NULL;
      int rec_int;
      printf("going into %s\n", directory->files[ind].name);
      find_by_inode(inode, directory->directories[ind], &rec_dir, &rec_int);
      if (rec_dir != NULL) {
        *directory_out = rec_dir;
        *ind_out = rec_int;
        if (parent_directory_out == NULL) {
          parent_directory_out = directory;
          parent_ind_out = ind;
        }
        return;
      }
    }
  }

  directory_out = NULL;
  return;
}


int execute_message(struct Message* message) {
  message->return_code = MESSAGE_RETURN_CODE_ERROR;

  printf("Got request #%d\n", message->type);

  struct Directory* directory = NULL;
  int index = -1;
  struct Directory* directory_2 = NULL;
  int index_2 = -1;

  switch (message->type) {
    case MESSAGE_TYPE_ITERATE:
      printf("need to iterate through %u\n", message->iterate.inode.inode);
      find_by_inode(message->iterate.inode.inode, root, &directory, &index);

      if (directory == NULL) {
        return -1;
      }
      else if (directory->directories[index] == NULL) {
        message->iterate.files_count = 0;
        return -1;
      }
      else {
        for (int ind = 0; ind < directory->directories[index]->files_count; ++ind)
          message->iterate.files[ind] = directory->directories[index]->files[ind];
        message->iterate.files_count = directory->directories[index]->files_count;
        printf("found %d files", message->iterate.files_count);
      }
      break;
    case MESSAGE_TYPE_READ:
      find_by_inode(message->read.inode.inode, root, &directory, &index);

      printf("found: %d\n", directory->file_descriptors[index]);
      fflush(stdout);

      if (directory == NULL) {
        return -1;
      }

      memset(message->read.data.data, 0, BUFFER_SIZE);
      message->read.data.size = pread(directory->file_descriptors[index], message->read.data.data,
                                      BUFFER_SIZE, message->read.offset);

      printf("read %d: %s\n", message->read.data.size, message->read.data.data);
      fflush(stdout);

      if (message->read.data.size < 0) {
        return -1;
      }
      break;
    case MESSAGE_TYPE_WRITE:
      printf("writing %s",message->write.data.data);
      find_by_inode(message->write.inode.inode, root, &directory, &index);

      if (directory == NULL) {
        return -1;
      }

      if (message->write.offset == 0) {
        ftruncate(directory->file_descriptors[index], 0);
      }

      int size = pwrite(directory->file_descriptors[index], message->write.data.data,
                        message->write.data.size, message->write.offset);

      printf("wrote %d: %s\n", message->write.data.size, message->write.data.data);
      fflush(stdout);

      if (size < 0) {
        return -1;
      }
      break;
    case MESSAGE_TYPE_LOOKUP:
      printf("lookup on #%u %s\n", message->lookup.parent_inode.inode, message->lookup.file.name);

      find_by_inode(message->lookup.parent_inode.inode, root, &directory, &index);

      if (directory == NULL || directory->directories[index] == NULL) {
        return -1;
      }

      int found = 0;

      for (int ind = 0; ind < directory->directories[index]->files_count; ++ind) {
        if (strcmp(directory->directories[index]->files[ind].name, message->lookup.file.name) == 0) {
          message->lookup.file = directory->directories[index]->files[ind];
          found = 1;
        }
      }
      if (!found) {
        return -1;
      }
      break;
    case MESSAGE_TYPE_CREATE:
      printf("create\n");
      fflush(stdout);

      find_by_inode(message->create.parent_inode.inode, root, &directory, &index);

      if (directory == NULL || directory->directories[index] == NULL) {
        return -1;
      }

      if (fchdir(directory->file_descriptors[index]) < 0)
        return -1;

      int fd;
      struct stat st;

      if (message->create.file.type == FILE_TYPE_FILE) {
        fd = creat(message->create.file.name, (S_IRWXU) | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH));
        if (fd < 0)
          return -1;
        if (fchmod(fd, (S_IRWXU) | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)) < 0)
          return -1;
        if (fstat(fd, &st) < 0)
          return -1;
      }
      else {
        if (mkdir(message->create.file.name, (S_IRWXU) | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)) < 0)
          return -1;
        fd = open(message->create.file.name, 0);
        if (fd < 0)
          return -1;
        if (fchmod(fd, (S_IRWXU) | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)) < 0)
          return -1;
        if (fstat(fd, &st) < 0)
          return -1;
      }
      message->create.file.inode = (struct INode) {st.st_ino};

      printf("adding\n");
      fflush(stdout);

      reboot_fileserver();

      break;
    case MESSAGE_TYPE_REMOVE:
      printf("remove on #%u: %s\n", message->remove.parent_inode.inode, message->remove.file.name);

      find_by_inode(message->remove.parent_inode.inode, root, &directory, &index);

      if (directory == NULL) {
        return -1;
      }

      if (fchdir(directory->file_descriptors[index]) < 0)
        return -1;

      if (message->remove.file.type == FILE_TYPE_FILE) {
        unlink(message->remove.file.name);
      }
      else {
        if (rmdir(message->remove.file.name) < 0)
          return -1;
      }

      reboot_fileserver();
      break;
    case MESSAGE_TYPE_LINK:
      printf("remove on #%u: %s\n", message->link.parent_inode.inode, message->link.file.name);

      find_by_inode(message->link.parent_inode.inode, root, &directory, &index);
      parent_directory_out = NULL;
      find_by_inode(message->link.source_inode.inode, root, &directory_2, &index_2);

      if (directory == NULL || directory_2 == NULL) {
        return -1;
      }

      if (linkat(parent_directory_out->file_descriptors[parent_ind_out], directory_2->files[index_2].name,
                 directory->file_descriptors[index], message->link.file.name, 0) < 0) {
        printf("link error\n");
        fflush(stdout);
        return -1;
      }

      reboot_fileserver();
      break;
    default:
      fprintf(stderr, "Got not implemented type\n");
      fflush(stderr);
      break;
  }

  message->return_code = MESSAGE_RETURN_CODE_OK;

  return 0;
}

int main(int argc, char** argv) {
  signal(SIGINT, sig_handler);

  char err_msg[100] = {0};
  if (argc != 3) {
    fprintf(stderr, "Pass path and port as arguments!");
    return -1;
  }

  root_fd = open(argv[1], 0);
  init_fileserver();

  uint16_t port = atoi(argv[2]);

  int sock_fd, conn_fd;
  uint len;
  struct sockaddr_in server_addr, cli;

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    strcpy(err_msg, "socket creation failed...\n");
    goto EXIT;
  } else {
    printf("Socket created..\n");
  }
  bzero(&server_addr, sizeof(struct sockaddr_in));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  // Binding newly created socket to given IP and verification
  if ((bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in))) != 0) {
    strcpy(err_msg, "socket bind failed...\n");
    goto EXIT;
  } else {
    printf("Socket bound..\n");
  }

  // Now server is ready to listen and verification
  if ((listen(sock_fd, 5)) != 0) {
    strcpy(err_msg, "Listen failed...\n");
    goto EXIT;
  } else {
    printf("Server listening..\n");
  }
  len = sizeof(cli);

  struct Message msg = {0};


  while(keep_running) {
    conn_fd = accept(sock_fd, (struct sockaddr *) &cli, &len);
    if (conn_fd < 0) {
      strcpy(err_msg, "server accept failed...\n");
      goto EXIT;
    } else {
      printf("server accepted the client.\n");
    }

    if (read(conn_fd, &msg, sizeof(struct Message)) == 0) {
      printf("Client exited\n");
      continue;
    }

    execute_message(&msg);

    write(conn_fd, &msg, sizeof(struct Message));


  }

EXIT:
  close(sock_fd);
  stop_fileserver();
  if (err_msg[0] == '\0') {
    printf("Exited gracefully");
    return 0;
  }
  else
    fprintf(stderr, "%s", err_msg);

  return 0;
}