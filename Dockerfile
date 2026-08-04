# 1. Usamos la imagen oficial de Node.js basada en Debian
FROM node:20-bullseye

# 2. Instalamos el compilador de C++ (g++)
RUN apt-get update && \
    apt-get install -y build-essential && \
    rm -rf /var/lib/apt/lists/*

# 3. Establecemos el directorio de trabajo
WORKDIR /app

# 4. Copiamos los archivos de dependencias
COPY package*.json ./

# 5. Instalamos las dependencias de Node
RUN npm install

# 6. Copiamos todo el código fuente al contenedor
COPY . .

# 7. Compilamos tu código C++
# Usamos el nombre exacto de tu archivo: AnalizadorCLS.cpp
RUN g++ -o analizador_bin AnalizadorCLS.cpp

# 8. Exponemos el puerto de tu API (asumo el 3000, cámbialo si usas otro)
EXPOSE 3000

# 9. Arrancamos el servidor usando tu archivo principal
CMD ["node", "server.js"]