
# Torneo Komikan Concurrente

Sistema concurrente en tiempo real para el juego Komikan en C++17 utilizando primitivas POSIX (`pthread`, `pthread_mutex_t`, `pthread_cond_t`, `sem_t`).

## Compilación

```bash
make torneo
```

## Ejecución

```bash
./torneo N M [config.txt]
```

- `N`: Número total de jugadores (`N >= 2`).
- `M`: Límite de mesas simultáneas (`M >= 1`).
- `config.txt`: Archivo de configuración opcional.

## Archivo de Configuración (`config.txt`)

Formato `clave = valor`:

- `min_delay_ms`: Retardo mínimo entre decisiones (ms).
- `max_delay_ms`: Retardo máximo entre decisiones (ms).
- `seed`: Semilla para el generador RNG.
- `initial_board`: Estado inicial (28 caracteres: `P`, `D`, `.`).
- `abandon`: Simulación de desconexión (`puma:N` o `dogs:N`).

## Sincronización y Diseño

- **Servidor y Mesas:** Control de concurrencia mediante semáforos (`sem_t`) y variables de condición para el emparejamiento.
- **Tablero:** Locking de grano fino con 28 mutexes (uno por casilla).
- **Prevención de Deadlocks:** Adquisición de mutexes en orden estrictamente creciente de índice (`std::sort`), evitando la espera circular durante saltos de captura.

## Pruebas y Diagnóstico

### Test de estrés del servidor

```bash
make test_server
./test_server
```

### Análisis de memoria

```bash
make valgrind
```
