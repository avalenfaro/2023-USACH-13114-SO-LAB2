/**
 * @file      coordinador.c
 * @author    Álvaro Valenzuela
 * @brief     Archivo coordinador que orquesta los llamados a las funciones de mapeo y reduce.
 * @version   0.1
 * @date      2023-05-05
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "coordinador.h"

#define ROW_LENGHT 1000 /* */
#define FILE_SIZE 9924

#define LECTURA 0
#define ESCRITURA 1

void create_process(int *pid)
{
  *pid = fork();
}

/**
 * @brief Captura los argumentos proporcionados por consola.
 *
 * @param argc
 * @param argv
 * @param Coordinador
 */
void get_flags(int argc, char const *argv[], Coordinador *c)
{
  int opt;
  c->verbose = 0;
  while ((opt = getopt(argc, (char *const *)argv, "i:c:n:d")) != -1)
  {
    switch (opt)
    {
    case 'i':
      c->nombre_archivo = optarg;
      break;
    case 'c':
      c->total_lineas = atoi(optarg);
      break;
    case 'd':
      c->verbose = 1;
      break;
    case 'n':
      c->n = atoi(optarg);
      break;
    case '?':
      printf("No existe el flag %c\n", optopt);
      break;
    default:
      abort();
    }
  }
}

/**
 * @brief Busca el archivo proporcionado en los argumentos para después leer sus filas.
 *
 * @param nombre_archivo
 * @throw File not found
 * @return FILE*
 */
FILE *read_file(char nombre_archivo[])
{
  FILE *fp = fopen((const char *)nombre_archivo, "r");
  int errnum;
  if (fp == NULL)
  {
    errnum = errno;
    printf("Error al abrir el archivo: %s\n", strerror(errnum));
    exit(-1);
  }

  return fp;
}

/**
 * @brief Busca un token en particular (palabra) en una fila de caracteres.
 * @warning La fila proporcionada en los argumentos debe estar limitada por ;
 * @param row
 * @param col_number
 * @return char*
 */
char *find_token(char *row, int col_number)
{
  char *row_copy = (char *)malloc(ROW_LENGHT);
  char *token;
  strcpy(row_copy, row);
  for (int i = 0; i < col_number; i++)
  {
    if (i == 0)
    {
      token = strtok(row_copy, ";");
      continue;
    }

    token = strtok(NULL, ";");
  }

  return token;
}

/**
 * @brief Lee las lineas de un archivo de texto
 *
 * @param fp            Archivo a leer
 * @param vehiculos     Arreglo vacio para almacenar valores
 * @param total_lineas  Total de lineas a leer del archivo
 * @warning Esta función NO se ha probado con valores de total_lineas < 0 y total_lineas > 10000.
 */
void read_lines(FILE *fp, Vehiculo *vehiculos, int total_lineas, int start, int end)
{
  char row[FILE_SIZE];
  int vehicle_idx = 0;
  for (int i = 0; fgets(row, FILE_SIZE, fp) != NULL; i++)
  {
    if (i == 0)
    {
      continue;
    }

    if (i > start && i <= end)
    {
      strcpy(vehiculos[vehicle_idx].grupo_vehiculo, find_token(row, 1));
      vehiculos[vehicle_idx].tasacion = atoi(find_token(row, 6));
      vehiculos[vehicle_idx].valor_pagado = atoi(find_token(row, 11));
      vehiculos[vehicle_idx].puertas = atoi(find_token(row, 23));
      vehicle_idx++;
    }

    if (i > end)
    {
      break;
    }
  }
}

/**
 * @brief Función utilitaria para leer las n primeras filas de un arreglo de vehiculos.
 *
 * @param n         Numero de filas
 * @param vehiculos Arreglo de vehiculos
 */
void head_vehiculos(int n, Vehiculo *vehiculos)
{
  for (int i = 0; i < n; i++)
  {
    printf("grupo vehiculo: %s\n", vehiculos[i].grupo_vehiculo);
    printf("tasacion: %i\n", vehiculos[i].tasacion);
    printf("valor pagado: %i\n", vehiculos[i].valor_pagado);
    printf("puertas: %d\n", vehiculos[i].puertas);
    printf("--------------------\n");
  }
}

// /**
//  * @brief Función utilitaria para leer las n primeras filas de un arreglo de mapeos.
//  *
//  * @param n   Numero de filas
//  * @param map Arreglo de mapeos
//  */
// void head_mapeo(int n, Map map[])
// {
//   for (int i = 0; i < n; i++)
//   {
//     printf("grupo vehiculo: %.2f\n", map[i].vehiculo_liviano);
//     printf("carga: %.2f\n", map[i].carga);
//     printf("transporte publico: %.2f\n", map[i].transporte_publico);
//     printf("--------------------\n");
//   }
// }

void divide_array(int workers, int worker_number, int *chunk_position)
{
  int chunk_size = 1000 / workers;
  int start = worker_number * chunk_size;
  int end = start + chunk_size;

  chunk_position[0] = start;
  chunk_position[1] = end;
}

int main(int argc, char const *argv[])
{
  Coordinador coordinador;
  get_flags(argc, argv, &coordinador);
  int pipes[coordinador.n][2];

  for (int i = 0; i < coordinador.n; i++)
  {
    if (pipe(pipes[i]) < 0)
    {
      fprintf(stderr, "Error al crear el pipe %d\n", i);
      exit(1);
    }
  }

  for (int i = 0; i < coordinador.n; i++)
  {
    pid_t pid = fork();
    if (pid == 0)
    {
      Vehiculo *vehiculos = (Vehiculo *)malloc(sizeof(Vehiculo *) * coordinador.total_lineas);
      char *grupo = (char *)malloc(sizeof(char) * coordinador.total_lineas);
      close(pipes[i][ESCRITURA]);
      dup2(pipes[i][LECTURA], STDIN_FILENO);
      char *argv[] = {NULL};
      char buffer[64];
      snprintf(buffer, sizeof buffer, "%d", i);
      char *envp[] = {buffer, NULL};
      execve("./map", argv, envp);
      free(vehiculos);
      exit(0);
    }
    else if (pid < 0)
    {
      fprintf(stderr, "Error al crear el proceso hijo %d\n", i + 1);
      exit(1);
    }
  }

  int chunk[2];

  // Enviar datos a cada hijo a través de los pipes
  for (int i = 0; i < coordinador.n; i++)
  {
    Vehiculo *vehiculos = (Vehiculo *)malloc(sizeof(Vehiculo *) * coordinador.total_lineas);

    FILE *file = read_file(coordinador.nombre_archivo);
    divide_array(coordinador.n, i, chunk);
    read_lines(file, vehiculos, coordinador.total_lineas, chunk[0], chunk[1]);

    write(pipes[i][ESCRITURA], vehiculos, sizeof(Vehiculo) * coordinador.total_lineas);
    close(pipes[i][ESCRITURA]); // Cerrar el extremo de escritura del pipe en el padre

    free(vehiculos);
  }

  // Esperar a que todos los hijos terminen
  for (int i = 0; i < coordinador.n; i++)
  {
    wait(0);
  }

  // Map *tasaciones = map_tasaciones(vehiculos, coordinador.total_lineas);
  // Map *valor_pagado = map_valor_pagado(vehiculos, coordinador.total_lineas);
  // Map *puertas = map_puertas(vehiculos, coordinador.total_lineas);
  // reduce_tasacion(tasaciones, coordinador.verbose, coordinador.total_lineas);
  // reduce_valor_pagado(valor_pagado, coordinador.verbose, coordinador.total_lineas);
  // reduce_puertas(puertas, coordinador.verbose, coordinador.total_lineas);
  return 0;
}
