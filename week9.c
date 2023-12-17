#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <linux/limits.h>



int fw;
char ch_2_be_found;
int child_parent_pipe[2];
char *destiation_directory, *curent_path;
int child_process_counter;
int line_counter;

DIR *current;
void child_process_factory();

void system_function_error_wrapper(char *error_message, int return_code){ //tratam erorile care pot fi tratate de functiile standard din UNIX
    if(return_code < 0){
        perror(error_message);
        exit(EXIT_FAILURE);
    }
}

void open_pipe(int p[]){ //creaza pipe-ul
    system_function_error_wrapper("Failed to open pipe",
        pipe(p)
    );
}

void close_pipe_end(int pipe_id){ //inchide un capat din pipe
    system_function_error_wrapper("Function call error: 'close' on pipe end",
        close(pipe_id)
    );
}

void close_pipe_ends(int p[]){ //inchide intreb pipe-ul
    close_pipe_end(p[0]);
    close_pipe_end(p[1]);
}

void stdout_on_pipe(int p[]){ //redirecteaza standardul output catre capatul de iesire
    system_function_error_wrapper("Failed to redirect standard output on pipe",
        dup2(p[1], STDOUT_FILENO)
    );
}

void stdin_on_pipe(int p[]){ //redirecteaza standardul input catre capatul de citire
    system_function_error_wrapper("Failed to redirect standard input on pipe",
        dup2(p[0], STDIN_FILENO)
    );
}


void execute_sub_process(char *command){
    execl("/usr/bin/bash", "bash", "-c", command, (char*)NULL);
    perror("Fail to execute subprocess");
    exit(EXIT_FAILURE);
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
    if (argument_counter != 4){ //avem nevoie de 4 argumente, 3 argumente+executabilul
        perror("Usage error: The binary require only three arguments, the first two, should represent valid path to a directory, and the last one should be alphanumeric character");
        exit(EXIT_FAILURE);
    }
    curent_path = string_safe_alloc(PATH_MAX); //variabila globala
    if(!getcwd(curent_path, PATH_MAX)){        //path-ul din care executam programul
        perror("Function call error: 'getcwd' for current directory");
        exit(EXIT_FAILURE);
    }

    system_function_error_wrapper("Usage error: The second argument must be a path to a directory", //daca e o cale invalida, vom avea eroare, avem nevoie de un director. argumentul 0 - executabil, arg 1 - argument de intrare, arg 2 - argument de iesire, arg 3 - caracterul care trebuie cautat
        chdir(arguments[2]) //argument 2 de iesire
    );

    destiation_directory = string_safe_alloc(PATH_MAX); //var globala
    if(!getcwd(destiation_directory, PATH_MAX)){ //salvam calea catre directorul de output
        perror("Function call error: 'getcwd' for destination directory");
        exit(EXIT_FAILURE);
    }

    system_function_error_wrapper("Function call error: 'chdir' going back to current directory", //trebuie sa ne intoarcem in directorul de unde am executat programul
        chdir(curent_path)
    );

    system_function_error_wrapper("Usage error: The argument must be a path to a directory", //intram in directorul de intrare
        chdir(arguments[1])
    );

    if(strlen(arguments[3]) > 1 || !isalnum(arguments[3][0])){ //verificam ca este caracter alfanumeric
        perror("The last argument must be an alphanumeric character");
        exit(EXIT_FAILURE);
    }
    ch_2_be_found = arguments[3][0]; //salvam argumentul in ch_to_be_found
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
    line_counter++;
}



void store_file_dimensions(int in_file){
   
    // use 2 as offset, to skip signature
    system_function_error_wrapper("Function call error: 'lseek', at first call", 
        lseek(in_file, 2, SEEK_SET) //suntem la inceputul fisierului
    );

    unsigned int file_size_bytes;
    system_function_error_wrapper("Function call error: 'read', at file size reading",
        read(in_file, &file_size_bytes, 4) //cursorul se deplaseaza cu 4 biti ca sa citeasca file size-ul
    );

    system_function_error_wrapper("Function call error: 'lseek', at second call", 
        lseek(in_file, 12, SEEK_CUR) //pozitia curenta a cursorului
    );

    unsigned int width;
    system_function_error_wrapper("Function call error: 'read', at width reading",
        read(in_file, &width, 4) //citim latimea
    );

    unsigned int heigth;
    system_function_error_wrapper("Function call error: 'read', at heigth reading",
        read(in_file, &heigth, 4) //citim inaltimea
    );

    add_statistical_record("inaltime", convert_uint_to_string(heigth)); //scriem datele in statistica, convertindu-le
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

void store_file_information_regular_files(struct stat file_information){ //adaugam informatii legate de UID
    add_statistical_record("identificatorul utilizatorului", convert_uint_to_string(file_information.st_uid));
    add_statistical_record("contorul de legaturi", convert_uint_to_string(file_information.st_nlink)); //contorizam cate hard link-uri avem
    add_statistical_record("timpul ultimei modificari", parse_unix_time(file_information.st_mtime));
    inspect_file_permissions(file_information.st_mode); //verificam drepturile
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
    add_statistical_record("dimensiune", convert_uint_to_string(file_info.st_size)); //legatura simbolica
    add_statistical_record("dimensiune fisier", convert_uint_to_string(target_file_info.st_size)); //fisierul pointat
    inspect_file_permissions(file_info.st_mode);
}

int calculate_grey_intensity(int fd){
    unsigned int rgb_intensity;
    double grey_intensity=0;

    system_function_error_wrapper("Function call error: 'read', red intensity", 
        read(fd, &rgb_intensity, 1)
    );
    grey_intensity += 0.299 * rgb_intensity;

    system_function_error_wrapper("Function call error: 'read', green intensity", 
        read(fd, &rgb_intensity, 1)
    );
    grey_intensity += 0.587 * rgb_intensity;

    system_function_error_wrapper("Function call error: 'read', blue intensity", 
        read(fd, &rgb_intensity, 1)
    );
    grey_intensity += 0.114 * rgb_intensity;

    system_function_error_wrapper("Function call error: 'lseek, going back 3 bytes",
        lseek(fd, -3, SEEK_CUR)
    );

    // in order to round the number to the closest integer
    grey_intensity += 0.5;

    return (int)grey_intensity;
}


void discolor_bmp_file_process(char *entry_name){
    int fd, grey_intensity;
    unsigned int raster_data_offset, width, height;
    fd = open(entry_name, O_RDWR); //deschidem fisierul bmp
    system_function_error_wrapper("Function call error: 'open' in discolor process", fd);
    
    system_function_error_wrapper("Function call error: 'lseek' for data offset", 
        lseek(fd, 10, SEEK_SET)
    );
    system_function_error_wrapper("Function call error: 'read' for data offset",
        read(fd, &raster_data_offset, 4)
    );

    system_function_error_wrapper("Function call error: 'lseek' for width in discolor process", 
        lseek(fd, 4, SEEK_CUR)
    );
    system_function_error_wrapper("Function call error: 'read' for width in discolor process",
        read(fd, &width, 4)
    );
    system_function_error_wrapper("Function call error: 'read' for heigth in discolor process",
        read(fd, &height, 4)
    );

    system_function_error_wrapper("Function call error: 'lseek' going to raster data",
        lseek(fd, raster_data_offset, SEEK_SET)
    );

    long iterations = width * height;
    for(long i=0; i<iterations; i++){
        grey_intensity = calculate_grey_intensity(fd);
        for(int j=0; j<3; j++)
            system_function_error_wrapper("Function call error: 'write' writing grey bytes",
                write(fd, &grey_intensity, 1)
            );
    }
    exit(iterations);
}

void open_destination_file(char *file_name){
    char *destiation_file = string_safe_alloc(PATH_MAX);
    sprintf(destiation_file, "%s/%s_statistica.txt", destiation_directory, file_name); //creaza statistica (string) in destination file
    fw = open(destiation_file,  O_CREAT  | O_WRONLY | O_APPEND, 0777);
    system_function_error_wrapper("Function call error: 'open'", fw);
    free(destiation_file);
}

void close_destination_file(){
    system_function_error_wrapper("Function call error: 'close' output",
        close(fw)
    );
}

void create_entry_process_for_bmp_files(char *file_name, struct stat file_info){
    pid_t pid1, pid2;
    pid1 = fork(); //creaza un proces care extrage informatii despre un fisier regulat
    system_function_error_wrapper("Failed to create child process", pid1);
    child_process_counter++;
    if(!pid1){
        close_pipe_ends(child_parent_pipe);
        open_destination_file(file_name);
        extract_statistics_from_reqular_files(file_name, file_info);
        close_destination_file();
        exit(line_counter); //iesim don proces, returnand nr-ul de linii
    }else{
        pid2 = fork(); //mai creem un proces fiu pentur a decolora bmp-ul
        system_function_error_wrapper("Failed to create child process", pid2);
        child_process_counter++;
        if(!pid2){
            close_pipe_ends(child_parent_pipe); //inchidem pipe-ul de parinte-copil
            discolor_bmp_file_process(file_name);
        }else
            return child_process_factory();
    }
}

void create_entry_process_for_regular_files(char *file_name, struct stat file_info){
    pid_t pid1, pid2;
    int regular_files_pipe[2];
    open_pipe(regular_files_pipe);
    pid1 = fork();
    system_function_error_wrapper("Failed to create child process", pid1);
    child_process_counter++;
    if(!pid1){
        close_pipe_ends(child_parent_pipe);
        close_pipe_end(regular_files_pipe[0]); //inchidem capul de citire
        stdout_on_pipe(regular_files_pipe);
        open_destination_file(file_name);
        extract_statistics_from_reqular_files(file_name, file_info);
        close_destination_file();
        char command[PATH_MAX] = {0};
        sprintf(command, "/usr/bin/cat %s; exit %d", file_name, line_counter);
        execute_sub_process(command); //se va executa comanda cat - returneaza continutul fisierului in pipe
        exit(line_counter);
    }else{
        pid2 = fork();
        system_function_error_wrapper("Failed to create child process", pid2);
        child_process_counter++;
        if(!pid2){
            close_pipe_end(child_parent_pipe[0]);  //inchide capul dintre parinte si copil
            close_pipe_end(regular_files_pipe[1]); //inchidem capul de scriere
            stdin_on_pipe(regular_files_pipe); //redirectam intrarea sa citim de la pipe
            stdout_on_pipe(child_parent_pipe); //am redirectat scrierea de la iesirea standard de la parinte la copil catre pipe-ul global
            char command[128] = {0};
            sprintf(command, "source %s/propoziti.sh %c; count_corect_sentances",curent_path, ch_2_be_found);
            execute_sub_process(command);
        }else{
            close_pipe_ends(regular_files_pipe); //inchidem ambele capete de comunicare intre frati
            return child_process_factory();
        }
    }
}

void create_entry_process_for_directories(char *file_name, struct stat file_info){
    pid_t pid = fork();
    system_function_error_wrapper("Failed to create child process", pid);
    child_process_counter++;
    if(!pid){
        close_pipe_ends(child_parent_pipe);
        open_destination_file(file_name);
        extract_statistics_from_directories(file_name, file_info);
        close_destination_file();
        exit(line_counter);
    }else
        return child_process_factory();
}

void create_entry_process_for_links(char *file_name, struct stat file_info){
    pid_t pid = fork();
    system_function_error_wrapper("Failed to create child process", pid);
    child_process_counter++;
    if(!pid){
        close_pipe_ends(child_parent_pipe);
        open_destination_file(file_name);
        extract_statistics_from_symbolic_links(file_name, file_info);
        close_destination_file();
        exit(line_counter);
    }else
        return child_process_factory();
}


void create_entry_processes(char *entry_name){
    struct stat entry_info;
    system_function_error_wrapper("Function call error: 'lstat'",
        lstat(entry_name, &entry_info)
    );
    if(S_ISREG(entry_info.st_mode)) //verificam daca fisierul este regular
        if(is_bit_map_file(entry_name)) //luam numele fisierului si verificam daca e de tip .bmp
            return create_entry_process_for_bmp_files(entry_name, entry_info);
        else
            return create_entry_process_for_regular_files(entry_name, entry_info);   
    else if(S_ISDIR(entry_info.st_mode)) //verificam daca e un director
        return create_entry_process_for_directories(entry_name, entry_info);
    else if(S_ISLNK(entry_info.st_mode)) //verificam daca e legatura simbolica
        return create_entry_process_for_links(entry_name, entry_info);
    else
        return child_process_factory();
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

void count_correct_sentences(){
    close_pipe_end(child_parent_pipe[1]);//inchidem capul de scriere
    pid_t pid = fork();
    system_function_error_wrapper("Failed to create a child process", pid);
    if(!pid){
        stdin_on_pipe(child_parent_pipe); //redirectam intrarea standart, pentru ca astepta input din partea pipe-ului
        char command[128] = {0}; //proces fiu care aduna nr-ul de propozitii corecte
        sprintf(command, "source %s/propoziti.sh %c; accumulate_corect_sentances", curent_path, ch_2_be_found);
        execute_sub_process(command);
    }else{
        close_pipe_end(child_parent_pipe[0]);
        int status;
        system_function_error_wrapper("Function call error: 'waitpid'",
            waitpid(pid, &status, 0) //asteptam terminarea procesului fiu creat in aceasta functie
        );
    }

}

void child_process_factory(){
    errno = 0;
    struct dirent *entry = readdir(current);
    if(errno){
        perror("Function call error: 'readdir'");
        exit(EXIT_FAILURE);
    }
    if(!entry) return count_correct_sentences(); //am terminat de iterat directorul de intrare
    if(is_relative_path(entry->d_name)) //ne ajutam sa ignoram path-urile relative (directorul curent si directorul parinte)
        return child_process_factory(); //trecem la intrarea urmatoare
    else
        return create_entry_processes(entry->d_name); //cream un proces specific pentru intrare
}

void iterate_directory(){
    current = opendir("."); //var globala, assignata cu un pointer de tip dirent catre directorul de intrare
    open_pipe(child_parent_pipe); //deschidem pipe-ul dintre parinte si copii (doar pentru fisiere regulate)
    if(!current){
        perror("Failed to open dir"); //verificam daca e null
        exit(EXIT_FAILURE);
    }
    child_process_factory(); 
}


void wait_for_childs(){
    int status, pid;
    char *message = string_safe_alloc(256);
    for(int i=0; i<child_process_counter; i++){
        pid = wait(&status);
        system_function_error_wrapper("Function call error: 'wait'", pid);
        if(WIFEXITED(status)){
            sprintf(message, "S-a incheiat procesul cu pid-ul %d si codul %d\n", pid, WEXITSTATUS(status));
            system_function_error_wrapper("Function call error: 'write' into 'wait_for_childs'",
                write(STDOUT_FILENO, message, strlen(message)*sizeof(char))
            );
        }
    }
    free(message);
    free(curent_path);
    free(destiation_directory);
}


int main(int argc, char **argv){ 
    check_command_line_argument(argc, argv);
    iterate_directory();
    wait_for_childs();
    return 0;
}
