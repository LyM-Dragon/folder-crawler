#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

// static const int MAX_INITIAL_PATH_LENGTH = 2000;
static const char *DIR_SEPARATOR = "/";
static const char *CURRENT_DIR = ".";
static const char *PREVIOUS_DIR = "..";
static const char *OUTPUT_DIR_NAME = "Información";
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

int crawlFolders(DIR *baseDir, char *parentPath, char *separator){
  // printf("CRAWFOLDERS PASO 1\n");
  if(baseDir == NULL){
    return 3;
  }

  struct dirent *de;  // Pointer for directory entry 
  // printf("CRAWFOLDERS PASO 2\n");
  while ((de = readdir(baseDir)) != NULL) {
    // printf("CRAWFOLDERS PASO 3\n");
    if(isCurrentOrPreviousDir(de->d_name) == 1){
      continue;
    }
    // printf("CRAWFOLDERS PASO 4\n");
    char *fullPath = malloc(strlen(parentPath) + strlen(de->d_name) +1);
    strcpy(fullPath, parentPath);
    strcat(fullPath , separator);
    strcat(fullPath, de->d_name);
    printf("%s\n", fullPath);
    DIR *baseDir = opendir(de->d_name); // opendir() returns a pointer of DIR type.  
    if (baseDir != NULL){  // opendir returns NULL if couldn't open directory
      // printf("CRAWFOLDERS PASO 5\n");
      crawlFolders(baseDir, fullPath, separator);
      
    }
    free(fullPath);
    // printf("CRAWFOLDERS PASO 6\n");
  }
  // printf("CRAWFOLDERS PASO 7\n");
  return 0;
}

int main(int argc, char *argv[]){
  printf("PASO 1\n");
  char *basePath;
  char *separator = strcpy(separator, DIR_SEPARATOR);

  if(argc > 2){
    printf(
      "Se espera máximo un solo argumento con el path relativo o absoluto con el "
      "que se va a trabajar.\n");
    return 1;
  }

  printf("PASO 2\n");
  if(argc == 1 ){
    basePath = CURRENT_DIR;
  }else{
    basePath = argv[1];
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

  printf("PASO 5\n");
  int finalStatus = crawlFolders(baseDir, basePath, separator);

  return finalStatus;
}