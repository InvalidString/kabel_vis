#include "dyn_arr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define KABEL_PRIV
#include "kabel.h"

#define dbg(f, x) fprintf(stderr, #x ": " f "\n", x);

void* alloc(size_t size){
    void* data = malloc(size);
    if(!data) {
        perror("alloc");
        abort();
    }
    return data;
}


KabelHdr* kabel_hdr(void* kabel){
    return (KabelHdr*)((u8*)kabel - sizeof(KabelHdr));
}

KSim* new_ksim(){
    KSim* out = alloc(sizeof(KSim));
    *out = (KSim){
        .cur_list = 0,
        .todolist = {0},
    };
    return out;
}

void ksim_schedule_update(KSim *sim, KabelHdr *hdr){
    TodoList *lst = &sim->todolist[sim->cur_list];
    // NOLINTBEGIN array of pointers needs to sizeof a pointer
    apush(lst, hdr);
    // NOLINTEND
}

bool ksim_step(KSim* sim){
    TodoList *lst = &sim->todolist[sim->cur_list];
    sim->cur_list = !sim->cur_list;
    for (size_t i = 0; i < lst->len; i++) {
        KabelHdr *h = aget(lst, i);
        for (size_t i = 0; i < h->on_change.len; i++) {
            Update u = aget(&h->on_change, i);
            u.update(sim, u.captured);
        }
    }
    lst->len = 0;
    return sim->todolist[sim->cur_list].len != 0;
}

void* new_kabel_impl(KSim *sim, size_t size){
    KabelHdr *hdr = alloc(sizeof(KabelHdr) + size);
    memset(hdr, 0, sizeof(*hdr));
    hdr->data_size = size;
    hdr->on_change = (ArrOfUpdate){0};
    void* kabel = (u8*)hdr + sizeof(KabelHdr);
    memset(kabel, 0, size);
    ksim_schedule_update(sim, hdr);
    return kabel;
}

void kabel_watch(void* kabel, Update fn){
    KabelHdr *header = kabel_hdr(kabel);
    apush(&header->on_change, fn);
}

void kabel_write_ptr(KSim *sim, void* kabel, void* data){
    KabelHdr *hdr = kabel_hdr(kabel);
    size_t size = hdr->data_size;
    if(memcmp(kabel, data, size)!=0){
        memcpy(kabel, data, size);
        bool found = 0;
        TodoList *lst = &sim->todolist[sim->cur_list];
        for (size_t i = 0; i < lst->len; i++) {
            if(aget(lst, i) == hdr){
                found = 1;
                break;
            }
        }
        if(!found){
            ksim_schedule_update(sim, hdr);
        }
    }
}


static void b_not_update(KSim *sim, void* cap){
    struct{bool *in; bool *out;} *data = cap;

    bool val = !*data->in;
    kabel_write_ptr(sim, data->out, &val);
}
void b_not(bool *in, bool *out){
    struct{bool *in; bool *out;} *data;
    data = malloc(sizeof(*data));
    data->in = in;
    data->out = out;
    kabel_watch(in, (Update){b_not_update, data});
}

static void b_wire_update(KSim *sim, void* cap){
    struct{bool *in; bool *out;} *data = cap;
    kabel_write_ptr(sim, data->out, data->in);
}
void b_wire(bool *in, bool *out){
    struct{bool *in; bool *out;} *data;
    data = malloc(sizeof(*data));
    data->in = in;
    data->out = out;
    kabel_watch(in, (Update){b_not_update, data});
}

#define GATE2(name, op) \
    static void b_ ## name ## _update(KSim *sim, void* cap){ \
        struct{bool *a; bool *b;  bool *out;} *data = cap; \
        bool val = (*data->a) op (*data->b); \
        kabel_write_ptr(sim, data->out, &val); \
    } \
    void b_ ## name (bool *a, bool *b, bool *out) { \
        struct{bool *a; bool *b;  bool *out;} *data; \
        data = malloc(sizeof(*data)); \
        data->a = a; \
        data->b = b; \
        data->out = out; \
        kabel_watch(a, (Update){b_ ## name ## _update, data}); \
        kabel_watch(b, (Update){b_ ## name ## _update, data}); \
    }

GATE2(and, &&)
GATE2(or, ||)
GATE2(xor, !=)
