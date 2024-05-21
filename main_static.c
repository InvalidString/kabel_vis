#include <raylib.h>

#define WIDTH 600
#define HEIGHT 400

typedef void* (init_t)();
typedef void (update_t)(void*);

init_t init;
update_t update;

int main(){

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WIDTH, HEIGHT, "Kabel Vis");
    SetTargetFPS(60);


    void* state = init();

    while(!WindowShouldClose()){
        update(state);
    }

    CloseWindow();
    return 0;
}
