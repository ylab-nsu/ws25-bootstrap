// Data types
typedef char file_name_t[1024];

typedef short int16_t;
typedef int int32_t;

typedef char* ptr_t;

typedef struct {
    char unused[4];
    ptr_t file_addr;
    int32_t length;
    char unused2[4];
} file_descriptor_t;

typedef struct {
    int32_t logical_sector;
    int16_t offset;
    char unused[8];
} stdin_locator_t;

typedef struct { int32_t global_file_index; int32_t current_offset; } process_descriptor_files_t;

typedef struct {
    ptr_t                       process_address;
    ptr_t                       brk;
    ptr_t                       saved_stack_pointer_first;
    ptr_t                       saved_stack_pointer;
    int32_t                     forked;
    ptr_t                       saved_brk;
    int32_t                     child_exit_code;
    ptr_t                       saved_process_memory;
    int32_t                     process_memory_length;
    ptr_t                       saved_process_stack;
    int32_t                     process_stack_length;
    char                        unused[212];
    char                        current_directory[0x100];
    process_descriptor_files_t  open_files[448];
} process_descriptor_t;


typedef struct {
    unsigned char   e_ident[16];
    int16_t         e_type;
    int16_t         e_machine;
    int32_t         e_version;
    ptr_t           e_entry;
    int32_t         e_phoff;
    int32_t         e_shoff;
    int32_t         e_flags;
    int16_t         e_ehsize;
    int16_t         e_phentsize;
    int16_t         e_phnum;
    int16_t         e_shentsize;
    int16_t         e_shnum;
    int16_t         e_shstrndx;
} elf32_elf_hdr_t;

typedef struct {
    int32_t     p_type;
    int32_t     p_offset;
    ptr_t       p_vaddr;
    ptr_t       p_paddr;
    int32_t     p_filesz;
    int32_t     p_memsz;
    int32_t     p_flags;
    int32_t     p_align;
} elf32_prg_hdr_t;

// Memory

const ptr_t stack_bottom = (ptr_t) 0x08000000;

int32_t next_filenum = 4;

ptr_t next_file_address = (ptr_t) 0x54000000;

file_name_t *file_names = (file_name_t*) 0x200000;

int32_t next_process_num = 1;

file_descriptor_t *file_descriptors = (file_descriptor_t*) 0x01000000;

process_descriptor_t *pproc_descriptors = (process_descriptor_t*) 0x20000;

const ptr_t scratch_buffer = (ptr_t) 0x50000;

ptr_t next_save_process_address = (ptr_t) 0x30000000;

const ptr_t current_running_process = (ptr_t) 0x08048000;
const int32_t current_running_process_size = 0x26FB8000;

// Functions

void console_putc(char value);  //TODO: implement

void read_sectors(ptr_t dest_addr, int32_t first_sec, int32_t num_secs);    //TODO: implement

void write_sectors(ptr_t source_addr, int32_t first_sec, int32_t num_secs);    //TODO: implement

void reboot();      //TODO: implement

void setup_int_handlers();      //TODO:

#define SAVE_REGISTERS
#define RESTORE_REGISTERS

char strcmp(ptr_t a, ptr_t b) {
    while (1) {
        char ac = *(a++);
        char bc = *(b++);
        if (ac == bc) {
            if (ac == 0) return 0;
        } else if (ac < bc) {
            return -1;
        } else {
            return 1;
        }
    }
}

ptr_t stpcpy(ptr_t dest, ptr_t src) {
    while (*src) {
        *(dest++) = *(src++);
    }
    *dest = 0;
    return dest;
}

void memcpy(ptr_t dest, ptr_t src, int32_t len) {
    while (len--) {
        *(dest++) = *(src++);
    }
}

void memset(ptr_t dest, unsigned char value, int32_t len) {
    while (len--) {
        *(dest++) = value;
    }
}

void console_puts(ptr_t data) {
    while (*data) {
        console_putc(*data);
        data++;
    }
    console_putc('\n');
}

void console_put_hex(unsigned char value) {
    unsigned char v0 = (value >> 4) & 0xf;
    unsigned char v1 = value & 0xf;

    if (v0 >= 0xA) {
        console_putc(v0 - 0xA + 'a');
    } else {
        console_putc(v0 + '0');
    }
    if (v1 >= 0xA) {
        console_putc(v1 - 0xA + 'a');
    } else {
        console_putc(v1 + '0');
    }
}

void stub_interrupt_handler() {
    return;
}

ptr_t absolute_path(ptr_t path) {
    ptr_t buf = scratch_buffer;
    if (path[0] != '/') {
        int32_t proc_num = next_process_num - 1;
        ptr_t curdir = pproc_descriptors[proc_num].current_directory;
        buf = stpcpy(buf, curdir);
    }
    while (path[0] == '.' && path[1] == '/') {
        path += 2;
    }
    char prev = 0;
    while (*path) {
        if (*path != '/' || prev != '/') *buf = *path;
        prev = *path;
        buf++;
        path++;
    }
    if (prev == '/') {
        buf[-1] = 0;
    }

    ptr_t parent = buf = scratch_buffer;
    while (*buf) {
        if (buf[0] == '/') {
            if (buf[1] == '.' && buf[2] == '.') {
                stpcpy(parent, buf + 3);
                parent = buf = scratch_buffer;
                continue;
            } else if (buf[1] == '.') {
                stpcpy(buf, buf + 2);
                parent = buf = scratch_buffer;
                continue;
            } else {
                parent = buf;
            }
        }
        buf++;
    }

    buf = scratch_buffer;
    if (*buf == 0) {
        buf[0] = '/';
        buf[1] = 0;
    }

    return buf;
}

typedef enum {
    EXIT = 0x01,
    FORK = 0x02,
    READ = 0x03,
    WRITE = 0x04,
    OPEN = 0x05,
    CLOSE = 0x06,
    WAITPID = 0x07,
    EXECVE = 0x0B,
    LSEEK = 0x13,
    BRK = 0x2D,
    CHDIR = 0x0C,
    ACCESS = 0x21,
    MKDIR = 0x27,
    WAIT4 = 0x72,
    GETCWD = 0xB7
} syscall_t;

const int32_t O_CREAT = 0x40;
const int32_t O_WRONLY = 0x01;

int32_t find_file(ptr_t filename) {
    int32_t filenum = next_filenum - 1;
    while (filenum > 2) {
        if (strcmp(file_names[filenum], filename) == 0) {
            return filenum;
        }
        filenum--;
    }
    return -1;      //NOT FOUND
}

int32_t handle_syscall_open(ptr_t filename, int32_t flags) {
    filename = absolute_path(filename);
    int32_t filenum;
    if (flags & O_CREAT) {
        ptr_t filename_end = filename;
        while (*filename_end) {
            filename_end++;
        }
        while (*filename_end != '/') {
            filename_end--;
        }
        if (filename != filename_end) {
            *filename_end = 0;
            int32_t dirnum = find_file(filename);
            *filename_end = '/';
            if (dirnum == -1) { 
                return -1;
            }
        }
        memcpy(file_names[next_filenum], filename, sizeof(file_name_t));
        file_descriptor_t *pfile_descriptor = &file_descriptors[next_filenum];
        pfile_descriptor->file_addr = next_file_address;
        pfile_descriptor->length = 0;

        filenum = next_filenum++;
    } else {
        filenum = find_file(filename);
        if (filenum == -1) return -1;
    }

    int32_t proc_num = next_process_num - 1;
    process_descriptor_files_t* pproc_descriptor_file;
    for (int32_t fd = 4;;fd++) {
        process_descriptor_files_t *pproc_descriptor_file = &pproc_descriptors[proc_num].open_files[fd];
        if (pproc_descriptor_file->global_file_index == 0) {
            pproc_descriptor_file->global_file_index = filenum;
            pproc_descriptor_file->current_offset = 0;
            return fd;
        }
    }
}

int32_t handle_syscall_close(int32_t fd) {
    int32_t proc_num = next_process_num - 1;
    process_descriptor_files_t *pproc_descriptor_file = &pproc_descriptors[proc_num].open_files[fd];
    pproc_descriptor_file->global_file_index = 0;
    return 0;
}

int32_t fd_to_file_index(int32_t fd) {
    int32_t proc_num = next_process_num - 1;
    process_descriptor_files_t *pproc_descriptor_file = &pproc_descriptors[proc_num].open_files[fd];
    return pproc_descriptor_file->global_file_index;
}

int32_t handle_syscall_read(ptr_t target, int32_t fd, int32_t length) {
    if (length == 0) return 0;

    if (fd == 0) {
        // stdin_locator_t *fin = (stdin_locator_t*) &file_descriptors[0];
        // if (fin->offset == 0x7fff) {

        // }
        //TODO: implement
        return 0;
    } else {
        int32_t filenum = fd_to_file_index(fd);
        file_descriptor_t* pfile_descriptor = &file_descriptors[filenum];

        int32_t proc_num = next_process_num - 1;
        process_descriptor_files_t *pproc_descriptor_file = &pproc_descriptors[proc_num].open_files[fd];

        int32_t readed = pfile_descriptor->length - pproc_descriptor_file->current_offset;
        readed = readed < length ? readed : length;

        ptr_t frm = pfile_descriptor->file_addr + pproc_descriptor_file->current_offset;
        pproc_descriptor_file->current_offset += readed;
        memcpy(target, frm, readed);
        return readed;
    }
}

ptr_t handle_syscall_brk(ptr_t val) {
    int32_t proc_num = next_process_num - 1;
    process_descriptor_t* pproc = &pproc_descriptors[proc_num];
    if (val == 0) {
        return pproc->brk;
    }

    int32_t len = val - pproc->brk;
    memset(pproc->brk, 0, len);
    return pproc->brk = val;
}

int32_t handle_syscall_write(int32_t fd, ptr_t src, int32_t length) {
    if (fd <= 2) {
        int32_t written = length;
        while (length--) {
            console_putc(*(src++));
        }
        return written;
    } else {
        int32_t filenum = fd_to_file_index(fd);
        file_descriptor_t* pfile_descriptor = &file_descriptors[filenum];

        int32_t proc_num = next_process_num - 1;
        process_descriptor_files_t *pproc_descriptor_file = &pproc_descriptors[proc_num].open_files[fd];

        if (pfile_descriptor->length < pproc_descriptor_file->current_offset + length) {
            int32_t diff = pproc_descriptor_file->current_offset + length - pfile_descriptor->length;
            pfile_descriptor->length += diff;
            next_file_address += diff;
        }

        ptr_t to = pfile_descriptor->file_addr + pproc_descriptor_file->current_offset;
        pproc_descriptor_file->current_offset += length;
        memcpy(to, src, length);   // TODO: parallel write support (wtf?)
        return length;
    }
}

//TODO: reread and validate
int32_t handle_syscall_fork() {
    ptr_t esp;
    // :TODO: save registers on stack
    // :TODO: save SP to `esp`

    int32_t proc_num = next_process_num - 1;
    process_descriptor_t *pproc_descriptor = &pproc_descriptors[proc_num];

    pproc_descriptor->saved_brk = pproc_descriptor->brk;
    pproc_descriptor->saved_stack_pointer = esp;
    pproc_descriptor->forked = 1;

    int32_t stack_size = stack_bottom - esp;
    pproc_descriptor->saved_process_stack = next_save_process_address;
    pproc_descriptor->process_stack_length = stack_size;
    memcpy(next_save_process_address, esp, stack_size);
    next_save_process_address += stack_size;

    int32_t mem_size = pproc_descriptor->brk - pproc_descriptor->process_address;
    pproc_descriptor->saved_process_memory = next_save_process_address;
    pproc_descriptor->process_memory_length = mem_size;
    memcpy(next_save_process_address, pproc_descriptor->process_address, mem_size);
    next_save_process_address += mem_size;

    return 0;
}

void handle_syscall_execve(ptr_t program_name, ptr_t *argv, ptr_t *env) {
    int32_t proc_num = next_process_num - 1;
    if (proc_num == 0) {
        process_descriptor_t *pproc_descriptor = &pproc_descriptors[1];
        //:TODO: save esp to `pproc_descriptor->saved_stack_pointer_first`
    }

    ptr_t save_process_address = next_save_process_address;

    // :TODO: push `0` to stack
    while (*env) {
        // push `save_process_address` to stack
        memcpy(save_process_address, *env, 0x100);
        save_process_address += 0x100;
        env++;
    }

    // :TODO: push `0` to stack
    int32_t argc = 0;
    while (*argv) {
        argc++;
        argv++;
    }

    int32_t argc2 = argc;
    
    while (argc2--) {
        // :TODO: push `save_process_address` to stack
        argv--;
        memcpy(save_process_address, *argv, 0x100);
        save_process_address += 0x100;
    }

    // :TODO: push `argc` to stack

    next_save_process_address = save_process_address;

    stpcpy(pproc_descriptors[proc_num + 1].current_directory, pproc_descriptors[proc_num].current_directory);

    program_name = absolute_path(program_name);
    int32_t filenum = find_file(program_name);

    memset(current_running_process, 0, current_running_process_size);

    file_descriptor_t *pfile_descriptor = &pfile_descriptor[filenum];
    elf32_elf_hdr_t *elfhdr = (elf32_elf_hdr_t*) pfile_descriptor->file_addr;

    elf32_prg_hdr_t *prghdr = (elf32_prg_hdr_t*) (pfile_descriptor->file_addr + elfhdr->e_phoff);
    int32_t prg_hdr_cnt = elfhdr->e_phnum;

    ptr_t last_seg;
    while (prg_hdr_cnt--) {
        memcpy(prghdr->p_vaddr, pfile_descriptor->file_addr + prghdr->p_offset, prghdr->p_filesz);
        last_seg = prghdr->p_vaddr + prghdr->p_filesz;
        prghdr++;
    }

    if (proc_num == 0 || pproc_descriptors[proc_num].forked) {
        proc_num++;
        next_process_num++;
        pproc_descriptors[proc_num].forked = 0;
    }

    process_descriptor_t *pproc_descriptor = &pproc_descriptors[proc_num];

    prghdr = (elf32_prg_hdr_t*) (pfile_descriptor->file_addr + elfhdr->e_phoff);
    pproc_descriptor->process_address = prghdr->p_vaddr;
    pproc_descriptor->brk = last_seg + 0x20000;
    pproc_descriptor->forked = 0;
    memset((char*) pproc_descriptor->open_files, 0, 0xE00);


    // :TODO: null registers
    // :TODO: push `elfhdr->e_entry`
    return;         // jumps to `elfhdr->e_entry`
}

int32_t handle_syscall_chdir(ptr_t dir) {
    dir = absolute_path(dir);
    int32_t filenum = find_file(dir);
    if (filenum == -1) return -1;
    int32_t len = file_descriptors[filenum].length;
    if (len != 0) return -1;        //check if not file

    int32_t proc_num = next_process_num - 1;
    ptr_t dest = pproc_descriptors[proc_num].current_directory;

    // handle root (to avoid '//')
    if (dir[0] == '/' && dir[1] == 0) {
        dest[0] = '/';
        dest[1] = 0;
        return 0;
    }

    ptr_t tmp = stpcpy(dest, dir);
    tmp[0] = '/';
    tmp[1] = 0;
    return 0;
}

//TODO: reread and validate
int32_t handle_syscall_exit(int32_t exitcode) {
    next_process_num--;
    int32_t proc_num = next_process_num - 1;
    
    if (proc_num == 0) {
        // :TODO: restore SP from `pproc_descriptors[1].saved_stack_pointer_first`
        // :TODO: restore registers from stack
        return 0;
    }

    process_descriptor_t *pproc_descriptor = &pproc_descriptors[proc_num];
    pproc_descriptor->child_exit_code = exitcode;
    memcpy(pproc_descriptor->process_address, pproc_descriptor->saved_process_memory, pproc_descriptor->process_memory_length);
    next_save_process_address = pproc_descriptor->saved_process_stack;
    // :TODO: restore SP from `pproc_descriptor->saved_stack_pointer`
    // :TODO: restore registers
    pproc_descriptor->brk = pproc_descriptor->saved_brk;
    memcpy(next_save_process_address, pproc_descriptor->saved_process_stack, pproc_descriptor->process_stack_length);
    return 1;
}

int32_t handle_syscall_waitpid(int32_t pid, int32_t *status, int32_t options) {
    int32_t proc_num = next_process_num - 1;
    process_descriptor_t *pproc_descriptor = &pproc_descriptors[proc_num];
    *status = pproc_descriptor->child_exit_code << 8;
    return 0;
}

const int32_t SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2;

int32_t handle_syscall_lseek(int32_t fd, int32_t offset, int32_t method) {
    int32_t proc_num = next_process_num - 1;
    process_descriptor_files_t *pproc_descriptor_file = &pproc_descriptors[proc_num].open_files[fd];
    if (method == SEEK_CUR) {
        return pproc_descriptor_file->current_offset += offset;
    } else if (method == SEEK_SET) {
        return pproc_descriptor_file->current_offset = offset;
    } else if (method == SEEK_END) {
        int32_t filenum = fd_to_file_index(fd);
        file_descriptor_t* pfile_descriptor = &file_descriptors[filenum];
        return pproc_descriptor_file->current_offset = pfile_descriptor->length + offset;
    } else return -1;
}

int32_t handle_syscall_access(ptr_t path, int32_t mode) {
    path = absolute_path(path);
    int32_t filenum = find_file(path);
    if (filenum == -1) return -1;
    return 0;
}

int32_t handle_syscall_mkdir(ptr_t path, int32_t mode) {
    int32_t res = handle_syscall_open(path, O_CREAT | O_WRONLY);
    if (res == -1) return -1;
    return 0;
}

ptr_t handle_syscall_getcwd(ptr_t buf, int32_t length) {
    int32_t proc_num = next_process_num - 1;
    ptr_t path = pproc_descriptors[proc_num].current_directory;
    if (path[0] == '/' && path[1] == 0) {
        buf[0] = '/';
        buf[1] = 0;
        return buf;
    }
    ptr_t tmp = stpcpy(buf, path);
    tmp[-1] = 0;
    return buf;
}

int32_t handle_syscall_wait4(int32_t pid, ptr_t wstatus, int32_t options, ptr_t rusage) {
    *wstatus = 0;
    return 0;
}