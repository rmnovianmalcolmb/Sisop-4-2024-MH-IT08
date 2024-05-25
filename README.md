
# LAPORAN RESMI SOAL SHIFT MODUL 1 SISTEM OPERASI 2024
## ANGGOTA KELOMPOK IT08

1. Naufal Syafi' Hakim          (5027231022)
2. RM. Novian Malcolm Bayuputra (5027231035)
3. Abid Ubaidillah Adam         (5027231089)

## soal 2
program diminta untuk membuat folder yang dikirimkan oleh CEO inikaryakita menjadi lebih canggih dengan ketentuan folder `pesan` harus men-decode semua file yang memiliki prefix base64, rev, rot13, dan hex. Dan pada folder rahasia-berkas harus menggunakan kunci akses yaitu password yang dibuat oleh kode kita agar bisa mengakses folder tersebut. Berikut merupakan kode penyelesaiannya.

### Fungsi Headerfile

```bash
#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
// Define password dan nama logfile
#define LOG_FILE "logs-fuse.log"
#define PASSWORD "kopi_luwak" 

static char *root_path = NULL;
static int access_granted = 0;
```
Kode diatas adalah header file dari kode ini yang tentu menggunakan fuse

### Fungsi log-fuse.log
```bash
void log_operation(const char *status, const char *tag, const char *info) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log == NULL) {
        perror("Failed to open log file");
        return;
    }
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%s]::%02d/%02d/%04d-%02d:%02d:%02d::[%s]::[%s]\n",
            status, t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, 
            t->tm_hour, t->tm_min, t->tm_sec, tag, info);
    fclose(log);
}
```
Fungsi diatas ialah untuk mencatat semua yang kita lakukan pada folder sensitif.

### Fungsi decode
```bash
char* decode_content(const char *path, const char *content, size_t size) {
    const char *filename = strrchr(path, '/') + 1;
    char *decoded = NULL;

    if (strncmp(filename, "notes-", 6) == 0) {
        decoded = (char*)malloc(((size + 3) / 4) * 3 + 1);
        size_t len = 0;
        for (size_t i = 0; i < size; i += 4) {
            char b64[5] = { 0 };
            size_t bytesToCopy = (size - i >= 4) ? 4 : size - i;
            memcpy(b64, content + i, bytesToCopy);
            for (int j = 0; j < 4 && b64[j] != 0; j++) {
                b64[j] = strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/", b64[j]) - "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            }
            decoded[len++] = (b64[0] << 2) | (b64[1] >> 4);
            if (len < ((size + 3) / 4) * 3) decoded[len++] = (b64[1] << 4) | (b64[2] >> 2);
            if (len < ((size + 3) / 4) * 3) decoded[len++] = (b64[2] << 6) | b64[3];
        }
        decoded[len] = '\0';
    } else if (strncmp(filename, "enkripsi_", 9) == 0) {
        decoded = strdup(content);
        for (size_t i = 0; i < size; i++) {
            if ((content[i] >= 'A' && content[i] <= 'Z') || (content[i] >= 'a' && content[i] <= 'z')) {
                if ((content[i] >= 'A' && content[i] <= 'M') || (content[i] >= 'a' && content[i] <= 'm')) {
                    decoded[i] = content[i] + 13;
                } else {
                    decoded[i] = content[i] - 13;
                }
            }
        }
    } else if (strncmp(filename, "new-", 4) == 0) {
        decoded = (char*)malloc((size / 2) + 1);
        size_t len = 0;
        for (size_t i = 0; i < size; i += 2) {
            char hex[3] = { content[i], (i + 1 < size) ? content[i + 1] : '0', '\0' };
            decoded[len++] = (char)strtol(hex, NULL, 16);
        }
        decoded[len] = '\0';
    } else if (strncmp(filename, "rev-", 4) == 0) {
        decoded = (char*)malloc(size + 1);
        size_t j = 0;
        for (ssize_t i = size - 1; i >= 0; i--) {
            decoded[j++] = content[i];
        }
        decoded[size] = '\0';
    } else {
        decoded = strdup(content);
    }

    return decoded;
}
```
Fungsi diatas untuk decode pada folder pesan yang didalamnya terdapat file dengan prefix base64,rot13,rev, dan hex.

### Fungsi passwd
```bash
int check_password() {
    char password[256];
    printf("Enter password to access this folder: ");
    if (fgets(password, sizeof(password), stdin) == NULL) {
        return 0;
    }
    // hapus newline dari password
    password[strcspn(password, "\n")] = '\0';
    if (strcmp(password, PASSWORD) == 0) {
        return 1;
    }
    return 0;
}
```
Fungsi diatas untuk mengecek password yang dimasukan apakah sudah benar atau tidak ketika membuka folder rahasia berkas

### Fuse Function
```bash
static int do_getattr(const char *path, struct stat *st) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    memset(st, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        if (stat(full_path, st) == -1) {
            return -errno;
        }
    }
    return 0;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    if (strstr(path, "rahasia-berkas") != NULL && !access_granted) {
        if (!check_password()) {
            log_operation("FAILED", "access", full_path);
            return -EACCES;
        }
        access_granted = 1;
        log_operation("SUCCESS", "access", full_path);
    }

    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    dp = opendir(full_path);
    if (dp == NULL) {
        return -errno;
    }

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0)) {
            break;
        }
    }

    closedir(dp);
    return 0;
}

static int do_open(const char *path, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    if (strstr(path, "rahasia-berkas") != NULL && !access_granted) {
        if (!check_password()) {
            log_operation("FAILED", "access", full_path);
            return -EACCES;
        }
        access_granted = 1;
        log_operation("SUCCESS", "access", full_path);
    }

    int res = open(full_path, fi->flags);
    if (res == -1) {
        return -errno;
    }
    close(res);
    return 0;
}

static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    int fd = open(full_path, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }

    char *file_content = (char*)malloc(size);
    int res = pread(fd, file_content, size, offset);
    if (res == -1) {
        res = -errno;
    }

    close(fd);

    if (res > 0) {
        char *decoded = decode_content(path, file_content, res);
        size_t decoded_len = strlen(decoded);
        memcpy(buf, decoded, decoded_len);
        free(decoded);
        res = decoded_len;
    }

    free(file_content);
    log_operation("SUCCESS", "read", full_path);
    return res;
}

static struct fuse_operations operations = {
    .getattr = do_getattr,
    .readdir = do_readdir,
    .open = do_open,
    .read = do_read,
};
```
Fungsi diatas ialah fungsi Fuse yang dapat kita panggil untuk mengerjakan soal

### Fungsi main
```bash
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <root_directory> <mount_point>\n", argv[0]);
        return 1;
    }

    root_path = realpath(argv[1], NULL);
    if (root_path == NULL) {
        perror("Error resolving root path");
        return 1;
    }

    log_operation("INFO", "mount", root_path);

    return fuse_main(argc - 1, argv + 1, &operations, NULL);
}
```
Fungsi main untuk menerima argumen dari si user

### cara kerja
Gunakan -f agar fuse berjalan foreground ketika menjalankan kode ini 
`./pastibisa /path/to/sensitif /path/to/mountfile -f`
