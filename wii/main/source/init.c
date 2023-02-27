#include "main.h"

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
        exiPrintf("Error opening path: %s\n", path);
        //delayFrames(60);
        return -abs(err);
    }
    while((pent=readdir(pdir)) != NULL) {
        //printf("readdir OK \"%s\"\r\n", pent->d_name); //delayFrames(60);
        //checkForExitButton();

        if(strcmp(".",  pent->d_name) == 0
        || strcmp("..", pent->d_name) == 0) continue;

        snprintf(&path[pathEnd], pathLen-pathEnd, "/%s", pent->d_name);
        //printf("stat...\r\n"); //delayFrames(15);
        if(stat(path, &statbuf)) {
            int err = errno;
            exiPrintf("stat(%s) err %d\n", path, err);
            //delayFrames(60);
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
    else strcpy(gameRootDir, "sd:/apps/SFA");
    exiPrintf("Look for game files in: \"%s\"\r\n", gameRootDir);
    //delayFrames(60);
    int err = _scanForGameFiles(gameRootDir, sizeof(gameRootDir), 0);
    if(err > 0) { //success
        exiPrintf("Found game files in: \"%s\"\r\n", gameRootDir);
        //delayFrames(60);
        return;
    }

    //printf("Looking for game files... (START to exit)\n");
    //printf("Tip: place them at %s to skip this step!\n", defaultRootDir);
    //delayFrames(60);
    strcpy(gameRootDir, "/");
    err = _scanForGameFiles(gameRootDir, sizeof(gameRootDir), 0);
    if(err > 0) { //success
        exiPrintf("Found game files:\r\n\"%s\"\r\n", gameRootDir);
        //delayFrames(60);
    }
    else if(err < 0) PANIC("Error scanning for game files\n");
    else PANIC("Game files not found.\n");
}
