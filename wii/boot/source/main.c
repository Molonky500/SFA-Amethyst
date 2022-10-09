#include "main.h"

/** This is the application's "bootloader".
 *  What it does is find the second-stage bootloader,
 *  store the path to it in a fixed location, load
 *  it, and execute it.
 *  The reason is that the standard homebrew loader
 *  uses the 9xxxxxxx memory region, so it's not safe
 *  for our application to have sections there, but we
 *  want to have sections there so that we don't need
 *  to worry about stomping on the game, so we let the
 *  standard loader load this program at the default
 *  address, then manually load the next program at
 *  whatever address we damn well please.
 *  It's a bit ugly, but it works.
 */

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
static char gameRootDir[512];
extern DISC_INTERFACE __io_wiisd;
bool gIsVideoInit = false;

int initVideo() {
    /** Init video system.
     *  @returns 0 on success, nonzero on failure.
     */
	VIDEO_Init(); //Initialise the video system

    // Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate memory for the display in the uncached region
	//xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	xfb = MEM_K0_TO_K1(0x8048e480);

    // Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode); //set up selected video mode
	VIDEO_SetNextFramebuffer(xfb); //tell GPU where our buffers are
	VIDEO_SetBlack(FALSE); //main screen turn on
    VIDEO_Flush();
    VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
    gIsVideoInit = true;

    return 0;
}

void quit(const char *msg) {
    initVideo();
    if(msg != NULL) puts(msg);

    int countdown = 10*60;
    while(countdown--) {
        VIDEO_WaitVSync();
        PAD_ScanPads();
        if(PAD_ButtonsDown(PAD_CHAN0) &
            (PAD_BUTTON_START|PAD_BUTTON_A)) break;
    }
    printf("\n  Exiting  \n");
    exit(0);
}

int initFilesystem() {
    /** Init FAT driver.
     *  @returns 0 on success, nonzero on failure.
     */
    if(!fatInitDefault()) {
        printf("FAT init failed\n");
        return 1;
    }
    return 0;
}

int init() {
    /** Init application.
     *  @returns 0 on success, nonzero on failure.
     */
    int err = 0;
    exiPrintInit();
    initVideo();
    if(!err) err = initFilesystem();
    return err;
}

int _scanForGameFiles(char *path, size_t pathLen,
int _depth) {
    /** Find the game's root directory.
     *  @param path Path to scan.
     *  @param pathLen Size of path buffer.
     *  @param _depth Recursion depth.
     *  @returns 1 if found, 0 if not, negative if error.
     *  @note The string pointed to by path will be modified.
     *    On success, it will contain the path of the directory
     *    containing the game's 'files' and 'sys' directories.
     *    On failure, its contents are unspecified.
     */
    if(_depth > 10) return 0; //don't recurse forever

    DIR *pdir;
    struct dirent *pent;
	struct stat statbuf;
    size_t pathEnd = strlen(path);

    //skip some unlikely places
    //note double slash here, that's fine...
    if(!strcmp(path, "//Private")) return 0;
    if(!strcmp(path, "//private")) return 0;
    if(!strcmp(path, "//ios")) return 0;
    if(!strcmp(path, "//bootmii")) return 0;
    if(!strcmp(path, "//apps/homebrew_browser")) return 0;

    //printf("\x1B[2;0H\x1B[2K"); //home, clear line
    //for(size_t i=0; i<pathEnd; i++) printf(".");
    //printf("\x1B[2;0HScanning: %s     ", path);
    //printf("%s\n", path);

    pdir = opendir(path);
    if(!pdir) {
        int err = errno;
        if(err == ENOTDIR) return 0;
        //printf("Error opening path: %s\n", path);
        return -abs(err);
    }
    while((pent=readdir(pdir)) != NULL) {
        checkForExitButton();

        if(strcmp(".",  pent->d_name) == 0
        || strcmp("..", pent->d_name) == 0) continue;

        snprintf(&path[pathEnd], pathLen-pathEnd, "/%s", pent->d_name);
        if(stat(path, &statbuf)) {
            int err = errno;
            //printf("stat(%s) err %d\n", path, err);
            return -abs(err);
        }

        //for(int i=0; i<5; i++) VIDEO_WaitVSync();
        if(S_ISDIR(statbuf.st_mode)) {
			int r = _scanForGameFiles(path, pathLen, _depth+1);
            if(r != 0) {
                closedir(pdir);
                return r;
            }
        }
        else if(!strcmp(pent->d_name, "sfa_root.flag")) {
            path[pathEnd+1] = 0; //undo appending the file name
            closedir(pdir);
            return 1;
        }
        path[pathEnd+1] = 0; //undo appending the file name
    }
    closedir(pdir);
    return 0;
}

void initGameFiles() {
    /** Find the game files.
     *  If not found, displays an error and exits the application.
     */

    //temp override
    strcpy(gameRootDir, "sd:/apps/SFA");
    return;

    //printf("Looking for game files... (START to exit)\n");
    strcpy(gameRootDir, "/");
    //strcpy(gameRootDir, "//apps/SFA");
    int err = _scanForGameFiles(gameRootDir, sizeof(gameRootDir), 0);
    //printf("\n");
    if(err > 0) { //success
        exiPrintf("Found game files:\n\"%s\"\n", gameRootDir);
    }
    else if(err < 0) quit("Error scanning for game files\n");
    else quit("Game files not found.\n");
}

void printDolHeader(DolHeader *header) {
    exiPrintf("Sect## Offset Address  Size\n");
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        exiPrintf("text%2d %06X %08X %08X\n", i,
            header->textOffset[i], header->textAddr[i],
            header->textSize[i]);
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        exiPrintf("data%2d %06X %08X %08X\n", i,
            header->dataOffset[i], header->dataAddr[i],
            header->dataSize[i]);
    }
    exiPrintf("bss    ------ %08X %08X\n", header->bssAddr,
        header->bssSize);
    exiPrintf("Entry  ------ %08X ------\n", header->entryPoint);
}

void loadDol(FILE *dol, DolHeader *header) {
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        if(header->textSize[i] > 0) {
            exiPrintf("Loading text%d...\n", i);
            fseek(dol, header->textOffset[i], SEEK_SET);
            fread((void*)header->textAddr[i], 1, header->textSize[i], dol);
        }
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        if(header->dataSize[i] > 0) {
            exiPrintf("Loading data%d...\n", i);
            fseek(dol, header->dataOffset[i], SEEK_SET);
            fread((void*)header->dataAddr[i], 1, header->dataSize[i], dol);
        }
    }
    exiPrintf("Init bss...\n");
    memset((void*)header->bssAddr, 0, header->bssSize);
}

void loadGame() {
    //load the game program itself at a fixed address.
    //the second-stage loader can then copy it to the
    //appropriate memory and boot it without needing
    //to init libfat.
    char path[4096];
    snprintf(path, sizeof(path), "%s/sys/main.dol", gameRootDir);
    FILE *file = fopen(path, "rb");
    if(!file) {
        initVideo();
        printf("[stage1] Error opening %s: %d\n", path, errno);
        quit(NULL);
    }
    fseek(file, 0, SEEK_END);
    u32 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //this address will later be used by the game as ARAM.
    //that means the loader isn't using it at all and it
    //won't go to waste storing a copy of the DOL.
    //ARAM is 16MB and the DOL is < 4MB so this works out.
    void *dest = (void*)0x90000000;
    int r = 0;
    u32 offset = 0;
    while(offset < size) {
        r = fread(dest, 1, size-offset, file);
        if(r < 0) {
            initVideo();
            printf("[stage1] Error reading %s: %d\n", path, errno);
            quit(NULL);
        }
        offset += r;
        dest += r;
    }
}

__attribute__((noreturn)) void loadAppDol() {
    /** Load the second stage bootloader DOL.
     */
    char path[4096];
    snprintf(path, sizeof(path), "%s/files/splashScreen.bin", gameRootDir);
    FILE *splash = fopen(path, "rb");
    if(splash) {
        fread(xfb, 2, 640*480, splash);
        fclose(splash);
    }

    snprintf(path, sizeof(path), "%s/main.dol", gameRootDir);
    FILE *dol = fopen(path, "rb");
    if(!dol) {
        initVideo();
        printf("[stage1] Error opening %s: %d\n", path, errno);
        quit(NULL);
    }

    //load the DOL
    DolHeader header;
    fread(&header, sizeof(DolHeader), 1, dol);
    //printDolHeader(&header);
    loadDol(dol, &header);
    fclose(dol);

    //find the magic placeholder string for the path
    static const char *magic = "*** GAME ROOT DIR ***";
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        if(header.dataSize[i] > 0) {
            for(u32 j=header.dataAddr[i];
            j<header.dataAddr[i]+header.dataSize[i]; j++) {
                if(!strncmp((char*)j, magic, 21)) {
                    //printf("Found magic at %08X\n", j);
                    strncpy((char*)j, gameRootDir, sizeof(gameRootDir));
                }
            }
        }
    }

    loadGame();
    fatUnmount("sd");
    __io_wiisd.shutdown();
    //delay to let SD tidy itself up
    u64 then = SYS_Time() + 10000000;
    while(SYS_Time() < then);
    __IOS_ShutdownSubsystems();
    then = SYS_Time() + 10000000;
    while(SYS_Time() < then);

    //for(int i=0; i<120; i++) VIDEO_WaitVSync();
    exiPrintf("Exec...\n");
    /*while(1) {
        PAD_ScanPads();
        u32 buttons = PAD_ButtonsDown(PAD_CHAN0);
        if(buttons & PAD_BUTTON_B) exit(0);
        if(buttons & (PAD_BUTTON_START|PAD_BUTTON_A)) break;
    }*/
    VIDEO_SetBlack(TRUE);
    void (*boot)(void) = (void (*)())header.entryPoint;
    IRQ_Disable();
    boot(); //shouldn't return
    while(1); //but compiler doesn't believe me
}

int main(int argc, char **argv) {
    int err = init();
    if(err) quit("Startup failed\n");
    initGameFiles();
    loadAppDol();
	return 0;
}
