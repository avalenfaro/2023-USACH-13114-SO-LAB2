typedef struct
{
  char *nombre_archivo;
  int total_lineas;
  int verbose;
  int n;
} Coordinador;

typedef struct
{
  char grupo_vehiculo[20];
  float tasacion;
  float valor_pagado;
  int puertas;
} Vehiculo;