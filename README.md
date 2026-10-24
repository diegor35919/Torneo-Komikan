# Torneo de Komikan Concurrente - Tarea 2 Sistemas Operativos

Implementación de un torneo concurrente en tiempo real para el juego tradicional mapuche Komikan, desarrollado en C++17 utilizando primitivas POSIX (`pthread`, mutexes y semáforos).

## Compilación

Para compilar el proyecto:
```bash
make torneo
```

## Ejecución

```bash
./torneo N M [config.txt]
```

- `N`: Cantidad total de jugadores a procesar (N >= 2).
- `M`: Límite máximo de mesas activas en simultáneo (M >= 1).
- `config.txt`: (Opcional) Archivo de configuración con parámetros de retardo y semilla aleatoria.

## Pruebas

Para compilar y correr el test del servidor:
```bash
make test_server
./test_server
```
