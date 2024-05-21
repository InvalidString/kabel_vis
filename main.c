#include <raylib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 600
#define HEIGHT 400

typedef void* (*init_t)();
typedef void (*update_t)(void*);

init_t init = 0;
update_t update = 0;
update_t before_reload = 0;
update_t after_reload = 0;
void* live_lib = 0;

void reload(){

    if(live_lib){
        dlclose(live_lib);
        live_lib = 0;
    }
    live_lib = dlopen("live.so", RTLD_NOW);
    if(!live_lib){
        printf("libload fail\n");
        exit(1);
    }

    init = dlsym(live_lib, "init");
    if(!init){
        printf("loading init symbol fail\n");
        exit(1);
    }
    update = dlsym(live_lib, "update");
    if(!update){
        printf("loading update symbol fail\n");
        exit(1);
    }
    after_reload = dlsym(live_lib, "after_reload");
    if(!after_reload){
        printf("loading after_reload symbol fail\n");
        exit(1);
    }
    before_reload = dlsym(live_lib, "before_reload");
    if(!before_reload){
        printf("loading before_reload symbol fail\n");
        exit(1);
    }
}

int main(){

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WIDTH, HEIGHT, "Kabel Vis");
    SetTargetFPS(60);

    reload();

    void* state = init();

    while(!WindowShouldClose()){
        update(state);
        if(IsKeyPressed(KEY_R)){
            if(!system("./live-build.sh")){
                before_reload(state);
                if(IsKeyDown(KEY_LEFT_CONTROL)){
                    CloseWindow();
                    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
                    InitWindow(WIDTH, HEIGHT, "Kabel Vis");
                    SetTargetFPS(60);
                }
                reload();
                after_reload(state);
                puts("reloaded!");
            }
        }
    }

    CloseWindow();
    dlclose(live_lib);
    return 0;
}
