#ifndef OS_LAB4_SOURCES_LIBS_MESSAGE_H_
#define OS_LAB4_SOURCES_LIBS_MESSAGE_H_

#define BUFFER_SIZE 1024
#define FILENAME_SIZE 32
#define FILES_SIZE (BUFFER_SIZE / FILENAME_SIZE)

struct INode {
  uint32_t inode;
} __attribute__((packed));

struct Data {
  uint8_t data[BUFFER_SIZE];
  uint16_t size;
};

struct IterateMessage {
  struct INode inode;                         // in

  struct File {
    enum FILE_TYPE {
      FILE_TYPE_DIR = 0,
      FILE_TYPE_FILE = 1
    } __attribute__((packed)) type;

    struct INode inode;
    char name[FILENAME_SIZE];
  }__attribute__((packed)) files[FILES_SIZE]; // out

  uint16_t files_count;                       // out
} __attribute__((packed));

struct ReadMessage {
  struct INode inode; // in
  uint32_t offset;    // in
  struct Data data;   // out
} __attribute__((packed));

struct WriteMessage {
  struct INode inode; // in
  uint32_t offset;    // in
  struct Data data;   // in
} __attribute__((packed));

struct LookupMessage {
  struct INode parent_inode;  // in
  struct File file;           // in(filename) + out(type+name)
} __attribute__((packed));

struct CreateMessage {
  struct INode parent_inode;  // in
  struct File file;           // in(type+name) + out(inode)
} __attribute__((packed));

struct RemoveMessage {
  struct INode parent_inode;  // in
  struct File file;           // in(type+name) *inode unused*
} __attribute__((packed));

struct LinkMessage {
  struct INode source_inode;  // in
  struct INode parent_inode;  // in
  struct File file;           // in(name) *inode+type unused*
} __attribute__((packed));


struct Message {
  enum {
    MESSAGE_TYPE_ITERATE = 0,
    MESSAGE_TYPE_READ,
    MESSAGE_TYPE_WRITE,
    MESSAGE_TYPE_LOOKUP,
    MESSAGE_TYPE_CREATE,
    MESSAGE_TYPE_REMOVE,
    MESSAGE_TYPE_LINK,
  } __attribute__((packed)) type;

  union {
    struct IterateMessage iterate;
    struct ReadMessage read;
    struct WriteMessage write;
    struct LookupMessage lookup;
    struct CreateMessage create;
    struct RemoveMessage remove;
    struct LinkMessage link;
  } __attribute__((packed));

  enum {
    MESSAGE_RETURN_CODE_OK = 0,
    MESSAGE_RETURN_CODE_ERROR,
    MESSAGE_RETURN_CODE_EMPTY,
  } __attribute__((packed)) return_code;
} __attribute__((packed));


#endif //OS_LAB4_SOURCES_LIBS_MESSAGE_H_
