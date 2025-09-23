#include <pspsdk.h>
#include <stdio.h>
#include <string.h>
#include <systemctrl.h>

PSP_MODULE_INFO("KFE Launcher", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int main(int argc, char **argv) {
    struct SceKernelLoadExecVSHParam param;
    char path[256];

    strcpy(path, argv[0]);
    char *p = strrchr(path, '/');
    if (p) {
        strcpy(p + 1, "APP.PBP");
    }

    memset(&param, 0, sizeof(param));
    param.size = sizeof(param);
    param.key = "game";
    param.args = strlen(path) + 1;
    param.argp = path;

    sctrlKernelLoadExecVSHWithApitype(PSP_INIT_APITYPE_MS2, path, &param);

    sceKernelExitGame();
    return 0;
}