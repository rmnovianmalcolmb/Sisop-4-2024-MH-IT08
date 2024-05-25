
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

## SOAL NOMOR 3

### archeology.c

**1. Membuat deklarasi untuk path direktori yang menjadi sumber dari direktori mount**
```c
static const char *root_path = "/home/ubuntu/sisop4soal3/relics";
```

**2. `xmp_getattr` untuk mendapatkan atribut dari file atau direktori**
```c
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
```

**3. `xmp_readdir` untuk membaca isi direktori**
```c
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
```

**4. `xmp_open` untuk membuka file**
```c
static int xmp_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}
```

**5. `xmp_read` untuk membaca isi dari file**
```c
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
```

**6. `xmp_write` untuk menulis data ke dalam file dan juga berperan dalam pemecahan file menjadi maksimal 10kb per file**
```c
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
```

**7. `xmp_unlink` untuk menghapus file beserta pecahannya di direktori root**
```c
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
```

**8. `xmp_create` untuk membuat file baru**
```c
static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s.000", root_path, path);

    int res = creat(fpath, mode);
    if (res == -1) return -errno;

    close(res);
    return 0;
}
```

**9. `xmp_truncate` untuk memecah file dengan command truncate**
```c
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
```

**10. `xmp_oper` untuk menyimpan semua fungsi di atas agar bisa dijalankan**
```c
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
```

**11. `main` untuk menjalankan fuse_main dengan operasi yang telah didefinisikan dalam xmp_oper**
```c
int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
```

### smb.conf
```
#======================= Global Settings =======================

[global]

## Browsing/Identification ###

   workgroup = WORKGROUP

   server string = %h server (Samba, Ubuntu)


#### Networking ####

;   interfaces = 127.0.0.0/8 eth0

;   bind interfaces only = yes


#### Debugging/Accounting ####

   log file = /var/log/samba/log.%m

   max log size = 1000

   logging = file

   panic action = /usr/share/samba/panic-action %d


####### Authentication #######

   server role = standalone server

   obey pam restrictions = yes

   unix password sync = yes

   passwd program = /usr/bin/passwd %u
   passwd chat = *Enter\snew\s*\spassword:* %n\n *Retype\snew\s*\spassword:* %n\n *password\supdated\ssuccessfully* .

   pam password change = yes

   map to guest = bad user

########## Domains ###########

;   logon path = \\%N\profiles\%U

;   logon drive = H:

;   logon script = logon.cmd

; add user script = /usr/sbin/adduser --quiet --disabled-password --gecos "" %u

; add machine script  = /usr/sbin/useradd -g machines -c "%u machine account" -d /var/lib/samba -s /bin/false %u

; add group script = /usr/sbin/addgroup --force-badname %g

############ Misc ############

;   include = /home/samba/etc/smb.conf.%m

;   idmap config * :              backend = tdb
;   idmap config * :              range   = 3000-7999
;   idmap config YOURDOMAINHERE : backend = tdb
;   idmap config YOURDOMAINHERE : range   = 100000-999999
;   template shell = /bin/bash

#   usershare max shares = 100

   usershare allow guests = yes

#======================= Share Definitions =======================

;[homes]
;   comment = Home Directories
;   browseable = no
;   read only = yes
;   create mask = 0700
;   directory mask = 0700

[bagibagi]
    comment = Samba on Ubuntu
    path = /home/ubuntu/sisop4soal3/report
    read only = no
    browsable = yes
    writable = yes
    guest ok = no
```

**Melakukan mount FUSE dengan `./archeology iwak` dan melakukan listing ke direkori mount**
![image](https://github.com/rmnovianmalcolmb/Sisop-4-2024-MH-IT08/assets/122516105/68dbd8fb-0612-4f0d-b0a2-f1e60ef847fa)

**Melakukan copy file dari dalam direkori mount keluar dari direktori mount**
![image](https://github.com/rmnovianmalcolmb/Sisop-4-2024-MH-IT08/assets/122516105/c7d756c0-0a68-4eba-bd32-da92c098e855)

**Melakukan copy file dari luar direktori mount ke dalam direktori mount**
![image](https://github.com/rmnovianmalcolmb/Sisop-4-2024-MH-IT08/assets/122516105/6fe383fe-39fb-4ea4-9882-8d2065789032)

**Melakukan penghapusan di direktori mount yang mana juga menghapus pecahan file di direktori relics**
![image](https://github.com/rmnovianmalcolmb/Sisop-4-2024-MH-IT08/assets/122516105/3cea4098-a887-41e2-a9fb-91acab55fbd0)

**Samba file dari direktori report**
![image](https://github.com/rmnovianmalcolmb/Sisop-4-2024-MH-IT08/assets/122516105/7dc1b52b-3293-47ea-9602-272b07dbd831)




