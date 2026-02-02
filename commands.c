#include "pignoufs.h"

void read_file(filesystem* fs, iinode* inode)
{
    for(int i = 0; i < 900; i++)
    {
        if(inode->blocs[i] == -1){ return; }
        bloc* data = get_bloc(fs, inode->blocs[i]);

        printf("%s", data->effectif);
    }

    if(inode->double_indirection_bloc > 0)
    {
        bloc* indirection_double = get_bloc(fs, inode->double_indirection_bloc);
        int32_t* addr_blocs = (int32_t*)indirection_double->effectif;

        for(int i = 0; i < 1000; i++)
        {
            if(addr_blocs[i] == -1){ return; }

            bloc* indirection_simple = get_bloc(fs, addr_blocs[i]);
            int32_t* addr_simple = (int32_t*)indirection_simple->effectif;

            for (int j = 0; j < 1000; j++) {
                if (addr_simple[j] == -1) break;

                bloc* data = get_bloc(fs, addr_simple[j]);
                printf("%s", data->effectif);
            }
        }
    }
}

int mkdirr(filesystem* fs, const char* filename)
{
    alloc_inode(fs, filename, 0, 1);

    return 0;
}

int touch(filesystem* fs, const char* filename)
{
    alloc_inode(fs, filename, 0, 0);
    return 1;
}

int cat(filesystem* fs, char* filename)
{

    pignoufs* superbloc = get_superbloc(fs);
    if (superbloc == NULL) {
        printf("Superbloc non valide\n");
        return -1;
    }
    
    int inode_id = get_inode(fs, filename);
    if(inode_id == -1){ 
        printf("Le fichier n'existe pas\n");
        return -1;
    }

    iinode* inode = get_inode_at(fs, inode_id);
    read_file(fs, inode);
    return 1;
}

int input(filesystem* fs, const char* filename)
{
    pignoufs* superbloc = get_superbloc(fs);
    if (superbloc == NULL) { return -1; }

    char buff[4000];
    printf("Entrez le contenu du fichier: ");
    fflush(stdout);
    int inode_id = alloc_inode(fs, filename, 0, 0);
    iinode* inode = get_inode_at(fs, inode_id);
    if (inode_id < 0) {
        return -1;
    }

    int i = 0;
    ssize_t bytes;
    while ((bytes = fread(buff, 1, 4000, stdin)) > 0) {
        inode->blocs[i] = alloc_bloc(fs, BLOC_DONNEE);
        bloc* bloc = get_bloc(fs, inode->blocs[i]);

        memcpy(bloc, buff, bytes);
        if (bytes < 4000) {
            memset(bloc->effectif + bytes, 0, 4000 - bytes);
            return 1;
        }
        
        i++;
    }

    return 1;
}

int ls(filesystem* fs, int argc, char** argv)
{
    ls_args args = parse_ls_args(argc, argv);
    pignoufs* superbloc = get_superbloc(fs);
    if(args.target)
    {
        for(int i = 0; i < superbloc->nb_i; i++)
        {
            iinode* inode = get_inode_at(fs, i);
            if(inode->flags != 0 && strcmp(inode->filename, args.target) == 0) // il existe
            {
                print_lsarg(inode, args.flag_l);
                free((void*)args.target);
                //update_inode(inode, 0);
                return 1;
            }
        }

        return 0; // il existe pas
    }

    for(int i = 0; i < superbloc->nb_i; i++)
    {
        iinode* inode = get_inode_at(fs, i);
        if(inode->flags != 0)
        {
            print_lsarg(inode, args.flag_l);
            //update_inode(inode, 0);
        }
    }
    
    return 1;
    // là pareil avec flag_t mais on va aussi add le tri mais flemme là
}

// go ternaire après, là flemme
void print_lsarg(iinode* inode, int flags)
{
    char droit_ec = (inode->flags & (1 << 1)) ? 'r' : '-';
    char droit_lec = (inode->flags & (1 << 2)) ? 'w' : '-';
    char type = (inode->flags & (1 << 5)) ? 'R' : 'F';
    
    if (flags) {
        printf("%c%c %d %d %s\n", type, droit_lec, droit_ec, inode->size, inode->filename);
    } else {
        printf("%s\n", inode->filename);
    }
}

int chmodd(filesystem* fs, char* filename, char* arg)
{
    pignoufs* superbloc = get_superbloc(fs);
    for(int i = 0; i < superbloc->nb_i; i++)
    {   
        iinode* inode = get_inode_at(fs, i);
        if(inode->flags != 0 && strcmp(inode->filename, filename) == 0) // il existe
        {
            if(strcmp(arg, "+r")){ bit_toone(&inode->flags, 1); return 1;}
            if(strcmp(arg, "-r")){ bit_tozero(&inode->flags, 1); return 1;}
            if(strcmp(arg, "+w")){ bit_toone(&inode->flags, 2); return 1;}
            if(strcmp(arg, "-r")){ bit_tozero(&inode->flags, 2); return 1;}

            return 1;
        }
    }

    return 0; // jsp je suppose ça a pas marché (le fichier existe pas ou arg bizarre?)
}
    
// flm de parse y a qu'un seul flag possible
int df(filesystem* fs, int flag)
{
    pignoufs* superbloc = get_superbloc(fs);
    switch(flag)
    {
        case 0:
            printf("Il y a %d blocs libres.\n", superbloc->nb_l);
            break;
        case 1:
            printf("Il y a %d inodes libres.\n", superbloc->nb_i - superbloc->nb_f);
            break;
        case 2:
            printf("Il y a %d octets libres.\n", superbloc->nb_l*4000);
            break;
        default:
            break;
    }

    return 1;
}

int copy_file_to_fs(filesystem* fs, const char* source, const char* out)
{
    int fdsource = open(source, O_RDONLY);
    if(fdsource == -1){ perror("open source file"); return -1; }

    off_t size = lseek(fdsource, 0, SEEK_END);
    if(size == -1 || size > 4003600000){ perror("size source file"); close(fdsource); return -1; }

    lseek(fdsource, 0, SEEK_SET);

    int nb_blocs = (size + 3999) / 4000;

    int inode_id = alloc_inode(fs, out, size, 0);

    if (inode_id < 0) {
        close(fdsource);
        return -1;
    }

    iinode* inode = get_inode_at(fs, inode_id);

    // lecture et copie dans les blocs directs
    for (int i = 0; i < nb_blocs && i < 900; i++) {
        int bloc_id = inode->blocs[i];
        bloc* b = get_bloc(fs, bloc_id);

        ssize_t bytes = read(fdsource, b->effectif, 4000);
        if (bytes < 4000) {
            memset(b->effectif + bytes, 0, 4000 - bytes);
            close(fdsource);
            break;
        }
    }

    //blocs indirects si nb_blocs > 900
    if (nb_blocs > 900) {
        bloc* indir_double = get_bloc(fs, inode->double_indirection_bloc);
        int32_t* table_double = (int32_t*)indir_double->effectif;

        int bloc_index = 900;

        for (int i = 0; i < 1000 && bloc_index < nb_blocs; i++) {
            if (table_double[i] == -1) break;

            bloc* indir_simple = get_bloc(fs, table_double[i]);
            int32_t* table_simple = (int32_t*)indir_simple->effectif;

            for (int j = 0; j < 1000 && bloc_index < nb_blocs; j++) {
                if (table_simple[j] == -1) break;

                bloc* b = get_bloc(fs, table_simple[j]);
                ssize_t bytes = read(fdsource, b->effectif, 4000);
                if (bytes < 4000) {
                    memset(b->effectif + bytes, 0, 4000 - bytes);
                    break;
                }

                bloc_index++;
            }
        }
    }

    close(fdsource);
    return 1;
}


int copy_outie(filesystem* fs, const char* source, const char* out)
{
    int fd = open(out, O_CREAT | O_RDWR | O_TRUNC, 0644);

    int inode_id = get_inode(fs, source);
    if(inode_id == -1){ return -1; }
    iinode* inode = get_inode_at(fs, inode_id);

    ssize_t written = 0;
    ssize_t reste = inode->size;

    // chelou le calcul à revoir !!! (revu à revoir au cas où)
    for(int i = 0; i < (int)inode->size/4000; i++) 
    {
        int32_t bloc_index = inode->blocs[i];
        if (bloc_index == -1) break; // vu qu'on fait 0XFF au début là
        
        bloc* b = get_bloc(fs, bloc_index);
        if(reste < 4000)
        {
            written += write(fd, b->effectif, reste);
            return 1;
        }

        written += write(fd, b->effectif, 4000);
        reste -= 4000;
    }   

    return 1; // pour l'instant
}

 // #SEVERANCE ;) 😂👌 out existe pas à priori et source est un fichier
int copy_innie(filesystem* fs, const char* source, const char* out)
{
    int inode_id = get_inode(fs, source);
    iinode* inode_source = get_inode_at(fs, inode_id);

    int inode_id_out = alloc_inode(fs, out, inode_source->size, 0);
    iinode* inode_out = get_inode_at(fs, inode_id_out);

    pignoufs* super = get_superbloc(fs);

    if(super == NULL){ printf("Superbloc non valide\n"); return -1; }
    //void read_file(iinode* inode)

    if(inode_source->double_indirection_bloc > 0)
    {
        bloc* indirection_double_source = get_bloc(fs, inode_source->double_indirection_bloc);
        bloc* indirection_double_out = get_bloc(fs, inode_out->double_indirection_bloc);

        int32_t* addr_blocs_source = (int32_t*)indirection_double_source->effectif;
        int32_t* addr_blocs_out = (int32_t*)indirection_double_out->effectif;

        for(int i = 0; i < 1000; i++)
        {
            if(addr_blocs_source[i] == -1){ break; }
            bloc* data_source = get_bloc(fs, addr_blocs_source[i]);
            bloc* data_out = get_bloc(fs, addr_blocs_out[i]);
            
            memcpy(data_out->effectif, data_source->effectif, 4000);
        }
    }

    for(int i = 0; i < BLOCS; i++)
    {
        bloc* bloc_source = get_bloc(fs, inode_source->blocs[i]);
        bloc* bloc_out = get_bloc(fs, inode_out->blocs[i]);

        memcpy(bloc_source->effectif, bloc_out->effectif, 4000);
    }

    return 1;
}

int cp(filesystem* fs, int argc, char** argv)
{
    cp_args args = parse_cp_args(argc, argv);
    
    printf("%d type et source %s et out %s\n", args.type, args.source, args.out);
    switch(args.type)
    {
        case 0:
            return copy_file_to_fs(fs, args.source, args.out);
        case 1:
            return copy_outie(fs, args.source, args.out);
        case 2:
            return copy_innie(fs, args.source, args.out);
        default:
            return -1;
    }
}

int rm(filesystem* fs, const char* filename)
{
    int inode_id = get_inode(fs, filename);
    if(inode_id == -1){ perror("Le fichier n'existe pas.\n"); return 0; }

    iinode* inode = get_inode_at(fs, inode_id);
    unuse_array_bloc(fs, inode->blocs, 0);
    
    if(inode->double_indirection_bloc != -1)
    {
        bloc* indir_double = get_bloc(fs, inode->double_indirection_bloc);
        int32_t* addr_blocs_ind = (int32_t*)indir_double->effectif;

        for(int i = 0; addr_blocs_ind[i] > 0 && i < 1000; i++)
        {
            bloc* indir_simple = get_bloc(fs, addr_blocs_ind[i]);
            int32_t* addr_blocs_simple = (int32_t*)indir_simple->effectif;

            unuse_array_bloc(fs, addr_blocs_simple, 1);
        }
    }

    return 1;
}

int rm_folder(filesystem* fs, const char* filename)
{
    int inode_id = get_inode(fs, filename);
    iinode* inode = get_inode_at(fs, inode_id);

    if(!(inode->flags & (1 << 5)))
    {
        perror("pas un dossier.\n");
        return 0;
    }

    for(int i = 0; i < 900; i++)
    {
        if(inode->blocs[i] == -1){ continue; }
        inode = get_inode_at(fs, inode->blocs[i]);

        const char* filename = inode->filename;
        rm(fs, filename);
    }

    rm(fs, filename);

    return 1;
}

void unuse_array_bloc(filesystem* fs, int32_t* adresses, int indirection)
{   
    int bloc_index, bitmap_index, bit_index_bloc, bit_index; //, byte_index

    int count = indirection ? 1000 : 900; // qu'on sache si c'est les blocs de l'inode ou pas
    for(int i = 0; i < count; i++)
    {
        if(adresses[i] == -1){ break; }
        bloc_index = adresses[i];
        bitmap_index = bloc_index / (BITMAP);
        bit_index_bloc = i % (BLOC_SIZE * 8);
        //byte_index = bit_index_bloc / 8;
        bit_index = bit_index_bloc % 8;

        unsigned char* cur_bitmap = (unsigned char*) fs->bitmaps[bitmap_index].effectif;
        deuse_bitmap(cur_bitmap, bit_index);
    }
}   

int mv(filesystem* fs, const char* dir, const char* filename)
{
    int dir_index = get_inode(fs, dir);
    iinode* folder = get_inode_at(fs, dir_index);

    if(!(folder->flags & (1 << 5)))
    {
        perror("Pas un dossier.\n");
        return 0;
    }

    int file = get_inode(fs, filename);
    iinode* inode = get_inode_at(fs, file);

    if(inode->dir_parent != -1)
    {
        iinode* parent = get_inode_at(fs, inode->dir_parent);
        for(int i = 0; i < 900; i++)
        {
            if(parent->blocs[i] == file)
            {
                parent->blocs[i] = -1;
                break;
            }
        }
    }

    for(int i = 0; i < 900; i++)
    {
        if(folder->blocs[i] == -1)
        {
            folder->blocs[i] = file;
            inode->dir_parent = dir_index;

            return 1;
        }
    }

    return 0; // plus de places
}

int add(filesystem* fs, const char* source, const char* dest) 
{
    int inode_dest = get_inode(fs, dest);
    if(inode_dest == -1){ 
        printf("Le fichier %s n'existe pas\n",dest);
        return -1;
    }
    iinode* iinode_dest = get_inode_at(fs, inode_dest);

    int fdsource = open(source, O_RDONLY);
    if(fdsource == -1){ perror("open source file"); return -1; }
    off_t size = lseek(fdsource, 0, SEEK_END);
    if(size == -1 || size > 4003600000 - iinode_dest->size){
        perror("size source file");
        close(fdsource); return -1;
    }
    lseek(fdsource, 0, SEEK_SET);
    
   
    for(int i = 0; i < 900; i++)
    {
        if(iinode_dest->blocs[i] == -1){
            
            iinode_dest->blocs[i] = alloc_bloc(fs, 0);
            bloc* b = get_bloc(fs, iinode_dest->blocs[i]);

            ssize_t bytes = read(fdsource, b->effectif, 4000);
            if (bytes < 4000) {
                memset(b->effectif + bytes, 0, 4000 - bytes);
                close(fdsource);
                break;
            }
        }
    }

    close(fdsource);

    return 1;
}

/*
peut-être ln



*/