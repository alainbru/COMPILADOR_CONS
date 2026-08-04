# 1. Imagen base oficial de Node.js en Debian
FROM node:20-bullseye

# 2. Instalamos el compilador de C++ (g++)
RUN apt-get update && \
    apt-get install -y build-essential && \
    rm -rf /var/lib/apt/lists/*

# 3. Directorio de trabajo
WORKDIR /app

# 4. Copiamos archivos de dependencias
COPY package*.json ./

# 5. Instalamos dependencias de Node.js
RUN npm install

# 6. Copiamos todo el código fuente al contenedor
COPY . .

# 7. Creamos la carpeta 'output' y compilamos el C++ dentro de ella
# tal como lo espera tu server.js
RUN mkdir -p output && g++ AnalizadorCLS.cpp -o output/AnalizadorCLS

# 8. Exponemos el puerto (Render usará process.env.PORT automáticamente)
EXPOSE 3000

# 9. Iniciamos la API
CMD ["node", "server.js"]