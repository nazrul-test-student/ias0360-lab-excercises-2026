#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"

#include "ff.h"
#include "sd_card.h"
#include "f_util.h"
#include "hw_config.h"
#include "LCD_Driver.h"
#include "LCD_Touch.h"
#include "LCD_GUI.h"
#include "DEV_Config.h"

#define PATH_MAX_LEN 256
#define MAX_FILE_WRITE 3


mutex_t mutex;
TP_DATA tp_data;

// --------- Globals (FatFs requires the FS to outlive the mount) ----------
static FATFS fs;                 // must be static/global (lives as long as the mount)
static sd_card_t *g_sd = NULL;   // active SD card
static const char *g_drive = NULL; // typically "0:"

// ------------------------- Utility / Error -------------------------------
static void die(FRESULT fr, const char *op) {
    printf("%s failed: %s (%d)\n", op, FRESULT_str(fr), fr);
    multicore_fifo_push_blocking(WRITE_FAILED_FLAG);
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
    
    printf("write_to_file: About to write %u bytes from pointer %p\n", len, data);
    printf("write_to_file: First few bytes: %02X %02X %02X %02X\n",
           ((uint8_t*)data)[0], ((uint8_t*)data)[1], 
           ((uint8_t*)data)[2], ((uint8_t*)data)[3]);
    
    FRESULT fr = f_write(file, data, len, bytes_written);
    printf("write_to_file: f_write returned FR=%d, wrote %u bytes\n", fr, *bytes_written);
    
    if (fr == FR_OK) {
        fr = f_sync(file); // ensure data hits the card
        printf("write_to_file: f_sync returned FR=%d\n", fr);
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

    printf("Core 1 entry: write to SD card\n");
    sleep_ms(2000);

    // 1) Init + mount
    if (!sd_init_and_mount()) {
        loop_forever_msg("SD init/mount failed.");
    }

    int count = 0;
    FRESULT fr;

    while (count < MAX_FILE_WRITE) {

        uint32_t msg = multicore_fifo_pop_blocking();
        if (msg != DATA_READY_FLAG) {
            printf("Core 1 received unexpected message: 0x%08lX\n", msg);
            continue;
        }

        printf("Writing data to a file\n");

        // Build absolute file path: <drive>/lcd_sd_card_example_<iteration>.txt
        char path[PATH_MAX_LEN];
        char name[64];
        snprintf(name, sizeof(name), "lcd_sd_card_example_%d.txt", count);
        join_path(path, sizeof path, g_drive, name);

        printf("Core 1: Creating and writing to file: %s\n", path);
        // 2) Create the file
        FIL f;
        fr = create_file(path, &f);
        if (fr != FR_OK) die(fr, "f_open(create)");

        mutex_enter_blocking(&mutex);

        // 3) Write data
        UINT bw = 0;

        // Check if data is valid
        printf("Core 1: tp_data.data_len = %zu\n", tp_data.data_len);
        printf("Core 1: tp_data.data pointer = %p\n", (void*)tp_data.data);
        
        if (tp_data.data == NULL || tp_data.data_len == 0) {
            printf("ERROR: tp_data.data is NULL or data_len is 0!\n");
            mutex_exit(&mutex);
            f_close(&f);
            continue;
        }

        // Print all the data stored in tp_data.data
        printf("Data contents (%zu bytes): ", tp_data.data_len);
        for (size_t i = 0; i < tp_data.data_len; i++) {
            printf("%u ", tp_data.data[i]);
            if ((i + 1) % BOX_W == 0) printf("\n");
        }
        if (tp_data.data_len % 16 != 0) printf("\n");

        // Convert binary 0/1 to ASCII '0'/'1' for human-readable text file
        char *ascii_buffer = (char *)malloc(tp_data.data_len);
        if (ascii_buffer == NULL) {
            printf("ERROR: Failed to allocate ASCII buffer\n");
            mutex_exit(&mutex);
            f_close(&f);
            die(FR_NOT_ENOUGH_CORE, "malloc");
        }
        
        for (size_t i = 0; i < tp_data.data_len; i++) {
            ascii_buffer[i] = tp_data.data[i] ? '1' : '0';  // Convert to ASCII '0' or '1'
        }
        
        fr = write_to_file(&f, ascii_buffer, (UINT)tp_data.data_len, &bw);
        free(ascii_buffer);
        
        printf("Core 1: write_to_file returned FR=%d, bytes_written=%u (expected %zu)\n", 
               fr, bw, tp_data.data_len);
        if (fr != FR_OK || bw != tp_data.data_len) {
            printf("ERROR: Write failed or incomplete! FR=%d, wrote %u/%zu bytes\n", 
                   fr, bw, tp_data.data_len);
            die(fr, "f_write/f_sync");
        }
        
        // Close the file
        f_close(&f);


        mutex_exit(&mutex);
        
        count++;
        printf("----- File write iteration %d -----\n", count);
    }

    // Optional: unmount
    fr = f_unmount(g_drive);
    printf("f_unmount -> %s (%d)\n", FRESULT_str(fr), fr);

    sleep_ms(1000);  // optional flush delay
    multicore_fifo_push_blocking(TASK_COMPLETE_FLAG); // acknowledge successful send

    printf("Core 1 task complete.\n");

    while (1) { tight_loop_contents(); }
}


int main(void) {

    System_Init();
    sleep_ms(3000);

    mutex_init(&mutex);  // Initialize the mutex

	LCD_SCAN_DIR  lcd_scan_dir = SCAN_DIR_DFT;
	LCD_Init(lcd_scan_dir,1000);
	TP_Init(lcd_scan_dir, &tp_data, &mutex);
	TP_GetAdFac();
	TP_Dialog();

    multicore_launch_core1(core1_entry);

	while(1){
        if (multicore_fifo_rvalid()) {
            uint32_t msg = multicore_fifo_pop_blocking();
            if (msg == TASK_COMPLETE_FLAG) {
                printf("Core 0: Core 1 task complete.\n");
                break;
            } else if (msg == WRITE_FAILED_FLAG) {
                printf("Core 0: Core 1 reported write failure.\n");
                loop_forever_msg("Write failed on Core 1.");
            }
        } else {
			LCD_SetBackLight(1000);
			TP_DrawBoard();
        }
	}

    printf("All tasks complete.\n");
    mutex_exit(&mutex);
    multicore_reset_core1();

    printf("Exiting main().\n");
    
    return 0;
}
