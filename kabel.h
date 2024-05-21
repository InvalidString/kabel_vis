#include "dyn_arr.h"
#include <stdbool.h>
#include <stddef.h>

typedef unsigned char u8;

#define KABEL_PRIV
#ifdef KABEL_PRIV

typedef struct {
  struct KabelHdr_s **ptr;
  size_t len;
  size_t cap;
} TodoList;

struct KSim_s{
    TodoList todolist[2];
    int cur_list;
};

struct Update_s{
    void (*update)(struct KSim_s *sim, void* captured);
    void* captured;
};

typedef struct {
  struct Update_s *ptr;
  size_t len;
  size_t cap;
} ArrOfUpdate;

struct KabelHdr_s{
    size_t data_size;
    ArrOfUpdate on_change;
};



typedef struct Update_s Update;
typedef struct KSim_s KSim;
typedef struct KabelHdr_s KabelHdr;

#else

typedef struct{} KSim;

#endif /* ifdef KABEL_PRIV */

KSim* new_ksim();

// returns true if calling ksim_step again will change the state
bool ksim_step(KSim* sim);

void ksim_schedule_update(KSim *sim, KabelHdr *hdr);

#define new_kabel(sim, Type) ((Type*)new_kabel_impl(sim, sizeof(Type)))
void* new_kabel_impl(KSim *sim, size_t size);

KabelHdr* kabel_hdr(void* kabel);

void kabel_write_ptr(KSim *sim, void* kabel, void* data);

void b_not(bool *in, bool *out);
void b_wire(bool *in, bool *out);
void b_and(bool *a, bool *b, bool *out);
void b_or(bool *a, bool *b, bool *out);
void b_xor(bool *a, bool *b, bool *out);
