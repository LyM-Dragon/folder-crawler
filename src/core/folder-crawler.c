#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

static const int DIRECTORIOS_INDENT_SIZE = 2; 
static const int MAX_SIZE_DIRECTORIOS_LINE = 1000; 
static const int MAX_SIZE_INITIAL_PATH = 1000; 
static const char DIR_SEPARATOR = '/';
static const char *CURRENT_DIR = ".";
static const char *PREVIOUS_DIR = "..";
static const char *OUTPUT_DIR_NAME = "Información";
static const char *RECORRIDO_FILE_NAME = "Recorrido.txt";
static const char *DIRECTORIOS_FILE_NAME = "Directorios.txt";
static const char *ARCHIVOS_FILE_NAME = "Archivos.txt";
static const int TOO_MANY_ARGS = 1;
static const int NOT_A_DIR = 2;
static const int CRAWL_ERROR = 3;
static const int MEM_ALLOCATION_ERROR = 4;

struct quantity {
  int subDirQuantity;
  int filesQuantity;
};

void prepend(char* s, const char* t)
{
    size_t len = strlen(t);
    memmove(s + len, s, strlen(s) + 1);
    memcpy(s, t, len);
}

void createDirectory(const char *dirName){
  struct stat st;
  if (stat(dirName, &st) == -1)
  {
    mkdir(dirName, 0700);
  }
}

int isCurrentOrPreviousDir(char *path){
  if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0)
  {
    return 1;
  }
  return 0;
}

char *buildPath(char *builtPath, const char *currentPath, const char *separator, char *addedToPath){
  strcpy(builtPath, currentPath);
  // printf("parentPath: %s\n", currentPath);
  strncat(builtPath , separator, 1);
  // printf("separator: %c\n", *separator);
  strcat(builtPath, addedToPath);
  // printf("de->d_name: %s\n", de->d_name);
}

struct quantity countSubDirectoriesAndFiles(const char *currentPath, const char *separator){
  int countSubDir = 0;
  int countFiles = 0;
  struct quantity subDirAndFiles = {0, 0};
  struct dirent *de;
  DIR *dir = opendir(currentPath);
  while ((de = readdir(dir)) != NULL) {
    if(isCurrentOrPreviousDir(de->d_name) == 1){
      continue;
    }
    struct stat sb;
    char *nextPath = malloc(strlen(currentPath) + strlen(separator) + strlen(de->d_name) +1);
    if(nextPath != NULL){
      buildPath(nextPath, currentPath, separator, de->d_name);
      if (stat(nextPath, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        subDirAndFiles.subDirQuantity++;
      } else if (stat(nextPath, &sb) == 0) {
        subDirAndFiles.filesQuantity++;
      }
      free(nextPath);
    } else {
      printf("[countSubDirectoriesAndFiles] Error al tratar de resevar memoria para formar nextPath");
      break;
    }
  }
  closedir(dir);
  return subDirAndFiles;
}

void writeLineToDirectorios(const char *currentDirName, FILE *directorios, 
const char *currentPath, const char *separator, int indentation){
  struct quantity quantities = countSubDirectoriesAndFiles(currentPath, separator);
  char *direcotioriosTemplate = "Directorio %s contiene: %d carpetas y %d archivos";
  char *directoriosLine = malloc(strlen(direcotioriosTemplate) + MAX_SIZE_DIRECTORIOS_LINE);
  char *spaces = malloc(indentation + 1);
  if(directoriosLine != NULL && spaces != NULL){
    sprintf(directoriosLine, direcotioriosTemplate, currentDirName,
    quantities.subDirQuantity, quantities.filesQuantity);
    memset(spaces, ' ', indentation);
    prepend(directoriosLine, spaces);
    fputs(directoriosLine, directorios);
    fputc('\n', directorios);
    free(directoriosLine);
  }else{
    printf("Error al trata de escribir linea a archivo Directorios.txt");
  }
}

void writeLineToRecorrido( const char* currentDirName, FILE *recorrido){
  fputs(currentDirName, recorrido);
  fputc('\n', recorrido);
}

int crawlFolders(DIR *currentDir, const char *currentPath, const char *currentFormatedPath, 
const char *currentDirName, const char *separator, int directoriosIndent, FILE *recorrido, 
FILE *directorios){
  printf("CRAWFOLDERS PASO 1\n");
  if(currentDir == NULL){
    return 3;
  }

  printf("%s\n", currentFormatedPath);

  writeLineToRecorrido(currentDirName, recorrido);
  writeLineToDirectorios(currentDirName, directorios, currentPath, separator, directoriosIndent);

  printf("CRAWFOLDERS PASO 2\n");
  struct dirent *de;
  while ((de = readdir(currentDir)) != NULL) {
    printf("CRAWFOLDERS PASO 3\n");
    if(isCurrentOrPreviousDir(de->d_name) == 1){
      continue;
    }
    printf("CRAWFOLDERS PASO 4\n");
    char *nextPath = malloc(strlen(currentPath) + strlen(separator) + strlen(de->d_name) +1);
    if(nextPath == NULL){
      printf("[crawlFolders] Error al tratar de resevar memoria para formar nextPath");
      return 4;
    }
    buildPath(nextPath, currentPath, separator, de->d_name);
    DIR *newBaseDir = opendir(nextPath);
    
    if (newBaseDir != NULL){
      printf("CRAWFOLDERS PASO 5\n");
      char *nextFormatedPath = malloc(strlen(currentFormatedPath) + strlen(separator) + strlen(de->d_name) +1);
      if(nextFormatedPath == NULL){
        printf("[crawlFolders] Error al tratar de resevar memoria para formar nextFormatedPath");
        return 4;
      }
      buildPath(nextFormatedPath, currentFormatedPath, separator, de->d_name);
      int status = crawlFolders(newBaseDir, nextPath, nextFormatedPath, de->d_name, separator, 
      directoriosIndent + DIRECTORIOS_INDENT_SIZE ,recorrido, directorios);
      free(nextPath);
      free(nextFormatedPath);
      printf("%s\n", currentFormatedPath);

      writeLineToRecorrido(currentDirName, recorrido);

      if (status != 0){
        return status;
      }
    }else{
    }
  }
  printf("CRAWFOLDERS PASO 7\n");
  closedir(currentDir);
  return 0;
}

int curr_crawlFolders(DIR *currentDir, const char *currentPath, const char *currentFormatedPath, 
const char *currentDirName, FILE *recorrido, FILE *directorios){
  return crawlFolders(currentDir, currentPath, currentFormatedPath, currentDirName, 
  &DIR_SEPARATOR, 0,recorrido, directorios);
}

int main(int argc, char *argv[]){
  printf("PASO 1\n");
  char const *basePath;
  char const *baseFormatedPath;
  char const *baseDirName;
  char tmpBasePath[MAX_SIZE_INITIAL_PATH];
  // char separator[1];
  // strcpy(separator, &DIR_SEPARATOR);
  
  if(argc > 2){
    printf(
      "Se espera máximo un solo argumento con el path relativo o absoluto con el "
      "que se va a trabajar.\n");
    return 1;
  }

  printf("PASO 2\n");
  if(argc == 1 ){
    if (getcwd(tmpBasePath, MAX_SIZE_INITIAL_PATH) != NULL) {
      printf("Current working dir: %s\n", tmpBasePath);
      baseFormatedPath = strrchr(tmpBasePath, DIR_SEPARATOR);
      baseDirName = baseFormatedPath + 1;
      printf("Base dir name: %s\n", baseDirName);
      basePath = CURRENT_DIR;
    } else {
    perror("getcwd() error");
    return 1;
    }
  } else {
    basePath = argv[1];
    baseFormatedPath = argv[1];
  }
  
  printf("PASO 3\n");
  DIR *baseDir;
  baseDir = opendir(basePath);
  if (baseDir == NULL){  // opendir returns NULL if couldn't open directory  
    printf(
      "El directorio base dado como argumento no se pudo abrir, verifique que exista "
      "y que corresponda a un directorio"); 
    return 2; 
  }

  printf("PASO 4\n");
  createDirectory(OUTPUT_DIR_NAME);
  char *recorridoPath = malloc(strlen(OUTPUT_DIR_NAME) + strlen(&DIR_SEPARATOR) + 
  strlen(RECORRIDO_FILE_NAME) +1);
  char *directoriosPath = malloc(strlen(OUTPUT_DIR_NAME) + strlen(&DIR_SEPARATOR) + 
  strlen(DIRECTORIOS_FILE_NAME) +1);
  char *archivosPath = malloc(strlen(OUTPUT_DIR_NAME) + strlen(&DIR_SEPARATOR) + 
  strlen(ARCHIVOS_FILE_NAME) +1);

  if(recorridoPath == NULL || directoriosPath == NULL || archivosPath == NULL){
    return 4;
  }

  strcpy(recorridoPath, OUTPUT_DIR_NAME);
  strncat(recorridoPath, &DIR_SEPARATOR, 1);
  strcat(recorridoPath, RECORRIDO_FILE_NAME);
  strcpy(directoriosPath, OUTPUT_DIR_NAME);
  strncat(directoriosPath, &DIR_SEPARATOR, 1);
  strcat(directoriosPath, DIRECTORIOS_FILE_NAME);
  strcpy(archivosPath, OUTPUT_DIR_NAME);
  strncat(archivosPath, &DIR_SEPARATOR, 1);
  strcat(archivosPath, ARCHIVOS_FILE_NAME);
  FILE *recorrido = fopen(recorridoPath, "w");
  FILE *directorios = fopen(directoriosPath, "w");
  FILE *archivos = fopen(archivosPath, "w");

  printf("PASO 5\n");
  int finalStatus = curr_crawlFolders(baseDir, basePath, baseFormatedPath, baseDirName,
   recorrido, directorios);

  printf("PASO 6\n");
  printf("final status: %d\n", finalStatus);

  return finalStatus;
}