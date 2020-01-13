#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

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

int crawlFolders(DIR *baseDir, const char *currentPath, const char *currentFormatedPath, 
const char *separator, const char *currentDirName, FILE *recorrido){
  // printf("CRAWFOLDERS PASO 1\n");
  if(baseDir == NULL){
    return 3;
  }

  //escribe carpeta a archivo Recorrido.txt
  fputs(currentDirName, recorrido);
  fputc('\n', recorrido);

  printf("%s\n", currentFormatedPath);

  struct dirent *de;
  // printf("CRAWFOLDERS PASO 2\n");
  while ((de = readdir(baseDir)) != NULL) {
    // printf("CRAWFOLDERS PASO 3\n");
    if(isCurrentOrPreviousDir(de->d_name) == 1){
      continue;
    }
    // printf("CRAWFOLDERS PASO 4\n");
    char *nextPath = malloc(strlen(currentPath) + strlen(separator) + strlen(de->d_name) +1);
    buildPath(nextPath, currentPath, separator, de->d_name);
    DIR *newBaseDir = opendir(nextPath);
    
    if (newBaseDir != NULL){
      // printf("CRAWFOLDERS PASO 5\n");
      char *nextFormatedPath = malloc(strlen(currentFormatedPath) + strlen(separator) + strlen(de->d_name) +1);
      buildPath(nextFormatedPath, currentFormatedPath, separator, de->d_name);
      int status = crawlFolders(newBaseDir, nextPath, nextFormatedPath,separator, de->d_name, recorrido);
      free(nextPath);
      free(nextFormatedPath);
      printf("%s\n", currentFormatedPath);

      //escribe carpeta a archivo Recorrido.txt
      fputs(currentDirName, recorrido);
      fputc('\n', recorrido);

      if (status != 0){
        return status;
      }
    }else{
      // printf("IS DIR: %d\n", 0);
      // printf("%s\n", fullPath);
    }    
  }
  // printf("CRAWFOLDERS PASO 7\n");
  closedir(baseDir);
  return 0;
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
  int finalStatus = crawlFolders(baseDir, basePath, baseFormatedPath, &DIR_SEPARATOR, 
  baseDirName, recorrido);

  return finalStatus;
}