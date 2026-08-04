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

// 3. Ruta de compilación
app.post('/compile', (req, res) => {
    const { code } = req.body;

    if (!code || typeof code !== 'string' || !code.trim()) {
        return res.status(400).json(buildCompilationResponse({
            success: false,
            error: 'No se proporcionó código.',
            code
        }));
    }

    const outputDir = path.join(__dirname, 'output');
    if (!fs.existsSync(outputDir)) {
        fs.mkdirSync(outputDir, { recursive: true });
    }

    // Nombre único con UUID para el archivo temporal
    const uniqueId = crypto.randomUUID();
    const inputFile = path.join(outputDir, `codigo_${uniqueId}.csl`);
    const normalizedCode = code.replace(/^\uFEFF/, '');
    fs.writeFileSync(inputFile, normalizedCode, 'utf8');

    // Función para limpiar archivos temporales
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
        return res.status(500).json(buildCompilationResponse({
            success: false,
            error: `No se encontró el ejecutable del analizador en ${exePath}.`,
            code
        }));
    }

    // Proceso con timeout de 5 segundos
    const EXEC_TIMEOUT_MS = 5000;
    const child = spawn(exePath, [inputFile], { 
        cwd: __dirname, 
        shell: false,
        timeout: EXEC_TIMEOUT_MS
    });

    let stdout = '';
    let stderr = '';

    child.stdout.on('data', (chunk) => {
        stdout += chunk.toString();
    });

    child.stderr.on('data', (chunk) => {
        stderr += chunk.toString();
    });

    child.on('error', (err) => {
        cleanupFile();
        res.status(500).json(buildCompilationResponse({
            success: false,
            error: `No se pudo ejecutar el analizador: ${err.message}`,
            code
        }));
    });

    child.on('close', (exitCode, signal) => {
        cleanupFile();

        if (signal === 'SIGTERM') {
            return res.status(508).json(buildCompilationResponse({
                success: false,
                error: `Tiempo límite excedido (${EXEC_TIMEOUT_MS / 1000}s). Posible bucle infinito en el código enviando.`,
                code
            }));
        }

        if (exitCode !== 0) {
            return res.status(500).json(buildCompilationResponse({
                success: false,
                error: stderr || stdout || `El analizador finalizó con el código ${exitCode}.`,
                code
            }));
        }

        res.json(buildCompilationResponse({
            success: true,
            output: stdout || stderr || 'Análisis completado.',
            code
        }));
    });
});

// 4. Iniciar el servidor (siempre al final)
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`🚀 Servidor API corriendo en el puerto ${PORT}`);
});