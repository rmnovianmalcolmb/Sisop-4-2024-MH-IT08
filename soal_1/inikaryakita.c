#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>

static const char *rootDir = "/Desktop/sisop/soal_1/portofolio";
static const char *galleryDir = "/Desktop/sisop/soal_1/portofolio/gallery";
static const char *bahayaDir = "/Desktop/sisop/soal_1/portofolio/bahaya";

static int ikk_getattr(const char *path, struct stat *stbuf) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("Getting attributes of: %s\n", fpath);  // Debug log
    int res = lstat(fpath, stbuf);
    return res == -1 ? -errno : 0;
}

static int ikk_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("Reading directory: %s\n", fpath);  // Debug log
    DIR *dp = opendir(fpath);
    if (!dp) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0)) break;
    }
    closedir(dp);
    return 0;
}

static int ikk_open(const char *path, struct fuse_file_info *fi) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("Opening file: %s\n", fpath);  // Debug log
    int res = open(fpath, fi->flags);
    if (res == -1) return -errno;
    close(res);
    return 0;
}

static int ikk_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("Reading file: %s\n", fpath);  // Debug log
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;
    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;
    close(fd);
    return res;
}

static int ikk_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("Writing to file: %s\n", fpath);  // Debug log

    int fd;
    int res;
    if (strstr(path, "/test") != NULL) {
        char *reversed_buf = strndup(buf, size);
        if (!reversed_buf) return -ENOMEM;
        for (size_t i = 0; i < size / 2; i++) {
            char tmp = reversed_buf[i];
            reversed_buf[i] = reversed_buf[size - i - 1];
            reversed_buf[size - i - 1] = tmp;
        }
        fd = open(fpath, O_WRONLY);
        if (fd == -1) {
            free(reversed_buf);
            return -errno;
        }
        res = pwrite(fd, reversed_buf, size, offset);
        free(reversed_buf);
    } else {
        fd = open(fpath, O_WRONLY);
        if (fd == -1) return -errno;
        res = pwrite(fd, buf, size, offset);
    }
    if (res == -1) res = -errno;
    close(fd);
    return res;
}

static int ikk_rename(const char *from, const char *to) {
    char ffrom[1000], fto[1000];
    snprintf(ffrom, sizeof(ffrom), "%s%s", rootDir, from);
    snprintf(fto, sizeof(fto), "%s%s", rootDir, to);
    printf("Renaming from: %s to: %s\n", ffrom, fto);  // Debug log

    int res = rename(ffrom, fto);
    if (res == -1) return -errno;

    if (strncmp(to, "/wm.", 4) == 0) {
        char command[2000];
        snprintf(command, sizeof(command), "convert %.900s -gravity SouthEast -pointsize 24 -draw \"text 0,0 'inikaryakita.id'\" %.900s", fto, fto);
        printf("Executing command: %s\n", command);  // Debug log
        res = system(command);
        if (res == -1) return -errno;
    }

    return 0;
}

static int ikk_chmod(const char *path, mode_t mode) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("Changing mode of: %s\n", fpath);  // Debug log

    if (strstr(path, "/bahaya/script.sh") != NULL) {
        mode = 0700; // Make executable
    }

    int res = chmod(fpath, mode);
    return res == -1 ? -errno : 0;
}

static struct fuse_operations ikk_oper = {
    .getattr = ikk_getattr,
    .readdir = ikk_readdir,
    .open    = ikk_open,
    .read    = ikk_read,
    .write   = ikk_write,
    .rename  = ikk_rename,
    .chmod   = ikk_chmod,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &ikk_oper, NULL);
}
