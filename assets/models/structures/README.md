# Modelos de Estructuras (Structure Models)

Esta carpeta contiene las definiciones de mallas modulares para estructuras estáticas, monumentos y puntos de interés del mundo.

## Formato de Archivo (.txt)

Cada línea define un cubo o prisma rectangular que compone la estructura:
```text
BOX PosX PosY PosZ ScaleX ScaleY ScaleZ RotX RotY RotZ R G B Name
```

- **Pos (X, Y, Z)**: Posición relativa al centro/base del modelo.
- **Scale (X, Y, Z)**: Dimensiones del bloque (ancho, alto, profundidad).
- **Rot (X, Y, Z)**: Rotaciones en radianes (Euler XYZ).
- **Color (R, G, B)**: Valores de color normalizados entre `0.0` y `1.0`.
- **Name**: Identificador opcional de la pieza para depuración o animación.

## Modelos Incluidos

1. **`ruin_arch.txt`**: Arcos y columnas de ruinas antiguas en piedra.
2. **`ancient_chest.txt`**: Cofre de madera y herrajes dorados con tesoros.
3. **`sacrifice_altar.txt`**: Altar ceremonial oscuro con cuenco y 4 antorchas en las esquinas.
