# Reglas Críticas del Proyecto (Directrices para Asistentes y Desarrolladores)

Este documento define restricciones fundamentales para evitar regresiones y roturas en entornos de producción (Web / WebAssembly / Desktop).

---

## 1. ⚠️ Prohibido Agregar Filtros Visuales o Efectos No Solicitados
* **NO agregar scanlines, líneas de escaneo, efectos de TV de tubo (CRT), aberración cromática ni filtros CSS/overlays sobre el canvas.**
* La tipografía retro pixel art debe mantenerse **100% limpia, nítida y legible** en todo momento.
* **Principio rector:** NUNCA añadir "mejoras cosméticas" o filtros visuales no solicitados expresamente por el usuario.

---

## 2. 📺 Manejo de Pantalla y Dimensiones (Web / Emscripten)
* **El canvas y su contenedor DEBEN ser 100% dinámicos:**
  * En `index.html`, `web/index.html` y `web/shell.html`, el canvas debe ocupar siempre el 100% del espacio provisto (`width: 100% !important; height: 100% !important; border: none;`).
  * **NUNCA volver a fijar proporciones estáticas forzadas ni tamaños fijos (como 300x150 o cajas rígidas) que generen bordes innecesarios o achiquen la pantalla.**
* En C++ (`GameApp.cpp`), el motor ajusta dinámicamente el tamaño del framebuffer y el aspect ratio (`ResizeFBO`) ante cualquier cambio de resolución o cambio a pantalla completa. **NO alterar este ciclo de redimensionamiento dinámico.**

---

## 3. 🖱️ Sistema de Cursor e Inputs en Web
* El cursor retro en navegador funciona mediante callbacks nativos de HTML5 (`emscripten_set_mousemove_callback`, `movementX`/`movementY`, `emscripten_set_mousedown_callback`).
* **NO condicionar los inputs a `glfwGetWindowAttrib(m_window, GLFW_FOCUSED)` en web**, ya que los botones DOM (como "JUGAR AHORA") dejan el foco fuera de GLFW y congelan el mouse/cámara.
* El cursor virtual debe seguir tanto el modo Pointer Lock (con movimiento relativo) como el modo libre (coordenadas directas de canvas).

---

## 4. 🚀 Política de Despliegue y Cambios
* Todo cambio en la capa web debe respetar las plantillas `web/shell.html`, `web/index.html` e `index.html`.
* Antes de commitear o sugerir cambios de UI/pantalla, verificar que compilen limpiamente y no rompan la experiencia en producción (Vercel / Netlify / Desktop).
