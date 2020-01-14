#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>


static const int VALID_ARG_STARTER_LENGTH = 2;
static const int DIRECTORIOS_INDENT_SIZE = 4;
static const int MAX_EXTRA_SIZE_ARCHIVOS_LINE = 300;
static const int MAX_EXTRA_SIZE_DIRECTORIOS_LINE = 300;
static const int MAX_SIZE_INITIAL_PATH = 1000; 
static const char DIR_SEPARATOR = '/';
static const char *VALID_ARG_STARTER = "./";
static const char *CURRENT_DIR = ".";
static const char *PREVIOUS_DIR = "..";
static const char *OUTPUT_DIR_NAME = "Información";
static const char *RECORRIDO_FILE_NAME = "Recorrido.txt";
static const char *DIRECTORIOS_FILE_NAME = "Directorios.txt";
static const char *ARCHIVOS_FILE_NAME = "Archivos.txt";
static const char *FILES_FIRST_LINE_TEMPLATE = "id_usuario: %d id_programa: %d\n";
static const int SUCCESS = 0;
static const int GENERIC_ERROR = 1;
static const int TOO_MANY_ARGS = 2; 
static const int ARGS_ERROR = 3; 
static const int NOT_A_DIR = 4;
static const int CRAWL_ERROR = 5;
static const int MEM_ALLOCATION_ERROR = 6;

struct files {
  FILE *recorrido;
  FILE *directorios;
  FILE *archivos;
};
struct base_values {
  char *baseFormatedPath;
  char *baseDirName;
  const char *basePath;
};
struct quantity {
  int subDirQuantity;
  int filesQuantity;
};

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
    return GENERIC_ERROR;
  }
  return SUCCESS;
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
  // printf("[countSubDirectoriesAndFiles] PASO 1\n");
  int countSubDir = 0;
  int countFiles = 0;
  struct quantity subDirAndFiles = {0, 0};
  struct dirent *de;
  DIR *dir = opendir(currentPath);
  while ((de = readdir(dir)) != NULL) {
    if(isCurrentOrPreviousDir(de->d_name) == 1){
      continue;
    }
    // printf("[countSubDirectoriesAndFiles] PASO 2\n");
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
      printf("[countSubDirectoriesAndFiles] Error al tratar de resevar memoria para formar nextPath\n");
      break;
    }
  }
  closedir(dir);
  return subDirAndFiles;
}

void writeLineToDirectorios(const char *currentDirName, FILE *directorios, 
const char *currentPath, const char *separator, int indentation){
  // printf("[writeLineToDirectorios] PASO 1");
  struct quantity quantities = countSubDirectoriesAndFiles(currentPath, separator);
  // printf("[writeLineToDirectorios] PASO 2");
  char *direcotioriosTemplate = "Directorio %s contiene: %d carpetas y %d archivos";
  char *directoriosLine = malloc(indentation + strlen(direcotioriosTemplate) + MAX_EXTRA_SIZE_DIRECTORIOS_LINE);
  if(indentation > 0){
    memset(directoriosLine, ' ', indentation);
  }
  if(directoriosLine != NULL){
    sprintf(directoriosLine + indentation, direcotioriosTemplate, currentDirName,
    quantities.subDirQuantity, quantities.filesQuantity);
    fputs(directoriosLine, directorios);
    fputc('\n', directorios);
    free(directoriosLine);
  }else{
    printf("Error al trata de escribir linea a archivo Directorios.txt\n");
  }
}

void writeLineToArchivos(const char *fileName, const char *dirName, char *path ,FILE *archivos){
  char *lectura = "Lectura";
  char *escritura = "Escritura";
  char *ejecucion = "Ejecución";
  int extraChars = 5; //comas y espacios + /0
  char *lineTemplate = "%s ubicado en %s tiene permisos de: %s";
  int permissionsLength = strlen(lectura) + strlen(escritura) + strlen(ejecucion) + extraChars;
  char *permissions = malloc(permissionsLength);
  if (permissions == NULL){
    printf("[writeLineToArchivos] Error al intentar reservar memoria para char *permissions");
    return;
  }
  
  int finalLineLength = strlen(lineTemplate) + strlen(fileName) + strlen(dirName) + 
  permissionsLength + 1;
  char *finalLine = malloc(finalLineLength);
  if (finalLine == NULL){
    printf("[writeLineToArchivos] Error al intentar reservar memoria para char *finalLine");
    return;
  }

  struct stat fileStat;
  if(stat(path, &fileStat) == 0){
    if(fileStat.st_mode & S_IRUSR){
      strcpy(permissions, lectura);
    }
    if(fileStat.st_mode & S_IWUSR){
      strcat(permissions, ", ");
      strcat(permissions, escritura);
    }
    if(fileStat.st_mode & S_IXUSR){
      strcat(permissions, ", ");
      strcat(permissions, ejecucion);
    }

    sprintf(finalLine ,lineTemplate, fileName, dirName, permissions);
    fputs(finalLine, archivos);
    fputc('\n', archivos);
    free(finalLine);
    free(permissions);
  }
}

void writeLineToRecorrido( const char* currentDirName, FILE *recorrido){
  fputs(currentDirName, recorrido);
  fputc('\n', recorrido);
}

int crawlFolders(DIR *currentDir, struct base_values baseValues, const char *separator, 
int directoriosIndent, struct files file){
  if(currentDir == NULL){
    return NOT_A_DIR;
  }

  // printf("[crawlFolders] PASO 1\n");
  printf("%s\n", baseValues.baseFormatedPath);

  // printf("[crawlFolders] PASO 2\n");
  // printf("[crawlFolders] baseValues.baseDirName : %s\n", baseValues.baseDirName);
  writeLineToRecorrido(baseValues.baseDirName, file.recorrido);
  // printf("[crawlFolders] PASO 3\n");
  writeLineToDirectorios(baseValues.baseDirName, file.directorios, baseValues.basePath, 
  separator, directoriosIndent);

  struct dirent *de;
  while ((de = readdir(currentDir)) != NULL) { // Lee contenidos de direcotorio actual
    if(isCurrentOrPreviousDir(de->d_name) == 1){
      continue;
    }

    char *nextPath = malloc(strlen(baseValues.basePath) + strlen(separator) + strlen(de->d_name) +1);
    if(nextPath == NULL){
      printf("[crawlFolders] Error al tratar de resevar memoria para formar nextPath\n");
      return MEM_ALLOCATION_ERROR;
    }
    buildPath(nextPath, baseValues.basePath, separator, de->d_name);
    DIR *newBaseDir = opendir(nextPath);
    
    if (newBaseDir != NULL){
      // Entra y se llama recursivamente si el path corresponde a un directorio
      char *nextFormatedPath = malloc(strlen(baseValues.baseFormatedPath) + strlen(separator) + 
      strlen(de->d_name) +1);
      if(nextFormatedPath == NULL){
        printf("[crawlFolders] Error al tratar de resevar memoria para formar nextFormatedPath\n");
        return MEM_ALLOCATION_ERROR;
      }
      buildPath(nextFormatedPath, baseValues.baseFormatedPath, separator, de->d_name);
      struct base_values nextBaseValues = {nextFormatedPath, de->d_name, nextPath};
      int status = crawlFolders(newBaseDir, nextBaseValues, separator, 
      directoriosIndent + DIRECTORIOS_INDENT_SIZE , file);
      free(nextFormatedPath);

      printf("%s\n", baseValues.baseFormatedPath);

      writeLineToRecorrido(baseValues.baseDirName, file.recorrido);

      if (status != 0){
        return status;
      }
    }else{
      // printf("[crawlFolders] Escribe linea a Archivos.txt\n");
      writeLineToArchivos(de->d_name, baseValues.baseDirName, nextPath, file.archivos);
    }
    free(nextPath);
  }
  // printf("[crawlFolders] PASO 4\n");
  closedir(currentDir);
  return SUCCESS;
}

int validateArgLength(int argc){
  if(argc > 2){
    printf(
      "Se espera máximo un solo argumento con el path relativo o absoluto con el "
      "que se va a trabajar.\n");
    return TOO_MANY_ARGS;
  }

  return SUCCESS;
}

int setBaseValues(int argc, char* arg, struct base_values *values){
  char tmpBasePath[MAX_SIZE_INITIAL_PATH];
  if(argc == 1 ){
    printf("[setBaseValues] PASO 1\n");
    if (getcwd(tmpBasePath, MAX_SIZE_INITIAL_PATH) != NULL) {
      printf("Current working dir: %s\n", tmpBasePath);
      values->baseFormatedPath = strrchr(tmpBasePath, DIR_SEPARATOR);
      values->baseDirName = values->baseFormatedPath + 1;
      printf("Base dir name: %s\n", values->baseDirName);
      values->basePath = CURRENT_DIR;
    } else {
      perror("getcwd() error");
      return GENERIC_ERROR;
    }
  } else {
    printf("[setBaseValues] PASO 2\n");
    char * currentPathStarter = malloc(VALID_ARG_STARTER_LENGTH + 1);
    char * firstCharPath = malloc(2);
    if(currentPathStarter == NULL || firstCharPath == NULL){
      printf("[setBaseValues] Error al tratar de resevar memoria para formar inicio de path\n");
      return MEM_ALLOCATION_ERROR;
    }
    printf("[setBaseValues] PASO 3\n");
    strncpy(currentPathStarter, arg, VALID_ARG_STARTER_LENGTH);
    strncpy(firstCharPath, arg, 1);
    printf("[setBaseValues] firstCharPath : %s\n", firstCharPath);
    printf("[setBaseValues] current path starter %s\n", currentPathStarter);
    char delimiter[2] = "\0";
    delimiter[0] = DIR_SEPARATOR;
    if(strcmp(currentPathStarter, VALID_ARG_STARTER) != 0 && strcmp(firstCharPath, delimiter) != 0){
      printf("[setBaseValues] arg no empieza con './' ni con '/'\n");
      
      char *newBasePath = malloc(strlen(arg) + 1);
      if(newBasePath == NULL){
        printf("[setBaseValues] Error al tratar de resevar memoria para formar newBasePath\n");
        return MEM_ALLOCATION_ERROR;
      }   
      strcpy(newBasePath, "/");
      strcat(newBasePath, arg);
      values->basePath = newBasePath;
    }else{
      // es path absoluto?
      printf("[setBaseValues] empieza con './'\n");
      values->basePath = arg;
    }

    printf("[setBaseValues] basePath: %s\n", values->basePath);
    values->baseFormatedPath = strrchr(values->basePath, DIR_SEPARATOR);
    values->baseDirName = values->baseFormatedPath + 1;
    printf("[setBaseValues] values->baseFormatedPath : %s\n", values->baseFormatedPath);
    printf("[setBaseValues] values->baseDirName: %s\n", values->baseDirName);
    free(currentPathStarter);
  }

  printf("[setBaseValues] PASO 5\n");
  return SUCCESS;
}

int validateDirectory(DIR *dir){
  if (dir == NULL){ 
    printf(
      "El directorio base dado como argumento no se pudo abrir, verifique que:\n"
      "1. Ruta exista\n"
      "2. Ruta corresponda a un directorio\n"
      "3. La ruta sea absoluta o respete el formato de ruta relativa './example/hello'\n"); 
    return NOT_A_DIR; 
  }
  return SUCCESS;
}

int createDirectoryAndFiles(struct files *file){
  uid_t userEffectiveId = geteuid();
  pid_t processId = getpid();

  createDirectory(OUTPUT_DIR_NAME);

  // reserva memoria para paths de cada archivo
  char *recorridoPath = malloc(strlen(OUTPUT_DIR_NAME) + strlen(&DIR_SEPARATOR) + 
  strlen(RECORRIDO_FILE_NAME) +1);
  char *directoriosPath = malloc(strlen(OUTPUT_DIR_NAME) + strlen(&DIR_SEPARATOR) + 
  strlen(DIRECTORIOS_FILE_NAME) +1);
  char *archivosPath = malloc(strlen(OUTPUT_DIR_NAME) + strlen(&DIR_SEPARATOR) + 
  strlen(ARCHIVOS_FILE_NAME) +1);

  if(recorridoPath == NULL || directoriosPath == NULL || archivosPath == NULL){
    printf("[setBaseValues] Error al tratar de resevar memoria para formar path de archivos\n");
    return MEM_ALLOCATION_ERROR;
  }

  //construye path para cada archivo
  strcpy(recorridoPath, OUTPUT_DIR_NAME);
  strncat(recorridoPath, &DIR_SEPARATOR, 1);
  strcat(recorridoPath, RECORRIDO_FILE_NAME);
  strcpy(directoriosPath, OUTPUT_DIR_NAME);
  strncat(directoriosPath, &DIR_SEPARATOR, 1);
  strcat(directoriosPath, DIRECTORIOS_FILE_NAME);
  strcpy(archivosPath, OUTPUT_DIR_NAME);
  strncat(archivosPath, &DIR_SEPARATOR, 1);
  strcat(archivosPath, ARCHIVOS_FILE_NAME);

  file->recorrido = fopen(recorridoPath, "w");
  file->directorios = fopen(directoriosPath, "w");
  file->archivos = fopen(archivosPath, "w");

  free(recorridoPath);
  free(directoriosPath);
  free(archivosPath);

  // 12 para almacenar hasta max int
  int firstLineSize = 12 + 12 + strlen(FILES_FIRST_LINE_TEMPLATE) +1; 
  char *firstLine = malloc(firstLineSize);
  if(firstLine == NULL){
    printf("[setBaseValues] Error al tratar de resevar memoria para formar firstLine\n");
    return MEM_ALLOCATION_ERROR;
  }
  sprintf(firstLine, FILES_FIRST_LINE_TEMPLATE, userEffectiveId, processId);

  // Escribe user id y process id en primera linea de cada archivo
  fputs(firstLine, file->recorrido);
  fputs(firstLine, file->directorios);
  fputs(firstLine, file->archivos);

  free(firstLine);
}

int curr_crawlFolders(DIR *currentDir, struct base_values baseValues, struct files file){
  return crawlFolders(currentDir, baseValues, &DIR_SEPARATOR, 0,file);
}

int main(int argc, char *argv[]){
  int finalStatus = 0;
  char const *basePath;
  char const *baseFormatedPath;
  char const *baseDirName;

  printf("PASO 1\n");
  finalStatus = validateArgLength(argc);
  if(finalStatus != 0){
    return finalStatus;
  }

  printf("PASO 2\n");
  struct base_values baseValues;
  finalStatus = setBaseValues(argc, argv[1], &baseValues);
  if(finalStatus != 0){
    return finalStatus;
  }
  
  printf("PASO 3\n");
  printf("baseValues.basePath = %s\n", baseValues.basePath);
  printf("baseValues.baseDirName = %s\n", baseValues.baseDirName);
  printf("baseValues.baseFormatedPath = %s\n", baseValues.baseFormatedPath);
  DIR *baseDir = opendir(baseValues.basePath);
  finalStatus = validateDirectory(baseDir);
  if(finalStatus != 0){
    return finalStatus;
  }

  printf("PASO 4\n");
  struct files file = {NULL, NULL, NULL};
  finalStatus = createDirectoryAndFiles(&file);
  if(finalStatus != 0){
    return finalStatus;
  }

  printf("PASO 5\n");
  finalStatus = curr_crawlFolders(baseDir, baseValues, file);

  printf("PASO 6\n");
  printf("final status: %d\n", finalStatus);

  return finalStatus;
}