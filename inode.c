#include "pignoufs.h"

// si size != 0 on copie un fichier
// à priori faudrait ajouter type dossier

pignoufs* get_superbloc(filesystem* fs)
{
    pignoufs* super = (pignoufs*)fs->superbloc->effectif;
    return super;
}

// à mon avis ça servira que si c'est trop grand
int alloc_iblocs(filesystem* fs, iinode* inode, int size)
{
    int nb_blocs = (size + 3999) / 4000;
    for(int i = 0; i < 900; i++){ inode->blocs[i] = -1; }
    for(int i = 0; i < nb_blocs && i < 900 ; i++)
    {
        inode->blocs[i] = alloc_bloc(fs, BLOC_DONNEE);
    }
    
    if(size > 3600000)
    {
        int indir_double_id = alloc_bloc(fs, BLOC_INDIRECTION_DOUBLE);
        inode->double_indirection_bloc = indir_double_id;
        bloc* indir_double = get_bloc(fs, indir_double_id);
        int32_t* double_table = (int32_t*)indir_double->effectif;
        for (int i = 0; i < 1000; i++) double_table[i] = -1;

        for(int i = 0; i < nb_blocs; i++)
        {
            int indir_simple_id = alloc_bloc(fs, BLOC_INDIRECTION_SIMPLE);
            bloc* bloc = get_bloc(fs, indir_simple_id);
            double_table[i] = indir_simple_id; // l'index

            int32_t* blocs_simple = (int32_t*)bloc->effectif;

            memset(bloc->effectif, -1, BLOC_SIZE);
            for(int j = 0; j < 1000; j++)
            {
                int bloc_donnee = alloc_bloc(fs, BLOC_DONNEE);
                blocs_simple[j] = bloc_donnee; // l'index encore
            }
        }
    }

    return 1;
}

int alloc_inode(filesystem* fs, const char* filename, int size, int dir)
{
    if(file_exists(fs, filename)) { return -1; }

    pignoufs* super = get_superbloc(fs);

    if (super == NULL) {
        printf("Superbloc non valide\n");
        return -1;
    }
    
    for(int i = 0; i < super->nb_i; i++)
    {
        bloc* inode_bloc = &fs->inodes[i];
        iinode* inode = (iinode*)inode_bloc->effectif;

        if (inode->flags == 0) // inode libre
        {
            bit_toone(&inode->flags, 0);
            bit_toone(&inode->flags, 1);
            bit_toone(&inode->flags, 2);
            if(dir){ bit_toone(&inode->flags, 5); }
            inode->size = size;

            strncpy(inode->filename, filename, sizeof(inode->filename) - 1);
            inode->filename[sizeof(inode->filename) - 1] = '\0';

            inode->created_date = time(NULL);
            inode->last_access = inode->created_date;
            inode->last_modification = inode->created_date;
            inode->dir_parent = -1;

            super->nb_f++; // et un fichier de +
            
            int resultat = alloc_iblocs(fs, inode, size);

            if(resultat) return i; // retourne l’index de l’inode alloué
        }
    }

    return -1; // rien
}

void update_inode(iinode* i, int modified)
{
    i->last_access = time(NULL);
    if(modified)
    {
        i->last_modification = time(NULL);
    }
}

int update_bloc(bloc* bloc)
{
    if(sha1_tester(bloc)){ return 1; }

    return 0;
}

int add_file_to_folder(filesystem* fs, const char* dir, const char* filename)
{  
    int dir_index = get_inode(fs, dir);
    iinode* folder = get_inode_at(fs, dir_index);

    if(!(folder->flags & (1 << 5)))
    {
        perror("Pas un dossier.\n");
        return 0;
    }

    for(int i = 0; i < 900; i++)
    {
        if(folder->blocs[i] != -1)
        {
            folder->blocs[i] = get_inode(fs, filename);
            iinode* inode = get_inode_at(fs, folder->blocs[i]);
            inode->dir_parent = dir_index;
            return 1;
        }
    }

    return 0; // trop de fichiers déjà flemme de mettre des indirections dossiers
}

iinode* get_inode_at(filesystem* fs, int i)
{
    return (iinode*)&fs->inodes[i].effectif;;
}

int get_inode(filesystem* fs, const char* filename)
{
    pignoufs* superbloc = get_superbloc(fs);    

    for(int i = 0; i < superbloc->nb_i; i++)
    {
        iinode* inode = (iinode*)fs->inodes[i].effectif;
        if(strcmp(inode->filename, filename) == 0)
        {
            return i;
        }
    }

    return -1;
}

int alloc_bloc(filesystem* fs, bloctype type) 
{
    pignoufs* super = get_superbloc(fs);
    unsigned char* bitmap = (unsigned char*)fs->bitmaps;

    for (int i = 0; i < super->nb_a; i++) {
        int bitmap_index = i / (BLOC_SIZE * 8);      //  bloc de bitmap
        int bit_index_block = i % (BLOC_SIZE * 8);
        int byte_index = bit_index_block / 8;
        int bit_index = bit_index_block % 8;

        unsigned char* cur_bitmap = bitmap + bitmap_index * BLOC_SIZE;

        if (((cur_bitmap[byte_index] >> bit_index) & 1) == 1) {
            cur_bitmap[byte_index] &= ~(1 << bit_index);

            super->nb_l--;
            fs->adresses[i].type = type;
            return i;
        }
    }

    return -1;
}

// bloc de 0 à nb_a
bloc* get_bloc(filesystem* fs, int index) 
{
    return (bloc*)((char*)fs->adresses + index * BLOC_SIZE);
}

int droit_lecture(iinode* inode)
{
    return inode->flags & PERM_READ;
}

int droit_ecriture(iinode* inode)
{
    return inode->flags & PERM_WRITE;
}

int file_exists(filesystem* fs, const char* filename)
{
    pignoufs* superbloc = get_superbloc(fs);
    for(int i = 0; i < superbloc->nb_i; i++)
    {
        iinode* inode = (iinode*)fs->inodes[i].effectif;
        if(inode->flags != 0 && strcmp(inode->filename, filename) == 0 && inode->dir_parent == -1) // il existe
        {
            return 1;
        }
    }

    return 0;
}