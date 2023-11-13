#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

int fw;

void system_function_error_wrapper(char *error_message, int return_code){
    if(return_code < 0){
        perror(error_message);
        exit(EXIT_FAILURE);
    }
}

const int CHAR_SIZE = sizeof(char);
char* string_safe_alloc(int size){
    char *s = (char*)calloc(size, CHAR_SIZE);
    if(!s){
        perror("Fail to alloc");
        exit(EXIT_FAILURE);
    }
    return s;
}

void check_command_line_argument(int argument_counter, char **arguments){
    if (argument_counter != 2){
        perror("Usage error: The binary require only one argument, which represent a path to a directory");
        exit(EXIT_FAILURE);
    }

    system_function_error_wrapper("Usage error: The argument must be a path to a directory",
        chdir(arguments[1])
    );
}


int is_bit_map_file(char *file_name){
    const char bitmap_extenstion[5] = ".bmp";
    int flag = strlen(file_name) - 4;
    for(int i=0; i < 4; i++)
        if (file_name[flag+i] != bitmap_extenstion[i])
            return 0;
    return 1;
}

char* convert_uint_to_string(long data){
    char* buffer = string_safe_alloc(32);
    sprintf(buffer, "%lu", data);
    return buffer;
}

void add_statistical_record(const char* promt, char* data){
    //use 5 more bytes for whitcespaces and punctionations
    char *buffer = string_safe_alloc(strlen(promt) + strlen(data) + 5);
    sprintf(buffer, "%s: %s\n", promt, data);
    system_function_error_wrapper("Function call error: 'write'",
        write(fw, buffer, strlen(buffer))
    );
    free(buffer);
}


void store_file_dimensions(int in_file){
   
    // use 2 as offset, to skip signature
    system_function_error_wrapper("Function call error: 'lseek', at first call", 
        lseek(in_file, 2, SEEK_SET)
    );

    unsigned int file_size_bytes;
    system_function_error_wrapper("Function call error: 'read', at file size reading",
        read(in_file, &file_size_bytes, 4)
    );

    system_function_error_wrapper("Function call error: 'lseek', at second call", 
        lseek(in_file, 12, SEEK_CUR)
    );

    unsigned int width;
    system_function_error_wrapper("Function call error: 'read', at width reading",
        read(in_file, &width, 4)
    );

    unsigned int heigth;
    system_function_error_wrapper("Function call error: 'read', at heigth reading",
        read(in_file, &heigth, 4)
    );

    add_statistical_record("inaltime", convert_uint_to_string(heigth));
    add_statistical_record("lungime", convert_uint_to_string(width));
    add_statistical_record("dimensiune", convert_uint_to_string(file_size_bytes));
}

char* parse_unix_time(time_t timestamp){
    struct tm *ts = localtime(&timestamp);
    char* time_rep = string_safe_alloc(11);
    sprintf(time_rep, "%d.%d.%d", ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
    return time_rep;
}

void inspect_file_permissions(unsigned int file_mode){
    char rights[4] = "---\0";

    if((S_IRUSR & file_mode))
        rights[0] = 'R';
    else
        rights[0] = '-';
    if((S_IWUSR & file_mode))
        rights[1] = 'W';
    else
        rights[1] = '-';
    if((S_IXUSR & file_mode))
        rights[2] = 'X';
    else
        rights[2] = '-';
    add_statistical_record("drepturi de acces user", rights);

    if((S_IRGRP & file_mode))
        rights[0] = 'R';
    else
        rights[0] = '-';
    if((S_IWGRP & file_mode))
        rights[1] = 'W';
    else
        rights[1] = '-';
    if((S_IXGRP & file_mode))
        rights[2] = 'X';
    else
        rights[2] = '-';
    add_statistical_record("drepturi de acces grop", rights);

    if((S_IROTH & file_mode))
        rights[0] = 'R';
    else
        rights[0] = '-';
    if((S_IWOTH & file_mode))
        rights[1] = 'W';
    else
        rights[1] = '-';
    if((S_IXOTH & file_mode))
        rights[2] = 'X';
    else
        rights[2] = '-';
    add_statistical_record("drepturi de acces altii", rights);
}

void store_file_information_regular_files(struct stat file_information){
    add_statistical_record("identificatorul utilizatorului", convert_uint_to_string(file_information.st_uid));
    add_statistical_record("contorul de legaturi", convert_uint_to_string(file_information.st_nlink));
    add_statistical_record("timpul ultimei modificari", parse_unix_time(file_information.st_mtime));
    inspect_file_permissions(file_information.st_mode);
}

void extract_statistics_from_reqular_files(char *file_name, struct stat file_info){
    int fr = open(file_name, O_RDONLY);
    system_function_error_wrapper("Function call error: 'open' input", fr);
    
    add_statistical_record("nume fisier", file_name);
    if (is_bit_map_file(file_name))
        store_file_dimensions(fr);
    store_file_information_regular_files(file_info);

    system_function_error_wrapper("Function call error: 'close' input",
        close(fr)
    );
}

void extract_statistics_from_directories(char *file_name, struct stat file_info){
    add_statistical_record("nume director", file_name);
    add_statistical_record("identificatorul utilizatorului", convert_uint_to_string(file_info.st_uid));
    inspect_file_permissions(file_info.st_mode);
}

void extract_statistics_from_symbolic_links(char *file_name, struct stat file_info){
    struct stat target_file_info;
    system_function_error_wrapper("Function call error: 'stat'",
        stat(file_name, &target_file_info) 
    );

    if (!S_ISREG(target_file_info.st_mode))
        return;
    
    add_statistical_record("nume legatura", file_name);
    add_statistical_record("dimensiune", convert_uint_to_string(file_info.st_size));
    add_statistical_record("dimensiune fisier", convert_uint_to_string(target_file_info.st_size));
    inspect_file_permissions(file_info.st_mode);
}


void check_entry_type(char *entry_name){
    struct stat entry_info;
    system_function_error_wrapper("Function call error: 'lstat'",
        lstat(entry_name, &entry_info)
    );
    if(S_ISREG(entry_info.st_mode))
        extract_statistics_from_reqular_files(entry_name, entry_info);
    else if(S_ISDIR(entry_info.st_mode))
        extract_statistics_from_directories(entry_name, entry_info);
    else if(S_ISLNK(entry_info.st_mode))
        extract_statistics_from_symbolic_links(entry_name, entry_info);
}

int is_relative_path(char *name){
    if (name[0] != '.')
        return 0;
    if (name[1] == '\0')
        return 1;
    if (name[1] != '.')
        return 0;
    return name[2] == '\0' ? 1 : 0;
}

void iterate_directory(){
    DIR *current = opendir(".");
    if(!current){
        perror("Failed to open dir");
        exit(EXIT_FAILURE);
    }

    struct dirent *parser;
    while(1){
        errno = 0;
        parser = readdir(current);
        if (!parser) break;
        if (errno){
            perror("Function call error: 'readdir'");
            exit(EXIT_FAILURE);
        }
        if(is_relative_path(parser->d_name))
            continue;
        check_entry_type(parser->d_name);
        system_function_error_wrapper("Function call error: 'write' empty lines",
            write(fw, "\n\n", 2)
        );
    }
    system_function_error_wrapper("Function call error: 'closedir",
        closedir(current)
    );
}



int main(int argc, char **argv){
    fw = open("statistica.txt", O_CREAT | O_TRUNC | O_WRONLY | O_APPEND);
    system_function_error_wrapper("Function call error: 'open' output", fw);
    check_command_line_argument(argc, argv);
    iterate_directory();
    system_function_error_wrapper("Function call error: 'close' ouput",
        close(fw)
    );
    return 0;
}
