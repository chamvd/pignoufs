#ifndef UTILS_H
#define UTILS_H

//commands key_from_string(const char *key);

typedef struct {
    int flag_l; // pour -l et -t respectivement
    int flag_t;
    const char* target; // NULL si pas spécifié
} ls_args;

typedef struct {
    char source[256];
    char out[256];
    int type; // ça sera 0, 1 et 2
} cp_args;

typedef enum {
    INCONNU = -1,
    MKFS = 1, LS, CAT,
    DF, CP, RM,
    LOCK, CHMOD, INPUT,
    ADD
} commands;

typedef struct {
    const char *key;
    commands val;
} s_command;

static const s_command looklook[] = {
    {"INCONNU", INCONNU},
    {"mkfs", MKFS},
    {"ls", LS},
    {"cat", CAT},
    {"df", DF},
    {"cp", CP},
    {"rm", RM},
    {"lock", LOCK},
    {"chmod", CHMOD},
    {"input", INPUT},
    {"add", ADD}
};

ls_args parse_ls_args(int argc, char** argv);
cp_args parse_cp_args(int argc, char** argv);

commands key_from_string(const char *key);

#define NKEYS (sizeof(looklook)/sizeof(s_command))

void bit_toone(uint32_t* flags, int bit);
void bit_tozero(uint32_t* flags, int bit);
void set_flags(uint32_t* flags, int bit, int index);
int bloc_used(unsigned char* bitmap, int index);
void use_bitmap(unsigned char* bitmap, int index);
void deuse_bitmap(unsigned char* bitmap, int index);

#endif // UTILS_H
