# Informe del Trabajo Práctico 1: ChompChamps

## Resumen del Proyecto

ChompChamps es un juego multijugador implementado en C que simula un juego de estrategia donde múltiples jugadores compiten en un tablero bidimensional. El proyecto utiliza técnicas avanzadas de programación de sistemas, incluyendo memoria compartida, semáforos POSIX, pipes y comunicación entre procesos para coordinar el juego en tiempo real.

## Decisiones Tomadas Durante el Desarrollo

Durante el desarrollo del proyecto ChompChamps, se tomaron varias decisiones arquitectónicas fundamentales que determinaron la estructura y funcionamiento del sistema. La primera decisión importante fue implementar una arquitectura maestro-esclavo con procesos separados para cada componente. Esta elección permite modularidad, escalabilidad y facilita la depuración, ya que el proceso maestro coordina el juego mientras que cada jugador y la vista son procesos independientes que pueden ser desarrollados y probados por separado.

Para el sistema de comunicación entre procesos, se decidió utilizar memoria compartida para el estado del juego y semáforos POSIX para la sincronización. La memoria compartida permite acceso eficiente al estado del juego por parte de todos los procesos, mientras que los semáforos garantizan la sincronización correcta y evitan condiciones de carrera. Esta combinación proporciona un balance óptimo entre rendimiento y seguridad.

La estrategia de sincronización implementada utiliza un sistema de semáforos con múltiples niveles identificados como A, B, C, D, E y G[0-8]. Este diseño permite control granular de acceso, donde A y B manejan la comunicación maestro-vista, C y D proporcionan exclusión mutua del estado del juego, E controla contadores, y G[i] gestiona los turnos individuales de cada jugador.

En cuanto a los algoritmos de jugadores, se decidió implementar múltiples estrategias incluyendo greedy, random, competitive y clever. Esta diversidad permite comparar diferentes enfoques y crear un sistema de torneo robusto que puede evaluar la efectividad de distintas estrategias de juego.

Finalmente, se implementó un sistema robusto de manejo de errores y recuperación que incluye timeouts y detección de jugadores bloqueados. Esta decisión evita que el juego se cuelgue indefinidamente si un jugador falla o se bloquea, garantizando la estabilidad del sistema.

## Instrucciones de Compilación y Ejecución

Para compilar y ejecutar el proyecto ChompChamps, es necesario cumplir con ciertos prerrequisitos del sistema. El proyecto requiere un compilador C compatible con el estándar C99, como gcc o clang, la biblioteca ncurses para la interfaz gráfica, y un sistema operativo POSIX como Linux o macOS.

La compilación del proyecto se realiza mediante el sistema de construcción Make, que facilita la gestión de dependencias y la compilación de múltiples componentes. El comando `make all` compila todos los ejecutables del proyecto, incluyendo el proceso maestro, la interfaz gráfica y todos los tipos de jugadores. También es posible compilar componentes específicos utilizando comandos como `make master` para el proceso maestro, `make view` para la interfaz gráfica, `make player_competitive` para el jugador competitivo, `make player_greedy` para el jugador codicioso, y `make player_random` para el jugador aleatorio.

La ejecución del juego se realiza a través del proceso maestro, que acepta diferentes configuraciones de parámetros. La forma más básica de ejecutar el juego es con `./master 10 10`, que utiliza jugadores por defecto en un tablero de 10x10 celdas. Para configuraciones más específicas, se puede utilizar `./master 15 15 ./player_competitive ./player_greedy ./player_random` para ejecutar tres jugadores diferentes en un tablero de 15x15. El formato general es `./master <ancho> <alto> [ejecutable_jugador_1] [ejecutable_jugador_2] ...` donde el ancho y alto del tablero deben estar entre 5 y 50 celdas, y se pueden especificar hasta 9 jugadores simultáneos.

## Rutas Relativas para el Torneo

Para la participación en el torneo, se han identificado las rutas relativas específicas de los componentes principales del sistema. El proceso de visualización se encuentra en la ruta `./view` y es responsable de mostrar el tablero y las estadísticas en tiempo real durante el desarrollo del juego.

El jugador recomendado para el torneo es `./player_competitive`, que implementa un algoritmo avanzado con múltiples estrategias sofisticadas. Este jugador incluye análisis de densidad de recompensas para evaluar áreas de alto valor, cálculo de movilidad y eficiencia para optimizar la planificación de movimientos, implementación de lookahead con beam search para anticipar consecuencias futuras, adaptación de estrategia según la fase del juego, y detección de posiciones peligrosas para evitar situaciones comprometidas.

Además del jugador competitivo, el sistema incluye otros tipos de jugadores disponibles para comparación y testing. El `./player_greedy` implementa una estrategia codiciosa simple que siempre elige la celda con mayor valor inmediato. El `./player_random` realiza movimientos aleatorios entre las opciones válidas disponibles. El `./player_clever` presenta una estrategia intermedia entre la codiciosa y la competitiva. Finalmente, `./player1` y `./player2` son jugadores básicos que sirven como referencia para el funcionamiento del sistema.

## Limitaciones

El sistema ChompChamps presenta varias limitaciones que deben ser consideradas durante su uso y desarrollo. En términos de limitaciones técnicas, el tamaño máximo del tablero está restringido a 50x50 celdas, el número máximo de jugadores simultáneos es de 9, existe una dependencia de la biblioteca ncurses que requiere un terminal compatible, y el sistema solo es compatible con sistemas operativos POSIX como Linux y macOS.

En cuanto a las limitaciones de rendimiento, los pipes utilizados para la comunicación pueden introducir pequeñas latencias que afectan la velocidad de respuesta del sistema. El estado del juego se mantiene completamente en memoria compartida, lo que puede impactar el uso de memoria del sistema. Además, los semáforos utilizados para la sincronización pueden crear cuellos de botella cuando múltiples procesos intentan acceder simultáneamente a recursos compartidos.

Las limitaciones de escalabilidad incluyen el hecho de que cada jugador requiere un proceso separado, lo que limita el número total de jugadores que pueden participar simultáneamente. El tamaño de la memoria compartida crece proporcionalmente con el tamaño del tablero, lo que puede afectar el rendimiento en tableros muy grandes. Finalmente, la complejidad de sincronización aumenta significativamente con el número de jugadores, lo que puede impactar la estabilidad del sistema en configuraciones con muchos participantes.

## Problemas Encontrados y Soluciones

Durante el desarrollo del proyecto ChompChamps, se enfrentaron varios desafíos técnicos que requirieron soluciones innovadoras. Uno de los problemas más críticos fue la aparición de deadlocks en la sincronización, donde los jugadores podían quedar bloqueados indefinidamente esperando turnos. Para resolver este problema, se implementó un sistema de timeouts y detección de inactividad que termina el juego después de 5 segundos sin movimientos válidos, garantizando que el sistema no se cuelgue indefinidamente.

Otro desafío significativo fueron las race conditions en el acceso a memoria compartida, donde múltiples procesos intentaban acceder simultáneamente al estado del juego. La solución implementada consistió en la creación de semáforos de exclusión mutua identificados como C y D, junto con un sistema robusto de lectura y escritura que garantiza la integridad de los datos compartidos.

La compatibilidad con macOS presentó desafíos adicionales debido a las diferencias en la implementación de semáforos POSIX entre Linux y macOS. Para resolver este problema, se adoptó el uso de semáforos nombrados y se realizaron ajustes específicos en los flags de compilación para Darwin, asegurando la compatibilidad multiplataforma.

El manejo de pipes no bloqueantes representó otro desafío técnico importante, ya que los pipes bloqueantes causaban que el proceso maestro se detuviera indefinidamente esperando datos de los jugadores. La solución implementada consistió en la utilización de la función `poll()` con pipes configurados en modo no bloqueante, permitiendo la lectura asíncrona y mejorando significativamente la responsividad del sistema.

Finalmente, la detección de jugadores bloqueados presentó dificultades para determinar cuándo un jugador no podía realizar movimientos válidos. Para abordar este problema, se implementó la función `has_valid_move()` que verifica sistemáticamente las posibilidades de movimiento antes de cada turno, permitiendo una detección temprana y precisa de jugadores que han agotado sus opciones de movimiento.

## Citas de Código Reutilizado

En el desarrollo del proyecto ChompChamps, se utilizaron patrones y conceptos estándar de programación de sistemas, aunque todo el código implementado es original. La estructura de semáforos se basa en patrones estándar de sincronización POSIX, implementando una estructura que incluye semáforos para comunicación maestro-vista (A y B), mutex para evitar inanición del maestro (C), mutex del estado del juego (D), mutex para contadores (E), un contador de jugadores leyendo (F), y semáforos individuales por jugador (G[9]).

El algoritmo de búsqueda en anchura (BFS) utilizado para el cálculo de movilidad sigue la implementación estándar de este algoritmo clásico, utilizando estructuras de datos como matrices de visitados y colas para realizar la exploración sistemática del espacio de estados. El manejo de memoria compartida implementa el patrón estándar de POSIX utilizando `shm_open()` para crear el segmento de memoria compartida y `mmap()` para mapearlo en el espacio de direcciones del proceso.

La interfaz gráfica utiliza la biblioteca ncurses siguiendo los patrones estándar de inicialización y configuración, incluyendo llamadas como `initscr()`, `cbreak()`, y `noecho()` para configurar el terminal, junto con la gestión de ventanas y colores según las convenciones de la biblioteca.

Es importante destacar que todo el código implementado es original y desarrollado específicamente para este proyecto, basándose únicamente en conceptos estándar de programación de sistemas y algoritmos clásicos, sin realizar copia directa de código fuente externo.

## Conclusiones

El proyecto ChompChamps demuestra una implementación robusta de un sistema multijugador utilizando técnicas avanzadas de programación de sistemas. La arquitectura modular permite fácil extensión y mantenimiento, mientras que el sistema de sincronización garantiza la integridad de los datos compartidos. El algoritmo competitivo implementado muestra sofisticación en la toma de decisiones, combinando múltiples estrategias para adaptarse a diferentes fases del juego.

La implementación exitosa de este proyecto requiere comprensión profunda de:
- Comunicación entre procesos (IPC)
- Sincronización con semáforos
- Manejo de memoria compartida
- Algoritmos de inteligencia artificial para juegos
- Programación de sistemas en C

El sistema está listo para ser utilizado en un entorno de torneo, con el jugador competitivo como la opción recomendada para competir contra otros sistemas.
