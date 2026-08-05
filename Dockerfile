# 1. Imagen base con Node.js
FROM node:20-bullseye

# 2. Instalar compilador de C++ (g++)
RUN apt-get update && \
    apt-get install -y build-essential && \
    rm -rf /var/lib/apt/lists/*

# 3. Directorio de trabajo
WORKDIR /app

# 4. Copiar e instalar dependencias de Node
COPY package*.json ./
RUN npm install

# 5. Copiar el código fuente
COPY . .

# 6. Crear la carpeta output y compilar C++ con el nombre exacto que espera server.js
RUN mkdir -p output && g++ -o output/AnalizadorCLS AnalizadorCLS.cpp

# 7. Puerto del servicio
EXPOSE 3000

# 8. Iniciar la API
CMD ["node", "server.js"]