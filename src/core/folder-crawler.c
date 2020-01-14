#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

// Alumnos:
// Luis Molina
// Mariana Encalada

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
  const char *baseFormatedPath;
  const char *baseDirName;
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
  strncat(builtPath , separator, 1);
  strcat(builtPath, addedToPath);
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
      printf("[countSubDirectoriesAndFiles] Error al tratar de resevar memoria para formar nextPath\n");
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
    printf("[writeLineToDirectorios] Error al trata de escribir linea a archivo Directorios.txt\n");
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

int crawlFolders(DIR *currentDir, const struct base_values baseValues, const char *separator, 
int directoriosIndent, struct files file){
  if(currentDir == NULL){
    return NOT_A_DIR;
  }

  char *currentFormatedPath = malloc(strlen(baseValues.baseFormatedPath) + 1);
  char *currentPath = malloc(strlen(baseValues.basePath) + 1);
  char *currentDirName = malloc(strlen(baseValues.baseDirName) + 1);
  strcpy(currentFormatedPath, baseValues.baseFormatedPath);
  strcpy(currentPath, baseValues.basePath);
  strcpy(currentDirName, baseValues.baseDirName);

  printf("%s\n", currentFormatedPath);

  writeLineToRecorrido(currentDirName, file.recorrido);
  writeLineToDirectorios(currentDirName, file.directorios, currentPath, 
  separator, directoriosIndent);

  struct dirent *de;
  while ((de = readdir(currentDir)) != NULL) { // Lee contenidos de direcotorio actual
    if(isCurrentOrPreviousDir(de->d_name) == 1){
      continue;
    }

    char *nextPath = malloc(strlen(currentPath) + strlen(separator) + strlen(de->d_name) +1);
    if(nextPath == NULL){
      printf("[crawlFolders] Error al tratar de resevar memoria para formar nextPath\n");
      return MEM_ALLOCATION_ERROR;
    }
    buildPath(nextPath, currentPath, separator, de->d_name);
    DIR *newBaseDir = opendir(nextPath);
    
    if (newBaseDir != NULL){
      // Entra y se llama recursivamente si el path corresponde a un directorio
      char *nextFormatedPath = malloc(strlen(currentFormatedPath) + strlen(separator) + 
      strlen(de->d_name) +1);
      if(nextFormatedPath == NULL){
        printf("[crawlFolders] Error al tratar de resevar memoria para formar nextFormatedPath\n");
        return MEM_ALLOCATION_ERROR;
      }
      buildPath(nextFormatedPath, currentFormatedPath, separator, de->d_name);
      struct base_values nextBaseValues = {nextFormatedPath, de->d_name, nextPath};
      int status = crawlFolders(newBaseDir, nextBaseValues, separator, 
      directoriosIndent + DIRECTORIOS_INDENT_SIZE , file);
      free(nextFormatedPath);

      printf("%s\n", currentFormatedPath);

      writeLineToRecorrido(currentDirName, file.recorrido);

      if (status != 0){
        return status;
      }
    }else{
      writeLineToArchivos(de->d_name, currentDirName, nextPath, file.archivos);
    }
    free(nextPath);
  }
  free(currentFormatedPath);
  free(currentPath);
  free(currentDirName);
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

int initWithCurrentPath (struct base_values *values){
    char tmpBasePath[MAX_SIZE_INITIAL_PATH];
    if (getcwd(tmpBasePath, MAX_SIZE_INITIAL_PATH) != NULL) {
      fputs("\033c", stdout);
      values->baseFormatedPath = strrchr(tmpBasePath, DIR_SEPARATOR);
      values->baseDirName = values->baseFormatedPath + 1;
      values->basePath = CURRENT_DIR;
    } else {
      perror("getcwd() error");
      return GENERIC_ERROR;
    }

    return SUCCESS;
}

int setBaseValues(int argc, char* arg, struct base_values *values){
  int status = SUCCESS;
  if(argc == 1 ){
    status = initWithCurrentPath(values);
  } else if((strlen(arg) == 1 && strcmp(arg, CURRENT_DIR) == 0) || 
  (strlen(arg) == 2 && strcmp(arg, VALID_ARG_STARTER) == 0)){
    status = initWithCurrentPath(values);
  } else{
    char * currentPathStarter = malloc(VALID_ARG_STARTER_LENGTH + 1);
    char * firstCharPath = malloc(2);
    char delimiter[2] = "\0";
    delimiter[0] = DIR_SEPARATOR;
    if(currentPathStarter == NULL || firstCharPath == NULL){
      printf("[setBaseValues] Error al tratar de resevar memoria para formar inicio de path\n");
      return MEM_ALLOCATION_ERROR;
    }
    strncpy(currentPathStarter, arg, VALID_ARG_STARTER_LENGTH);
    strncpy(firstCharPath, arg, 1);

    if(strcmp(currentPathStarter, VALID_ARG_STARTER) != 0 && strcmp(firstCharPath, delimiter) != 0){
      // Se agrega caracter '/' al inicio de argumento dado  
      char *newBasePath = malloc(strlen(arg) + 1);
      if(newBasePath == NULL){
        printf("[setBaseValues] Error al tratar de resevar memoria para formar newBasePath\n");
        return MEM_ALLOCATION_ERROR;
      }   
      strcpy(newBasePath, "/");
      strcat(newBasePath, arg);
      values->basePath = newBasePath;
    }else{
      // arg empieza con './' o '/''
      values->basePath = arg;
    }

    values->baseFormatedPath = strrchr(values->basePath, DIR_SEPARATOR);
    values->baseDirName = values->baseFormatedPath + 1;
    free(currentPathStarter);
  }

  return status;
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
    printf("[createDirectoryAndFiles] Error al tratar de resevar memoria para formar path de archivos\n");
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
    printf("[createDirectoryAndFiles] Error al tratar de resevar memoria para formar firstLine\n");
    return MEM_ALLOCATION_ERROR;
  }
  sprintf(firstLine, FILES_FIRST_LINE_TEMPLATE, userEffectiveId, processId);

  // Escribe user id y process id en primera linea de cada archivo
  fputs(firstLine, file->recorrido);
  fputs(firstLine, file->directorios);
  fputs(firstLine, file->archivos);

  free(firstLine);
}

int curr_crawlFolders(DIR *currentDir, const struct base_values baseValues, struct files file){
  return crawlFolders(currentDir, baseValues, &DIR_SEPARATOR, 0,file);
}

int main(int argc, char *argv[]){
  int finalStatus = 0;

  finalStatus = validateArgLength(argc);
  if(finalStatus != 0){
    return finalStatus;
  }

  struct base_values baseValues;
  finalStatus = setBaseValues(argc, argv[1], &baseValues);
  if(finalStatus != 0){
    return finalStatus;
  }
  
  DIR *baseDir = opendir(baseValues.basePath);
  finalStatus = validateDirectory(baseDir);
  if(finalStatus != 0){
    return finalStatus;
  }

  struct files file = {NULL, NULL, NULL};
  finalStatus = createDirectoryAndFiles(&file);
  if(finalStatus != 0){
    return finalStatus;
  }

  finalStatus = curr_crawlFolders(baseDir, baseValues, file);

  return finalStatus;
}