const express = require('express');
const cors = require('cors');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const crypto = require('crypto');

const app = express();

// 1. Middlewares globales
app.use(cors());
app.use(express.json());

// Servir archivos estáticos de la carpeta del proyecto (CSS, JS, imágenes)
app.use(express.static(__dirname));

// 2. Ruta principal (Servir el HTML)
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'Principal.html'));
});

// ==========================================================
// MOTOR DE SIMULACION (Node.js) - construye datos para graficas
// ==========================================================

// Extrae bloques del tipo "palabraClave NOMBRE { ... }" respetando
// llaves anidadas (p.ej. actores dentro de un escenario).
function extraerBloques(texto, palabraClave) {
    const resultado = [];
    const regexInicio = new RegExp(`\\b${palabraClave}\\s+(\\w+)\\s*\\{`, 'g');
    let match;
    while ((match = regexInicio.exec(texto)) !== null) {
        const nombre = match[1];
        let profundidad = 1;
        let i = regexInicio.lastIndex;
        const inicioContenido = i;
        while (i < texto.length && profundidad > 0) {
            if (texto[i] === '{') profundidad++;
            else if (texto[i] === '}') profundidad--;
            i++;
        }
        resultado.push({ nombre, contenido: texto.slice(inicioContenido, i - 1) });
        regexInicio.lastIndex = i;
    }
    return resultado;
}

// Extrae pares atributo = valor; dentro de un bloque (actor, etc.)
function extraerAtributos(bloque) {
    const attrs = {};
    const regexAttr = /(\w+)\s*=\s*("[^"]*"|-?\d+(?:\.\d+)?|true|false)\s*;/g;
    let m;
    while ((m = regexAttr.exec(bloque)) !== null) {
        let valor = m[2];
        if (valor.startsWith('"')) valor = valor.slice(1, -1);
        else if (valor === 'true') valor = true;
        else if (valor === 'false') valor = false;
        else valor = parseFloat(valor);
        attrs[m[1]] = valor;
    }
    return attrs;
}

// Construye actores + evolucion del conflicto + red de relaciones
// a partir del codigo CSL ya validado por el analizador en C++.
function construirSimulacion(codigo) {
    const escenarios = extraerBloques(codigo, 'escenario');
    if (!escenarios.length) return null;

    // Si hay un bloque 'simular' que referencia un escenario, usamos ese.
    let escenarioElegido = escenarios[0];
    const matchSimular = codigo.match(/simular\s+\w+\s*\{[\s\S]*?escenario\s*=\s*(\w+)\s*;/);
    if (matchSimular) {
        const encontrado = escenarios.find(e => e.nombre === matchSimular[1]);
        if (encontrado) escenarioElegido = encontrado;
    }

    const actores = extraerBloques(escenarioElegido.contenido, 'actor').map(a => {
        const attrs = extraerAtributos(a.contenido);
        return {
            nombre: a.nombre,
            tipo: attrs.tipo || 'DESCONOCIDO',
            postura: typeof attrs.postura === 'number' ? attrs.postura : 50,
            poder: typeof attrs.poder === 'number' ? attrs.poder : 0.5
        };
    });

    if (!actores.length) return null;

    // --- Evolucion del conflicto: modelo iterativo de acercamiento
    // ponderado por poder (actores con mas poder ceden menos). ---
    const PASOS = 12;
    const TASA = 0.18;
    let posturas = actores.map(a => a.postura);
    const poderes = actores.map(a => a.poder);
    const sumaPoder = poderes.reduce((s, p) => s + p, 0) || 1;

    const evolucionConflicto = [];
    for (let t = 0; t <= PASOS; t++) {
        const promedioPonderado = posturas.reduce((s, p, i) => s + p * poderes[i], 0) / sumaPoder;
        const varianzaPonderada = posturas.reduce(
            (s, p, i) => s + poderes[i] * Math.pow(p - promedioPonderado, 2), 0
        ) / sumaPoder;
        const conflicto = Math.min(100, Math.sqrt(varianzaPonderada));
        evolucionConflicto.push({ paso: t, conflicto: Math.round(conflicto * 10) / 10 });

        posturas = posturas.map((p, i) => p + TASA * (1 - poderes[i]) * (promedioPonderado - p));
    }

    // --- Red de relaciones: tension entre cada par de actores,
    // proporcional a la diferencia de postura. ---
    const nodos = actores.map((a, i) => ({ id: i, nombre: a.nombre, tipo: a.tipo }));
    const enlaces = [];
    for (let i = 0; i < actores.length; i++) {
        for (let j = i + 1; j < actores.length; j++) {
            const diferencia = Math.abs(actores[i].postura - actores[j].postura);
            enlaces.push({ origen: i, destino: j, tension: Math.round((diferencia / 100) * 100) / 100 });
        }
    }

    return {
        escenario: escenarioElegido.nombre,
        actores,
        evolucionConflicto,
        red: { nodos, enlaces }
    };
}

// Helper para armar respuestas
function buildCompilationResponse({ success, output, error, code }) {
    const rawOutput = (output || error || '').trim();
    const lineCount = code ? code.split(/\r?\n/).filter(Boolean).length : 0;
    const hasOutput = Boolean(rawOutput);

    if (success) {
        return {
            status: 'success',
            summary: 'Compilación correcta. El análisis terminó sin errores y generó la salida esperada.',
            details: [
                `Líneas detectadas: ${lineCount}`,
                hasOutput ? 'Se recibió salida del analizador.' : 'No se generaron mensajes adicionales.',
                'Archivo procesado correctamente.'
            ],
            output: rawOutput || 'Análisis completado.'
        };
    }

    return {
        status: 'error',
        summary: 'La compilación no terminó correctamente. Revisa el mensaje del analizador.',
        details: [
            `Líneas detectadas: ${lineCount}`,
            hasOutput ? 'Se recibió un mensaje de error del analizador.' : 'No se obtuvo información adicional.',
            error || 'Error inesperado al ejecutar el analizador.'
        ],
        error: error || 'Error inesperado al ejecutar el analizador.'
    };
}

// Ejecuta el analizador AnalizadorCLS sobre un código CSL y devuelve
// una promesa con el resultado. Reutilizado por /compile y /simulate.
function ejecutarAnalizador(code) {
    return new Promise((resolve) => {
        const outputDir = path.join(__dirname, 'output');
        if (!fs.existsSync(outputDir)) {
            fs.mkdirSync(outputDir, { recursive: true });
        }

        const uniqueId = crypto.randomUUID();
        const inputFile = path.join(outputDir, `codigo_${uniqueId}.csl`);
        const normalizedCode = code.replace(/^\uFEFF/, '');
        fs.writeFileSync(inputFile, normalizedCode, 'utf8');

        const cleanupFile = () => {
            if (fs.existsSync(inputFile)) {
                fs.unlink(inputFile, (err) => {
                    if (err) console.error(`Error al eliminar ${inputFile}:`, err);
                });
            }
        };

        const exeName = process.platform === 'win32' ? 'AnalizadorCLS.exe' : 'AnalizadorCLS';
        const exePath = path.join(outputDir, exeName);

        if (!fs.existsSync(exePath)) {
            cleanupFile();
            return resolve({ ok: false, httpStatus: 500, error: `No se encontró el ejecutable del analizador en ${exePath}.` });
        }

        const EXEC_TIMEOUT_MS = 5000;
        const child = spawn(exePath, [inputFile], {
            cwd: __dirname,
            shell: false,
            timeout: EXEC_TIMEOUT_MS
        });

        let stdout = '';
        let stderr = '';

        child.stdout.on('data', (chunk) => { stdout += chunk.toString(); });
        child.stderr.on('data', (chunk) => { stderr += chunk.toString(); });

        child.on('error', (err) => {
            cleanupFile();
            resolve({ ok: false, httpStatus: 500, error: `No se pudo ejecutar el analizador: ${err.message}` });
        });

        child.on('close', (exitCode, signal) => {
            cleanupFile();

            if (signal === 'SIGTERM') {
                return resolve({
                    ok: false, httpStatus: 508,
                    error: `Tiempo límite excedido (${EXEC_TIMEOUT_MS / 1000}s). Posible bucle infinito en el código enviando.`
                });
            }

            if (exitCode !== 0) {
                return resolve({
                    ok: false, httpStatus: 500,
                    error: stderr || stdout || `El analizador finalizó con el código ${exitCode}.`
                });
            }

            resolve({ ok: true, stdout: stdout || stderr || 'Análisis completado.' });
        });
    });
}

// 3. Ruta de compilación
app.post('/compile', async (req, res) => {
    const { code } = req.body;

    if (!code || typeof code !== 'string' || !code.trim()) {
        return res.status(400).json(buildCompilationResponse({
            success: false,
            error: 'No se proporcionó código.',
            code
        }));
    }

    const resultado = await ejecutarAnalizador(code);

    if (!resultado.ok) {
        return res.status(resultado.httpStatus).json(buildCompilationResponse({
            success: false,
            error: resultado.error,
            code
        }));
    }

    res.json(buildCompilationResponse({
        success: true,
        output: resultado.stdout,
        code
    }));
});

// 3b. Ruta de simulación (gráficas): valida con el analizador C++ y,
// si el código es correcto, construye los datos de simulación en JS.
app.post('/simulate', async (req, res) => {
    const { code } = req.body;

    if (!code || typeof code !== 'string' || !code.trim()) {
        return res.status(400).json({ status: 'error', error: 'No se proporcionó código.' });
    }

    const resultado = await ejecutarAnalizador(code);

    if (!resultado.ok) {
        return res.status(resultado.httpStatus).json({
            status: 'error',
            error: resultado.error || 'El código no pasó la validación del analizador.'
        });
    }

    const simulacion = construirSimulacion(code);

    if (!simulacion) {
        return res.status(422).json({
            status: 'error',
            error: 'El código es válido, pero no se encontraron actores en ningún escenario para simular.'
        });
    }

    res.json({ status: 'success', simulacion });
});

// 4. Iniciar el servidor (siempre al final)
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`🚀 Servidor API corriendo en el puerto ${PORT}`);
});