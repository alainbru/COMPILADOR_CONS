const express = require('express');
const cors = require('cors');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');


const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`Servidor corriendo en el puerto ${PORT}`);
});


const app = express();
app.use(cors());
app.use(express.json());

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

    const inputFile = path.join(outputDir, 'codigo_entrada.csl');
    const normalizedCode = code.replace(/^\uFEFF/, '');
    fs.writeFileSync(inputFile, normalizedCode, 'utf8');

    const exeName = process.platform === 'win32' ? 'AnalizadorCLS.exe' : 'AnalizadorCLS';
    const exePath = path.join(outputDir, exeName);

    if (!fs.existsSync(exePath)) {
        return res.status(500).json(buildCompilationResponse({
            success: false,
            error: `No se encontró el ejecutable del analizador en ${exePath}. Compílalo primero con: g++ AnalizadorCLS.cpp -o output/AnalizadorCLS.exe`,
            code
        }));
    }

    const child = spawn(exePath, [inputFile], { cwd: __dirname, shell: false });
    let stdout = '';
    let stderr = '';

    child.stdout.on('data', (chunk) => {
        stdout += chunk.toString();
    });

    child.stderr.on('data', (chunk) => {
        stderr += chunk.toString();
    });

    child.on('error', (err) => {
        res.status(500).json(buildCompilationResponse({
            success: false,
            error: `No se pudo ejecutar el analizador: ${err.message}`,
            code: code
        }));
    });

    child.on('close', (exitCode) => {
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

const PORT = 3000;
app.listen(PORT, () => {
    console.log(`🚀 Servidor API Node.js corriendo en http://localhost:${PORT}`);
    console.log(`Compilador listo para usar con: ${path.join(__dirname, 'output', process.platform === 'win32' ? 'AnalizadorCLS.exe' : 'AnalizadorCLS')}`);
});
