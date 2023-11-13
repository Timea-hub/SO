#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>



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
        perror("Usage error: The binary require only one argument, which represent a path to a bitmap file");
        exit(EXIT_FAILURE);
    }
    const char bitmap_extenstion[5] = ".bmp";
    int flag = strlen(arguments[1]) - 4;
    for(int i=0; i < 4; i++)
        if (arguments[1][flag+i] != bitmap_extenstion[i]){
            perror("Usage error: the argument must a path of bitmap file");
            exit(EXIT_FAILURE);
        }
}

char* convert_uint_to_string(unsigned int data){
    // use 11 bytest cause 4294967295 that can fit in 4 bytes
    char* buffer = string_safe_alloc(11);
    sprintf(buffer, "%u", data);
    return buffer;
}

void add_statistical_record(int file, const char* promt, char* data){
    //use 5 more bytes for whitcespaces and punctionations
    char *buffer = string_safe_alloc(strlen(promt) + strlen(data) + 5);
    sprintf(buffer, "%s: %s\n", promt, data);
    system_function_error_wrapper("Function call error: 'write'",
        write(file, buffer, strlen(buffer))
    );
    free(buffer);
}


void store_file_dimensions(int in_file, int out_file){
   
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

    add_statistical_record(out_file, "inaltime", convert_uint_to_string(heigth));
    add_statistical_record(out_file, "lungime", convert_uint_to_string(width));
    add_statistical_record(out_file, "dimensiune", convert_uint_to_string(file_size_bytes));
}

char* parse_unix_time(time_t timestamp){
    struct tm *ts = localtime(&timestamp);
    char* time_rep = string_safe_alloc(11);
    sprintf(time_rep, "%d.%d.%d", ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
    return time_rep;
}

void inspect_file_permissions(int out_file, unsigned int file_mode){
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
    add_statistical_record(out_file, "drepturi de acces user", rights);

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
    add_statistical_record(out_file, "drepturi de acces grop", rights);

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
    add_statistical_record(out_file, "drepturi de acces altii", rights);
}

void store_file_information(int in_file, int out_file){
    struct stat file_information;
    system_function_error_wrapper("Function call error: 'fstat'",
        fstat(in_file, &file_information)
    );
    add_statistical_record(out_file, "identificatorul utilizatorului", convert_uint_to_string(file_information.st_uid));
    add_statistical_record(out_file, "contorul de legaturi", convert_uint_to_string(file_information.st_nlink));
    add_statistical_record(out_file, "timpul ultimei modificari", parse_unix_time(file_information.st_mtime));
    inspect_file_permissions(out_file, file_information.st_mode);
}

char* extract_name_from_path(char *path){
    int len = strlen(path);
    char* p_name = path + (len * CHAR_SIZE);
    while(len){
        if ((p_name - 1)[0] == '/')
            break;
        p_name -= CHAR_SIZE;
        len --;
    }
    return p_name;
}

void extract_statistics_from_bmp_file(char *file_path){
    int fr = open(file_path, O_RDONLY);
    system_function_error_wrapper("Function call error: 'open' input", fr);
    int fw = open("statistica.txt", O_CREAT | O_TRUNC | O_WRONLY | O_APPEND);
    system_function_error_wrapper("Function call error: 'open' output", fw);
    add_statistical_record(fw, "nume_fisier", extract_name_from_path(file_path));
    store_file_dimensions(fr, fw);
    store_file_information(fr, fw);
    system_function_error_wrapper("Function call error: 'close' input",
        close(fr)
    );
    system_function_error_wrapper("Function call error: 'close' ouput",
        close(fw)
    );
    
}
 

int main(int argc, char **argv){
    check_command_line_argument(argc, argv);
    extract_statistics_from_bmp_file(argv[1]);
    return 0;
}