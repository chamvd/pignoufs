#include "pignoufs.h"

int start(int argc, char** argv)
{
    if(argc < 2){ return -1; } // à priori trop peu d'args
    
    char* command = argv[1];

    if(key_from_string(command) == 1)
    {
        mkfs(argc, argv);
        return 1;
    }// à priori c'estl e seul moment où on aura pas besoin de mmap
    
    void* fs = get_filesystem(argv[2]);
    if(!fs){ return 0; }

    int resultat = 0;
    filesystem* fss = init_filesystem(fs);
    
    switch(key_from_string(command))
    {
        case LS:
            resultat = ls(fss, argc, argv);
            break;
        case CAT: 
            resultat = cat(fss, argv[3]);
            break;
        case DF:
            int flag = 0;
            if(argc == 4)
            {
                if(strcmp(argv[3], "-i") == 0){ flag = 1; }
                else if(strcmp(argv[3], "-b") == 0){ flag = 2; }
                else{ return -1 ;}
            }
            resultat = df(fss, flag);
            break;
        case CP:
            resultat = cp(fss, argc, argv);
            break;
        case RM: 
            if(argc == 5){ 
                if(strcmp(argv[3], "-r") == 0){
                    resultat = rm_folder(fss, argv[4]);
                    break;
                }
                
                return -1;
            };

            resultat = rm(fss, argv[3]);
            break;
        case LOCK: break;
        case CHMOD:
            resultat = chmodd(fss, argv[3], argv[5]); break;
        case INPUT: 
            resultat = input(fss, argv[3]); break;
        case ADD: 
            resultat = add(fss, argv[3], argv[4]); break;

        case MKFS: // déjà géré en dehors du switch
        case INCONNU: 
            return -1; // flop
    }

    pignoufs* superbloc = get_superbloc(fss);
    if(resultat){ msync(fs, superbloc->size, MS_SYNC); }
    munmap(fs, superbloc->size);
    return 1; // jsp ça a marché i guess
}

void mkfs(int argc, char** argv)
{
    if (argc < 4) {
        fprintf(stderr, "%s mkfs\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* filename = argv[2];
    int nb_i = atoi(argv[3]);
    int nb_a = atoi(argv[4]);

    int nb_b = 0; int nb_1 = 0;
    int last_nb_1;

    do {
        last_nb_1 = nb_1;
        nb_b = 1 + nb_1 + nb_i + nb_a;
        nb_1 = (nb_b + 31999) / 32000;
    } while (nb_1 != last_nb_1);

    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) { perror("open"); exit(EXIT_FAILURE); }

    size_t taille = BLOC_SIZE * nb_b;

    if (ftruncate(fd, taille) == -1) {
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    void* fs = mmap(NULL, taille, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }

    filesystem fss;
    char* ptr = (char*)fs;

    fss.superbloc = (bloc*)ptr; ptr += BLOC_SIZE;
    fss.bitmaps    = (bloc*)ptr; ptr += nb_1 * BLOC_SIZE;
    fss.inodes    = (bloc*)ptr;        ptr += nb_i * BLOC_SIZE;
    fss.adresses = (bloc*)ptr;            ptr += nb_a * BLOC_SIZE;

    pignoufs* super = (pignoufs*)fss.superbloc->effectif;

    memset(fss.superbloc, 0, BLOC_SIZE);
    memcpy(super->magic, "pignoufs", 8);
 
    super->nb_b = nb_b;
    super->nb_i = nb_i;
    super->nb_a = nb_a;
    super->nb_l = nb_a;
    super->nb_f = 0;
    super->nb_1 = nb_1;
    super->size = taille;
    
    for(int i = 0; i < nb_1; i++)
    {
        memset(&fss.bitmaps[i], 0xFF, BLOC_SIZE); // tout libre 
        fss.bitmaps[i].type = BITMAP;    // mais type du bloc bitmap
    }

    for(int i = 0; i < nb_i; i++)
    {
        memset(&fss.inodes[i], 0, BLOC_SIZE);
        fss.inodes[i].type = INODE;
    }

    for (int i = 0; i < nb_a; i++) 
    {
        memset(&fss.adresses[i], 0, BLOC_SIZE);
        fss.adresses[i].type = BLOC_ALLOUABLE;
    }

    msync(fs, taille, MS_SYNC); 
    munmap(fs, taille);
    close(fd);
}

void* get_filesystem(const char* filesystem)
{
    int fd = open(filesystem, O_RDWR);
    if(fd == -1){ perror("open"); return NULL; }

    off_t size = lseek(fd, 0, SEEK_END);
    if(size == -1){ perror("lseek"); close(fd); return NULL; }

    void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED){ perror("mmap"); close(fd); return NULL; }

    close(fd);
    return addr;
}

filesystem* init_filesystem(void* fs)
{
    char* ptr = (char*)fs;

    filesystem* fss = malloc(sizeof(filesystem));
    if (!fss) { perror("malloc"); return NULL; }

    fss->superbloc = (bloc*)ptr;

    pignoufs* super = (pignoufs*)((bloc*)ptr)->effectif;
    
    if (memcmp(super->magic, "pignoufs", 8) != 0) {
        perror("Superbloc chelou\n");
        free(fss);
        return NULL;
    }

    ptr += BLOC_SIZE;

    fss->bitmaps  = (bloc*)ptr; ptr += super->nb_1 * BLOC_SIZE;
    fss->inodes   = (bloc*)ptr; ptr += super->nb_i * BLOC_SIZE;
    fss->adresses = (bloc*)ptr;

    return fss;    
}