#include "pignoufs.h"

volatile int hash_err = 0;
pthread_mutex_t err = PTHREAD_MUTEX_INITIALIZER;

void* sha1_tester(void* arg)
{
    bloc* b = (bloc*)arg;
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(b->effectif, 4000, hash);

    if (memcmp(hash, b->hash, SHA_DIGEST_LENGTH) != 0)
    {
        perror("hash invalide");
        pthread_mutex_lock(&err);
        hash_err = 1;
        pthread_mutex_unlock(&err);    
    }

    return NULL;
}

/*void catch_sigterm(int sig)
{
 
}

//verrouillage par bloc
void init_bloc_semaphore(bloc* bloc)
{
    if(sem_init(&(bloc->semaphore), 0, 1) != 0)
    {
        perror("err init sempahore");
    }
}

void lock_bloc(bloc* bloc)
{
    if(sem_wait(&bloc->semaphore) == -1)
    {
        perror("err lock bloc");
        exit(0);
    }
}

void unlock_bloc(bloc* bloc)
{
    if(sem_post(&bloc->semaphore) == -1)
    {
        perror("err unlock bloc");
        exit(0);
    }
}

// verrouillage par fichier test
int lock_read(iinode* inode)
{
    if(inode->flags & LOCK_WRITE) return 0;
    inode->flags |= LOCK_READ;
    return 1;
}

int lock_write(iinode* inode)
{
    if (inode->flags & (LOCK_READ | LOCK_WRITE)) return 0;
    inode->flags |= LOCK_WRITE;
    return 1;
}

void unlock_read(iinode* inode) {
    inode->flags &= ~LOCK_READ;
}

void unlock_write(iinode* inode) {
    inode->flags &= ~LOCK_WRITE;
}*/