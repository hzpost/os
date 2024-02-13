#include "file.h"
#include "check.h"
#include "config.h"
#include "fat16.h"
#include "kheap.h"
#include "status.h"
#include "stdio.h"
#include "string.h"

struct filesystem *filesystems[MAX_FILESYSTEMS];
struct file_descriptor *file_descriptors[MAX_FILE_DESCRIPTORS];

static struct filesystem **fs_get_free_filesystem() {
    int i;
    for (i = 0; i < MAX_FILESYSTEMS; i++) {
        if (filesystems[i] == 0) {
            return &filesystems[i];
        }
    }

    return 0;
}

void fs_insert_filesystem(struct filesystem *filesystem) {
    struct filesystem **fs;
    fs = fs_get_free_filesystem();
    if (!fs) {
        PANIC("Problem inserting filesystem");
    }

    *fs = filesystem;
}

static void fs_static_load() {
    fs_insert_filesystem(fat16_init());
}

void fs_load() {
    fs_static_load();
}

void init_fs() {
    fs_load();
}

struct filesystem *fs_resolve(struct disk *disk) {
    struct filesystem *fs = 0;
    for (int i = 0; i < MAX_FILESYSTEMS; i++) {
        if (filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0) {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}

static int file_new_descriptor(struct file_descriptor **desc_out) {
    int res = -ENOMEM;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (file_descriptors[i] == 0) {
            struct file_descriptor *desc =
                kcalloc(sizeof(struct file_descriptor));
            // Descriptors start at 1
            desc->index = i + 1;
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }

    return res;
}

static struct file_descriptor *file_get_descriptor(int fd) {
    if (fd <= 0 || fd >= MAX_FILE_DESCRIPTORS) {
        return 0;
    }

    // Descriptors start at 1
    int index = fd - 1;
    return file_descriptors[index];
}

FILE_MODE file_get_mode_by_string(const char *str) {
    FILE_MODE mode = FILE_MODE_INVALID;
    if (strncmp(str, "r", 1) == 0) {
        mode = FILE_MODE_READ;
    } else if (strncmp(str, "w", 1) == 0) {
        mode = FILE_MODE_WRITE;
    } else if (strncmp(str, "a", 1) == 0) {
        mode = FILE_MODE_APPEND;
    }
    return mode;
}

int fopen(const char *filename, const char *mode_str) {
    int32_t res = 0;
    struct path_root *root_path = pathparser_parse(filename);
    if (!root_path) {
        res = -EINVARG;
        goto out;
    }

    // We cannot have just a root path 0:/ 0:/test.txt
    if (!root_path->first) {
        res = -EINVARG;
        goto out;
    }

    // Ensure the disk we are reading from exists
    struct disk *disk = disk_get(root_path->drive_no);
    if (!disk) {
        res = -EIO;
        goto out;
    }

    if (!disk->filesystem) {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode = file_get_mode_by_string(mode_str);
    if (mode == FILE_MODE_INVALID) {
        res = -EINVARG;
        goto out;
    }

    void *descriptor_private_data =
        disk->filesystem->open(disk, root_path->first, mode, &res);

    if (ISERR(res)) {
        res = ERROR_I(res);
        goto out;
    }

    struct file_descriptor *desc = 0;
    res = file_new_descriptor(&desc);
    if (res < 0) {
        goto out;
    }
    desc->filesystem = disk->filesystem;
    desc->private = descriptor_private_data;
    desc->disk = disk;
    res = desc->index;

out:
    // fopen shouldnt return negative values
    if (res < 0) {
        res = 0;
    }

    return res;
}

int fread(void *ptr, uint32_t size, uint32_t nmemb, int fd) {
    int res = 0;
    if (size == 0 || nmemb == 0 || fd < 1) {
        res = -EINVARG;
        goto out;
    }

    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EINVARG;
        goto out;
    }

    res = desc->filesystem->read(desc->disk, desc->private, size, nmemb,
                                 (char *)ptr);
out:
    return res;
}
