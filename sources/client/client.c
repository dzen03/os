#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/inet.h>
#include <linux/net.h>

#include "../libs/message.h"
#include "../libs/util.h"

#define check_response_code(message, ret)           \
if (message->return_code != MESSAGE_RETURN_CODE_OK) \
{                                                   \
printk(KERN_ERR "call err\n");                      \
kfree(message);                                     \
return ret;                                         \
}


struct server {
  char* ip;
  uint16_t port;
};

int send_message(struct server* server, struct Message* message);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ilya @dzen03");
MODULE_DESCRIPTION("A simple NFS!");
MODULE_VERSION("0.1");

void nfs_kill_sb(struct super_block* sb);

struct inode* nfs_get_inode(struct super_block* sb, const struct inode* dir, umode_t mode, int i_ino);
int nfs_fill_super(struct super_block* sb, void* data, int silent);
struct dentry* nfs_mount(struct file_system_type* type, int flags, const char* addr, void* data);

int nfs_iterate(struct file* f, struct dir_context* ctxt);
ssize_t nfs_read(struct file* f, char* buffer, size_t len, loff_t* off);
ssize_t nfs_write(struct file* f, const char* buffer, size_t len, loff_t* off);
struct dentry* nfs_lookup(struct inode* parent_inode, struct dentry* child_dentry, unsigned int flag);
int nfs_create(struct mnt_idmap* mnt_idmap, struct inode* parent_inode, struct dentry* child_dentry, umode_t mode, bool b);
int nfs_mkdir(struct mnt_idmap* mnt_idmap, struct inode* parent_inode, struct dentry* child_dentry, umode_t mode);
int nfs_unlink(struct inode* parent_inode, struct dentry* child_dentry);
int nfs_rmdir(struct inode* parent_inode, struct dentry* child_dentry);
int nfs_link(struct dentry *old_dentry, struct inode *parent_inode, struct dentry *new_dentry);

struct file_system_type nfs_fs_type = { .name = "lab4", .mount = nfs_mount, .kill_sb = nfs_kill_sb };
struct file_operations nfs_dir_ops = {
    .iterate_shared = nfs_iterate,
    .read = nfs_read,
    .write = nfs_write,
};

struct inode_operations nfs_inode_ops = {
    .lookup = nfs_lookup,
    .create = nfs_create,
    .mkdir = nfs_mkdir,
    .rmdir = nfs_rmdir,
    .link = nfs_link,
    .unlink = nfs_unlink,
};

int init_nfs(void)
{
  int res = register_filesystem(&nfs_fs_type);
  printk(KERN_INFO "registered nfs\n");
  return res;
}

void exit_nfs(void)
{
  unregister_filesystem(&nfs_fs_type);
  printk(KERN_INFO "unregistered nfs\n");
}

module_init(init_nfs);
module_exit(exit_nfs);

struct inode* nfs_get_inode(struct super_block* sb, const struct inode* dir, umode_t mode, int i_ino)
{
  struct inode* inode;
  inode = new_inode(sb);
  if (inode != NULL) {
    inode->i_ino = i_ino;
    inode->i_op = &nfs_inode_ops;
    inode->i_fop = &nfs_dir_ops;
    inode_init_owner(&nop_mnt_idmap, inode, dir, mode);
  }
  return inode;
}


int nfs_fill_super(struct super_block* sb, void* data, int silent)
{
  struct inode* inode;
  inode = nfs_get_inode(sb, NULL, S_IFDIR | 0777, ROOT_INODE); // drwxr-xr-x
  sb->s_root = d_make_root(inode);
  if (sb->s_root == NULL) {
    return -1;
  }
  return 0;
}

struct dentry* nfs_mount(struct file_system_type* type, int flags, const char* addr, void* data)
{
  printk(KERN_INFO "trying to mount from %s\n", addr);

  struct dentry* ret;
  uint16_t it = 0;
  while (addr[it] != 0)
  {
    if (addr[it] == ':')
      break;
    it++;
  }
  uint16_t port;
  if (addr[it] == 0 || (kstrtou16(addr + it + 1, 10, &port) < 0))
  {
    printk(KERN_ERR "incorrect address or port\n");
    return ERR_PTR(-ENXIO);
  }

  ret = mount_nodev(type, flags, data, nfs_fill_super);

  if (IS_ERR(ret))
    return ret;

  ret->d_sb->s_fs_info = 0;

  struct server* serv = kmalloc(sizeof(struct server), GFP_KERNEL);
  *serv = (struct server){.ip = kmalloc(it + 1, GFP_KERNEL), .port = port};
  memcpy(serv->ip, addr, it);
  serv->ip[it] = '\0';

  ret->d_sb->s_fs_info = (void*)serv;

  printk(KERN_INFO "mounted\n");

  return ret;
}

void nfs_kill_sb(struct super_block* sb)
{
  struct server* serv = sb->s_fs_info;
  if (serv != 0)
    kfree(serv->ip);
  kfree(serv);
  printk(KERN_INFO "killed superblock\n");
}

int nfs_iterate(struct file* f, struct dir_context* ctxt)
{
  struct inode* inode = f->f_inode;
  struct Message* message = kmalloc(sizeof(struct Message), GFP_KERNEL);
  message->type = MESSAGE_TYPE_ITERATE;
  message->iterate = (struct IterateMessage){.inode = {inode->i_ino}};
  if (send_message(inode->i_sb->s_fs_info, message) < 0)
  {
    printk(KERN_ERR "iterate err\n");
    kfree(message);
    return -EHOSTUNREACH;
  }

  check_response_code(message, -ENOENT);

  uint32_t ind = 1;
  for (ind = ctxt->pos; ind < message->iterate.files_count; ++ind, ++ctxt->pos)
  {
    printk(KERN_INFO "%d %d\n", message->iterate.files[ind].inode.inode, message->iterate.files[ind].type == FILE_TYPE_DIR);
    dir_emit(ctxt, message->iterate.files[ind].name, strlen(message->iterate.files[ind].name),
             message->iterate.files[ind].inode.inode, message->iterate.files[ind].type == FILE_TYPE_DIR ? DT_DIR : DT_REG);
  }

  kfree(message);
  return ind;
}

ssize_t nfs_read(struct file* f, char* buffer, size_t len, loff_t* off)
{
  struct Message* message = kmalloc(sizeof(struct Message), GFP_KERNEL);
  message->type = MESSAGE_TYPE_READ;
  message->read = (struct ReadMessage){.inode = {f->f_inode->i_ino}, .offset=*off};

  if (send_message(f->f_inode->i_sb->s_fs_info, message) < 0)
  {
    printk(KERN_ERR "read err on inode: %lu\n", f->f_inode->i_ino);
    kfree(message);
    return 0;
  }

  check_response_code(message, 0);

  ssize_t ret = 0;
  while (ret < message->read.data.size)
  {
    put_user(message->read.data.data[ret], buffer + ret);
    ret++;
    (*off)++;
  }
  kfree(message);
  return ret;
}

ssize_t nfs_write(struct file* f, const char* buffer, size_t len, loff_t* off)
{
  struct Message* message = kmalloc(sizeof(struct Message), GFP_KERNEL);
  message->type = MESSAGE_TYPE_WRITE;
  message->write = (struct WriteMessage) {.inode={f->f_inode->i_ino}, .offset=*off};
  message->write.data.size = (len < BUFFER_SIZE ? len : BUFFER_SIZE);

  ssize_t ret = 0;
  while (ret < message->write.data.size)
  {
    get_user(message->write.data.data[*off], buffer + ret);
    ret++;
    (*off)++;
  }

  if (send_message(f->f_inode->i_sb->s_fs_info, message) < 0)
  {
    printk(KERN_ERR "write err on inode: %lu\n", f->f_inode->i_ino);
    kfree(message);
    return 0;
  }

  check_response_code(message, 0);

  kfree(message);
  return ret;
}

struct dentry* nfs_lookup(struct inode* parent_inode, struct dentry* child_dentry, unsigned int flag)
{
  struct Message* message = kmalloc(sizeof(struct Message), GFP_KERNEL);
  message->type = MESSAGE_TYPE_LOOKUP;
  message->lookup = (struct LookupMessage) {.parent_inode={parent_inode->i_ino}};
  strcpy(message->lookup.file.name, child_dentry->d_name.name);

  if (send_message(parent_inode->i_sb->s_fs_info, message) < 0)
  {
    printk(KERN_ERR "lookup err\n");
    kfree(message);
    return ERR_PTR(-ENOENT);
  }

//  check_response_code(message, ERR_PTR(-ENOENT));
  check_response_code(message, 0);

  printk(KERN_INFO "Got inode #%u\n", message->lookup.file.inode.inode);

  struct inode* inode = nfs_get_inode(parent_inode->i_sb, 0,
      (message->lookup.file.type == FILE_TYPE_DIR ? S_IFDIR : S_IFREG) | 0777,
      message->lookup.file.inode.inode);
  if (inode)
    d_add(child_dentry, inode);

  kfree(message);
  return child_dentry;
}

int nfs_create(struct mnt_idmap* mnt_idmap, struct inode* parent_inode, struct dentry* child_dentry, umode_t mode, bool b)
{
  printk(KERN_INFO "creating\n");
  struct Message* message = kmalloc(sizeof(struct Message), GFP_KERNEL);
  message->type = MESSAGE_TYPE_CREATE;
  message->create = (struct CreateMessage) {.parent_inode={parent_inode->i_ino}, .file={.type=FILE_TYPE_FILE}};
  strcpy(message->create.file.name, child_dentry->d_name.name);

  if (send_message(parent_inode->i_sb->s_fs_info, message) < 0)
  {
    printk(KERN_ERR "create err\n");
    kfree(message);
    return -EHOSTUNREACH;
  }

  check_response_code(message, -EHOSTUNREACH);

  struct inode* inode = nfs_get_inode(parent_inode->i_sb, 0, S_IFREG | 0777,
                                      message->create.file.inode.inode);
  if (inode)
    d_add(child_dentry, inode);

  kfree(message);
  return 0;
}

int nfs_mkdir(struct mnt_idmap* mnt_idmap, struct inode* parent_inode, struct dentry* child_dentry, umode_t mode)
{
  printk(KERN_INFO "creating\n");
  struct Message* message = kmalloc(sizeof(struct Message), GFP_KERNEL);
  message->type = MESSAGE_TYPE_CREATE;
  message->create = (struct CreateMessage) {.parent_inode={parent_inode->i_ino}, .file={.type=FILE_TYPE_DIR}};
  strcpy(message->create.file.name, child_dentry->d_name.name);

  if (send_message(parent_inode->i_sb->s_fs_info, message) < 0)
  {
    printk(KERN_ERR "mkdir err\n");
    kfree(message);
    return -EHOSTUNREACH;
  }

  check_response_code(message, -EHOSTUNREACH);

  struct inode* inode = nfs_get_inode(parent_inode->i_sb, 0, S_IFDIR | 0777,
                                      message->create.file.inode.inode);
  if (inode)
    d_add(child_dentry, inode);

  kfree(message);
  return 0;
}

int nfs_unlink(struct inode* parent_inode, struct dentry* child_dentry)
{
  struct Message* message = kmalloc(sizeof(struct Message), GFP_KERNEL);
  message->type = MESSAGE_TYPE_REMOVE;
  message->remove = (struct RemoveMessage) {.parent_inode={parent_inode->i_ino}, .file={.type=FILE_TYPE_FILE}};
  strcpy(message->remove.file.name, child_dentry->d_name.name);

  if (send_message(parent_inode->i_sb->s_fs_info, message) < 0)
  {
    printk(KERN_ERR "remove err\n");
    kfree(message);
    return -EHOSTUNREACH;
  }

  check_response_code(message, -ENOENT);

  kfree(message);
  return 0;
}

int nfs_rmdir(struct inode* parent_inode, struct dentry* child_dentry)
{
  struct Message* message = kmalloc(sizeof(struct Message), GFP_KERNEL);
  message->type = MESSAGE_TYPE_REMOVE;
  message->remove = (struct RemoveMessage) {.parent_inode={parent_inode->i_ino}, .file={.type=FILE_TYPE_DIR}};
  strcpy(message->remove.file.name, child_dentry->d_name.name);

  if (send_message(parent_inode->i_sb->s_fs_info, message) < 0)
  {
    printk(KERN_ERR "rmdir err\n");
    kfree(message);
    return -EHOSTUNREACH;
  }

  check_response_code(message, -ENOENT);

  kfree(message);
  return 0;
}

int nfs_link(struct dentry *old_dentry, struct inode *parent_inode, struct dentry *new_dentry)
{
  struct Message* message = kmalloc(sizeof(struct Message), GFP_KERNEL);
  message->type = MESSAGE_TYPE_LINK;
  message->link = (struct LinkMessage) {.source_inode={old_dentry->d_inode->i_ino}, .parent_inode={parent_inode->i_ino}};
  strcpy(message->link.file.name, new_dentry->d_name.name);

  if (send_message(parent_inode->i_sb->s_fs_info, message) < 0)
  {
    printk(KERN_ERR "link err\n");
    kfree(message);
    return -EHOSTUNREACH;
  }

  check_response_code(message, -ENOENT);

  kfree(message);
  return 0;
}


int send_message(struct server* server, struct Message* message) {
  printk(KERN_INFO "sending %d", message->type);

  int ret_code = 0;

  struct socket* socket;
  if (sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &socket) < 0)
    return -1;

  struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr = {.s_addr = in_aton(server->ip)},
                            .sin_port = htons(server->port)};

  if (kernel_connect(socket, (struct sockaddr*) &addr, sizeof(struct sockaddr_in), 0) < 0)
  {
    printk(KERN_ERR "can't connect to server\n");
    ret_code = -1;
    goto EXIT_ERR;
  }

  struct msghdr hdr;
  memset(&hdr, 0, sizeof(struct msghdr));

  struct kvec vec;
  vec.iov_base = message;
  vec.iov_len = sizeof(struct Message);

  if (kernel_sendmsg(socket, &hdr, &vec, 1, vec.iov_len) < 0)
  {
    printk(KERN_ERR "sendmsg error\n");
    ret_code = -2;
    goto EXIT;
  }

  memset(&hdr, 0, sizeof(struct msghdr));

  vec.iov_len = sizeof(struct Message);

  while (vec.iov_len > 0)
  {
    int recv = kernel_recvmsg(socket, &hdr, &vec, 1, vec.iov_len, 0);
    if (recv == 0)
      break;
    if (recv < 0)
    {
      printk(KERN_ERR "recvmsg err\n");
      ret_code = -3;
      goto EXIT;
    }
    vec.iov_base += recv;
    vec.iov_len -= recv;
  }

EXIT:
  kernel_sock_shutdown(socket, SHUT_RDWR);
EXIT_ERR:
  sock_release(socket);

  return ret_code;
}


