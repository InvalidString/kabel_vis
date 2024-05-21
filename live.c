#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <rlgl.h>
#include "dyn_arr.h"
#include "kabel.h"

#define PORT_RADIUS 5
#define WIRE_THICK 4

// edit modes
typedef enum{
    E_NONE = 0,
    E_DRAG_GATE = 1,
    E_WIRE = 2,
}EDIT_MODE;

#define IsKeyPr(Key) (IsKeyPressed(Key) || IsKeyPressedRepeat(Key))

#define ARRLEN(arr) (sizeof (arr) / sizeof (arr)[0])
#define ALIGN(a, b) (((a)+(b)-1)/(b)*(b))
#define dbg(fmt, val) fprintf(stderr, "%s: " fmt "\n", #val, (val))

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define BG2 GetColor(0x222222ff)
#define WIRE_ON RED
#define WIRE_OFF ColorBrightness(RED, -0.5)


Rectangle pad(Rectangle r, float v){
    return (Rectangle){
        r.x-v,
        r.y-v,
        r.width+2*v,
        r.height+2*v,
    };
}

typedef struct{
    char* name;
    Color color;
    uint32_t inputs;
    uint32_t outputs;
    void (*build)(KSim *sim, void** ins, void** outs);
} GateType;

void half_adder(bool *a, bool *b, bool *sum, bool *carry){
    b_and(a, b, carry);
    b_xor(a, b, sum);
}
void build_half_adder(KSim *sim, void** ins, void** outs){
    half_adder(ins[0], ins[1], outs[0], outs[1]);
}

void build_dummy(KSim *sim, void** ins, void** outs){
}
void build_b_not(KSim *sim, void** ins, void** outs){
    b_not(*ins, *outs);
}
#define BUILD_2_1(name) \
    void build_ ## name(KSim *sim, void** ins, void** outs){ \
        name(ins[0], ins[1], outs[0]); \
    }

BUILD_2_1(b_and)
BUILD_2_1(b_or)
BUILD_2_1(b_xor)
#undef BUILD_2_1

#define T_DISP 0
#define T_SWCH 1
GateType gate_types[] = {
    {"disp", BLUE, 1, 0, build_dummy},
    {"swch", BLUE, 0, 1, build_dummy},
    {"not",  BLUE, 1, 1, build_b_not},
    {"and",  BLUE, 2, 1, build_b_and},
    {"or",   BLUE, 2, 1, build_b_or},
    {"xor",  BLUE, 2, 1, build_b_xor},
    {"ha",  ORANGE, 2, 2, build_half_adder},
};

// output kabels are owned, input kabels are not
typedef struct{
    uint32_t type_tag;
    Vector2 pos;
} VisGate;
// after this struct (nullable) outputs and inputs follow

VisGate* new_gate(uint32_t type_tag, Vector2 pos){
    GateType *t = &gate_types[type_tag];

    size_t size = 
        ALIGN(sizeof(VisGate), 8) + 
        sizeof(void*[t->outputs]) +
        sizeof(void*[t->inputs]);
    VisGate *gate = calloc(1, size);
    gate->type_tag = type_tag;
    gate->pos = pos;
    return gate;
}
void** gate_outputs(VisGate* gate){
    void** out = (void**)((char*)gate + ALIGN(sizeof(VisGate), 8));
    return out;
}
void** gate_inputs(VisGate* gate){
    GateType *t = &gate_types[gate->type_tag];
    void** out = (void**)
        ((char*)gate + ALIGN(sizeof(VisGate), 8) + sizeof(void*[t->outputs]));
    return out;
}


Rectangle gate_rec(VisGate *gate){
    float size = 50;
    if(gate->type_tag == 0 || gate->type_tag == 1)
        return (Rectangle){
            gate->pos.x - 0.5*size,
            gate->pos.y - 0.5*size,
            size,
            size
        };
    
    GateType *t = &gate_types[gate->type_tag];
    float height = (MAX(t->inputs, t->outputs)+1)*20.f;
    return (Rectangle){
        gate->pos.x - 0.5*size,
        gate->pos.y - 0.5*size,
        size,
        height
    };
}

int gate_inport_count(VisGate *gate){
    return gate_types[gate->type_tag].inputs;
}
int gate_outport_count(VisGate *gate){
    return gate_types[gate->type_tag].outputs;
}
Vector2 gate_inport_pos(VisGate *gate, int i){
    GateType *type = &gate_types[gate->type_tag];
    Rectangle r = gate_rec(gate);
    float step = r.height / type->inputs;
    return (Vector2){r.x, r.y + (i+0.5)*step};
}
Vector2 gate_outport_pos(VisGate *gate, int i){
    GateType *type = &gate_types[gate->type_tag];
    Rectangle r = gate_rec(gate);
    float step = r.height / type->outputs;
    return (Vector2){r.x+r.width, r.y + (i+0.5)*step};
}

void gate_draw(VisGate *gate, bool prewiew){
    GateType *type = &gate_types[gate->type_tag];
    Rectangle r = gate_rec(gate);
    DrawRectangleRec(r, type->color);
    switch(gate->type_tag){
        case T_DISP:{
            DrawRectangleRec(pad(r, -5), BG2);
            bool on = 0;
            if(!prewiew){
                bool *kabel = gate_inputs(gate)[0];
                on = kabel ? *kabel : 0;
            }
            char* text = on ? "1" : "0";
            Font f = GetFontDefault();
            float size = 30;
            float spacing = 3;
            Vector2 t_size = MeasureTextEx(f, text, size, spacing);
            Vector2 t_pos = {
                r.x + (r.width - t_size.x)*0.5,
                r.y + (r.height - t_size.y)*0.5,
            };
            DrawTextEx(f, text, t_pos, size, spacing, on ? WIRE_ON : WIRE_OFF);
        }break;
        case T_SWCH:{
            bool on = 0;
            if(!prewiew){
                bool *kabel = gate_outputs(gate)[0];
                on = kabel ? *kabel : 0;
            }
            DrawRectangleRec(pad(r, -5), BG2);
            Rectangle lever = pad(r, -20);
            DrawRectangleRec(pad(r, -15), on ? WIRE_ON : WIRE_OFF);
            lever.height += 18;
            if(!on) lever.y -= 18;
            DrawRectangleRec(lever, GRAY);
        }break;
        default:{
            Font f = GetFontDefault();
            float size = 20;
            float spacing = 3;
            Vector2 t_size = MeasureTextEx(f, type->name, size, spacing);
            Vector2 t_pos = {
                r.x + (r.width - t_size.x)*0.5,
                r.y + (r.height - t_size.y)*0.5,
            };
            DrawTextEx(f, type->name, t_pos, size, spacing, WHITE);
        }break;
    }
}
void gate_draw_ports(VisGate *gate){
    GateType *type = &gate_types[gate->type_tag];
    for (int i = 0; i < type->inputs; i++) {
        DrawCircleV(gate_inport_pos(gate, i), PORT_RADIUS, GRAY);
    }
    for (int i = 0; i < type->outputs; i++) {
        DrawCircleV(gate_outport_pos(gate, i), PORT_RADIUS, GRAY);
    }
}


typedef struct{
    Vector2 pos;
    bool flip;
}WirePoint;

DecDynArr(WirePoint);

typedef struct{
    ArrOfWirePoint path;
    int in_port, out_port;
    VisGate *in_gate, *out_gate;
    bool* kabel;
    bool wire_in;
    int color_i;
}VisWire;

DecDynArr(VisWire);

typedef struct {
  VisGate **ptr;
  size_t len;
  size_t cap;
} Gates;

DecDynArr(Color);

typedef struct{
    Camera2D cam;
    Gates gates;
    ArrOfVisWire wires;
    EDIT_MODE edit_mode;
    int gate_i;
    int port_i;
    bool wire_in;
    ArrOfWirePoint wire_path;
    int selected_gate_type;
    int selected_wire_color;
    KSim *sim;
    bool *dummy_kabel;
    ArrOfColor wire_palette;
} State;

ArrOfColor wire_palette;

void calc_wire_palette(){
    ArrOfColor *p = &wire_palette;
    p->len = 0;
    #define P(a, b) \
        apush(p, (a)); apush(p, (b));

    P(RED, ColorBrightness(RED, -0.5))
    P(GREEN, ColorBrightness(GREEN, -0.5))

    #undef P
}
Color wire_color(bool on, int variant){
    int n = wire_palette.len/2;
    return wire_palette.ptr[(variant%n)*2 + !on];
}

void delte_gate(State *state, int i){
    VisGate *gate = aget(&state->gates, i);

    aremove(&state->gates, i);
    GateType *t = &gate_types[gate->type_tag];
    void** out_kabels = gate_outputs(gate);
    for(int i = 0; i < t->outputs; i++){
        if(out_kabels[i]){
            // TODO delete kabel?
            // there may be references to it
        }
    }
    for (int i = 0; i < state->wires.len; i++) {
        VisWire *w = aget_ptr(&state->wires, i);
        if(w->out_gate == gate || w->in_gate == gate){
            gate_inputs(w->in_gate)[w->in_port] = state->dummy_kabel;
            free(w->path.ptr);
            aremove(&state->wires, i);
            i--;
        }
    }
    free(gate);
}

void rebuild_sim(State *state){
    state->sim->todolist[0].len = 0;
    state->sim->todolist[1].len = 0;
    for (size_t i = 0; i < state->gates.len; i++) {
        VisGate *g = aget(&state->gates, i);
        uint32_t out_count = gate_types[g->type_tag].outputs;
        void** out_kabels = gate_outputs(g);
        for (uint32_t j = 0; j < out_count; j++) {
            if(out_kabels[j]){
                KabelHdr *h = kabel_hdr(out_kabels[j]);
                h->on_change.len = 0;
            }else{
                out_kabels[j] = new_kabel(state->sim, bool);
            }
        }
        uint32_t in_count = gate_types[g->type_tag].inputs;
        void** in_kabels = gate_inputs(g);
        for (uint32_t j = 0; j < in_count; j++) {
            if(!in_kabels[j]){
                in_kabels[j] = state->dummy_kabel;
            }
        }
    }
    for (size_t i = 0; i < state->gates.len; i++) {
        VisGate *g = aget(&state->gates, i);
        GateType *t = &gate_types[g->type_tag];
        uint32_t out_count = t->outputs;
        void** out_kabels = gate_outputs(g);
        uint32_t in_count = t->inputs;
        void** in_kabels = gate_inputs(g);
        t->build(state->sim, in_kabels, out_kabels);
        for (uint32_t i = 0; i < out_count; i++) {
            void* kabel = out_kabels[i];
            assert(kabel);
            ksim_schedule_update(state->sim, kabel_hdr(kabel));
        }
    }
}

void before_reload(State* state){
    state->wire_palette = wire_palette;
}
void after_reload(State* state){
    SetExitKey(KEY_NULL);
    wire_palette = state->wire_palette;
    calc_wire_palette();
    rebuild_sim(state);
}

void drag_cam(Camera2D *cam){
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f/cam->zoom);

        cam->target = Vector2Add(cam->target, delta);
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0 && IsKeyDown(KEY_LEFT_SHIFT)){
        Vector2 mouseWorldPos = 
            GetScreenToWorld2D(GetMousePosition(), *cam);
        cam->offset = GetMousePosition();

        cam->target = mouseWorldPos;

        // Zoom increment
        const float zoomIncrement = 0.05f;

        cam->zoom *= 1 + (wheel*zoomIncrement);
        if (cam->zoom < zoomIncrement) 
            cam->zoom = zoomIncrement;
    }
}

void DrawWire(Vector2 from, WirePoint to, Color c){
    Vector2 a = from;
    Vector2 b = to.pos;
    if(to.flip){
        DrawLineEx(a, (Vector2){a.x, b.y}, WIRE_THICK, c);
        DrawLineEx(b, (Vector2){a.x, b.y}, WIRE_THICK, c);
    }else{
        DrawLineEx(a, (Vector2){b.x, a.y}, WIRE_THICK, c);
        DrawLineEx(b, (Vector2){b.x, a.y}, WIRE_THICK, c);
    }
}
void DrawWirePath(ArrOfWirePoint *path, Color c){
    if(path->len == 0) return;
    Vector2 last = aget(path, 0).pos;
    for (int i = 1; i < path->len; i++) {
        DrawWire(last, aget(path, i), c);
        last = aget(path, i).pos;
    }
}

void update(State* state){
    uint32_t screen_width = GetScreenWidth();
    uint32_t screen_height = GetScreenHeight();

    for (int i = 0; i < 200; i++) {
        if(!ksim_step(state->sim)){
            goto sim_end;
        }
    }
    fprintf(stderr, "sim limit reached\n");
    sim_end:;

    Rectangle side_panel = {
        0,
        0,
        screen_width*0.1,
        screen_height,
    };

    bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool click_r = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    Vector2 mpos = GetMousePosition();
    Vector2 mouseWorldPos = GetScreenToWorld2D(mpos, state->cam);

    //WASD
    {
        Camera2D *cam = &state->cam;
        Vector2 delta = {
            IsKeyDown(KEY_D) - IsKeyDown(KEY_A),
            IsKeyDown(KEY_S) - IsKeyDown(KEY_W),
        };
        delta = Vector2Scale(delta, 8.0f/cam->zoom);

        cam->target = Vector2Add(cam->target, delta);
    }
    // gate type selection
    {

        float wheel = GetMouseWheelMove() 
            * !IsKeyDown(KEY_LEFT_SHIFT)
            * !IsKeyDown(KEY_LEFT_CONTROL);
        state->selected_gate_type += 
            (IsKeyPr(KEY_DOWN) || wheel < 0)
            - (IsKeyPr(KEY_UP) || wheel > 0);
        state->selected_gate_type = 
            (state->selected_gate_type + ARRLEN(gate_types))
            % ARRLEN(gate_types);
    }
    // wire color selection
    {
        float wheel = GetMouseWheelMove() * IsKeyDown(KEY_LEFT_CONTROL);
        int s = state->selected_wire_color + 
            (IsKeyPr(KEY_RIGHT) || wheel < 0)
            - (IsKeyPr(KEY_LEFT) || wheel > 0);
        s = (s + (wire_palette.len/2)) % (wire_palette.len/2);
        state->selected_wire_color = s;
    }

    if(IsKeyPressed(KEY_ESCAPE)){
        state->edit_mode = E_NONE;
    }


    
    switch(state->edit_mode){
        case E_DRAG_GATE:{
            if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
                state->edit_mode = E_NONE;
                goto changed_mode;
            }else{
                VisGate *g = aget(&state->gates, state->gate_i);
                Vector2 delta = GetMouseDelta();
                delta = Vector2Scale(delta, 1.0f/state->cam.zoom);
                g->pos = Vector2Add(delta, g->pos);
            }

            // fix wire positions
            for (int i = 0; i < state->wires.len; i++) {
                VisWire *w = aget_ptr(&state->wires, i);
                Vector2 a = gate_inport_pos(w->in_gate, w->in_port);
                Vector2 b = gate_outport_pos(w->out_gate, w->out_port);
                if(w->wire_in){
                    aget_ptr(&w->path, 0)->pos = a;
                    alast_ptr(&w->path)->pos = b;
                }else{
                    aget_ptr(&w->path, 0)->pos = b;
                    alast_ptr(&w->path)->pos = a;
                }
            }
        }break;
        case E_WIRE:{

            if(click_r){
                state->edit_mode = E_NONE;
                click_r = 0;
                goto changed_mode;
            }
            if(click){
                for (int i = 0; i < state->gates.len; i++) {
                    VisGate *gate =  aget(&state->gates, i);
                    size_t cnt;
                    Vector2 (*get_pos)(VisGate*, int);
                    // we only care about the other type of port to connect
                    if(state->wire_in){
                        get_pos = gate_outport_pos;
                        cnt = gate_outport_count(gate);
                    }else{
                        get_pos = gate_inport_pos;
                        cnt = gate_inport_count(gate);
                    }
                    for(int j = 0; j < cnt; j++){
                        Vector2 pos = get_pos(gate, j);
                        if(CheckCollisionPointCircle(mouseWorldPos, pos, PORT_RADIUS)){
                            VisWire w = {0};
                            w.wire_in = state->wire_in;
                            if(state->wire_in){
                                w.in_gate = aget(&state->gates, state->gate_i);
                                w.in_port = state->port_i;
                                w.out_gate = aget(&state->gates, i);
                                w.out_port = j;
                            }else{
                                w.out_gate = aget(&state->gates, state->gate_i);
                                w.out_port = state->port_i;
                                w.in_gate = aget(&state->gates, i);
                                w.in_port = j;
                            }
                            w.path = state->wire_path;
                            w.color_i = state->selected_wire_color;

                            void* in_kabel = gate_inputs(w.in_gate)[w.in_port];
                            if(in_kabel && in_kabel != state->dummy_kabel){
                            }else{
                                void* out_kabel = gate_outputs(w.out_gate)[w.out_port];
                                if(!out_kabel){
                                    out_kabel = new_kabel(state->sim, bool);
                                    gate_outputs(w.out_gate)[w.out_port] = out_kabel;
                                }
                                gate_inputs(w.in_gate)[w.in_port] = out_kabel;
                                w.kabel = out_kabel;

                                rebuild_sim(state);

                                apush(&state->wires, w);
                            }
                            memset(&state->wire_path, 0, sizeof(state->wire_path));
                            state->edit_mode = E_NONE;
                            click = 0;
                            goto changed_mode;
                        }
                    }
                }
            }


            if(click){
                WirePoint p = alast(&state->wire_path);
                apush(&state->wire_path, p);
            }
            WirePoint *p = alast_ptr(&state->wire_path);
            p->flip = IsKeyDown(KEY_LEFT_SHIFT);
            p->pos = mouseWorldPos;

        }break;
        case E_NONE:{

            if(click){
                for (int i = 0; i < state->gates.len; i++) {
                    VisGate *gate =  aget(&state->gates, i);
                    for(int port_i = 0; port_i < gate_inport_count(gate); port_i++){
                        Vector2 pos = gate_inport_pos(gate, port_i);
                        if(CheckCollisionPointCircle(mouseWorldPos, pos, PORT_RADIUS)){
                            void* kabel = gate_inputs(gate)[port_i];
                            if(kabel && kabel != state->dummy_kabel){
                                gate_inputs(gate)[port_i] = state->dummy_kabel;
                                for(int k = 0; k < state->wires.len; k++){
                                    VisWire *w = aget_ptr(&state->wires, k);
                                    if(w->in_gate == gate && w->in_port == port_i){
                                        free(w->path.ptr);
                                        aremove(&state->wires, k);
                                        k--;
                                    }
                                }
                            }
                            state->edit_mode = E_WIRE;
                            state->gate_i = i;
                            state->port_i = port_i;
                            state->wire_in = 1;
                            state->wire_path.len = 0;
                            apush(&state->wire_path, ((WirePoint){pos, 0}));
                            apush(&state->wire_path, ((WirePoint){mouseWorldPos, 0}));
                            click = 0;
                            goto changed_mode;
                        }
                    }
                    for(int j = 0; j < gate_outport_count(gate); j++){
                        Vector2 pos = gate_outport_pos(gate, j);
                        if(CheckCollisionPointCircle(mouseWorldPos, pos, PORT_RADIUS)){
                            state->edit_mode = E_WIRE;
                            state->gate_i = i;
                            state->port_i = j;
                            state->wire_in = 0;
                            state->wire_path.len = 0;
                            apush(&state->wire_path, ((WirePoint){pos, 0}));
                            apush(&state->wire_path, ((WirePoint){mouseWorldPos, 0}));
                            click = 0;
                            goto changed_mode;
                        }
                    }
                }
            }

            for (int i = 0; i < state->gates.len; i++) {
                VisGate *gate =  aget(&state->gates, i);
                Rectangle r = gate_rec(gate);
                if(CheckCollisionPointRec(mouseWorldPos, r)){
                    if(click){
                        if(gate->type_tag == T_SWCH){
                            bool *kabel = gate_outputs(gate)[0];
                            bool data = !*kabel;
                            kabel_write_ptr(
                                state->sim, 
                                kabel,
                                &data
                            );
                            
                        }
                        state->edit_mode = E_DRAG_GATE;
                        state->gate_i = i;
                        click = 0;
                        goto changed_mode;
                    }
                    if(click_r){
                        delte_gate(state, i);
                        rebuild_sim(state);
                        click_r = 0;
                    }
                }
            }
            drag_cam(&state->cam);
            if(click_r){
                apush(&state->gates, new_gate(
                    state->selected_gate_type, 
                    mouseWorldPos)
                );
                rebuild_sim(state);
            }

        }break;
    }
    changed_mode:

    if(IsKeyDown(KEY_SPACE)){
        state->gates.len = 0;
        state->wires.len = 0;
        rebuild_sim(state);
    }

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode2D(state->cam);

    // Draw the 3d grid, rotated 90 degrees and centered around 0,0 
    // just so we have something in the XY plane
    //rlPushMatrix();
    //    rlTranslatef(0, 25*50, 0);
    //    rlRotatef(90, 1, 0, 0);
    //    DrawGrid(100, 50);
    //rlPopMatrix();

    for (int i = 0; i < state->gates.len; i++) {
        VisGate *gate =  aget(&state->gates, i);
        gate_draw(gate, 0);
    }

    if(state->edit_mode == E_WIRE){
        DrawWirePath(
            &state->wire_path, wire_color(0, state->selected_wire_color));
    }
    for (int i = 0; i < state->wires.len; i++) {
        VisWire *w = aget_ptr(&state->wires, i);
        bool on = *w->kabel;
        DrawWirePath(&w->path, wire_color(on, w->color_i));
    }

    for (int i = 0; i < state->gates.len; i++) {
        VisGate *gate =  aget(&state->gates, i);
        gate_draw_ports(gate);
    }

    EndMode2D();

    // side panel
    {
        DrawRectangleRec(side_panel, BG2);
        Camera2D cam = {0};
        cam.zoom = side_panel.width * 0.01;
        cam.offset = (Vector2){
            side_panel.x + side_panel.width*0.5,
            side_panel.y + side_panel.height*0.5,
        };
        BeginMode2D(cam);
        float y = 0;

        for (int i = 0; i < state->selected_gate_type; i++) {
            VisGate gate = {
                .type_tag = i,
                .pos = {0, y},
            };
            Rectangle r = gate_rec(&gate);
            y -= r.height + 20;
        }
        for (int i = 0; i < ARRLEN(gate_types); i++) {
            VisGate gate = {
                .type_tag = i,
                .pos = {0, y},
            };
            gate_draw(&gate, 1);
            gate_draw_ports(&gate);
            Rectangle r = gate_rec(&gate);
            if(i == state->selected_gate_type){
                DrawRectangleLinesEx(pad(r, 10), 2, WHITE);
            }
            y += r.height + 20;
        }
        EndMode2D();
    }

    EndDrawing();
}

State* init() {
    SetExitKey(KEY_NULL);
    State* state = calloc(8, 512); // should be enough
    if(!state){
        perror("buy more ram");
        exit(1);
    }

    state->sim = new_ksim();
    state->dummy_kabel = new_kabel(state->sim, bool);

    state->cam.zoom = 1;
    calc_wire_palette();

    return state;
}
