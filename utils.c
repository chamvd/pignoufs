#include "pignoufs.h"

ls_args parse_ls_args(int argc, char** argv) 
{
    ls_args args = {0, 0, NULL};

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            args.flag_l = 1;
        } else if (strcmp(argv[i], "-t") == 0) {
            args.flag_t = 1;
        } else if (strncmp(argv[i], "//", 2) == 0) {
            args.target = malloc(strlen(argv[i]) + 1);
            strcpy(args.target, argv[i]);
        }
    }

    return args;
}

cp_args parse_cp_args(int argc, char** argv)
{
    cp_args args;
    memset(&args, 0, sizeof(cp_args)); // copie entrante

    if(strncmp(argv[3], "//", 2) == 0){ args.type = 1; } // copie sortante

    if(strncmp(argv[4], "//", 2) == 0 && args.type == 1){ args.type = 2;} //copie interne

    strcpy(args.source, argv[3]);
    strcpy(args.out, argv[4]);

    return args;
}

void bit_toone(uint32_t* flags, int bit){ *flags |= (1 << bit); }
void bit_tozero(uint32_t* flags, int bit){ *flags &= ~(1 << bit); }

// bit pour si on passe de 0 ou 1, et index c'est juste l'endroit
void set_flags(uint32_t* flags, int bit, int index)
{
    if(bit){ bit_toone(flags, index); }
    else{ bit_tozero(flags, index); }
}

int bloc_used(unsigned char* bitmap, int index)
{
    int bitmap_index = index / 8;
    int bit = index % 8;

    return !((bitmap[bitmap_index] >> bit) & 1);
}

void use_bitmap(unsigned char* bitmap, int index)
{
    int bitmap_index = index / 8;
    int bit = index % 8;

    bitmap[bitmap_index] &= ~(1 << bit);
}

void deuse_bitmap(unsigned char* bitmap, int index)
{
    int bitmap_index = index / 8;
    int bit = index % 8;

    bitmap[bitmap_index] |= (1 << bit);
}

commands key_from_string(const char *key) {
    for (size_t i = 0; i < NKEYS; ++i) {
        if (strcmp(looklook[i].key, key) == 0) {
            return looklook[i].val;
        }
    }
    return INCONNU;
}

