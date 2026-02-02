#include "pignoufs.h"
#include "utils.h"

void lire_fichier(const char *filename) {
    int fd = open(filename, O_RDONLY);

    if (lseek(fd, 0, SEEK_SET) == -1) {
        close(fd);
        return;
    }

    unsigned char *buffer = (unsigned char *)malloc(BLOC_SIZE);
    ssize_t bytes_read = 0;
    while ((bytes_read = read(fd, buffer, BLOC_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            printf("%c", buffer[i]); // Affichage hexadécimal
        }
        printf("\n");
    }


    free(buffer);
    close(fd);
}

int main(int argc, char** argv) 
{
    //lire_fichier("fsname");
    return start(argc, argv);
}