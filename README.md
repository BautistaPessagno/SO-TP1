# ChompChamps — TP1 de Sistemas Operativos

Este proyecto implementa un juego tipo *snake* multijugador llamado **ChompChamps**, como parte del Trabajo Práctico Nº1 de la materia Sistemas Operativos (ITBA).
Está desarrollado en C y utiliza memoria compartida, semáforos y pipes para comunicar tres procesos distintos:

- `master`: coordina el juego y la sincronización
- `view`: renderiza el estado del juego en consola (usando `ncurses`)
- `player`: jugador automático que decide sus movimientos (`player_cente` en este repo)

## 🐧 Cómo compilar y correr (usando Docker)

1. Ingresar a un contenedor de docker con la imagen oficial de la cátedra `agodio/itba-so-multi-platform:3.0` o bien correr:

```bash
make docker_cont
```

Esto abre un contenedor temporal con la imagen y monta el proyecto en `/root`.

2. Una vez adentro del contenedor, ir a la carpeta montada (ya dentro de `/root`) y correr:

```bash
make
```

Esto va a instalar automáticamente las dependencias (`libncurses-dev`, `ncurses-term`) si no están, y compilar los tres binarios: `master`, `view` y `player_cente`.

3. Para ejecutar una partida de prueba con configuración estándar (10x10, 200ms delay, 3 jugadores automáticos), usá:

```bash
make run
```

O bien se puede ejecutar directamente con parámetros a elección, por ejemplo:

```bash
./master -w 15 -h 10 -d 100 -t 8 -v ./view -p ./player_cente ./player_cente ./player_cente
```

### 📥 Significado de los parámetros

- `-w 15`: ancho del tablero (mínimo 10)
- `-h 10`: alto del tablero (mínimo 10)
- `-d 100`: delay entre fotogramas, en milisegundos (100 ms = 0.1 segundos)
- `-t 8`: timeout por inactividad (en segundos). Si no hay movimientos válidos durante este tiempo, el juego termina
- `-s 1234`: (opcional) semilla para el generador de recompensas (por default usa `time(NULL)`)
- `-v ./view`: ruta al binario de la vista (puede omitirse para jugar sin vista)
- `-p jugador1 jugador2 ...`: último parámetro obligatorio. Lista de ejecutables de jugadores (entre 1 y 9). Cada uno es lanzado como un proceso hijo.

### 📌 Notas
- El orden de los jugadores determina su letra (A, B, C...).
- Los jugadores reciben el tamaño del tablero como argumentos (`width height`).
- El `master` los atiende con política round-robin.

---

## 📦 Dockerfile para instalar ncurses

Este repositorio incluye un `Dockerfile` que parte de la imagen oficial `agodio/itba-so-multi-platform:3.0` y asegura la instalación de `ncurses` y herramientas de build.

Uso básico para construir la imagen localmente:

```bash
# Desde la carpeta src/
docker build -t chompchamps-ncurses:latest .
```

Luego podés entrar a un contenedor con:

```bash
docker run --rm -v "${PWD}:/root" --privileged -ti chompchamps-ncurses:latest
```

Sin embargo, la cátedra recomienda usar directamente la imagen oficial `agodio/itba-so-multi-platform:3.0`.

## ▶️ Entrar/crear la imagen de Docker oficial

Si preferís no construir nada, podés usar la imagen publicada por la cátedra. Está documentada en el repo de referencia: [ITBA-72.11-SO/docker](https://github.com/alejoaquili/ITBA-72.11-SO/tree/main/docker).

Para entrar al contenedor con el proyecto montado en `/root`:

```bash
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0
```

> Este mismo comando es el que invoca la tarea `make docker_cont`.

## 🛠️ Uso de make dentro del contenedor

Principales comandos disponibles:

- `make`: compila todo (y en contenedores Debian/Ubuntu instala automáticamente `libncurses-dev`, `ncurses-term`, `pkg-config`, `build-essential` si faltan).
- `make run`: ejecuta una partida de prueba con 3 jugadores automáticos y `view`.
- `make clean`: elimina binarios y objetos compilados.
- `make info`: muestra información de la configuración y los targets disponibles.
- `make test`: verifica que los binarios generados sean ejecutables.
- Targets específicos: `make build-master`, `make build-view`, `make build-player_cente`, `make ipc`, `make modules`.

## 🎮 Correr el juego y variables

El binario principal es `master`. El único parámetro obligatorio es `-p` seguido de la lista de ejecutables de jugadores (al menos 1). Ejemplos:

```bash
# Con vista y 2 jugadores automáticos
./master -w 12 -h 10 -d 150 -t 8 -v ./view -p ./player_cente ./player_cente

# Sin vista (modo headless) y 1 jugador
./master -w 10 -h 10 -d 200 -t 8 -p ./player_cente
```

Parámetros más usados:
- `-w <ancho>` y `-h <alto>`: tamaño del tablero (mínimo 10x10).
- `-d <ms>`: delay entre frames (ms).
- `-t <seg>`: timeout por inactividad.
- `-s <seed>`: semilla opcional.
- `-v <ruta-view>`: ejecutable de la vista (opcional).
- `-p <jugadores...>`: lista de ejecutables de jugadores (obligatorio).

## 🚪 Cómo salir del contenedor Docker

- Escribí `exit` o presioná `Ctrl-D` para terminar la sesión de shell.
- Si el contenedor corre un proceso en foreground, interrumpí con `Ctrl-C` y luego `exit`.

---

## ✅ Requisitos del entorno

- Este TP debe ejecutarse y compilarse dentro de la imagen oficial de la materia: `agodio/itba-so-multi-platform:3.0`.
- `make` se encarga de instalar lo necesario si usás el contenedor y tenés red (instala `libncurses-dev` y `ncurses-term` si faltan).

## ℹ️ Referencias

- Imagen oficial de la cátedra y documentación Docker: [ITBA-72.11-SO/docker](https://github.com/alejoaquili/ITBA-72.11-SO/tree/main/docker)
