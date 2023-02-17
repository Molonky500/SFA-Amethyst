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

static const char *defaultRootDir = "sd:/apps/SFA";

extern void __exception_closeall ();

int initVideo() {
    /** Init video system.
     *  @returns 0 on success, nonzero on failure.
     */
	VIDEO_Init(); //Initialise the video system

    // Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    //this is the address the game uses.
    //XXX this is probably unsafe...
	//xfb = MEM_K0_TO_K1(0x8048e480);

    // Initialise the console, required for printf
	console_init(xfb,20,20,
        rmode->fbWidth,rmode->xfbHeight,
        rmode->fbWidth*VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode); //set up selected video mode
	VIDEO_SetNextFramebuffer(xfb); //tell GPU where our buffers are
	VIDEO_SetBlack(FALSE); //main screen turn on
    VIDEO_Flush();
    VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
    gIsVideoInit = true;
    printf("\r\nInit EXI...\r\n");

    return 0;
}

void delayFrames(int n) {
    while(n--) VIDEO_WaitVSync();
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

void checkForExitButton() {
    //PAD_ScanPads();
    //if(PAD_ButtonsDown(PAD_CHAN0) & PAD_BUTTON_START) exit(0);
}

int initFilesystem() {
    /** Init FAT driver.
     *  @returns 0 on success, nonzero on failure.
     */
    printf("Init FAT...\r\n");
    //delayFrames(15);
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
    //err = IOS_ReloadIOS(36);
    initVideo();
    exiPrintInit();
    if(!err) {
        //exiPrintf("IOS reload: %d\n", err);
        err = initFilesystem();
    }
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
    if(!path) return 0;

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
    //printf("\"%s\"\r\n", path);
    //delayFrames(20);

    strncat(path, "/sfa_root.flag", pathLen);
    if(!stat(path, &statbuf)) {
        path[pathEnd+1] = 0; //undo appending the file name
        return 1;
    }

    pdir = opendir(path);
    //printf("opendir OK\r\n");
    if(!pdir) {
        int err = errno;
        if(err == ENOTDIR) return 0;
        printf("Error opening path: %s\n", path);
        delayFrames(60);
        return -abs(err);
    }
    while((pent=readdir(pdir)) != NULL) {
        //printf("readdir OK \"%s\"\r\n", pent->d_name); //delayFrames(60);
        checkForExitButton();

        if(strcmp(".",  pent->d_name) == 0
        || strcmp("..", pent->d_name) == 0) continue;

        snprintf(&path[pathEnd], pathLen-pathEnd, "/%s", pent->d_name);
        //printf("stat...\r\n"); //delayFrames(15);
        if(stat(path, &statbuf)) {
            int err = errno;
            printf("stat(%s) err %d\n", path, err);
            delayFrames(60);
            return -abs(err);
        }

        //delayFrames(5);
        if(S_ISDIR(statbuf.st_mode)) {
            //printf("recurse: %s\r\n", path); //delayFrames(15);
			int r = _scanForGameFiles(path, pathLen, _depth+1);
            if(r != 0) {
                //printf("closedir...\r\n"); //delayFrames(15);
                closedir(pdir);
                return r;
            }
        }
        else if(!strcmp(pent->d_name, "sfa_root.flag")) {
            //printf("found\r\n"); delayFrames(15);
            path[pathEnd+1] = 0; //undo appending the file name
            closedir(pdir);
            return 1;
        }
        path[pathEnd+1] = 0; //undo appending the file name
    }
    //printf("closedir end\r\n"); //delayFrames(15);
    closedir(pdir);
    return 0;
}

void initGameFiles(const char *appPath) {
    /** Find the game files.
     *  If not found, displays an error and exits the application.
     *  appPath: path to this executable, passed by most loaders.
     *   not by Dolphin when running from the local FS though,
     *   because how would that even work?
     */
    if(appPath) {
        strncpy(gameRootDir, appPath, sizeof(gameRootDir));
        //cut off the name
        int n = strlen(gameRootDir) - 1;
        while(n >= 0) {
            if(gameRootDir[n] == '/') {
                gameRootDir[n] = 0;
                break;
            }
            else n--;
        }
    }
    else strcpy(gameRootDir, defaultRootDir);
    exiPrintf("Look for game files in: \"%s\"\r\n", gameRootDir);
    //delayFrames(60);
    int err = _scanForGameFiles(gameRootDir, sizeof(gameRootDir), 0);
    if(err > 0) { //success
        exiPrintf("Found game files in: \"%s\"\r\n", gameRootDir);
        //delayFrames(60);
        return;
    }

    printf("Looking for game files... (START to exit)\n");
    printf("Tip: place them at %s to skip this step!\n", defaultRootDir);
    delayFrames(60);
    strcpy(gameRootDir, "/");
    err = _scanForGameFiles(gameRootDir, sizeof(gameRootDir), 0);
    if(err > 0) { //success
        exiPrintf("Found game files:\r\n\"%s\"\r\n", gameRootDir);
        //delayFrames(60);
    }
    else if(err < 0) quit("Error scanning for game files\n");
    else quit("Game files not found.\n");
}

void printDolHeader(DolHeader *header) {
    exiPrintf("Sect## Offset Address  Size\r\n");
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        exiPrintf("text%2d %06X %08X %08X\r\n", i,
            header->textOffset[i], header->textAddr[i],
            header->textSize[i]);
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        exiPrintf("data%2d %06X %08X %08X\r\n", i,
            header->dataOffset[i], header->dataAddr[i],
            header->dataSize[i]);
    }
    exiPrintf("bss    ------ %08X %08X\r\n", header->bssAddr,
        header->bssSize);
    exiPrintf("Entry  ------ %08X ------\r\n", header->entryPoint);
}

void loadDol(FILE *dol, DolHeader *header) {
    for(int i=0; i<DOL_NUM_TEXT_SECTIONS; i++) {
        if(header->textSize[i] > 0) {
            exiPrintf("Loading text%d...\r\n", i);
            fseek(dol, header->textOffset[i], SEEK_SET);
            fread((void*)header->textAddr[i], 1, header->textSize[i], dol);
        }
    }
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        if(header->dataSize[i] > 0) {
            exiPrintf("Loading data%d...\r\n", i);
            fseek(dol, header->dataOffset[i], SEEK_SET);
            fread((void*)header->dataAddr[i], 1, header->dataSize[i], dol);
        }
    }
    exiPrintf("Init bss...\r\n");
    memset((void*)header->bssAddr, 0, header->bssSize);
}

void* loadGame() {
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

    void *dest  = (void*)DOL_LOAD_ADDR;
    void *start = dest;
    int r = 0;
    u32 offset = 0;
    while(offset < size) {
        r = fread(dest, 1, MIN(1024, size-offset), file);
        if(r < 0) {
            initVideo();
            printf("[stage1] Error reading %s: %d\n", path, errno);
            quit(NULL);
        }
        printf("\x1B[2;0H\x1B[2KLoading: %5.2f%%  ", //home, clear line
            ((float)offset/(float)size) * 100.0f);
        DCFlushRange(dest, r);
        ICInvalidateRange(dest, r);
        offset += r;
        dest += r;
    }

    u32 cksum = 0;
    u8 *data  = (u8*)DOL_LOAD_ADDR;
    for(u32 i=0; i<size; i++) cksum += data[i];
    exiPrintf("DOL CKSUM = %08X\r\n", cksum);
    *(u32*)(DOL_LOAD_ADDR-4) = size;
    *(u32*)(DOL_LOAD_ADDR-8) = cksum;

    printDolHeader((DolHeader*)start);
    //udelay(700000);
    /*uint32_t *dumpCode = (uint32_t*)0x90008510;
    for(int i=0; i<16; i++) {
        exiPrintf("%08X%s", *(dumpCode++),
            ((i&3)==3) ? "\r\n" : " ");
    }*/

    return start;
}

__attribute__((noreturn)) void loadAppDol() {
    /** Load the second stage bootloader DOL.
     */
    char path[4096];

    //display a nice splash screen that's on the game
    //disc but normally unused.
    snprintf(path, sizeof(path), "%s/files/splashScreen.bin", gameRootDir);
    FILE *splash = fopen(path, "rb");
    if(splash) {
        fread(xfb, 2, 640*480, splash);
        fclose(splash);
    }

    //load the DOL.
    snprintf(path, sizeof(path), "%s/main.dol", gameRootDir);
    FILE *dol = fopen(path, "rb");
    if(!dol) {
        initVideo();
        printf("[stage1] Error opening %s: %d\n", path, errno);
        quit(NULL);
    }
    DolHeader header;
    fread(&header, sizeof(DolHeader), 1, dol);
    //printDolHeader(&header);
    loadDol(dol, &header);
    fclose(dol);

    void *dolAddr = loadGame();

    //find the magic placeholder string for the path
    static const char *magic = "*** GAME ROOT DIR ***";
    for(int i=0; i<DOL_NUM_DATA_SECTIONS; i++) {
        if(header.dataSize[i] > 0) {
            for(u32 j=header.dataAddr[i];
            j<header.dataAddr[i]+header.dataSize[i]; j++) {
                if(!strncmp((char*)j, magic, 21)) {
                    //printf("Found magic at %08X\n", j);
                    //suppress warning about using the wrong thing
                    //for the size to strncpy (we're not)
                    size_t size = sizeof(gameRootDir);
                    strncpy((char*)j, gameRootDir, size);
                    //also pass the address we loaded the DOL at.
                    //snprintf((char*)j, size, "%08X%s",
                    //    dolAddr, gameRootDir);
                }
            }
        }
    }

    exiPuts("Shutting down SD\r\n");
    fatUnmount("sd");
    __io_wiisd.shutdown();
    //delay to let SD tidy itself up
    u64 later = SYS_Time() + 10000000;
    while(SYS_Time() < later);

    //prevent game crashing on reset
    //shouldn't be necessary since we do SYS_ResetSystem instead.
    /*__IOS_ShutdownSubsystems();
    later = SYS_Time() + 10000000;
    while(SYS_Time() < later);*/

    exiPrintf("Exec...\n");
    void (*boot)(void) = (void (*)())header.entryPoint;
    //DSP_Reset();
    //DSP_Halt();
    //Amazingly, this does NOT shut down the system.
    //Use SYS_POWEROFF for that.
    //What this does do is un-init all the OS stuff,
    //to be ready to reboot or load another program.
    SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
    __exception_closeall();
    IRQ_Disable();
    boot(); //shouldn't return
    while(1); //but compiler doesn't believe me
}

int main(int argc, char **argv) {
    int err = init();
    if(err) quit("Startup failed\n");
    printf("argc=%d\n", argc);
    for(int i=0; i<argc; i++) {
        printf("argv[%d]=\"%s\"\n", i, argv[i]);
    }
    //delayFrames(60);
    initGameFiles(argc > 0 ? argv[0] : NULL);
    //delayFrames(60);
    //exiPrintf("WPAD_Init...\n");
    //err = WPAD_Init();
    //exiPrintf("WPAD_Init: %d\n", err);
    exiPrintf("Boot game...\r\n");
    //delayFrames(60);
    loadAppDol();
	return 0;
}
