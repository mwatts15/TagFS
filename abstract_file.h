#ifndef ABSTRACT_FILE_H
#define ABSTRACT_FILE_H
#include <glib.h>
#include <stdint.h> /* for file_id_t */
#include <semaphore.h>

typedef uint64_t file_id_t;
#define MAX_FILE_NAME_LENGTH 256
#define FILE_ID_SEPARATOR "#"
#define FIS FILE_ID_SEPARATOR
#define FIS_LENGTH 1

typedef struct AbstractFile
{
    file_id_t id;
    /* The file name
       Previously stored in in the TagTable under the "name" tag but moved for
       easier access. File names don't have to be unique to the file. */
    char name[MAX_FILE_NAME_LENGTH];
    sem_t file_lock;
} AbstractFile;

void abstract_file_init (AbstractFile *f, const char *name);
void abstract_file_destroy (AbstractFile *f);
char *abstract_file_to_string (AbstractFile *f, char buffer[MAX_FILE_NAME_LENGTH]);
const char *abstract_file_get_name (AbstractFile *f);
void _set_name (AbstractFile *f, const char *new_name);
int file_id_cmp (AbstractFile *f1, AbstractFile *f2);
int file_name_cmp (AbstractFile *f1, AbstractFile *f2);
int file_name_str_cmp (AbstractFile *f, char *name);
file_id_t get_file_id (AbstractFile *f);
void set_file_id (AbstractFile *f, file_id_t);

#define set_name(_f,_n) _set_name((AbstractFile*)_f,_n)
#define abstract_file_lock(__f) sem_wait((&((AbstractFile*)__f))->file_lock)
#define abstract_file_unlock(__f) sem_post((&((AbstractFile*)__f))->file_lock)
#define lock_timed_out(__status) ((__status) == -1 && (errno == ETIMEDOUT))
#endif /* ABSTRACT_FILE_H */
