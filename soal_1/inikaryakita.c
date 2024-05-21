
#define FUSE_USE_VERSION 30

#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fuse.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>


static const char *rootDir = "/path/to/IniKaryaKita";


static int fungsi_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) 
{
    DIR *dp;
    struct dirent *de;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("readdir: %s\n", fpath);  // Debug log
    dp = opendir(fpath);
    if (dp == NULL) return -errno;
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



static int fungsi_getattr(const char *path, struct stat *stbuf) 
{
    int res;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("getattr: %s\n", fpath);  // Debug log
    res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}



static int fungsi_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
    int fd;
    int res;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("read: %s\n", fpath);  // Debug log
    fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;
    res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;
    close(fd);
    return res;
}

static int fungsi_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("write: %s\n", fpath);  // Debug log

    if (strstr(path, "/test") != NULL) {
        // Reverse the content
        char *reversed_buf = strndup(buf, size);
        for (size_t i = 0; i < size / 2; i++) {
            char tmp = reversed_buf[i];
            reversed_buf[i] = reversed_buf[size - i - 1];
            reversed_buf[size - i - 1] = tmp;
        }
        fd = open(fpath, O_WRONLY);
        if (fd == -1) return -errno;
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

static int fungsi_open(const char *path, struct fuse_file_info *fi) 
{
    int res;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("open: %s\n", fpath);  // Debug log
    res = open(fpath, fi->flags);
    if (res == -1) return -errno;
    close(res);
    return 0;
}






static int fungsi_chmod(const char *path, mode_t mode) 
{
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", rootDir, path);
    printf("chmod: %s\n", fpath);  // Debug log

    if (strstr(path, "/bahaya/script.sh") != NULL) {
        mode = 0700; // Change permission to executable
    }

    int res = chmod(fpath, mode);
    if (res == -1) return -errno;
    return 0;
}


static int fungsi_rename(const char *from, const char *to) 
{
    char ffrom[1000], fto[1000];
    snprintf(ffrom, sizeof(ffrom), "%s%s", rootDir, from);
    snprintf(fto, sizeof(fto), "%s%s", rootDir, to);
    printf("rename from: %s to: %s\n", ffrom, fto);  // Debug log

    // Handle watermarking
    if (strncmp(to, "/wm.", 4) == 0) {
        int res = rename(ffrom, fto);
        if (res == -1) return -errno;

        // Add watermark
        char command[2000];
        snprintf(command, sizeof(command), "convert %.900s -gravity SouthEast -pointsize 24 -draw \"text 0,0 'inikaryakita.id'\" %.900s", fto, fto);
        printf("command: %s\n", command);  // Debug log
        res = system(command);
        if (res == -1) return -errno;
    } else {
        int res = rename(ffrom, fto);
        if (res == -1) return -errno;
    }

    return 0;
}



static struct fuse_operations fungsi_oper = 
{
    .readdir    = fungsi_readdir,
    .getattr    = fungsi_getattr,
    .read       = fungsi_read,
    .write      = fungsi_write,
    .open       = fungsi_open,
    .chmod      = fungsi_chmod,
    .rename     = fungsi_rename,
   
};



int main(int argc, char *argv[]) 
{

    return fuse_main(argc, argv, &fungsi_oper, NULL);
    
    
}
