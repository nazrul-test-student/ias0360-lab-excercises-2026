#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "ff.h"
#include "sd_card.h"
#include "f_util.h"
#include "hw_config.h"

#define PATH_MAX_LEN 256

// --------- Globals (FatFs requires the FS to outlive the mount) ----------
static FATFS fs;                 // must be static/global (lives as long as the mount)
static sd_card_t *g_sd = NULL;   // active SD card
static const char *g_drive = NULL; // typically "0:"

// ------------------------- Utility / Error -------------------------------
static void die(FRESULT fr, const char *op) {
    printf("%s failed: %s (%d)\n", op, FRESULT_str(fr), fr);
    while (1) tight_loop_contents();
}

static void loop_forever_msg(const char *msg) {
    printf("%s\n", msg);
    while (1) tight_loop_contents();
}

static void join_path(char *out, size_t out_sz, const char *drive, const char *rel) {
    // drive = "0:" or "0:/", ensure exactly one slash when joining
    if (rel && rel[0] == '/') rel++; // avoid double slashes
    if (drive && drive[strlen(drive) - 1] == '/')
        snprintf(out, out_sz, "%s%s", drive, rel ? rel : "");
    else
        snprintf(out, out_sz, "%s/%s", drive, rel ? rel : "");
}

// ------------------------- 1) Initialization -----------------------------
static bool sd_init_and_mount(void) {
    if (!sd_init_driver()) {
        printf("sd_init_driver() failed\n");
        return false;
    }

    g_sd = sd_get_by_num(0);
    if (!g_sd) {
        printf("No SD config found (sd_get_by_num(0) == NULL)\n");
        return false;
    }

    g_drive = sd_get_drive_prefix(g_sd);  // usually "0:"
    if (!g_drive) {
        printf("sd_get_drive_prefix() returned NULL\n");
        return false;
    }

    FRESULT fr = f_mount(&fs, g_drive, 1);
    printf("f_mount -> %s (%d)\n", FRESULT_str(fr), fr);

    if (fr == FR_NO_FILESYSTEM) {
        BYTE work[4096]; // >= FF_MAX_SS
        MKFS_PARM opt = { FM_FAT | FM_SFD, 0, 0, 0, 0 };
        fr = f_mkfs(g_drive, &opt, work, sizeof work);
        printf("f_mkfs -> %s (%d)\n", FRESULT_str(fr), fr);
        if (fr == FR_OK) {
            fr = f_mount(&fs, g_drive, 1);
            printf("f_mount(after mkfs) -> %s (%d)\n", FRESULT_str(fr), fr);
        }
    }

    if (fr != FR_OK) {
        printf("Mount failed: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }

    return true;
}

// ------------------------- 2) File creation ------------------------------
static FRESULT create_file(const char *abs_path, FIL *out_file) {
    // Creates/truncates a file and opens it for writing
    return f_open(out_file, abs_path, FA_WRITE | FA_CREATE_ALWAYS);
}

// ------------------------- 3) File writing -------------------------------
static FRESULT write_to_file(FIL *file, const void *data, UINT len, UINT *bytes_written) {
    *bytes_written = 0;
    FRESULT fr = f_write(file, data, len, bytes_written);
    if (fr == FR_OK) {
        fr = f_sync(file); // ensure data hits the card
    }
    return fr;
}

// ------------------------- 4) File checking/listing ----------------------
typedef struct {
    uint32_t files;
    uint32_t dirs;
    uint64_t total_bytes;
} list_stats_t;

static bool is_dot_or_dotdot(const char *name) {
    return (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')));
}

static FRESULT list_dir_recursive(const char *path, list_stats_t *stats) {
    DIR dir;
    FILINFO fno;
    FRESULT fr = f_opendir(&dir, path);
    if (fr != FR_OK) {
        printf("f_opendir('%s') -> %s (%d)\n", path, FRESULT_str(fr), fr);
        return fr;
    }

    for (;;) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK) {
            printf("f_readdir('%s') -> %s (%d)\n", path, FRESULT_str(fr), fr);
            break;
        }
        if (fno.fname[0] == '\0') break; // end of directory

        if (is_dot_or_dotdot(fno.fname)) continue;

        if (fno.fattrib & AM_DIR) {
            stats->dirs++;
            char subpath[PATH_MAX_LEN];
            snprintf(subpath, sizeof subpath, "%s/%s", path, fno.fname);
            printf("[DIR]  %s\n", subpath);
            fr = list_dir_recursive(subpath, stats);
            if (fr != FR_OK) break;
        } else {
            stats->files++;
            stats->total_bytes += (uint64_t)fno.fsize;
            printf("[FILE] %s/%s  (%lu bytes)\n", path, fno.fname, (unsigned long)fno.fsize);
        }
    }

    FRESULT frc = f_closedir(&dir);
    if (fr == FR_OK && frc != FR_OK) fr = frc;
    return fr;
}

// Public checker: lists all files and sizes, and tells if any exist
static FRESULT check_and_list_files(const char *root_drive) {
    // Build root path "0:/"
    char root[PATH_MAX_LEN];
    join_path(root, sizeof root, root_drive, ""); // ensures a trailing slash when we add children

    list_stats_t stats = {0};
    printf("\n--- SD Card File Listing for '%s' ---\n", root_drive);
    FRESULT fr = list_dir_recursive(root_drive, &stats);
    if (fr != FR_OK && fr != FR_NO_PATH) {
        printf("Directory listing aborted due to error.\n");
        return fr;
    }

    if (stats.files == 0 && stats.dirs == 0) {
        printf("No files or directories found on the SD card.\n");
    } else if (stats.files == 0) {
        printf("No files found (but %u director%s present).\n", stats.dirs, (stats.dirs == 1 ? "y" : "ies"));
    } else {
        printf("\nSummary: %u file%s in %u director%s, total %llu bytes.\n",
               stats.files, (stats.files == 1 ? "" : "s"),
               stats.dirs, (stats.dirs == 1 ? "y" : "ies"),
               (unsigned long long)stats.total_bytes);
    }
    return FR_OK;
}

// ------------------------------ Main -------------------------------------

void core1_entry() {

    // 1) Init + mount
    if (!sd_init_and_mount()) {
        loop_forever_msg("SD init/mount failed.");
    }

    // Build absolute file path: <drive>/test.txt
    char path[PATH_MAX_LEN];
    join_path(path, sizeof path, g_drive, "test3.txt");

    // 2) Create the file
    FIL f;
    FRESULT fr = create_file(path, &f);
    if (fr != FR_OK) die(fr, "f_open(create)");

    // 3) Write data
    const char *msg = "data blahblah test!\n";
    UINT bw = 0;
    fr = write_to_file(&f, msg, (UINT)strlen(msg), &bw);
    if (fr != FR_OK || bw != strlen(msg)) die(fr, "f_write/f_sync");
    printf("Wrote %u bytes to %s\n", bw, path);

    // Close the file
    f_close(&f);

    // 4) Check and list files (recursively) on the card
    fr = check_and_list_files(g_drive);
    if (fr != FR_OK) die(fr, "check_and_list_files");

    // Optional: unmount
    fr = f_unmount(g_drive);
    printf("f_unmount -> %s (%d)\n", FRESULT_str(fr), fr);

    while (1) { tight_loop_contents(); }
}


int main(void) {

    stdio_init_all();
    sleep_ms(3000);

    multicore_launch_core1(core1_entry);
    while (1) { sleep_ms(1000); }
}
