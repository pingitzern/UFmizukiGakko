# Sistema de Ultrafiltración Dual

Proyecto basado en PlatformIO para Arduino UNO que controla un sistema de ultrafiltración dual utilizando un LCD Keypad Shield 16×2 (DFRobot/Nubbeo) y un módulo de 4 relés.

## Hardware soportado

- **Controlador:** Arduino UNO.
- **Display:** LCD Keypad Shield 16×2 estándar (pines D4–D7, RS=D8, EN=D9, teclado analógico en A0).
- **Relés:** Módulo de 4 relés con disparo activo en bajo (configurable).

### Asignación de pines de relés

| Función | Pin digital | Descripción |
| --- | --- | --- |
| R_PERM | D2 | Válvula de permeado (NO, energizar = cerrar) |
| R_WA | D3 | Válvula de lavado A (NC, energizar = abrir) |
| R_WB | D11 | Válvula de lavado B (NC, energizar = abrir) |
| R_FREE | D12 | Reserva futura |

> Al encender o resetear el sistema, todos los relés quedan en estado seguro (desenergizados).

## Máquina de estados

Estados disponibles: `INIT`, `SERVICE`, `FLUSH_A`, `FLUSH_B`, `PAUSE`.

Secuencia automática:

1. **SERVICE** — Todas las válvulas desenergizadas durante `T_SERVICIO` (valor por defecto 60 minutos).
2. **FLUSH_A** — Activa `R_PERM`, espera `T_SETTLE` (2 s) y activa `R_WA` durante `T_FLUSH` (por defecto 60 s).
3. **FLUSH_B** — Mantiene `R_PERM` activado, apaga `R_WA` y activa `R_WB` durante `T_FLUSH`. Al finalizar, apaga `R_WB`, espera `T_SETTLE` y apaga `R_PERM` antes de volver a `SERVICE`.

El estado `PAUSE` mantiene todas las salidas apagadas. Una combinación de teclas (`NEXT`) permite forzar el salto al siguiente estado para pruebas.

### Opcional: limpieza de arranque

Se puede habilitar una purga de arranque de 30 segundos (`STARTUP_FLUSH`) que alterna entre las válvulas A y B con `R_PERM` activado antes de iniciar el ciclo de servicio. Por defecto está deshabilitada; puede activarse editando el código.

## Valores por defecto y ajustes desde el teclado

- `T_SERVICIO`: 60 minutos (ajustable con teclas `UP/DOWN` en pasos de 5 minutos, rango 20–120 minutos).
- `T_FLUSH`: 60 segundos (ajustable con teclas `LEFT/RIGHT` en pasos de 10 segundos, rango 20–120 segundos).
- `T_SETTLE`: 2 segundos (constante).

La pantalla LCD muestra en la primera línea el estado actual y una cuenta regresiva `mm:ss`. En la segunda línea se visualizan los tiempos configurados: `TS=XXm TF=YYs`.

## Compilación y carga

1. Instalar [PlatformIO](https://platformio.org/).
2. Desde la raíz del proyecto ejecutar:

```bash
pio run
pio run --target upload
```

3. Para abrir el monitor serie a 115200 baudios:

```bash
pio device monitor
```

## Configuración `ACTIVE_LOW`

El archivo `platformio.ini` define el flag de compilación `ACTIVE_LOW=1` que invierte la lógica de activación de los relés (útil para módulos trigger-LOW). Si se utiliza un módulo activo en alto, modificar la sección `build_flags` a `-D ACTIVE_LOW=0` y recompilar.

## Estructura del proyecto

```
ultra-filtracion-v1/
├─ platformio.ini
├─ include/
├─ src/
│  ├─ main.cpp
│  ├─ keypad.hpp
│  ├─ keypad.cpp
│  ├─ ui.hpp
│  ├─ ui.cpp
│  ├─ relays.hpp
│  ├─ relays.cpp
│  ├─ fsm.hpp
│  ├─ fsm.cpp
└─ README.md
```

Los tiempos configurados se almacenan en memoria RAM; la estructura de constantes facilita su futura persistencia en EEPROM.
