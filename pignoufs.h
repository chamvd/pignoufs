#ifndef PIGNOUFS_H
#define PIGNOUFS_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "utils.h"

#define SIZE_MAX_PARAM 24
#define BLOCS 900
#define BLOC_SIZE 4096
#define MAX_BLOCS 32000
#define MAX_INODES 100
#define BITMAP_SIZE (MAX_BLOCS / 8)

#define LOCK_READ (1 << 3)
#define LOCK_WRITE (1 << 4)

#define PERM_READ   (1 << 1)
#define PERM_WRITE  (1 << 2)

typedef enum {
    SUPERBLOC,
    BITMAP,
    INODE,
    BLOC_ALLOUABLE, // n'a pas à être effacé quand libéré
    BLOC_DONNEE,    // contient les données d'un fichier
    BLOC_INDIRECTION_SIMPLE,    // contient 1000 adresses de blocs
    BLOC_INDIRECTION_DOUBLE     // contient 1000 adresses de blocs ind simple
} bloctype;

// SUPERBLOC (bloc 1)
typedef struct {
    char magic[8];
    int32_t nb_b;
    int32_t nb_i;
    int32_t nb_a;
    int32_t nb_l;
    int32_t nb_f;
    int32_t nb_1; // nb_bitmaps en vrai
    uint32_t size;
    char zero[3964]; // j'ai réduit un peu pour avoir size et nb_bitmaps d'inscrits pour munmap
} pignoufs;

// juste un bloc
typedef struct {
    char effectif[4000];
    char hash[20];
    bloctype type;
    sem_t semaphore;
    char dispo[68];
} bloc;

// INODES (bloc 3)
typedef struct {
    uint32_t flags;
    uint32_t size;
    uint32_t created_date;
    uint32_t last_access;
    uint32_t last_modification;
    char filename[256];
    int32_t blocs[BLOCS]; // index des blocs (avant adresses mais flemme wow)
    int32_t double_indirection_bloc;
    int32_t dir_parent; // si dans un dossier sinon -1
    char extension[120];
} iinode;

typedef struct{
    bloc* superbloc;
    bloc* bitmaps;
    bloc* inodes;
    bloc* adresses;
} filesystem;

// FILESYSTEM
int start(int argc, char** argv);
void mkfs(int count_args, char** commands);
void* get_filesystem(const char* filesystem);
filesystem* init_filesystem(void* fs);

// COMMANDS
int ls(filesystem* fs, int argc, char** argv);
void print_lsarg(iinode* inode, int flags);
int cat(filesystem* fs, char* filename);

int cp(filesystem* fs, int argc, char** argv);
int copy_file_to_fs(filesystem* fs, const char* source, const char* out);
int copy_outie(filesystem* fs, const char* source, const char* out);
int copy_innie(filesystem* fs, const char* source, const char* out);

int df(filesystem* fs, int flags);

int mkdirr(filesystem* fs, const char* filename);
int touch(filesystem* fs, const char* filename);

int rm(filesystem* fs, const char* filename);
int rm_folder(filesystem* fs, const char* filename);

int chmodd(filesystem* fs, char* filename, char* arg);

int input(filesystem* fs, const char* filename);
int add(filesystem* fs, const char* source, const char* dest);


// INODES
void update_inode(iinode* inode, int modified);
int alloc_inode(filesystem* fs, const char* fichier, int size, int dir);
void free_inode(filesystem* fs, int index); // ou filename jsp
int get_inode(filesystem* fs, const char* filename);
iinode* get_inode_at(filesystem* fs, int i);
int check_filesystem_name(char** argv);
int get_inode_of(filesystem* fs, const char* filename);

// BlOC en soit INODE c'est pareil mais pas trop
int alloc_bloc(filesystem* fs, bloctype type);
bloc* get_bloc(filesystem* fs, int bloc);
void unuse_array_bloc(filesystem* fs, int32_t* adresses, int indirection);
// RESTE

pignoufs* get_superbloc(filesystem* fs);

void* sha1_tester(void* bloc);

void read_file(filesystem* fs, iinode* inode);

int file_exists(filesystem* fs, const char* filename);


// DOSSIER

int mv(filesystem* fs, const char* dir, const char* filename);


#endif // PIGNOUFS_H