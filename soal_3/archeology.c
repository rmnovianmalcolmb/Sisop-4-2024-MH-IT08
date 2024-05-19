#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

static const char *root_path = "/home/ubuntu/sisop4soal3/relics";

static int xmp_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        char fpath[1000];
        snprintf(fpath, sizeof(fpath), "%s%s", root_path, path);
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 0;

        int i = 0;
        char part_path[1100];
        FILE *fp;

        while (1) {
            snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, i++);
            fp = fopen(part_path, "rb");
            if (!fp) break;

            fseek(fp, 0L, SEEK_END);
            stbuf->st_size += ftell(fp);
            fclose(fp);
        }

        if (i == 1) return -ENOENT;
    }
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR *dp;
    struct dirent *de;
    dp = opendir(root_path);
    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        if (strstr(de->d_name, ".000") != NULL) {
            char base_name[256];
            strncpy(base_name, de->d_name, strlen(de->d_name) - 4);
            base_name[strlen(de->d_name) - 4] = '\0';
            filler(buf, base_name, NULL, 0);
        }
    }
    closedir(dp);
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    size_t len;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", root_path, path);

    int i = 0;
    char part_path[1100];
    size_t read_size = 0;

    while (size > 0) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, i++);
        FILE *fp = fopen(part_path, "rb");
        if (!fp) break;

        fseek(fp, 0L, SEEK_END);
        size_t part_size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        if (offset >= part_size) {
            offset -= part_size;
            fclose(fp);
            continue;
        }

        fseek(fp, offset, SEEK_SET);
        len = fread(buf, 1, size, fp);
        fclose(fp);

        buf += len;
        size -= len;
        read_size += len;
        offset = 0;
    }
    return read_size;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", root_path, path);

    int part_num = offset / 10000;
    size_t part_offset = offset % 10000;
    size_t written_size = 0;
    char part_path[1100];

    while (size > 0) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, part_num++);
        FILE *fp = fopen(part_path, "r+b");
        if (!fp) {
            fp = fopen(part_path, "wb");
            if (!fp) return -errno;
        }

        fseek(fp, part_offset, SEEK_SET);
        size_t write_size = size > (10000 - part_offset) ? (10000 - part_offset) : size;
        fwrite(buf, 1, write_size, fp);
        fclose(fp);

        buf += write_size;
        size -= write_size;
        written_size += write_size;
        part_offset = 0;
    }
    return written_size;
}

static int xmp_unlink(const char *path) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", root_path, path);

    int part_num = 0;
    char part_path[1100];
    int res = 0;

    while (1) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, part_num++);
        res = unlink(part_path);
        if (res == -1 && errno == ENOENT) break;
        else if (res == -1) return -errno;
    }
    return 0;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s.000", root_path, path);

    int res = creat(fpath, mode);
    if (res == -1) return -errno;

    close(res);
    return 0;
}

static int xmp_truncate(const char *path, off_t size) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", root_path, path);

    int part_num = 0;
    char part_path[1100];
    off_t remaining_size = size;

    while (remaining_size > 0) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, part_num++);
        size_t part_size = remaining_size > 10000 ? 10000 : remaining_size;
        int res = truncate(part_path, part_size);
        if (res == -1) return -errno;
        remaining_size -= part_size;
    }

    while (1) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, part_num++);
        int res = unlink(part_path);
        if (res == -1 && errno == ENOENT) break;
        else if (res == -1) return -errno;
    }

    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
    .unlink = xmp_unlink,
    .create = xmp_create,
    .truncate = xmp_truncate,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
