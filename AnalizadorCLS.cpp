#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <string>
#include <cctype>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>

using namespace std;

//====================================================
// CONSTANTES GENERALES
//====================================================
#define NULL_STR "NULL"
#define VACIO    "-"

// ESTRUCTURAS PRINCIPALES
#define TOK_ACTOR                  0
#define TOK_ACTORES                1
#define TOK_EVENTO                 2
#define TOK_EVENTOS                3
#define TOK_ESCENARIO              4
#define TOK_ENTORNO                5
#define TOK_REGLA                  6
#define TOK_REGLAS                 7
#define TOK_VARIABLE               8
#define TOK_VARIABLES              9

// ESTRUCTURAS AVANZADAS (Redes y Coaliciones)
#define TOK_RED                    10
#define TOK_NODO                   11
#define TOK_INFLUENCIA             12
#define TOK_COALICION              13
#define TOK_MIEMBRO                14

// CONTROL DE FLUJO
#define TOK_CUANDO                 15
#define TOK_ENTONCES               16
#define TOK_SIMULAR                17
#define TOK_SALIDA                 18

// FUNCIONES DEL MOTOR
#define TOK_REGISTRAR              19
#define TOK_EVALUAR                20
#define TOK_ESTABLECER             23
#define TOK_DISPARAR               24
#define TOK_ALERTA                 25
#define TOK_CALCULAR               26

// ATRIBUTOS DE SISTEMA
#define TOK_TIPO                   27
#define TOK_INTERES                28
#define TOK_PODER                  29
#define TOK_POSTURA                30
#define TOK_PRIORIDAD              31
#define TOK_FLEXIBLE                32
#define TOK_VALOR                  33
#define TOK_UNIDAD                 34
#define TOK_UMBRAL                 35
#define TOK_TENDENCIA              36

// SIMBOLOS DE AGRUPACION Y PUNTUACION
#define TOK_LLAVE_A                37
#define TOK_LLAVE_C                38
#define TOK_PAR_A                  39
#define TOK_PAR_C                  40
#define TOK_COR_A                  41
#define TOK_COR_C                  42
#define TOK_PCOMA                  43
#define TOK_DOSPUNTOS              44
#define TOK_COMA                   45
#define TOK_PUNTO                  46

// OPERADORES RELACIONALES Y DE ASIGNACION
#define TOK_IGUAL                  47
#define TOK_IGUALIGUAL              48
#define TOK_DISTINTO               49
#define TOK_MENOR                  50
#define TOK_MAYOR                  51
#define TOK_MENORIGUAL             52
#define TOK_MAYORIGUAL             53

// TOKENS DINAMICOS Y GENERICOS
#define TOK_IDENTIFICADOR          54
#define TOK_NUMERO                 55
#define TOK_CADENA                 56

// PALABRAS RESERVADAS
#define TOK_NEGOCIACION            57
#define TOK_PARTE                  58
#define TOK_VERDADERO              59
#define TOK_FALSO                  60

// OPERADORES ARITMETICOS
#define TOK_MAS                    61
#define TOK_MENOS                  62
#define TOK_MULT                   63
#define TOK_DIV                    64

// METRICAS DEL DOMINIO
#define TOK_CONFLICTO              65
#define TOK_ACUERDO                66
#define TOK_CONSENSO               67

#define TOK_ESTADO                 68

// PALABRAS RESERVADAS ADICIONALES REQUERIDAS POR EL AUTOMATA DE >130 ESTADOS
// (RAMA NEGOCIACION q29->"Criterio", RAMA EVENTO q47->"Tiempo",
//  RAMA ACTOR q63->"Faccion")
#define TOK_CRITERIO                69
#define TOK_TIEMPO                  70
#define TOK_FACCION                 71
// Transicion vacia (epsilon) usada en q120, q128, q134, q138 del documento
#define TOK_EPSILON                 72

// CONTROL DE ARCHIVO
#define TOK_FIN                    666
#define TOK_ERROR                  999

//====================================================
// CLASE ATRIBUTOS
//====================================================
class Atributos {
public:
    string lexema;
    int    token;
    string tipo;
    string valor;
    string estado;

    Atributos() {
        lexema = "";
        token  = TOK_ERROR;
        tipo   = "";
        valor  = NULL_STR;
        estado = "";
    }

    Atributos(string lex, int tok, string tip, string val, string est) {
        lexema = lex;
        token  = tok;
        tipo   = tip;
        valor  = val;
        estado = est;
    }

    void Mostrar() const {
        cout << "Tipo("   << tipo   << ")\t";
        cout << "Lexema(" << lexema << ")\t";
        cout << "Token("  << token  << ")\t";
        cout << "Valor("  << valor  << ")\t";
        cout << "Estado(" << estado << ")" << endl;
    }
};

//====================================================
// TABLA DE SIMBOLOS
//====================================================
class TablaSimbolos {
private:
    list<Atributos> tabla;

public:
    void Insertar(string lex, int tok, string tip, string val, string est) {
        tabla.push_back(Atributos(lex, tok, tip, val, est));
    }

    bool Buscar(string lex, Atributos &attr) {
        for (auto &item : tabla) {
            if (item.lexema == lex) {
                attr = item;
                return true;
            }
        }
        return false;
    }

    bool BuscarPClave(string lex, Atributos &attr) {
        for (auto &item : tabla) {
            if (item.lexema == lex && item.tipo == "pclave") {
                attr = item;
                return true;
            }
        }
        return false;
    }

    // Actualiza en el sitio la entrada generica creada por el analizador
    // lexico (tipo "variable") con el rol semantico real que determina el
    // analizador sintactico (actor, escenario, regla, evento, red,
    // coalicion, negociacion, simulacion...). Si no existe una entrada
    // generica previa, se inserta una nueva.
    void Actualizar(const string &lex, int tok, const string &tip, const string &val, const string &est) {
        for (auto &item : tabla) {
            if (item.lexema == lex && item.tipo != "pclave") {
                item.token  = tok;
                item.tipo   = tip;
                item.valor  = val;
                item.estado = est;
                return;
            }
        }
        tabla.push_back(Atributos(lex, tok, tip, val, est));
    }

    void Mostrar() {
        for (auto &item : tabla) {
            item.Mostrar();
        }
    }
};

//====================================================
// DIMENSIONES Y ESTADOS ESPECIALES DEL AUTOMATA
//====================================================
// El automata descrito en "Estructura_Transiciones.txt" numera sus estados
// como q0_INICIAL, q1_LISTO, q2 ... q138 y cierra con q_FIN_ARCHIVO. Se usa
// directamente el numero de cada q_N como indice de fila de la matriz, por
// lo que las filas deben cubrir holgadamente hasta el estado 138.
#define FILAS_AUTOMATA   150
#define COLS_AUTOMATA    100

#define Q_INICIAL         0    // q0_INICIAL
#define Q_LISTO           1    // q1_LISTO
#define Q_HUB_ESCENARIO   8    // q8
#define Q_HUB_SIMULAR    75    // q75
#define Q_HUB_ACCIONES   97    // q97
#define Q_FIN_ARCHIVO   139    // q_FIN_ARCHIVO

//====================================================
// EXCEPCION DE ERROR SINTACTICO/SEMANTICO
//====================================================
struct ErrorSintactico {
    string mensaje;
    int    linea;
};

//====================================================================
// ============  MODELO DE DATOS DEL ANALISIS SEMANTICO  =============
// El analizador sintactico (descenso recursivo) va poblando estas
// estructuras "ligeras" (equivalentes a un AST resumido) a medida que
// reconoce cada bloque del DSL. Al finalizar la fase sintactica sin
// errores, el ANALIZADOR SEMANTICO recorre esta informacion para
// aplicar las 11 tablas de validacion (Alcance, Tipos, Dominio, Flujo,
// Visibilidad, Simulacion, Operaciones, Identidad, Relacional,
// Dependencias y Acciones/Cierre - hub q97).
//====================================================================

// Codigos de error semantico soportados (11 categorias / 33 codigos).
enum class CodErrSem {
    ERR_SEM_01, ERR_SEM_02, ERR_SEM_03, ERR_SEM_04, ERR_SEM_05,
    ERR_TYP_01, ERR_TYP_02, ERR_TYP_03, ERR_TYP_04,
    ERR_DOM_01, ERR_DOM_02, ERR_DOM_03,
    ERR_LOG_01, ERR_LOG_02, ERR_LOG_03,
    ERR_SCP_01, ERR_SCP_02, ERR_SCP_03,
    ERR_SIM_01, ERR_SIM_02, ERR_SIM_03,
    ERR_OP_01, ERR_OP_02, ERR_OP_03,
    ERR_ID_01, ERR_ID_02, ERR_ID_03, ERR_ID_04, ERR_ID_05,
    ERR_REL_01, ERR_REL_02, ERR_REL_03, ERR_REL_04,
    ERR_DEP_01, ERR_DEP_02, ERR_DEP_03,
    ERR_OUT_01, ERR_OUT_02, ERR_OUT_03
};

// Traduce el enum a su codigo textual (tal como aparece en el
// requerimiento) para que los reportes sean legibles y trazables.
inline string CodigoTexto(CodErrSem c) {
    switch (c) {
        case CodErrSem::ERR_SEM_01: return "ERR_SEM_01";
        case CodErrSem::ERR_SEM_02: return "ERR_SEM_02";
        case CodErrSem::ERR_SEM_03: return "ERR_SEM_03";
        case CodErrSem::ERR_SEM_04: return "ERR_SEM_04";
        case CodErrSem::ERR_SEM_05: return "ERR_SEM_05";
        case CodErrSem::ERR_TYP_01: return "ERR_TYP_01";
        case CodErrSem::ERR_TYP_02: return "ERR_TYP_02";
        case CodErrSem::ERR_TYP_03: return "ERR_TYP_03";
        case CodErrSem::ERR_TYP_04: return "ERR_TYP_04";
        case CodErrSem::ERR_DOM_01: return "ERR_DOM_01";
        case CodErrSem::ERR_DOM_02: return "ERR_DOM_02";
        case CodErrSem::ERR_DOM_03: return "ERR_DOM_03";
        case CodErrSem::ERR_LOG_01: return "ERR_LOG_01";
        case CodErrSem::ERR_LOG_02: return "ERR_LOG_02";
        case CodErrSem::ERR_LOG_03: return "ERR_LOG_03";
        case CodErrSem::ERR_SCP_01: return "ERR_SCP_01";
        case CodErrSem::ERR_SCP_02: return "ERR_SCP_02";
        case CodErrSem::ERR_SCP_03: return "ERR_SCP_03";
        case CodErrSem::ERR_SIM_01: return "ERR_SIM_01";
        case CodErrSem::ERR_SIM_02: return "ERR_SIM_02";
        case CodErrSem::ERR_SIM_03: return "ERR_SIM_03";
        case CodErrSem::ERR_OP_01:  return "ERR_OP_01";
        case CodErrSem::ERR_OP_02:  return "ERR_OP_02";
        case CodErrSem::ERR_OP_03:  return "ERR_OP_03";
        case CodErrSem::ERR_ID_01:  return "ERR_ID_01";
        case CodErrSem::ERR_ID_02:  return "ERR_ID_02";
        case CodErrSem::ERR_ID_03:  return "ERR_ID_03";
        case CodErrSem::ERR_ID_04:  return "ERR_ID_04";
        case CodErrSem::ERR_ID_05:  return "ERR_ID_05";
        case CodErrSem::ERR_REL_01: return "ERR_REL_01";
        case CodErrSem::ERR_REL_02: return "ERR_REL_02";
        case CodErrSem::ERR_REL_03: return "ERR_REL_03";
        case CodErrSem::ERR_REL_04: return "ERR_REL_04";
        case CodErrSem::ERR_DEP_01: return "ERR_DEP_01";
        case CodErrSem::ERR_DEP_02: return "ERR_DEP_02";
        case CodErrSem::ERR_DEP_03: return "ERR_DEP_03";
        case CodErrSem::ERR_OUT_01: return "ERR_OUT_01";
        case CodErrSem::ERR_OUT_02: return "ERR_OUT_02";
        case CodErrSem::ERR_OUT_03: return "ERR_OUT_03";
    }
    return "ERR_SEM_00";
}

// Un error/advertencia semantica ya localizada y clasificada.
struct ErrorSemantico {
    CodErrSem codigo;
    string    mensaje;
    int       linea;
    bool      bloqueante; // true = error duro (detiene ejecucion logica del DSL)
};

// Valor reconocido para un atributo ("Atributo ::= Nombre '=' Valor ';'").
// Guarda el tipo de token real con el que fue escrito en el fuente para
// que el analizador semantico pueda contrastarlo contra el tipo que el
// dominio espera (ERR_TYP_01 / ERR_TYP_04).
struct ValorAtributo {
    int    tokenValor;          // TOK_NUMERO, TOK_CADENA, TOK_VERDADERO/FALSO, TOK_IDENTIFICADOR, TOK_COR_A (lista)
    string textoValor;          // representacion textual del valor simple
    vector<pair<string,int>> listaIds; // (identificador, linea) si el valor es una lista [ ... ]
};

// Una asignacion o atributo "nombre = valor;" dentro de un bloque.
struct AtribInfo {
    string        nombre;
    ValorAtributo valor;
    int           linea;
};

// Resultado de reconocer un operando (Factor/Termino/Expresion): permite
// que el analizador semantico conozca, en el mismo recorrido, el tipo de
// token con el que fue escrito el operando y si es un literal aislado o
// el resultado de una sub-expresion compuesta (a+b, (x*y), etc.).
struct OperandoInfo {
    int    tok;        // token del literal si es simple (NUMERO/CADENA/VERDADERO/FALSO/IDENTIFICADOR)
    string texto;
    bool   compuesta;   // true si proviene de combinar 2+ factores/terminos
};

// Una condicion "Expresion OP Expresion" de un CUANDO.
struct CondInfo {
    string operandoIzq, operandoDer, operador;
    int    tokIzq, tokDer;      // tipos de token de cada operando (si son literales)
    int    linea;
};

// Una accion del bloque q97 (ENTONCES { ... }).
struct AccInfo {
    string tipo;   // "disparar" | "establecer" | "alerta" | "registrar" | "evaluar" | "calcular" | "salida" | "asignacion"
    string arg1, arg2;
    int    linea;
};

// Un bloque "CUANDO (cond) ENTONCES { acciones }" (top-level de una regla
// o de una simulacion).
struct CuandoInfo {
    CondInfo        condicion;
    vector<AccInfo> acciones;
    int             linea;
};

// Informacion recolectada de cualquier bloque con nombre del DSL:
// actor, evento, red, coalicion, negociacion, regla, escenario, simulacion.
struct BloqueInfo {
    string               categoria;      // "actor","evento","red","coalicion","negociacion","regla","escenario","simulacion"
    string               nombre;
    string               contextoPadre;  // escenario/simulacion contenedor (VACIO si es global)
    int                  linea;
    vector<AtribInfo>    atributos;      // en orden de aparicion (permite detectar duplicados)
    vector<CuandoInfo>   cuandos;        // bloques CUANDO..ENTONCES definidos dentro (reglas/simulaciones)
    // Datos especificos de "simular": referencia de escenario usada.
    string               escenarioRef;
    int                  escenarioRefLinea = -1;
    bool                 tieneEscenarioRef = false;
};

//====================================================
// ANALIZADOR LEXICO + SINTACTICO + SEMANTICO
//====================================================
class Analisis {
private:
    string fuente;
    size_t i;
    TablaSimbolos ts;

    string ultimoLexema;
    string ultimoNumero;
    string ultimaCadena;

    int lineaActual;

    // --- Estado del "token actual" usado por el descenso recursivo ---
    int    tokActual;
    string lexActual;
    string numActual;
    string cadActual;
    int    lineaTok;

    // --- Matriz de transiciones del automata (estado x token) ---
    // tTransicion[estado][token] = estado_destino  (TOK_ERROR si no hay transicion valida)
    int tTransicion[FILAS_AUTOMATA][COLS_AUTOMATA];

    //================================================
    // ESTADO DEL ANALIZADOR SEMANTICO
    //================================================
    // Bloques con nombre reconocidos durante el descenso recursivo
    // (actor/evento/red/coalicion/negociacion/regla/escenario/simulacion).
    // Sirven de "AST resumido" para la fase semantica posterior.
    list<BloqueInfo> registroBloques;

    // Puntero al bloque que se esta reconociendo actualmente (pila de
    // contexto): permite que Atributo()/ListaIdentificadores()/Accion()
    // registren su informacion en el bloque correcto incluso estando
    // anidados (p.ej. una REGLA dentro de un ESCENARIO).
    vector<BloqueInfo*> pilaContexto;

    // Bandeja de errores/advertencias semanticas acumuladas. Se llenan
    // tanto durante el recorrido sintactico (chequeos de alcance/tipo que
    // dependen del token que se esta leyendo en ese instante) como en la
    // pasada semantica final (chequeos estructurales/globales).
    vector<ErrorSemantico> erroresSemanticos;

    // Ultimo identificador de escenario que se declaro (para detectar
    // colisiones de nombre entre ITEMS de un mismo escenario, ERR_SCP_02).
    map<string, set<string>> itemsPorEscenario;

    // Identificadores que YA fueron declarados explicitamente por el
    // programador (a diferencia de la tabla de simbolos, que el
    // analizador lexico pre-llena con una entrada generica "variable"
    // para CUALQUIER identificador que aparezca en el texto). Es la base
    // real de ERR_SEM_02 (identificador no declarado).
    set<string> declarados;

    // Categoria semantica con la que cada nombre fue declarado por
    // primera vez (actor/evento/red/coalicion/negociacion/regla/
    // escenario/simulacion/variable). Permite detectar redeclaraciones
    // (ERR_SEM_01/ERR_ID_01) y colisiones de espacio de nombres global
    // (ERR_ID_02), asi como el "shadowing" de variables (ERR_SCP_02).
    map<string, string> categoriaDeNombre;

    // Punteros "scratch" que apuntan a donde debe registrarse la
    // condicion/las acciones del bloque CUANDO que se esta reconociendo
    // en este instante. Se activan justo antes de recorrer esa parte de
    // la gramatica y se desactivan (nullptr) inmediatamente despues.
    CondInfo*        colectorCondicionCuando = nullptr;
    vector<AccInfo>* colectorAcciones        = nullptr;

    // Valor recien reconocido por Valor()/ListaIdentificadores(), leido
    // por el llamador inmediatamente despues (Atributo(), DeclVariable()).
    string ultimoValorTexto;
    vector<pair<string,int>> ultimoValorLista;

    void RegistrarError(CodErrSem cod, const string &msg, int linea, bool bloqueante = false) {
        erroresSemanticos.push_back(ErrorSemantico{cod, msg, linea, bloqueante});
    }

    BloqueInfo* ContextoActual() {
        return pilaContexto.empty() ? nullptr : pilaContexto.back();
    }

public:
    Analisis() {
        i           = 0;
        lineaActual = 1;
        tokActual   = TOK_ERROR;
        lineaTok    = 1;
        cargarTablaSimbolos();
        cargarAutomata();
    }

    //================================================
    // CARGAR PALABRAS CLAVE (DICCIONARIO DEL LENGUAJE)
    //================================================
    void cargarTablaSimbolos() {

        // ESTRUCTURAS PRINCIPALES
        ts.Insertar("actor",       TOK_ACTOR,       "pclave", VACIO, VACIO);
        ts.Insertar("actores",     TOK_ACTORES,     "pclave", VACIO, VACIO);
        ts.Insertar("evento",      TOK_EVENTO,      "pclave", VACIO, VACIO);
        ts.Insertar("eventos",     TOK_EVENTOS,     "pclave", VACIO, VACIO);
        ts.Insertar("escenario",   TOK_ESCENARIO,   "pclave", VACIO, VACIO);
        ts.Insertar("entorno",     TOK_ENTORNO,     "pclave", VACIO, VACIO);
        ts.Insertar("regla",       TOK_REGLA,       "pclave", VACIO, VACIO);
        ts.Insertar("reglas",      TOK_REGLAS,      "pclave", VACIO, VACIO);
        ts.Insertar("variable",    TOK_VARIABLE,    "pclave", VACIO, VACIO);
        ts.Insertar("variables",   TOK_VARIABLES,   "pclave", VACIO, VACIO);

        // ESTRUCTURAS AVANZADAS
        ts.Insertar("red",         TOK_RED,         "pclave", VACIO, VACIO);
        ts.Insertar("nodo",        TOK_NODO,        "pclave", VACIO, VACIO);
        ts.Insertar("influencia",  TOK_INFLUENCIA,  "pclave", VACIO, VACIO);
        ts.Insertar("coalicion",   TOK_COALICION,   "pclave", VACIO, VACIO);
        ts.Insertar("miembro",     TOK_MIEMBRO,     "pclave", VACIO, VACIO);

        // CONTROL DE FLUJO
        ts.Insertar("cuando",      TOK_CUANDO,      "pclave", VACIO, VACIO);
        ts.Insertar("entonces",    TOK_ENTONCES,    "pclave", VACIO, VACIO);
        ts.Insertar("simular",     TOK_SIMULAR,     "pclave", VACIO, VACIO);
        ts.Insertar("salida",      TOK_SALIDA,      "pclave", VACIO, VACIO);

        // FUNCIONES DEL MOTOR
        ts.Insertar("registrar",   TOK_REGISTRAR,   "pclave", VACIO, VACIO);
        ts.Insertar("evaluar",     TOK_EVALUAR,     "pclave", VACIO, VACIO);
        ts.Insertar("establecer",  TOK_ESTABLECER,  "pclave", VACIO, VACIO);
        ts.Insertar("disparar",    TOK_DISPARAR,    "pclave", VACIO, VACIO);
        ts.Insertar("alerta",      TOK_ALERTA,      "pclave", VACIO, VACIO);
        ts.Insertar("calcular",    TOK_CALCULAR,    "pclave", VACIO, VACIO);

        // ATRIBUTOS DE SISTEMA
        ts.Insertar("tipo",        TOK_TIPO,        "pclave", VACIO, VACIO);
        ts.Insertar("interes",     TOK_INTERES,     "pclave", VACIO, VACIO);
        ts.Insertar("poder",       TOK_PODER,       "pclave", VACIO, VACIO);
        ts.Insertar("postura",     TOK_POSTURA,     "pclave", VACIO, VACIO);
        ts.Insertar("prioridad",   TOK_PRIORIDAD,   "pclave", VACIO, VACIO);
        ts.Insertar("flexible",    TOK_FLEXIBLE,    "pclave", VACIO, VACIO);
        ts.Insertar("valor",       TOK_VALOR,       "pclave", VACIO, VACIO);
        ts.Insertar("unidad",      TOK_UNIDAD,      "pclave", VACIO, VACIO);
        ts.Insertar("umbral",      TOK_UMBRAL,      "pclave", VACIO, VACIO);
        ts.Insertar("tendencia",   TOK_TENDENCIA,   "pclave", VACIO, VACIO);

        // PALABRAS RESERVADAS (Negociacion y booleanos)
        ts.Insertar("negociacion", TOK_NEGOCIACION, "pclave", VACIO, VACIO);
        ts.Insertar("parte",       TOK_PARTE,       "pclave", VACIO, VACIO);
        ts.Insertar("true",        TOK_VERDADERO,   "pclave", VACIO, VACIO);
        ts.Insertar("false",       TOK_FALSO,       "pclave", VACIO, VACIO);

        // METRICAS DEL DOMINIO
        ts.Insertar("conflicto",   TOK_CONFLICTO,   "pclave", VACIO, VACIO);
        ts.Insertar("acuerdo",     TOK_ACUERDO,     "pclave", VACIO, VACIO);
        ts.Insertar("consenso",    TOK_CONSENSO,    "pclave", VACIO, VACIO);

        // PALABRA RESERVADA ADICIONAL (rama REGISTRAR: ESTADO | ALERTA)
        ts.Insertar("estado",      TOK_ESTADO,      "pclave", VACIO, VACIO);

        // PALABRAS CLAVE ADICIONALES DEL AUTOMATA (Negociacion, Evento, Actor)
        ts.Insertar("criterio",    TOK_CRITERIO,    "pclave", VACIO, VACIO);
        ts.Insertar("tiempo",      TOK_TIEMPO,      "pclave", VACIO, VACIO);
        ts.Insertar("faccion",     TOK_FACCION,     "pclave", VACIO, VACIO);

        // OPERADORES RELACIONALES Y DE ASIGNACION
        ts.Insertar("=",  TOK_IGUAL,      "pclave", VACIO, VACIO);
        ts.Insertar("==", TOK_IGUALIGUAL, "pclave", VACIO, VACIO);
        ts.Insertar("!=", TOK_DISTINTO,   "pclave", VACIO, VACIO);
        ts.Insertar("<",  TOK_MENOR,      "pclave", VACIO, VACIO);
        ts.Insertar(">",  TOK_MAYOR,      "pclave", VACIO, VACIO);
        ts.Insertar("<=", TOK_MENORIGUAL, "pclave", VACIO, VACIO);
        ts.Insertar(">=", TOK_MAYORIGUAL, "pclave", VACIO, VACIO);

        // OPERADORES ARITMETICOS
        ts.Insertar("+",  TOK_MAS,        "pclave", VACIO, VACIO);
        ts.Insertar("-",  TOK_MENOS,      "pclave", VACIO, VACIO);
        ts.Insertar("*",  TOK_MULT,       "pclave", VACIO, VACIO);
        ts.Insertar("/",  TOK_DIV,        "pclave", VACIO, VACIO);

        // SIMBOLOS
        ts.Insertar("{", TOK_LLAVE_A,   "pclave", VACIO, VACIO);
        ts.Insertar("}", TOK_LLAVE_C,   "pclave", VACIO, VACIO);
        ts.Insertar("(", TOK_PAR_A,     "pclave", VACIO, VACIO);
        ts.Insertar(")", TOK_PAR_C,     "pclave", VACIO, VACIO);
        ts.Insertar("[", TOK_COR_A,     "pclave", VACIO, VACIO);
        ts.Insertar("]", TOK_COR_C,     "pclave", VACIO, VACIO);
        ts.Insertar(";", TOK_PCOMA,     "pclave", VACIO, VACIO);
        ts.Insertar(":", TOK_DOSPUNTOS, "pclave", VACIO, VACIO);
        ts.Insertar(",", TOK_COMA,      "pclave", VACIO, VACIO);
        ts.Insertar(".", TOK_PUNTO,     "pclave", VACIO, VACIO);
    }

    //================================================
    // AUTOMATA DE TRANSICIONES
    // Traduccion literal y completa de "Estructura_Transiciones.txt":
    //   q0_INICIAL/q1_LISTO ......... arranque y estructura global
    //   q2-q5 ........................ VARIABLE global
    //   q6-q8 ........................ definicion de ESCENARIO
    //   q8 (HUB ESCENARIO) ........... REGLA, NEGOCIACION, RED, EVENTO,
    //                                  COALICION, ACTOR
    //   q9-q26 ....................... rama REGLA
    //   q27-q36 ...................... rama NEGOCIACION
    //   q37-q44 ...................... rama RED
    //   q45-q50 ...................... rama EVENTO
    //   q51-q60 ...................... rama COALICION
    //   q61-q72 ...................... rama ACTOR
    //   q73-q74 ...................... entrada a SIMULAR
    //   q75 (HUB SIMULAR) ............ ESCENARIO, VARIABLE, CUANDO, cierre
    //   q76-q96 ...................... ramas del HUB SIMULAR
    //   q97 (HUB ACCIONES) ........... ESTABLECER, DISPARAR, ASIGNACION,
    //                                  ALERTA, REGISTRAR, EVALUAR, SALIDA
    //   q98-q138 ...................... ramas del HUB ACCIONES
    //   q_FIN_ARCHIVO ................ cierre final del archivo
    //================================================
    void cargarAutomata() {
        // Inicializacion de la matriz con el token de error
        for (int fila = 0; fila < FILAS_AUTOMATA; fila++) {
            for (int col = 0; col < COLS_AUTOMATA; col++) {
                tTransicion[fila][col] = TOK_ERROR;
            }
        }

        //--------------------------------------------
        // 1. ARRANQUE Y ESTRUCTURA GLOBAL
        //--------------------------------------------
        // q0_INICIAL
        tTransicion[0][TOK_VARIABLE]         = 2;
        tTransicion[0][TOK_ESCENARIO]        = 6;
        // q1_LISTO
        tTransicion[1][TOK_VARIABLE]         = 2;
        tTransicion[1][TOK_ESCENARIO]        = 6;
        tTransicion[1][TOK_SIMULAR]          = 73;

        // RAMA: VARIABLE GLOBAL (q2 a q5)
        tTransicion[2][TOK_IDENTIFICADOR]    = 3;
        tTransicion[3][TOK_IGUAL]            = 4;
        tTransicion[4][TOK_NUMERO]           = 5;
        tTransicion[4][TOK_CADENA]           = 5;
        tTransicion[4][TOK_VERDADERO]        = 5;
        tTransicion[4][TOK_FALSO]            = 5;
        tTransicion[5][TOK_PCOMA]            = 1;

        // RAMA: DEFINICION DE ESCENARIO (q6 a q8)
        tTransicion[6][TOK_IDENTIFICADOR]    = 7;
        tTransicion[7][TOK_LLAVE_A]          = 8;

        //--------------------------------------------
        // 2. HUB ESCENARIO (ESTADO q8)
        //--------------------------------------------
        tTransicion[8][TOK_LLAVE_C]          = 1;

        // RAMA: REGLA (q9 a q26)
        tTransicion[8][TOK_REGLA]            = 9;
        tTransicion[9][TOK_LLAVE_A]          = 10;
        tTransicion[10][TOK_CUANDO]          = 12;
        tTransicion[12][TOK_IDENTIFICADOR]   = 13;
        tTransicion[13][TOK_IGUAL]           = 14;
        tTransicion[14][TOK_NUMERO]          = 15;
        tTransicion[15][TOK_ENTONCES]        = 16;
        tTransicion[16][TOK_LLAVE_A]         = 17;
        tTransicion[17][TOK_IDENTIFICADOR]   = 18;
        // Sub-rama Regla: Asignacion
        tTransicion[18][TOK_IGUAL]           = 19;
        tTransicion[19][TOK_NUMERO]          = 20;
        tTransicion[20][TOK_PCOMA]           = 26;
        // Sub-rama Regla: Funcion/Coordenadas
        tTransicion[18][TOK_PAR_A]           = 21;
        tTransicion[21][TOK_NUMERO]          = 22;
        tTransicion[22][TOK_COMA]            = 23;
        tTransicion[23][TOK_NUMERO]          = 24;
        tTransicion[24][TOK_PAR_C]           = 25;
        tTransicion[25][TOK_PCOMA]           = 26;
        // Cierres de Regla
        tTransicion[26][TOK_LLAVE_C]         = 10;   // retorna para otra condicion
        tTransicion[10][TOK_LLAVE_C]         = 8;    // cierra la regla

        // RAMA: NEGOCIACION (q27 a q36)
        tTransicion[8][TOK_NEGOCIACION]      = 27;
        tTransicion[27][TOK_IDENTIFICADOR]   = 28;
        tTransicion[28][TOK_LLAVE_A]         = 29;
        // Sub-rama Negociacion: Partes
        tTransicion[29][TOK_PARTE]           = 30;
        tTransicion[30][TOK_IDENTIFICADOR]   = 31;
        tTransicion[31][TOK_COMA]            = 32;
        tTransicion[32][TOK_IDENTIFICADOR]   = 33;
        tTransicion[33][TOK_PCOMA]           = 29;
        // Sub-rama Negociacion: Criterio
        tTransicion[29][TOK_CRITERIO]        = 34;
        tTransicion[34][TOK_IGUAL]           = 35;
        tTransicion[35][TOK_NUMERO]          = 36;
        tTransicion[36][TOK_PCOMA]           = 29;
        // Cierre de Negociacion
        tTransicion[29][TOK_LLAVE_C]         = 8;

        // RAMA: RED (q37 a q44)
        tTransicion[8][TOK_RED]              = 37;
        tTransicion[37][TOK_IDENTIFICADOR]   = 38;
        tTransicion[38][TOK_LLAVE_A]         = 39;
        // Sub-rama Red: Nodos
        tTransicion[39][TOK_NODO]            = 40;
        tTransicion[40][TOK_IGUAL]           = 41;
        tTransicion[41][TOK_NUMERO]          = 44;
        // Sub-rama Red: Influencia
        tTransicion[39][TOK_INFLUENCIA]      = 42;
        tTransicion[42][TOK_IGUAL]           = 43;
        tTransicion[43][TOK_NUMERO]          = 44;
        // Cierre de atributos de Red
        tTransicion[44][TOK_PCOMA]           = 39;
        tTransicion[39][TOK_LLAVE_C]         = 8;

        // RAMA: EVENTO (q45 a q50)
        tTransicion[8][TOK_EVENTO]           = 45;
        tTransicion[45][TOK_IDENTIFICADOR]   = 46;
        tTransicion[46][TOK_LLAVE_A]         = 47;
        // Sub-rama Evento: Tiempo
        tTransicion[47][TOK_TIEMPO]          = 48;
        tTransicion[48][TOK_IGUAL]           = 49;
        tTransicion[49][TOK_NUMERO]          = 50;
        tTransicion[50][TOK_PCOMA]           = 47;
        // Cierre de Evento
        tTransicion[47][TOK_LLAVE_C]         = 8;

        // RAMA: COALICION (q51 a q60)
        tTransicion[8][TOK_COALICION]        = 51;
        tTransicion[51][TOK_IDENTIFICADOR]   = 52;
        tTransicion[52][TOK_LLAVE_A]         = 53;
        // Sub-rama Coalicion: Miembros
        tTransicion[53][TOK_MIEMBRO]         = 54;
        tTransicion[54][TOK_IGUAL]           = 55;
        tTransicion[55][TOK_IDENTIFICADOR]   = 56;
        tTransicion[56][TOK_PCOMA]           = 53;
        // Sub-rama Coalicion: Poder
        tTransicion[53][TOK_PODER]           = 58;
        tTransicion[58][TOK_IGUAL]           = 59;
        tTransicion[59][TOK_NUMERO]          = 60;
        tTransicion[60][TOK_PCOMA]           = 53;
        // Cierre de Coalicion
        tTransicion[53][TOK_LLAVE_C]         = 8;

        // RAMA: ACTOR (q61 a q72)
        tTransicion[8][TOK_ACTOR]            = 61;
        tTransicion[61][TOK_IDENTIFICADOR]   = 62;
        tTransicion[62][TOK_LLAVE_A]         = 63;
        // Sub-ramas Actor: Atributos de Identificador/Cadena
        tTransicion[63][TOK_TIPO]            = 64;
        tTransicion[63][TOK_ESTADO]          = 64;
        tTransicion[63][TOK_POSTURA]         = 64;
        tTransicion[64][TOK_IGUAL]           = 65;
        tTransicion[65][TOK_IDENTIFICADOR]   = 66;
        tTransicion[65][TOK_CADENA]          = 67;
        tTransicion[66][TOK_PCOMA]           = 63;
        tTransicion[67][TOK_PCOMA]           = 63;
        // Sub-ramas Actor: Atributos Numericos
        tTransicion[63][TOK_PRIORIDAD]       = 68;
        tTransicion[68][TOK_IGUAL]           = 69;
        tTransicion[69][TOK_NUMERO]          = 72;
        tTransicion[63][TOK_FACCION]         = 70;
        tTransicion[70][TOK_IGUAL]           = 71;
        tTransicion[71][TOK_NUMERO]          = 72;
        tTransicion[72][TOK_PCOMA]           = 63;
        // Cierre de Actor
        tTransicion[63][TOK_LLAVE_C]         = 8;

        //--------------------------------------------
        // 3. ENTRADA A SIMULACION (q73 a q74)
        //--------------------------------------------
        tTransicion[73][TOK_IDENTIFICADOR]   = 74;
        tTransicion[74][TOK_LLAVE_A]         = 75;

        //--------------------------------------------
        // 4. HUB SIMULAR (ESTADO q75)
        //--------------------------------------------
        tTransicion[75][TOK_LLAVE_C]         = Q_FIN_ARCHIVO;

        // RAMA: CARGAR ESCENARIO (q76 a q78)
        tTransicion[75][TOK_ESCENARIO]       = 76;
        tTransicion[76][TOK_IDENTIFICADOR]   = 77;
        tTransicion[77][TOK_PCOMA]           = 75;

        // RAMA: VARIABLE DE SIMULACION (q79 a q82)
        tTransicion[75][TOK_VARIABLE]        = 79;
        tTransicion[79][TOK_IDENTIFICADOR]   = 80;
        tTransicion[80][TOK_IGUAL]           = 81;
        tTransicion[81][TOK_NUMERO]          = 82;
        tTransicion[81][TOK_VERDADERO]       = 82;
        tTransicion[81][TOK_FALSO]           = 82;
        tTransicion[82][TOK_PCOMA]           = 75;

        // RAMA: CONDICIONAL CUANDO (q83 a q96)
        tTransicion[75][TOK_CUANDO]          = 83;
        tTransicion[83][TOK_IDENTIFICADOR]   = 84;
        // q84 -> SIMBOLO_RELACIONAL -> q85 (>, <, ==, !=, <=, >=)
        tTransicion[84][TOK_IGUALIGUAL]      = 85;
        tTransicion[84][TOK_DISTINTO]        = 85;
        tTransicion[84][TOK_MENOR]           = 85;
        tTransicion[84][TOK_MAYOR]           = 85;
        tTransicion[84][TOK_MENORIGUAL]      = 85;
        tTransicion[84][TOK_MAYORIGUAL]      = 85;
        tTransicion[85][TOK_NUMERO]          = 90;
        tTransicion[85][TOK_CADENA]          = 90;
        tTransicion[85][TOK_VERDADERO]       = 90;
        tTransicion[85][TOK_FALSO]           = 90;
        tTransicion[90][TOK_ENTONCES]        = 96;
        tTransicion[96][TOK_LLAVE_A]         = 97;

        //--------------------------------------------
        // 5. HUB ACCIONES (ESTADO q97)
        //--------------------------------------------
        tTransicion[97][TOK_LLAVE_C]         = 75;

        // RAMA ACCION: ESTABLECER (q98 a q100)
        tTransicion[97][TOK_ESTABLECER]      = 98;
        tTransicion[98][TOK_IDENTIFICADOR]   = 99;
        tTransicion[99][TOK_VERDADERO]       = 100;
        tTransicion[99][TOK_FALSO]           = 100;
        tTransicion[100][TOK_PCOMA]          = 97;

        // RAMA ACCION: DISPARAR (q101 a q104)
        tTransicion[97][TOK_DISPARAR]        = 101;
        tTransicion[101][TOK_IDENTIFICADOR]  = 102;
        tTransicion[102][TOK_PCOMA]          = 97;

        // RAMA ACCION: ASIGNACION A IDENTIFICADOR (q105 a q114)
        tTransicion[97][TOK_IDENTIFICADOR]   = 105;
        tTransicion[105][TOK_PUNTO]          = 106;
        tTransicion[106][TOK_IDENTIFICADOR]  = 107;
        tTransicion[107][TOK_IGUAL]          = 108;
        tTransicion[108][TOK_NUMERO]         = 114;
        tTransicion[108][TOK_CADENA]         = 114;
        tTransicion[114][TOK_PCOMA]          = 97;
        // Sub-rama Asignacion: Metodo con parametros
        tTransicion[107][TOK_PAR_A]          = 109;
        tTransicion[109][TOK_PODER]          = 110;
        tTransicion[110][TOK_COMA]           = 111;
        tTransicion[111][TOK_IDENTIFICADOR]  = 112;
        tTransicion[112][TOK_PAR_C]          = 113;
        tTransicion[113][TOK_PCOMA]          = 97;

        // RAMA ACCION: ALERTA (q116 a q120)
        tTransicion[97][TOK_ALERTA]          = 116;
        tTransicion[116][TOK_PAR_A]          = 117;
        tTransicion[117][TOK_CADENA]         = 118;
        tTransicion[118][TOK_PAR_C]          = 119;
        tTransicion[119][TOK_PCOMA]          = 120;
        tTransicion[120][TOK_EPSILON]        = 97;   // retorno directo tras terminar alerta

        // RAMA ACCION: REGISTRAR (q122 a q128)
        tTransicion[97][TOK_REGISTRAR]       = 122;
        tTransicion[122][TOK_ESTADO]         = 123;
        tTransicion[122][TOK_ALERTA]         = 124;
        tTransicion[123][TOK_PAR_A]          = 125;
        tTransicion[124][TOK_PAR_A]          = 125;
        tTransicion[125][TOK_CADENA]         = 126;
        tTransicion[126][TOK_PAR_C]          = 127;
        tTransicion[127][TOK_PCOMA]          = 128;
        tTransicion[128][TOK_EPSILON]        = 97;

        // RAMA ACCION: EVALUAR (q130 a q134)
        tTransicion[97][TOK_EVALUAR]         = 130;
        tTransicion[130][TOK_IDENTIFICADOR]  = 131;
        // q131 -> SIMBOLO_RELACIONAL -> q132
        tTransicion[131][TOK_IGUALIGUAL]     = 132;
        tTransicion[131][TOK_DISTINTO]       = 132;
        tTransicion[131][TOK_MENOR]          = 132;
        tTransicion[131][TOK_MAYOR]          = 132;
        tTransicion[131][TOK_MENORIGUAL]     = 132;
        tTransicion[131][TOK_MAYORIGUAL]     = 132;
        tTransicion[132][TOK_NUMERO]         = 133;
        tTransicion[133][TOK_PCOMA]          = 134;
        tTransicion[134][TOK_EPSILON]        = 97;

        // RAMA ACCION: SALIDA (q136 a q138)
        tTransicion[97][TOK_SALIDA]          = 136;
        tTransicion[136][TOK_VERDADERO]      = 137;
        tTransicion[136][TOK_FALSO]          = 137;
        tTransicion[137][TOK_PCOMA]          = 138;
        tTransicion[138][TOK_EPSILON]        = 97;
    }

    //================================================
    // LEER ARCHIVO
    //================================================
    bool leerArchivo(const char direccion[]) {
        ifstream archivo(direccion);
        if (!archivo.is_open()) return false;

        stringstream buffer;
        buffer << archivo.rdbuf();
        fuente = buffer.str();
        archivo.close();
        i           = 0;
        lineaActual = 1;
        return true;
    }

    //================================================
    // IGNORAR ESPACIOS Y COMENTARIOS
    //================================================
    void saltarEspaciosYComentarios() {
        while (i < fuente.size()) {

            if (isspace((unsigned char)fuente[i])) {
                if (fuente[i] == '\n') lineaActual++;
                i++;
                continue;
            }

            if (fuente[i] == '/' && i + 1 < fuente.size() && fuente[i + 1] == '/') {
                while (i < fuente.size() && fuente[i] != '\n') i++;
                continue;
            }

            if (fuente[i] == '/' && i + 1 < fuente.size() && fuente[i + 1] == '*') {
                i += 2;
                while (i + 1 < fuente.size() && !(fuente[i] == '*' && fuente[i + 1] == '/')) {
                    if (fuente[i] == '\n') lineaActual++;
                    i++;
                }
                if (i + 1 < fuente.size()) i += 2;
                continue;
            }

            break;
        }
    }

    bool esInicioIdentificador(char c) {
        return isalpha((unsigned char)c) || c == '_';
    }

    bool esParteIdentificador(char c) {
        return isalnum((unsigned char)c) || c == '_';
    }

    //================================================
    // ANALIZADOR LEXICO PRINCIPAL
    //================================================
    int getToken() {
        saltarEspaciosYComentarios();
        if (i >= fuente.size()) return TOK_FIN;

        char c = fuente[i];

        // CADENAS DE TEXTO ("...")
        if (c == '"') {
            i++;
            string tmp = "";
            while (i < fuente.size() && fuente[i] != '"') {
                if (fuente[i] == '\n') lineaActual++;
                tmp += fuente[i];
                i++;
            }
            if (i >= fuente.size()) {
                Error(101);
                return TOK_ERROR;
            }
            i++;
            ultimaCadena = tmp;
            return TOK_CADENA;
        }

        // NUMEROS (enteros y decimales)
        if (isdigit((unsigned char)c)) {
            string tmp  = "";
            bool punto  = false;
            while (i < fuente.size() && (isdigit((unsigned char)fuente[i]) || fuente[i] == '.')) {
                if (fuente[i] == '.') {
                    if (punto) {
                        Error(102);
                        return TOK_ERROR;
                    }
                    punto = true;
                }
                tmp += fuente[i];
                i++;
            }
            ultimoNumero = tmp;
            return TOK_NUMERO;
        }

        // IDENTIFICADORES Y PALABRAS CLAVE
        if (esInicioIdentificador(c)) {
            string tmp = "";
            while (i < fuente.size() && esParteIdentificador(fuente[i])) {
                tmp += fuente[i];
                i++;
            }
            ultimoLexema = tmp;
            Atributos attr;
            if (ts.BuscarPClave(tmp, attr)) {
                return attr.token;
            }
            return TOK_IDENTIFICADOR;
        }

        // OPERADORES DE DOS CARACTERES
        if (i + 1 < fuente.size()) {
            string op = "";
            op += fuente[i];
            op += fuente[i + 1];
            Atributos attr;
            if (ts.BuscarPClave(op, attr)) {
                i += 2;
                return attr.token;
            }
        }

        // OPERADORES DE UN CARACTER Y SIMBOLOS
        string op = "";
        op += fuente[i];
        Atributos attr;
        if (ts.BuscarPClave(op, attr)) {
            i++;
            return attr.token;
        }

        // CARACTER NO RECONOCIDO
        Error(100);
        i++;
        return TOK_ERROR;
    }

    //================================================
    // NOMBRE TOKEN
    //================================================
    string nombreToken(int token) {
        switch (token) {
            case TOK_ACTOR:         return "actor";
            case TOK_ACTORES:       return "actores";
            case TOK_EVENTO:        return "evento";
            case TOK_EVENTOS:       return "eventos";
            case TOK_ESCENARIO:     return "escenario";
            case TOK_ENTORNO:       return "entorno";
            case TOK_REGLA:         return "regla";
            case TOK_REGLAS:        return "reglas";
            case TOK_VARIABLE:      return "variable";
            case TOK_VARIABLES:     return "variables";
            case TOK_RED:           return "red";
            case TOK_NODO:          return "nodo";
            case TOK_INFLUENCIA:    return "influencia";
            case TOK_COALICION:     return "coalicion";
            case TOK_MIEMBRO:       return "miembro";
            case TOK_CUANDO:        return "cuando";
            case TOK_ENTONCES:      return "entonces";
            case TOK_SIMULAR:       return "simular";
            case TOK_SALIDA:        return "salida";
            case TOK_REGISTRAR:     return "registrar";
            case TOK_EVALUAR:       return "evaluar";
            case TOK_ESTABLECER:    return "establecer";
            case TOK_DISPARAR:      return "disparar";
            case TOK_ALERTA:        return "alerta";
            case TOK_CALCULAR:      return "calcular";
            case TOK_TIPO:          return "tipo";
            case TOK_INTERES:       return "interes";
            case TOK_PODER:         return "poder";
            case TOK_POSTURA:       return "postura";
            case TOK_PRIORIDAD:     return "prioridad";
            case TOK_FLEXIBLE:      return "flexible";
            case TOK_VALOR:         return "valor";
            case TOK_UNIDAD:        return "unidad";
            case TOK_UMBRAL:        return "umbral";
            case TOK_TENDENCIA:     return "tendencia";
            case TOK_ESTADO:        return "estado";
            case TOK_CRITERIO:      return "criterio";
            case TOK_TIEMPO:        return "tiempo";
            case TOK_FACCION:       return "faccion";
            case TOK_LLAVE_A:       return "'{'";
            case TOK_LLAVE_C:       return "'}'";
            case TOK_PAR_A:         return "'('";
            case TOK_PAR_C:         return "')'";
            case TOK_COR_A:         return "'['";
            case TOK_COR_C:         return "']'";
            case TOK_PCOMA:         return "';'";
            case TOK_DOSPUNTOS:     return "':'";
            case TOK_COMA:          return "','";
            case TOK_PUNTO:         return "'.'";
            case TOK_IGUAL:         return "'='";
            case TOK_IGUALIGUAL:    return "'=='";
            case TOK_DISTINTO:      return "'!='";
            case TOK_MENOR:         return "'<'";
            case TOK_MAYOR:         return "'>'";
            case TOK_MENORIGUAL:    return "'<='";
            case TOK_MAYORIGUAL:    return "'>='";
            case TOK_IDENTIFICADOR: return "IDENTIFICADOR";
            case TOK_NUMERO:        return "NUMERO";
            case TOK_CADENA:        return "CADENA";
            case TOK_NEGOCIACION:   return "negociacion";
            case TOK_PARTE:         return "parte";
            case TOK_VERDADERO:     return "true";
            case TOK_FALSO:         return "false";
            case TOK_MAS:           return "'+'";
            case TOK_MENOS:         return "'-'";
            case TOK_MULT:          return "'*'";
            case TOK_DIV:           return "'/'";
            case TOK_CONFLICTO:     return "conflicto";
            case TOK_ACUERDO:       return "acuerdo";
            case TOK_CONSENSO:      return "consenso";
            case TOK_FIN:           return "FIN_ARCHIVO";
            case TOK_ERROR:         return "ERROR_LEXICO";
            default:                return "TOKEN_DESCONOCIDO[" + to_string(token) + "]";
        }
    }

    //================================================
    // ERRORES LEXICOS
    //================================================
    void Error(int nroError) {
        cout << "\nERROR " << nroError << " (linea " << lineaActual << "): ";
        if (nroError == 100) {
            cout << "Caracter no reconocido: '" << fuente[i] << "'";
        }
        else if (nroError == 101) cout << "Cadena de texto no cerrada.";
        else if (nroError == 102) cout << "Numero decimal mal formado.";
        cout << endl;
    }

    //================================================
    // ANALISIS LEXICO COMPLETO
    //================================================
    bool Lexico() {
        i           = 0;
        lineaActual = 1;
        int errores = 0;

        while (true) {
            int token = getToken();

            if (token == TOK_FIN) {
                cout << "\n--- FIN DEL ANALISIS LEXICO ---";
                if (errores > 0)
                    cout << " (" << errores << " error(es) encontrado(s))";
                cout << "\n";
                return (errores == 0);
            }

            if (token == TOK_ERROR) {
                errores++;
                continue;
            }

            cout << "[L" << lineaActual << "] " << nombreToken(token);

            if (token == TOK_IDENTIFICADOR) {
                cout << " -> " << ultimoLexema;
                Atributos attr;
                if (!ts.Buscar(ultimoLexema, attr)) {
                    ts.Insertar(ultimoLexema, TOK_IDENTIFICADOR, "variable", NULL_STR, VACIO);
                }
            } else if (token == TOK_NUMERO) {
                cout << " -> " << ultimoNumero;
            } else if (token == TOK_CADENA) {
                cout << " -> \"" << ultimaCadena << "\"";
            }
            cout << endl;
        }
    }

    //================================================================
    // ============  ANALISIS SINTACTICO / SEMANTICO  ================
    // Descenso recursivo. Cada funcion documenta con que HUB / RAMA del
    // documento "Estructura_Transiciones.txt" se corresponde.
    //================================================================

    // Avanza consumiendo un token del analizador lexico y lo deja en
    // tokActual/lexActual/numActual/cadActual/lineaTok.
    void avanzar() {
        tokActual = getToken();
        lexActual = ultimoLexema;
        numActual = ultimoNumero;
        cadActual = ultimaCadena;
        lineaTok  = lineaActual;
        if (tokActual == TOK_ERROR) {
            throw ErrorSintactico{ "Token lexico no reconocido.", lineaTok };
        }
    }

    // Verifica que el token actual sea el esperado; si lo es, avanza.
    // Si no, lanza un error sintactico con mensaje descriptivo.
    void esperar(int tokEsperado, const string &descripcion) {
        if (tokActual != tokEsperado) {
            string encontrado = nombreToken(tokActual);
            if (tokActual == TOK_IDENTIFICADOR) encontrado += " (" + lexActual + ")";
            throw ErrorSintactico{
                "Se esperaba " + descripcion + ", pero se encontro " + encontrado + ".",
                lineaTok
            };
        }
        avanzar();
    }

    bool esNombreAtributoValido(int tok) {
        switch (tok) {
            case TOK_TIPO: case TOK_INTERES: case TOK_PODER: case TOK_POSTURA:
            case TOK_PRIORIDAD: case TOK_FLEXIBLE: case TOK_VALOR: case TOK_UNIDAD:
            case TOK_UMBRAL: case TOK_TENDENCIA: case TOK_NODO: case TOK_INFLUENCIA:
            case TOK_MIEMBRO: case TOK_PARTE: case TOK_ESTADO: case TOK_ESCENARIO: case TOK_TIEMPO:
            case TOK_FACCION: case TOK_CRITERIO:
                return true;
            default:
                return false;
        }
    }

    // Consume un nombre de atributo (IDENTIFICADOR o palabra clave de
    // atributo como poder/influencia/interes/etc.), usado tras el "."
    // en accesos ACTOR.ATRIBUTO.
    void esperarNombreAtributo(const string &contexto) {
        if (tokActual == TOK_IDENTIFICADOR || esNombreAtributoValido(tokActual)) {
            avanzar();
        } else {
            throw ErrorSintactico{
                "Se esperaba un nombre de atributo en " + contexto +
                ", se encontro " + nombreToken(tokActual) + ".",
                lineaTok
            };
        }
    }

    bool esOperadorRelacional(int tok) {
        switch (tok) {
            case TOK_IGUALIGUAL: case TOK_DISTINTO: case TOK_MENOR:
            case TOK_MAYOR: case TOK_MENORIGUAL: case TOK_MAYORIGUAL:
                return true;
            default:
                return false;
        }
    }

    //====================================================================
    // ================  FASE DE ANALISIS SEMANTICO  =====================
    // Los metodos de esta seccion implementan las validaciones de las 11
    // tablas de errores semanticos exigidas. Se dividen en dos momentos:
    //   (a) Chequeos "en linea" (Tabla de Simbolos, Tipos, Identidad y
    //       Alcance) que se disparan durante el propio descenso
    //       recursivo, en el instante exacto en que se lee cada
    //       identificador/valor -- igual que hace un compilador real de
    //       una sola pasada con acciones semanticas.
    //   (b) Un pase semantico GLOBAL (AnalisisSemantico(), justo despues
    //       de que Sintactico() concluye con exito) que recorre el
    //       "AST resumido" (registroBloques) para validar todo lo que
    //       requiere visión de conjunto: dependencias, ciclos,
    //       consistencia relacional, integridad de dominio y cierre de
    //       simulacion (hub q97).
    //====================================================================

    void AdvertenciaSemantica(const string &msg) {
        cout << "\n[SEMANTICA] Advertencia (linea " << lineaTok << "): " << msg << endl;
    }

    // ---- Tabla 1: Errores de Tabla de Simbolos (Alcance y Referencias) ----

    // ERR_SEM_02: Identificador No Declarado.
    void VerificarUso(const string &nombre, const string &contexto) {
        if (declarados.find(nombre) == declarados.end()) {
            RegistrarError(CodErrSem::ERR_SEM_02,
                "El identificador '" + nombre + "' se usa en " + contexto +
                " pero no fue declarado previamente.", lineaTok);
            return;
        }
        Atributos a;
        if (ts.Buscar(nombre, a)) VerificarAccesoPrematuro(nombre, a, contexto);
    }

    // ERR_SCP_03: Acceso Prematuro (variable usada en una ecuacion/condicion
    // sin tener aun un valor inicial asignado, es decir su "valor" en la
    // tabla de simbolos sigue en NULL_STR).
    void VerificarAccesoPrematuro(const string &nombre, const Atributos &a, const string &contexto) {
        if ((a.tipo == "variable") && a.valor == NULL_STR) {
            RegistrarError(CodErrSem::ERR_SCP_03,
                "La variable '" + nombre + "' se usa en " + contexto +
                " antes de recibir un valor inicial.", lineaTok);
        }
    }

    // ERR_SEM_04: Escenario No Encontrado (al simular un escenario no creado).
    void VerificarEscenarioDeclarado(const string &nombre, const string &contexto, int linea) {
        Atributos a;
        if (!ts.Buscar(nombre, a) || a.tipo != "escenario") {
            RegistrarError(CodErrSem::ERR_SEM_04,
                "'" + nombre + "' se referencia como escenario en " + contexto +
                " pero no fue declarado con 'escenario'.", linea);
        }
    }

    // ERR_SEM_05: Evento No Declarado al Disparar (o regla inexistente).
    void VerificarEventoORegla(const string &nombre, const string &contexto, int linea) {
        Atributos a;
        if (!ts.Buscar(nombre, a) || (a.tipo != "evento" && a.tipo != "regla")) {
            RegistrarError(CodErrSem::ERR_SEM_05,
                "'disparar' referencia a '" + nombre + "' en " + contexto +
                ", pero no fue declarado como evento ni como regla.", linea);
        }
    }

    // ERR_SEM_03: Referencia Huerfana en Colecciones (p.ej. un MIEMBRO de
    // COALICION, un NODO de RED o una PARTE de NEGOCIACION que no existe
    // como actor declarado). Se usa desde ListaIdentificadores().
    void VerificarReferenciaColeccion(const string &nombre, const string &contexto) {
        if (declarados.find(nombre) == declarados.end()) {
            RegistrarError(CodErrSem::ERR_SEM_03,
                "'" + nombre + "' aparece como miembro/nodo/parte en " + contexto +
                " pero no corresponde a ningun identificador declarado (referencia huerfana).",
                lineaTok);
        }
    }

    // ---- Tabla 2: Errores de Tipos (Type Checking) ----

    // Tipo semantico esperado ("NUMERO" | "CADENA" | "BOOLEANO" | "LISTA")
    // para cada nombre de atributo del dominio. Vacio == sin restriccion
    // conocida (se deja pasar, p.ej. atributos definidos libremente por
    // el usuario en variables globales).
    string TipoEsperadoAtributo(const string &nombreAtributo) {
        static const map<string, string> tipos = {
            {"interes", "NUMERO"}, {"poder", "NUMERO"}, {"prioridad", "NUMERO"},
            {"umbral", "NUMERO"}, {"tendencia", "NUMERO"}, {"valor", "NUMERO"},
            {"influencia", "NUMERO"},
            {"flexible", "BOOLEANO"},
            {"tipo", "CADENA"}, {"faccion", "CADENA"}, {"postura", "CADENA"},
            {"criterio", "CADENA"}, {"unidad", "CADENA"}, {"tiempo", "CADENA"},
            {"miembro", "LISTA"}, {"nodo", "LISTA"}, {"parte", "LISTA"}
        };
        auto it = tipos.find(nombreAtributo);
        return it == tipos.end() ? "" : it->second;
    }

    string TipoDeValor(int tokValor) {
        switch (tokValor) {
            case TOK_NUMERO:    return "NUMERO";
            case TOK_CADENA:    return "CADENA";
            case TOK_VERDADERO:
            case TOK_FALSO:     return "BOOLEANO";
            case TOK_COR_A:     return "LISTA";
            case TOK_IDENTIFICADOR: return "IDENTIFICADOR";
            default:            return "DESCONOCIDO";
        }
    }

    // ERR_TYP_01: Incompatibilidad en Asignacion (p.ej. prioridad = "Alta"
    // cuando el dominio espera NUMERO).
    void VerificarTipoAtributo(const string &nombreAtributo, int tokValor, const string &contexto) {
        string esperado = TipoEsperadoAtributo(nombreAtributo);
        if (esperado.empty()) return; // atributo libre / sin restriccion de dominio
        string real = TipoDeValor(tokValor);
        // Un IDENTIFICADOR puede referenciar una variable de cualquier
        // tipo declarado; ese caso se valida por separado (VerificarUso),
        // asi que aqui solo se marca error entre tipos literales
        // concretos (NUMERO/CADENA/BOOLEANO/LISTA) que no coinciden.
        if (real != "IDENTIFICADOR" && real != esperado) {
            RegistrarError(CodErrSem::ERR_TYP_01,
                "El atributo '" + nombreAtributo + "' en " + contexto + " espera un valor " +
                esperado + " pero recibio un valor " + real + ".", lineaTok);
        }
    }

    // ERR_TYP_02: Incompatibilidad Operacional (comparar/operar tipos
    // incompatibles, p.ej. un Actor [identificador de tipo actor] contra
    // un Booleano literal).
    void VerificarTipoOperacion(const string &tipoIzq, const string &tipoDer, const string &operador, int linea) {
        if (tipoIzq.empty() || tipoDer.empty() || tipoIzq == "IDENTIFICADOR" || tipoDer == "IDENTIFICADOR") return;
        if (tipoIzq != tipoDer) {
            RegistrarError(CodErrSem::ERR_TYP_02,
                "Operacion '" + operador + "' entre tipos incompatibles (" + tipoIzq +
                " y " + tipoDer + ").", linea);
        }
    }

    // ERR_TYP_03: Firma de Metodo Invalida (argumentos incorrectos en
    // metodos internos del motor: disparar/establecer/registrar/calcular).
    void VerificarFirma(const string &funcion, int cantidadEsperada, int cantidadRecibida, int linea) {
        if (cantidadEsperada != cantidadRecibida) {
            RegistrarError(CodErrSem::ERR_TYP_03,
                "La funcion '" + funcion + "' esperaba " + to_string(cantidadEsperada) +
                " argumento(s) y recibio " + to_string(cantidadRecibida) + ".", linea);
        }
    }

    // ERR_TYP_04: Tipo de Variable de Simulacion (reasignar dinamicamente
    // el tipo de una variable ya declarada, p.ej. de NUMERO a CADENA).
    void VerificarTipoVariableSimulacion(const string &nombre, int tokNuevoValor, const string &contexto) {
        Atributos a;
        if (ts.Buscar(nombre, a) && a.tipo == "variable" && a.estado != VACIO && a.estado != "") {
            string tipoPrevio = a.estado; // se reutiliza 'estado' para registrar el tipo actual de la variable
            string tipoNuevo  = TipoDeValor(tokNuevoValor);
            if (tipoNuevo != "IDENTIFICADOR" && tipoPrevio != tipoNuevo) {
                RegistrarError(CodErrSem::ERR_TYP_04,
                    "La variable de simulacion '" + nombre + "' cambia de tipo " + tipoPrevio +
                    " a " + tipoNuevo + " en " + contexto + " (las variables no pueden mutar de tipo).",
                    lineaTok);
            }
        }
    }

    // ---- Tabla 3: Errores de Integridad de Dominio (Propiedades) ----

    // Propiedades validas por categoria de bloque del dominio.
    set<string> PropiedadesValidas(const string &categoria) {
        static const map<string, set<string>> props = {
            {"actor",       {"tipo", "faccion", "interes", "poder", "postura", "prioridad", "flexible"}},
            {"red",         {"nodo", "influencia"}},
            {"coalicion",   {"miembro"}},
            {"negociacion", {"parte", "criterio"}},
            {"evento",      {"tipo", "tiempo", "valor", "umbral", "tendencia", "unidad"}}
        };
        auto it = props.find(categoria);
        return it == props.end() ? set<string>{} : it->second;
    }

    // Atributos obligatorios por categoria de bloque.
    vector<string> PropiedadesCriticas(const string &categoria) {
        static const map<string, vector<string>> criticas = {
            {"actor",       {"tipo", "faccion"}},
            {"red",         {"nodo"}},
            {"coalicion",   {"miembro"}},
            {"negociacion", {"parte"}},
            {"evento",      {"tipo"}}
        };
        auto it = criticas.find(categoria);
        return it == criticas.end() ? vector<string>{} : it->second;
    }

    // ERR_DOM_01: Propiedad Inexistente (p.ej. Gobierno.Corrupcion si
    // "Corrupcion" no es un atributo valido del dominio).
    void VerificarPropiedadValida(const string &categoria, const string &nombreAtributo, const string &contexto) {
        set<string> validas = PropiedadesValidas(categoria);
        if (validas.empty()) return; // categoria sin catalogo de propiedades restringido (p.ej. regla)
        if (validas.find(nombreAtributo) == validas.end()) {
            RegistrarError(CodErrSem::ERR_DOM_01,
                "'" + nombreAtributo + "' no es una propiedad valida de " + contexto +
                " (categoria '" + categoria + "').", lineaTok);
        }
    }

    // ERR_DOM_02: Modificacion de Propiedad de Solo Lectura (cambiar el
    // "tipo" de un actor/evento/etc. ya en ejecucion, es decir fuera de su
    // bloque de declaracion, dentro de una asignacion generica del motor).
    void VerificarPropiedadSoloLectura(const string &nombreBase, const string &nombreAtributo, const string &contexto) {
        static const set<string> soloLectura = {"tipo", "faccion"};
        Atributos a;
        if (soloLectura.count(nombreAtributo) && ts.Buscar(nombreBase, a) &&
            (a.tipo == "actor" || a.tipo == "evento")) {
            RegistrarError(CodErrSem::ERR_DOM_02,
                "'" + nombreAtributo + "' es de solo lectura y no puede reasignarse en tiempo de " +
                "ejecucion sobre '" + nombreBase + "' (" + contexto + ").", lineaTok);
        }
    }

    // ---- Tabla 8: Errores de Identidad, Duplicidad y Colision ----

    // Registra la declaracion "formal" de un componente con nombre
    // (actor/evento/red/coalicion/negociacion/regla/escenario/simulacion).
    // Centraliza aqui, en el mismo instante de la declaracion:
    //   ERR_SEM_01 (redeclaracion, misma categoria)
    //   ERR_ID_01  (redefinicion de entidad, categoria distinta y no es
    //               el caso especial actor/evento)
    //   ERR_ID_02  (colision de espacio de nombres global Actor<->Evento)
    void DeclararSimbolo(const string &nombre, int tok, const string &tipoTS,
                          const string &val, const string &est, int linea) {
        auto it = categoriaDeNombre.find(nombre);
        if (it != categoriaDeNombre.end()) {
            if (it->second == tipoTS) {
                RegistrarError(CodErrSem::ERR_SEM_01,
                    "Redeclaracion de identificador: '" + nombre + "' (" + tipoTS +
                    ") ya habia sido declarado previamente.", linea, true);
            } else if ((it->second == "actor" || it->second == "evento") &&
                       (tipoTS == "actor" || tipoTS == "evento")) {
                RegistrarError(CodErrSem::ERR_ID_02,
                    "'" + nombre + "' colisiona en el espacio de nombres global: ya existe como '" +
                    it->second + "' y se intenta declarar tambien como '" + tipoTS + "'.", linea, true);
            } else {
                RegistrarError(CodErrSem::ERR_ID_01,
                    "Redefinicion de entidad: '" + nombre + "' ya existia como '" + it->second +
                    "' y se redeclara como '" + tipoTS + "'.", linea, true);
            }
        }
        categoriaDeNombre[nombre] = tipoTS;
        declarados.insert(nombre);
        ts.Actualizar(nombre, tok, tipoTS, val, est);
    }

    // Declaracion de una VARIABLE (global o de simulacion): a diferencia
    // de los componentes con nombre, una variable puede reasignarse mas
    // adelante (no es "redeclaracion"), asi que solo se vigila que no
    // oculte (shadowing) a un componente global ya existente.
    void RegistrarDeclaracionVariable(const string &nombre, const string &val, const string &tipoValorTxt, int linea) {
        auto it = categoriaDeNombre.find(nombre);
        if (it != categoriaDeNombre.end() && it->second != "variable") {
            // ERR_SCP_02: Ocultamiento de Simbolos / Shadowing.
            RegistrarError(CodErrSem::ERR_SCP_02,
                "La variable '" + nombre + "' oculta (shadowing) a un componente global ya declarado como '" +
                it->second + "'.", linea);
        }
        categoriaDeNombre[nombre] = "variable";
        declarados.insert(nombre);
        ts.Actualizar(nombre, TOK_IDENTIFICADOR, "variable", val, tipoValorTxt);
    }

    // ---- Validaciones que se aplican al CERRAR un bloque (Tablas 3, 6, 9) ----

    // ERR_DOM_03 / ERR_SIM_01 / ERR_REL_01 / ERR_REL_03 / ERR_REL_04 / ERR_ID_04.
    void ValidarBloqueDominio(const BloqueInfo &b) {
        set<string> presentes;
        for (auto &a : b.atributos) presentes.insert(a.nombre);

        for (auto &critico : PropiedadesCriticas(b.categoria)) {
            if (!presentes.count(critico)) {
                RegistrarError(CodErrSem::ERR_DOM_03,
                    b.categoria + " '" + b.nombre + "' no define el atributo obligatorio '" + critico + "'.",
                    b.linea);
            }
        }

        if (b.categoria == "red") {
            auto itNodo = find_if(b.atributos.begin(), b.atributos.end(),
                                   [](const AtribInfo &a){ return a.nombre == "nodo"; });
            if (itNodo != b.atributos.end() && itNodo->valor.listaIds.empty()) {
                RegistrarError(CodErrSem::ERR_SIM_01,
                    "La red '" + b.nombre + "' declara 'nodo' con 0 elementos (parametro matematico invalido).",
                    itNodo->linea);
            }
            auto itInfl = find_if(b.atributos.begin(), b.atributos.end(),
                                   [](const AtribInfo &a){ return a.nombre == "influencia"; });
            if (itNodo != b.atributos.end() && !itNodo->valor.listaIds.empty() && itInfl == b.atributos.end()) {
                RegistrarError(CodErrSem::ERR_REL_03,
                    "La red '" + b.nombre + "' declara nodos pero ninguna 'influencia' que los conecte " +
                    "(posibles nodos aislados).", b.linea);
            }
        }

        if (b.categoria == "coalicion") {
            auto itM = find_if(b.atributos.begin(), b.atributos.end(),
                                [](const AtribInfo &a){ return a.nombre == "miembro"; });
            if (itM != b.atributos.end()) {
                for (auto &m : itM->valor.listaIds) {
                    if (m.first == b.nombre) {
                        RegistrarError(CodErrSem::ERR_ID_04,
                            "La coalicion '" + b.nombre + "' se incluye a si misma como miembro " +
                            "(autorreferencia circular).", m.second);
                    }
                }
            }
        }

        if (b.categoria == "negociacion") {
            auto itP = find_if(b.atributos.begin(), b.atributos.end(),
                                [](const AtribInfo &a){ return a.nombre == "parte"; });
            if (itP != b.atributos.end()) {
                if (itP->valor.listaIds.size() < 2) {
                    RegistrarError(CodErrSem::ERR_REL_01,
                        "La negociacion '" + b.nombre + "' define menos de dos 'parte' (negociacion unilateral).",
                        itP->linea);
                }
                set<string> vistos;
                for (auto &p : itP->valor.listaIds) {
                    if (!vistos.insert(p.first).second) {
                        RegistrarError(CodErrSem::ERR_REL_04,
                            "La negociacion '" + b.nombre + "' repite a '" + p.first +
                            "' como dos partes distintas (auto-negociacion).", p.second);
                    }
                }
            }
        }
    }

    // ERR_OUT_01 / ERR_OUT_02 / ERR_OUT_03: se evaluan al cerrar cada
    // bloque CUANDO..ENTONCES (hub q97 -- cierre de simulacion/regla).
    void ValidarCuando(const string &contexto, const CuandoInfo &ci, bool esPrimero) {
        if (esPrimero && ci.acciones.size() == 1 &&
            ci.acciones[0].tipo == "salida" && ci.acciones[0].arg1 == "true") {
            RegistrarError(CodErrSem::ERR_OUT_01,
                "'salida = true' aparece como unica accion del primer bloque CUANDO de " + contexto +
                " sin logica adicional previa (salida prematura incondicional).", ci.linea);
        }

        map<string,string> establecidos;
        bool salidaTrue = false, salidaFalse = false;
        for (auto &acc : ci.acciones) {
            if (acc.tipo == "establecer") {
                auto it = establecidos.find(acc.arg1);
                if (it != establecidos.end() && it->second != acc.arg2) {
                    RegistrarError(CodErrSem::ERR_OUT_02,
                        "El bloque ENTONCES de " + contexto + " establece '" + acc.arg1 +
                        "' con valores contradictorios (" + it->second + " y " + acc.arg2 +
                        ") de forma simultanea.", acc.linea);
                }
                establecidos[acc.arg1] = acc.arg2;
            } else if (acc.tipo == "salida") {
                if (acc.arg1 == "true") salidaTrue = true; else salidaFalse = true;
            } else if (acc.tipo == "alerta" && acc.arg1.empty()) {
                RegistrarError(CodErrSem::ERR_OUT_03,
                    "'alerta' se invoca con una cadena vacia (alerta sin contexto) en " + contexto + ".",
                    acc.linea);
            }
        }
        if (salidaTrue && salidaFalse) {
            RegistrarError(CodErrSem::ERR_OUT_02,
                "El bloque ENTONCES de " + contexto + " asigna 'salida' a true y false de forma simultanea " +
                "(disparadores contradictorios).", ci.linea);
        }
    }

    // ---- Pase semantico GLOBAL (Tablas 6, 9 y 10) ----
    // Se ejecuta una unica vez, justo despues de que Sintactico() reconoce
    // todo el programa sin errores, sobre el "AST resumido" acumulado en
    // registroBloques. Aqui se validan las propiedades que solo pueden
    // determinarse con visión de conjunto (relaciones cruzadas entre
    // bloques, ciclos de dependencia, firmas duplicadas).
    static string JuntarConComas(const vector<string> &items) {
        string out;
        for (size_t k = 0; k < items.size(); k++) {
            if (k) out += ", ";
            out += items[k];
        }
        return out;
    }

    static string FirmaDeRegla(const BloqueInfo &b) {
        vector<string> partes;
        for (auto &c : b.cuandos) {
            string p = c.condicion.operandoIzq + c.condicion.operador + c.condicion.operandoDer;
            for (auto &acc : c.acciones) p += "|" + acc.tipo + ":" + acc.arg1 + ":" + acc.arg2;
            partes.push_back(p);
        }
        sort(partes.begin(), partes.end());
        string firma;
        for (auto &p : partes) firma += p + ";";
        return firma;
    }

    // DFS simple sobre el grafo de "disparar()" entre reglas para
    // detectar dependencias ciclicas (ERR_DEP_01).
    bool ExisteCiclo(const string &objetivo, const string &actual,
                      map<string, vector<string>> &grafo, set<string> &visitados) {
        auto it = grafo.find(actual);
        if (it == grafo.end()) return false;
        for (auto &vecino : it->second) {
            if (vecino == objetivo && actual != objetivo) return true;
            if (!grafo.count(vecino) || visitados.count(vecino)) continue;
            visitados.insert(vecino);
            if (ExisteCiclo(objetivo, vecino, grafo, visitados)) return true;
        }
        return false;
    }

    void AnalisisSemantico() {
        // ---- Tabla 1 (Alcance y Referencias): ERR_SEM_04 / ERR_SEM_05 ----
        // Se validan aqui (con la tabla de simbolos ya completa) en vez de en
        // linea, porque tanto 'simular...escenario=X' como 'disparar(Y)'
        // pueden referenciar una declaracion que aparece mas ADELANTE en el
        // mismo archivo; el parser es de una sola pasada y en ese instante
        // aun no la conoceria.
        for (auto &b : registroBloques) {
            if (b.categoria == "simulacion" && b.tieneEscenarioRef) {
                VerificarEscenarioDeclarado(b.escenarioRef, "la simulacion '" + b.nombre + "'", b.escenarioRefLinea);
            }
            for (auto &c : b.cuandos) {
                for (auto &acc : c.acciones) {
                    if (acc.tipo != "disparar") continue;
                    string contextoAcc = b.categoria + " '" + b.nombre + "'";
                    VerificarEventoORegla(acc.arg1, contextoAcc, acc.linea);
                }
            }
        }

        // ---- Tabla 9 (Relacional) + ERR_SIM_02: consistencia entre coaliciones ----
        map<string, vector<string>> coalicionesPorMiembro;
        for (auto &b : registroBloques) {
            if (b.categoria != "coalicion") continue;
            for (auto &a : b.atributos) {
                if (a.nombre != "miembro") continue;
                for (auto &m : a.valor.listaIds) coalicionesPorMiembro[m.first].push_back(b.nombre);
            }
        }
        for (auto &par : coalicionesPorMiembro) {
            if (par.second.size() > 1) {
                RegistrarError(CodErrSem::ERR_SIM_02,
                    "El actor '" + par.first + "' participa simultaneamente en varias coaliciones (" +
                    JuntarConComas(par.second) + ") sin un evento que coordine la superposicion " +
                    "(duplicidad transversal incoherente).", 0);
            }
        }

        // ERR_REL_02: miembros con 'faccion' estaticamente incompatible dentro de una misma coalicion.
        map<string,string> faccionPorActor;
        for (auto &b : registroBloques) {
            if (b.categoria != "actor") continue;
            for (auto &a : b.atributos)
                if (a.nombre == "faccion") faccionPorActor[b.nombre] = a.valor.textoValor;
        }
        for (auto &b : registroBloques) {
            if (b.categoria != "coalicion") continue;
            for (auto &a : b.atributos) {
                if (a.nombre != "miembro") continue;
                string faccionRef; bool tieneRef = false;
                for (auto &m : a.valor.listaIds) {
                    auto it = faccionPorActor.find(m.first);
                    if (it == faccionPorActor.end()) continue;
                    if (!tieneRef) { faccionRef = it->second; tieneRef = true; }
                    else if (it->second != faccionRef) {
                        RegistrarError(CodErrSem::ERR_REL_02,
                            "La coalicion '" + b.nombre + "' agrupa miembros con facciones estaticamente " +
                            "incompatibles ('" + faccionRef + "' y '" + it->second + "').", a.linea);
                    }
                }
            }
        }

        // ---- ERR_ID_05: firma de regla duplicada ----
        map<string,string> firmasReglas;
        for (auto &b : registroBloques) {
            if (b.categoria != "regla") continue;
            string firma = FirmaDeRegla(b);
            if (firma.empty()) continue;
            auto it = firmasReglas.find(firma);
            if (it != firmasReglas.end()) {
                RegistrarError(CodErrSem::ERR_ID_05,
                    "La regla '" + b.nombre + "' tiene la misma firma logica (condiciones y acciones) que " +
                    "la regla '" + it->second + "'.", b.linea);
            } else {
                firmasReglas[firma] = b.nombre;
            }
        }

        // ---- Tabla 10 (Dependencias y Ciclos) ----
        map<string, vector<string>> grafoDisparo;
        for (auto &b : registroBloques) {
            if (b.categoria != "regla") continue;
            for (auto &c : b.cuandos) {
                for (auto &acc : c.acciones) {
                    if (acc.tipo != "disparar") continue;
                    if (acc.arg1 == b.nombre) {
                        RegistrarError(CodErrSem::ERR_DEP_03,
                            "La regla '" + b.nombre + "' se dispara a si misma ('disparar(" + acc.arg1 +
                            ")'), lo que produce una evaluacion condicional recursiva (bucle infinito).",
                            acc.linea);
                    } else {
                        grafoDisparo[b.nombre].push_back(acc.arg1);
                    }
                }
            }
        }
        for (auto &origen : grafoDisparo) {
            set<string> visitados;
            if (ExisteCiclo(origen.first, origen.first, grafoDisparo, visitados)) {
                RegistrarError(CodErrSem::ERR_DEP_01,
                    "Se detecto una dependencia ciclica de evaluacion que involucra a la regla '" +
                    origen.first + "' (A dispara a B y B termina disparando de vuelta hacia A).", 0);
            }
        }

        // ERR_DEP_02: uso de entidad incompleta (evento declarado sin ningun atributo).
        set<string> entidadesVacias;
        for (auto &b : registroBloques)
            if (b.categoria == "evento" && b.atributos.empty()) entidadesVacias.insert(b.nombre);
        for (auto &b : registroBloques) {
            for (auto &c : b.cuandos) {
                for (auto &acc : c.acciones) {
                    if (acc.tipo == "disparar" && entidadesVacias.count(acc.arg1)) {
                        RegistrarError(CodErrSem::ERR_DEP_02,
                            "'disparar(" + acc.arg1 + ")' usa un evento declarado sin ningun atributo " +
                            "(entidad incompleta).", acc.linea);
                    }
                }
            }
        }
    }

    void MostrarErroresSemanticos() {
        cout << "\n========== ANALISIS SEMANTICO ==========\n";
        if (erroresSemanticos.empty()) {
            cout << "No se encontraron errores ni advertencias semanticas.\n";
            return;
        }
        cout << erroresSemanticos.size() << " hallazgo(s) semantico(s):\n";
        for (auto &e : erroresSemanticos) {
            cout << "[" << CodigoTexto(e.codigo) << "]"
                 << (e.bloqueante ? " (ERROR) " : " (ADVERTENCIA) ")
                 << "linea " << e.linea << ": " << e.mensaje << "\n";
        }
    }

    bool HuboErroresSemanticosBloqueantes() {
        for (auto &e : erroresSemanticos) if (e.bloqueante) return true;
        return false;
    }

    //------------------------------------------------
    // q2-q5 : RAMA VARIABLE GLOBAL / VARIABLE DE SIMULACION (q79-q82)
    //------------------------------------------------
    void DeclVariable() {
        esperar(TOK_VARIABLE, "la palabra clave 'variable'");
        int lineaDecl = lineaTok;
        string nombre;
        if (tokActual == TOK_IDENTIFICADOR) {
            nombre = lexActual;
        } else if (esNombreAtributoValido(tokActual)) {
            // Palabras como "prioridad" tambien pueden usarse como nombre
            // de variable global/de simulacion (ademas de como atributo).
            nombre = nombreToken(tokActual);
        } else {
            throw ErrorSintactico{
                "Se esperaba un identificador de variable, se encontro " + nombreToken(tokActual) + ".",
                lineaTok
            };
        }
        avanzar();
        esperar(TOK_IGUAL, "'=' luego del nombre de la variable '" + nombre + "'");
        // ERR_TYP_04: Tipo de Variable de Simulacion (se compara el tipo con el
        // que ya estaba declarada la variable ANTES de consumir el nuevo valor).
        VerificarTipoVariableSimulacion(nombre, tokActual, "la declaracion de la variable '" + nombre + "'");
        int tokVal = Valor("la declaracion de la variable '" + nombre + "'");
        esperar(TOK_PCOMA, "';' al final de la declaracion de la variable '" + nombre + "'");
        string valTexto = ultimoValorTexto.empty() ? VACIO : ultimoValorTexto;
        RegistrarDeclaracionVariable(nombre, valTexto, TipoDeValor(tokVal), lineaDecl);
    }

    //------------------------------------------------
    // Valor de un atributo/variable. Devuelve el token con el que fue
    // escrito el valor (NUMERO/CADENA/VERDADERO/FALSO/IDENTIFICADOR/LISTA)
    // y deja el texto/lista reconocidos en ultimoValorTexto/ultimoValorLista
    // para que el llamador (Atributo/DeclVariable) los use en sus propios
    // chequeos semanticos (tipos, rangos, colecciones).
    //------------------------------------------------
    int Valor(const string &contexto, const string &nombreAtributoCtx = "") {
        ultimoValorTexto.clear();
        ultimoValorLista.clear();
        switch (tokActual) {
            case TOK_NUMERO:
                ultimoValorTexto = numActual;
                avanzar();
                return TOK_NUMERO;
            case TOK_CADENA:
                ultimoValorTexto = cadActual;
                avanzar();
                return TOK_CADENA;
            case TOK_VERDADERO:
            case TOK_FALSO: {
                int t = tokActual;
                ultimoValorTexto = (t == TOK_VERDADERO ? "true" : "false");
                avanzar();
                return t;
            }
            case TOK_IDENTIFICADOR: {
                string nombre = lexActual;
                avanzar();
                VerificarUso(nombre, contexto);
                ultimoValorTexto = nombre;
                return TOK_IDENTIFICADOR;
            }
            case TOK_COR_A:
                ListaIdentificadores(contexto, nombreAtributoCtx);
                return TOK_COR_A;
            default:
                throw ErrorSintactico{
                    "Se esperaba un valor valido (numero, cadena, true/false, identificador o lista) en " +
                    contexto + ", se encontro " + nombreToken(tokActual) + ".",
                    lineaTok
                };
        }
    }

    // ListaID ::= "[" [ ID { "," ID } ] "]"
    // Si nombreAtributoCtx es "miembro"/"nodo"/"parte" cada identificador se
    // valida como REFERENCIA A UNA COLECCION del dominio (ERR_SEM_03,
    // referencia huerfana) en vez de como simple uso de variable.
    void ListaIdentificadores(const string &contexto, const string &nombreAtributoCtx = "") {
        esperar(TOK_COR_A, "'[' de inicio de lista en " + contexto);
        bool esColeccionDominio = (nombreAtributoCtx == "miembro" || nombreAtributoCtx == "nodo" ||
                                    nombreAtributoCtx == "parte");
        if (tokActual != TOK_COR_C) {
            int linea1 = lineaTok;
            string nombre = lexActual;
            esperar(TOK_IDENTIFICADOR, "un identificador dentro de la lista en " + contexto);
            if (esColeccionDominio) VerificarReferenciaColeccion(nombre, contexto);
            else VerificarUso(nombre, contexto);
            ultimoValorLista.push_back({nombre, linea1});
            while (tokActual == TOK_COMA) {
                avanzar();
                int linea2 = lineaTok;
                string nombre2 = lexActual;
                esperar(TOK_IDENTIFICADOR, "un identificador dentro de la lista en " + contexto);
                if (esColeccionDominio) VerificarReferenciaColeccion(nombre2, contexto);
                else VerificarUso(nombre2, contexto);
                ultimoValorLista.push_back({nombre2, linea2});
            }
        }
        esperar(TOK_COR_C, "']' de cierre de lista en " + contexto);
    }

    //------------------------------------------------
    // Atributo ::= NombreAtributo "=" Valor ";"
    // (usado dentro de actor / negociacion / red / evento / coalicion)
    //
    // SECCION SEMANTICA: valida integridad de dominio (ERR_DOM_01),
    // duplicidad de atributos (ERR_ID_03), tipos (ERR_TYP_01) y rangos
    // numericos del dominio (ERR_LOG_02); ademas registra el atributo en
    // el BloqueInfo activo para que la fase global (AnalisisSemantico)
    // pueda validar integridad/relaciones/dependencias mas adelante.
    //------------------------------------------------
    void Atributo(const string &contexto) {
        int lineaAttr = lineaTok;
        string nombre;
        if (tokActual == TOK_IDENTIFICADOR) {
            nombre = lexActual;
        } else if (esNombreAtributoValido(tokActual)) {
            nombre = nombreToken(tokActual);
        } else {
            throw ErrorSintactico{
                "Se esperaba un nombre de atributo en " + contexto +
                ", se encontro" + nombreToken(tokActual) + ".",
                lineaTok
            };
        }
        avanzar();
        esperar(TOK_IGUAL, "'=' luego del atributo '" + nombre + "' en " + contexto);

        BloqueInfo *bi = ContextoActual();
        if (bi) {
            // ERR_DOM_01: Propiedad Inexistente.
            VerificarPropiedadValida(bi->categoria, nombre, contexto);
            // ERR_ID_03: Duplicidad de Atributos Internos.
            for (auto &prev : bi->atributos) {
                if (prev.nombre == nombre) {
                    RegistrarError(CodErrSem::ERR_ID_03,
                        "El atributo '" + nombre + "' se declara mas de una vez dentro de " + contexto + ".",
                        lineaAttr);
                    break;
                }
            }
        }

        int tokVal = Valor("el atributo '" + nombre + "' de " + contexto, nombre);

        // ERR_TYP_01: Incompatibilidad en Asignacion.
        VerificarTipoAtributo(nombre, tokVal, contexto);

        // ERR_LOG_02: Valores Fuera de Rango (magnitudes del dominio negativas).
        static const set<string> noNegativos = {"interes", "poder", "prioridad", "umbral", "influencia"};
        if (noNegativos.count(nombre) && tokVal == TOK_NUMERO) {
            try {
                if (stod(ultimoValorTexto) < 0) {
                    RegistrarError(CodErrSem::ERR_LOG_02,
                        "El atributo '" + nombre + "' de " + contexto + " recibe un valor negativo (" +
                        ultimoValorTexto + "), fuera del rango permitido para el dominio.", lineaAttr);
                }
            } catch (...) { /* valor no numerico bien formado: ya reportado por el lexico/tipos */ }
        }

        if (bi) {
            AtribInfo ai;
            ai.nombre = nombre;
            ai.linea  = lineaAttr;
            ai.valor.tokenValor = tokVal;
            ai.valor.textoValor = ultimoValorTexto;
            ai.valor.listaIds   = ultimoValorLista;
            bi->atributos.push_back(ai);
        }

        esperar(TOK_PCOMA, "';' al final del atributo '" + nombre + "' en " + contexto);
    }

    //------------------------------------------------
    // q61-q72 / q27-q44 / q45-q50 / q51-q60 : bloques simples
    // "PALABRA" IDENTIFICADOR "{" { Atributo } "}"
    //------------------------------------------------
    void DeclBloqueSimple(int tokPalabra, const string &descPalabra, const string &tipoTS, const string &contextoPadre) {
        esperar(tokPalabra, "la palabra clave '" + descPalabra + "'");
        int lineaDecl = lineaTok;
        string nombre = lexActual;
        esperar(TOK_IDENTIFICADOR, "un identificador de " + descPalabra);
        DeclararSimbolo(nombre, TOK_IDENTIFICADOR, tipoTS, VACIO, contextoPadre, lineaDecl);

        registroBloques.push_back(BloqueInfo{tipoTS, nombre, contextoPadre, lineaDecl, {}, {}, "", -1, false});
        pilaContexto.push_back(&registroBloques.back());

        esperar(TOK_LLAVE_A, "'{' al iniciar el bloque de " + descPalabra + " '" + nombre + "'");
        while (tokActual != TOK_LLAVE_C && tokActual != TOK_FIN) {
            Atributo(descPalabra + " '" + nombre + "'");
        }
        esperar(TOK_LLAVE_C, "'}' al cerrar el bloque de " + descPalabra + " '" + nombre + "'");

        // Tablas 3, 6 y 9: validaciones de integridad/dominio/relacionales
        // que solo pueden aplicarse con el bloque ya completo.
        ValidarBloqueDominio(*pilaContexto.back());
        pilaContexto.pop_back();
    }

    //------------------------------------------------
    // q9-q26 : RAMA REGLA
    // regla ID "{" { BloqueCuando } "}"
    //------------------------------------------------
    void DeclRegla(const string &contextoPadre) {
        esperar(TOK_REGLA, "la palabra clave 'regla'");
        int lineaDecl = lineaTok;
        string nombre = lexActual;
        esperar(TOK_IDENTIFICADOR, "un identificador de regla");
        DeclararSimbolo(nombre, TOK_IDENTIFICADOR, "regla", VACIO, contextoPadre, lineaDecl);

        registroBloques.push_back(BloqueInfo{"regla", nombre, contextoPadre, lineaDecl, {}, {}, "", -1, false});
        pilaContexto.push_back(&registroBloques.back());

        esperar(TOK_LLAVE_A, "'{' al iniciar la regla '" + nombre + "'");
        while (tokActual != TOK_LLAVE_C && tokActual != TOK_FIN) {
            BloqueCuando("la regla '" + nombre + "'");
        }
        esperar(TOK_LLAVE_C, "'}' al cerrar la regla '" + nombre + "'");

        // ERR_SIM_03: Regla Vacia o Estatica.
        if (pilaContexto.back()->cuandos.empty()) {
            RegistrarError(CodErrSem::ERR_SIM_03,
                "La regla '" + nombre + "' no contiene ningun bloque CUANDO..ENTONCES (regla vacia o estatica).",
                lineaDecl);
        }
        pilaContexto.pop_back();
    }

    //------------------------------------------------
    // q83-q97 (top-level) y sub-rama dentro de REGLA:
    // "cuando" "(" Condicion ")" "entonces" "{" { Accion } "}"
    //------------------------------------------------
    void BloqueCuando(const string &contexto) {
        esperar(TOK_CUANDO, "la palabra clave 'cuando' en " + contexto);
        esperar(TOK_PAR_A, "'(' luego de 'cuando' en " + contexto);

        CuandoInfo ci;
        ci.linea = lineaTok;
        colectorCondicionCuando = &ci.condicion;
        Condicion(contexto);
        colectorCondicionCuando = nullptr;

        esperar(TOK_PAR_C, "')' al cerrar la condicion de 'cuando' en " + contexto);
        esperar(TOK_ENTONCES, "la palabra clave 'entonces' en " + contexto);
        esperar(TOK_LLAVE_A, "'{' al iniciar el bloque de acciones en " + contexto);

        colectorAcciones = &ci.acciones;
        while (tokActual != TOK_LLAVE_C && tokActual != TOK_FIN) {
            Accion(contexto);
        }
        colectorAcciones = nullptr;
        esperar(TOK_LLAVE_C, "'}' al cerrar el bloque de acciones en " + contexto);

        BloqueInfo *bi = ContextoActual();
        bool esPrimero = bi && bi->cuandos.empty();
        if (bi) bi->cuandos.push_back(ci);

        // Tabla 11 (Acciones y Cierre de Simulacion, hub q97): ERR_OUT_01/02/03.
        ValidarCuando(contexto, ci, esPrimero);
    }

    // Condicion ::= Expresion OpRelacional Expresion
    void Condicion(const string &contexto) {
        OperandoInfo izq = Expresion(contexto);
        if (!esOperadorRelacional(tokActual)) {
            throw ErrorSintactico{
                "Se esperaba un operador relacional (==, !=, <, >, <=, >=) en " + contexto +
                ", se encontro " + nombreToken(tokActual) + ".",
                lineaTok
            };
        }
        string opTexto = nombreToken(tokActual);
        int lineaOp = lineaTok;
        avanzar();
        OperandoInfo der = Expresion(contexto);

        if (!izq.compuesta && !der.compuesta) {
            string tIzq = TipoDeValor(izq.tok);
            string tDer = TipoDeValor(der.tok);
            // ERR_OP_03: Operador Relacional en Tipos Invalidos.
            if (tIzq != "IDENTIFICADOR" && tDer != "IDENTIFICADOR" &&
                tIzq != "DESCONOCIDO" && tDer != "DESCONOCIDO" && tIzq != tDer) {
                RegistrarError(CodErrSem::ERR_OP_03,
                    "Operador relacional '" + opTexto + "' entre tipos incompatibles (" + tIzq +
                    " vs " + tDer + ") en " + contexto + ".", lineaOp);
            }
            // ERR_LOG_03: Condicion Vacia o Inalcanzable (dos literales booleanos constantes).
            if ((izq.tok == TOK_VERDADERO || izq.tok == TOK_FALSO) &&
                (der.tok == TOK_VERDADERO || der.tok == TOK_FALSO)) {
                RegistrarError(CodErrSem::ERR_LOG_03,
                    "La condicion en " + contexto + " compara dos literales booleanos constantes; " +
                    "su resultado es siempre el mismo (condicion inalcanzable o trivial).", lineaOp);
            }
        }

        if (colectorCondicionCuando) {
            colectorCondicionCuando->operandoIzq = izq.texto;
            colectorCondicionCuando->operandoDer = der.texto;
            colectorCondicionCuando->operador    = opTexto;
            colectorCondicionCuando->tokIzq      = izq.tok;
            colectorCondicionCuando->tokDer      = der.tok;
            colectorCondicionCuando->linea       = lineaOp;
        }
    }

    // Expresion ::= Termino { ("+"|"-") Termino }
    OperandoInfo Expresion(const string &contexto) {
        OperandoInfo izq = Termino(contexto);
        while (tokActual == TOK_MAS || tokActual == TOK_MENOS) {
            int lineaOp = lineaTok;
            avanzar();
            OperandoInfo der = Termino(contexto);
            // ERR_OP_02: Operador Matematico en Tipos Invalidos.
            if (EsNoNumerico(izq.tok) || EsNoNumerico(der.tok)) {
                RegistrarError(CodErrSem::ERR_OP_02,
                    "Operador aritmetico aplicado a un operando no numerico en " + contexto + ".", lineaOp);
            }
            izq = OperandoInfo{-1, "(expresion)", true};
        }
        return izq;
    }

    // Termino ::= Factor { ("*"|"/") Factor }
    OperandoInfo Termino(const string &contexto) {
        OperandoInfo izq = Factor(contexto);
        while (tokActual == TOK_MULT || tokActual == TOK_DIV) {
            int opTok = tokActual;
            int lineaOp = lineaTok;
            avanzar();
            OperandoInfo der = Factor(contexto);
            // ERR_OP_01: Division por Cero.
            if (opTok == TOK_DIV && der.tok == TOK_NUMERO && EsCero(der.texto)) {
                RegistrarError(CodErrSem::ERR_OP_01,
                    "Division por cero detectada en " + contexto + ".", lineaOp);
            }
            // ERR_OP_02: Operador Matematico en Tipos Invalidos.
            if (EsNoNumerico(izq.tok) || EsNoNumerico(der.tok)) {
                RegistrarError(CodErrSem::ERR_OP_02,
                    "Operador aritmetico aplicado a un operando no numerico en " + contexto + ".", lineaOp);
            }
            izq = OperandoInfo{-1, "(expresion)", true};
        }
        return izq;
    }

    static bool EsNoNumerico(int tok) {
        return tok == TOK_CADENA || tok == TOK_VERDADERO || tok == TOK_FALSO;
    }

    static bool EsCero(const string &numTexto) {
        try { return stod(numTexto) == 0.0; } catch (...) { return false; }
    }

    // Factor ::= NUMERO | CADENA | TRUE | FALSE | ID ["." ID] | "(" Expresion ")"
    OperandoInfo Factor(const string &contexto) {
        switch (tokActual) {
            case TOK_NUMERO: {
                OperandoInfo o{TOK_NUMERO, numActual, false};
                avanzar();
                return o;
            }
            case TOK_CADENA: {
                OperandoInfo o{TOK_CADENA, cadActual, false};
                avanzar();
                return o;
            }
            case TOK_VERDADERO:
            case TOK_FALSO: {
                OperandoInfo o{tokActual, (tokActual == TOK_VERDADERO ? "true" : "false"), false};
                avanzar();
                return o;
            }
            case TOK_PAR_A: {
                avanzar();
                OperandoInfo o = Expresion(contexto);
                esperar(TOK_PAR_C, "')' al cerrar la expresion en " + contexto);
                o.compuesta = true;
                return o;
            }
            case TOK_IDENTIFICADOR: {
                string nombre = lexActual;
                avanzar();
                VerificarUso(nombre, contexto);
                string nombreBase = nombre;
                if (tokActual == TOK_PUNTO) {
                    avanzar();
                    string attrNombre = (tokActual == TOK_IDENTIFICADOR) ? lexActual : nombreToken(tokActual);
                    esperarNombreAtributo("el acceso '" + nombre + ".' en " + contexto);
                    string categoria = categoriaDeNombre.count(nombreBase) ? categoriaDeNombre[nombreBase] : "";
                    VerificarPropiedadValida(categoria, attrNombre, "'" + nombreBase + "'");
                }
                return OperandoInfo{TOK_IDENTIFICADOR, nombre, false};
            }
            default:
                if (esNombreAtributoValido(tokActual)) {
                    // Palabras como "prioridad" tambien pueden usarse como
                    // operando (variable global homonima de un atributo).
                    string nombre = nombreToken(tokActual);
                    avanzar();
                    VerificarUso(nombre, contexto);
                    if (tokActual == TOK_PUNTO) {
                        avanzar();
                        esperarNombreAtributo("el acceso '" + nombre + ".' en " + contexto);
                    }
                    return OperandoInfo{TOK_IDENTIFICADOR, nombre, false};
                }
                throw ErrorSintactico{
                    "Se esperaba un operando valido (numero, cadena, true/false, identificador o '(') en " +
                    contexto + ", se encontro " + nombreToken(tokActual) + ".",
                    lineaTok
                };
        }
    }

    //------------------------------------------------
    // q97 HUB ACCIONES
    //------------------------------------------------
    void Accion(const string &contexto) {
        switch (tokActual) {
            case TOK_ALERTA:      LlamadaAlerta();            break;
            case TOK_REGISTRAR:   LlamadaRegistrar(contexto); break;
            case TOK_DISPARAR:    LlamadaDisparar(contexto);  break;
            case TOK_EVALUAR:     LlamadaEvaluar(contexto);   break;
            case TOK_ESTABLECER:  LlamadaEstablecer(contexto);break;
            case TOK_CALCULAR:    LlamadaCalcular(contexto);  break;
            case TOK_SALIDA:      AsignacionSalida();         break;
            default:
                if (tokActual == TOK_IDENTIFICADOR || esNombreAtributoValido(tokActual)) {
                    AsignacionGenerica(contexto);
                    break;
                }
                throw ErrorSintactico{
                    "Sentencia no valida dentro del bloque de acciones de " + contexto +
                    ", se encontro " + nombreToken(tokActual) + ".",
                    lineaTok
                };
        }
    }

    // q116-q120 : alerta "(" CADENA ")" ";"
    void LlamadaAlerta() {
        int lineaInicio = lineaTok;
        esperar(TOK_ALERTA, "la funcion 'alerta'");
        esperar(TOK_PAR_A, "'(' luego de 'alerta'");
        string cadenaTexto = cadActual;
        esperar(TOK_CADENA, "una cadena de texto como argumento de 'alerta'");
        esperar(TOK_PAR_C, "')' al cerrar la llamada a 'alerta'");
        esperar(TOK_PCOMA, "';' al finalizar la llamada a 'alerta'");
        if (colectorAcciones) colectorAcciones->push_back(AccInfo{"alerta", cadenaTexto, "", lineaInicio});
    }

    // q122-q128 : registrar "(" (estado|alerta|ID) "," CADENA ")" ";"
    void LlamadaRegistrar(const string &contexto) {
        int lineaInicio = lineaTok;
        esperar(TOK_REGISTRAR, "la funcion 'registrar'");
        esperar(TOK_PAR_A, "'(' luego de 'registrar'");
        string primerArg = (tokActual == TOK_IDENTIFICADOR) ? lexActual : nombreToken(tokActual);
        if (tokActual == TOK_ESTADO || tokActual == TOK_ALERTA || tokActual == TOK_IDENTIFICADOR) {
            avanzar();
        } else {
            throw ErrorSintactico{
                "Se esperaba 'estado', 'alerta' o un identificador como primer argumento de 'registrar' en " +
                contexto + ", se encontro " + nombreToken(tokActual) + ".",
                lineaTok
            };
        }
        esperar(TOK_COMA, "',' entre los argumentos de 'registrar'");
        string cadenaTexto = cadActual;
        esperar(TOK_CADENA, "una cadena de texto como segundo argumento de 'registrar'");
        esperar(TOK_PAR_C, "')' al cerrar la llamada a 'registrar'");
        esperar(TOK_PCOMA, "';' al finalizar la llamada a 'registrar'");
        if (colectorAcciones) colectorAcciones->push_back(AccInfo{"registrar", primerArg, cadenaTexto, lineaInicio});
    }

    // q101-q104 : disparar "(" ID ")" ";"  (ID debe ser evento o regla)
    void LlamadaDisparar([[maybe_unused]] const string &contexto) {
        int lineaInicio = lineaTok;
        esperar(TOK_DISPARAR, "la funcion 'disparar'");
        esperar(TOK_PAR_A, "'(' luego de 'disparar'");
        string nombre = lexActual;
        esperar(TOK_IDENTIFICADOR, "un identificador de evento o regla como argumento de 'disparar'");
        // ERR_SEM_05 (Evento No Declarado al Disparar) se valida en el pase
        // semantico GLOBAL (AnalisisSemantico), no aqui: 'disparar' puede
        // referenciar hacia adelante una regla que todavia no se ha
        // reconocido en este recorrido de una sola pasada (p.ej. la regla A
        // dispara a la regla B declarada mas abajo en el mismo archivo).
        esperar(TOK_PAR_C, "')' al cerrar la llamada a 'disparar'");
        esperar(TOK_PCOMA, "';' al finalizar la llamada a 'disparar'");
        if (colectorAcciones) colectorAcciones->push_back(AccInfo{"disparar", nombre, "", lineaInicio});
    }

    // q130-q134 : evaluar "(" Condicion ")" ";"
    void LlamadaEvaluar(const string &contexto) {
        int lineaInicio = lineaTok;
        esperar(TOK_EVALUAR, "la funcion 'evaluar'");
        esperar(TOK_PAR_A, "'(' luego de 'evaluar'");
        Condicion(contexto);
        esperar(TOK_PAR_C, "')' al cerrar la llamada a 'evaluar'");
        esperar(TOK_PCOMA, "';' al finalizar la llamada a 'evaluar'");
        if (colectorAcciones) colectorAcciones->push_back(AccInfo{"evaluar", "", "", lineaInicio});
    }

    // q98-q100 : establecer "(" NombreAtributo "," (true|false) ")" ";"
    void LlamadaEstablecer(const string &contexto) {
        int lineaInicio = lineaTok;
        esperar(TOK_ESTABLECER, "la funcion 'establecer'");
        esperar(TOK_PAR_A, "'(' luego de 'establecer'");
        string nombreAttr;
        if (tokActual == TOK_IDENTIFICADOR || esNombreAtributoValido(tokActual)) {
            nombreAttr = (tokActual == TOK_IDENTIFICADOR) ? lexActual : nombreToken(tokActual);
            avanzar();
        } else {
            throw ErrorSintactico{
                "Se esperaba un nombre de atributo como primer argumento de 'establecer' en " +
                contexto + ", se encontro " + nombreToken(tokActual) + ".",
                lineaTok
            };
        }
        esperar(TOK_COMA, "',' entre los argumentos de 'establecer'");
        string valorTexto;
        if (tokActual == TOK_VERDADERO || tokActual == TOK_FALSO) {
            valorTexto = (tokActual == TOK_VERDADERO ? "true" : "false");
            avanzar();
        } else {
            throw ErrorSintactico{
                "Se esperaba 'true' o 'false' como segundo argumento de 'establecer' en " +
                contexto + ", se encontro " + nombreToken(tokActual) + ".",
                lineaTok
            };
        }
        esperar(TOK_PAR_C, "')' al cerrar la llamada a 'establecer'");
        esperar(TOK_PCOMA, "';' al finalizar la llamada a 'establecer'");
        if (colectorAcciones) colectorAcciones->push_back(AccInfo{"establecer", nombreAttr, valorTexto, lineaInicio});
    }

    // Extension no presente en el documento original:
    // calcular "(" ID "," ("conflicto"|"acuerdo"|"consenso") ")" ";"
    void LlamadaCalcular(const string &contexto) {
        int lineaInicio = lineaTok;
        esperar(TOK_CALCULAR, "la funcion 'calcular'");
        esperar(TOK_PAR_A, "'(' luego de 'calcular'");
        string nombre = lexActual;
        esperar(TOK_IDENTIFICADOR, "un identificador de variable como primer argumento de 'calcular'");
        VerificarUso(nombre, contexto);
        // ERR_TYP_03: Firma de Metodo Invalida (argumento de una categoria
        // que 'calcular' no puede procesar, p.ej. una regla o un escenario).
        Atributos a;
        if (ts.Buscar(nombre, a) && (a.tipo == "regla" || a.tipo == "escenario" || a.tipo == "simulacion")) {
            RegistrarError(CodErrSem::ERR_TYP_03,
                "'calcular' recibe '" + nombre + "' (" + a.tipo + ") como argumento, pero se espera un " +
                "actor, red, coalicion o variable numerica.", lineaTok);
        }
        esperar(TOK_COMA, "',' entre los argumentos de 'calcular'");
        if (tokActual == TOK_CONFLICTO || tokActual == TOK_ACUERDO || tokActual == TOK_CONSENSO) {
            avanzar();
        } else {
            throw ErrorSintactico{
                "Se esperaba una metrica valida ('conflicto', 'acuerdo' o 'consenso') como segundo argumento de 'calcular' en " +
                contexto + ", se encontro " + nombreToken(tokActual) + ".",
                lineaTok
            };
        }
        esperar(TOK_PAR_C, "')' al cerrar la llamada a 'calcular'");
        esperar(TOK_PCOMA, "';' al finalizar la llamada a 'calcular'");
        if (colectorAcciones) colectorAcciones->push_back(AccInfo{"calcular", nombre, "", lineaInicio});
    }

    // q136-q138 : salida "=" (true|false) ";"
    void AsignacionSalida() {
        int lineaInicio = lineaTok;
        esperar(TOK_SALIDA, "la palabra clave 'salida'");
        esperar(TOK_IGUAL, "'=' luego de 'salida'");
        string valorTexto;
        if (tokActual == TOK_VERDADERO || tokActual == TOK_FALSO) {
            valorTexto = (tokActual == TOK_VERDADERO ? "true" : "false");
            avanzar();
        } else {
            throw ErrorSintactico{
                "Se esperaba 'true' o 'false' luego de 'salida =', se encontro " +
                nombreToken(tokActual) + ".",
                lineaTok
            };
        }
        esperar(TOK_PCOMA, "';' al finalizar la asignacion de 'salida'");
        if (colectorAcciones) colectorAcciones->push_back(AccInfo{"salida", valorTexto, "", lineaInicio});
    }

    // q105-q114 : ID ["." ID] "=" Expresion ";"
    void AsignacionGenerica(const string &contexto) {
        int lineaInicio = lineaTok;
        string nombre;
        if (tokActual == TOK_IDENTIFICADOR) {
            nombre = lexActual;
        } else if (esNombreAtributoValido(tokActual)) {
            nombre = nombreToken(tokActual);
        } else {
            throw ErrorSintactico{
                "Se esperaba un identificador en " + contexto +
                ", se encontro " + nombreToken(tokActual) + ".",
                lineaTok
            };
        }
        avanzar();
        VerificarUso(nombre, contexto);
        string categoria = categoriaDeNombre.count(nombre) ? categoriaDeNombre[nombre] : "";
        if (tokActual == TOK_PUNTO) {
            avanzar();
            string attrNombre = (tokActual == TOK_IDENTIFICADOR) ? lexActual : nombreToken(tokActual);
            esperarNombreAtributo("el acceso '" + nombre + ".' en " + contexto);
            // ERR_DOM_01 / ERR_DOM_02: propiedad valida / de solo lectura.
            VerificarPropiedadValida(categoria, attrNombre, "'" + nombre + "'");
            VerificarPropiedadSoloLectura(nombre, attrNombre, contexto);
        } else if (!categoria.empty() && categoria != "variable") {
            // ERR_SCP_01: Violacion de Ambito Cerrado (modificar un
            // componente del dominio sin usar la sintaxis de punto).
            RegistrarError(CodErrSem::ERR_SCP_01,
                "Se modifica '" + nombre + "' directamente sin usar la sintaxis de punto ('" + nombre +
                ".atributo = ...') requerida para componentes del dominio (" + categoria + ").", lineaTok);
        }
        esperar(TOK_IGUAL, "'=' en la asignacion de '" + nombre + "' en " + contexto);
        Expresion(contexto);
        esperar(TOK_PCOMA, "';' al finalizar la asignacion de '" + nombre + "' en " + contexto);
        if (colectorAcciones) colectorAcciones->push_back(AccInfo{"asignacion", nombre, "", lineaInicio});
    }

    //------------------------------------------------
    // q8 HUB ESCENARIO
    //------------------------------------------------
    void ItemEscenario(const string &nombreEscenario) {
        switch (tokActual) {
            case TOK_ACTOR:
                DeclBloqueSimple(TOK_ACTOR, "actor", "actor", nombreEscenario);
                break;
            case TOK_REGLA:
                DeclRegla(nombreEscenario);
                break;
            case TOK_NEGOCIACION:
                DeclBloqueSimple(TOK_NEGOCIACION, "negociacion", "negociacion", nombreEscenario);
                break;
            case TOK_RED:
                DeclBloqueSimple(TOK_RED, "red", "red", nombreEscenario);
                break;
            case TOK_EVENTO:
                DeclBloqueSimple(TOK_EVENTO, "evento", "evento", nombreEscenario);
                break;
            case TOK_COALICION:
                DeclBloqueSimple(TOK_COALICION, "coalicion", "coalicion", nombreEscenario);
                break;
            default:
                throw ErrorSintactico{
                    "Elemento no valido dentro del escenario '" + nombreEscenario +
                    "' (se esperaba actor, regla, negociacion, red, evento o coalicion), se encontro " +
                    nombreToken(tokActual) + ".",
                    lineaTok
                };
        }
    }

    // q6-q8 : escenario ID "{" { ItemEscenario } "}"
    void DeclEscenario() {
        esperar(TOK_ESCENARIO, "la palabra clave 'escenario'");
        int lineaDecl = lineaTok;
        string nombre = lexActual;
        esperar(TOK_IDENTIFICADOR, "un identificador de escenario");
        DeclararSimbolo(nombre, TOK_IDENTIFICADOR, "escenario", VACIO, VACIO, lineaDecl);
        esperar(TOK_LLAVE_A, "'{' al iniciar el escenario '" + nombre + "'");
        while (tokActual != TOK_LLAVE_C && tokActual != TOK_FIN) {
            ItemEscenario(nombre);
        }
        esperar(TOK_LLAVE_C, "'}' al cerrar el escenario '" + nombre + "'");
    }

    //------------------------------------------------
    // q75 HUB SIMULAR
    //------------------------------------------------
    void ItemSimular(const string &nombreSimulacion) {
        switch (tokActual) {
            case TOK_ESCENARIO: {
                // q76-q78 : ESCENARIO "=" ID ";"
                avanzar();
                esperar(TOK_IGUAL, "'=' luego de 'escenario' dentro de '" + nombreSimulacion + "'");
                int lineaRef = lineaTok;
                string refEscenario = lexActual;
                esperar(TOK_IDENTIFICADOR, "un identificador de escenario dentro de '" + nombreSimulacion + "'");
                // ERR_SEM_04: Escenario No Encontrado.
                // ERR_SEM_04 (Escenario No Encontrado) se valida en el pase
                // semantico GLOBAL: 'simular' puede referenciar un escenario
                // declarado mas abajo en el mismo archivo.
                BloqueInfo *bi = ContextoActual();
                if (bi) {
                    bi->escenarioRef = refEscenario;
                    bi->escenarioRefLinea = lineaRef;
                    bi->tieneEscenarioRef = true;
                }
                esperar(TOK_PCOMA, "';' luego de 'escenario = " + refEscenario + "'");
                break;
            }
            case TOK_VARIABLE:
                // q79-q82 : VARIABLE ID "=" Valor ";"
                DeclVariable();
                break;
            case TOK_CUANDO:
                BloqueCuando("la simulacion '" + nombreSimulacion + "'");
                break;
            case TOK_CALCULAR:
                LlamadaCalcular("la simulacion '" + nombreSimulacion + "'");
                break;
            default:
                throw ErrorSintactico{
                    "Elemento no valido dentro de 'simular " + nombreSimulacion +
                    "' (se esperaba escenario, variable, cuando o calcular), se encontro " +
                    nombreToken(tokActual) + ".",
                    lineaTok
                };
        }
    }

    // q73-q75 : simular ID "{" { ItemSimular } "}"
    void DeclSimular() {
        esperar(TOK_SIMULAR, "la palabra clave 'simular'");
        int lineaDecl = lineaTok;
        string nombre = lexActual;
        esperar(TOK_IDENTIFICADOR, "un identificador de simulacion");
        DeclararSimbolo(nombre, TOK_IDENTIFICADOR, "simulacion", VACIO, VACIO, lineaDecl);

        registroBloques.push_back(BloqueInfo{"simulacion", nombre, "", lineaDecl, {}, {}, "", -1, false});
        pilaContexto.push_back(&registroBloques.back());

        esperar(TOK_LLAVE_A, "'{' al iniciar 'simular " + nombre + "'");
        while (tokActual != TOK_LLAVE_C && tokActual != TOK_FIN) {
            ItemSimular(nombre);
        }
        esperar(TOK_LLAVE_C, "'}' al cerrar 'simular " + nombre + "'");

        // ERR_LOG_01: Simulacion sin Escenario Base.
        if (!pilaContexto.back()->tieneEscenarioRef) {
            RegistrarError(CodErrSem::ERR_LOG_01,
                "La simulacion '" + nombre + "' no define 'escenario = ...' (simulacion sin escenario base).",
                lineaDecl, true);
        }
        pilaContexto.pop_back();
    }

    //------------------------------------------------
    // q0_INICIAL / q1_LISTO : Programa ::= { DeclVariable | DeclEscenario | DeclSimular }
    //------------------------------------------------
    void Programa() {
        while (tokActual != TOK_FIN) {
            switch (tokActual) {
                case TOK_VARIABLE:  DeclVariable();  break;
                case TOK_ESCENARIO: DeclEscenario(); break;
                case TOK_SIMULAR:   DeclSimular();   break;
                default:
                    throw ErrorSintactico{
                        "Se esperaba 'variable', 'escenario' o 'simular' al nivel global, se encontro " +
                        nombreToken(tokActual) + ".",
                        lineaTok
                    };
            }
        }
    }

    //------------------------------------------------
    // Punto de entrada del analisis sintactico-semantico
    //------------------------------------------------
    bool Sintactico() {
        i           = 0;
        lineaActual = 1;
        try {
            avanzar();
            Programa();
            cout << "\n--- ANALISIS SINTACTICO FINALIZADO SIN ERRORES ---\n";

            // La fase semantica se ejecuta INMEDIATAMENTE despues de que la
            // fase sintactica concluye sin errores, tal como lo exige el
            // requerimiento: primero se valida la gramatica del DSL y,
            // solo si esta es correcta, se recorre el "AST resumido"
            // (registroBloques) para aplicar las 11 tablas de validacion
            // semantica sobre el programa completo.
            AnalisisSemantico();
            MostrarErroresSemanticos();

            if (HuboErroresSemanticosBloqueantes()) {
                cout << "\n--- ANALISIS SEMANTICO FINALIZADO CON ERRORES ---\n";
                return false;
            }
            cout << "\n--- ANALISIS SEMANTICO FINALIZADO SIN ERRORES BLOQUEANTES ---\n";
            return true;
        } catch (ErrorSintactico &e) {
            cout << "\nERROR SINTACTICO (linea " << e.linea << "): " << e.mensaje << "\n";
            return false;
        }
    }

    void MostrarTablaSimbolos() {
        cout << "\n========== TABLA DE SIMBOLOS ==========\n";
        ts.Mostrar();
    }

    string getLexema() { return ultimoLexema; }
    string getNumero() { return ultimoNumero; }
    string getCadena() { return ultimaCadena; }
    int    getLinea()  { return lineaActual;  }
};

//====================================================
// MAIN
//====================================================
int main(int argc, char* argv[]) {
    Analisis obj;
    string rutaFuente = "ejemplos_de_prueba.csl";

    if (argc > 1) {
        rutaFuente = argv[1];
    }

    if (!obj.leerArchivo(rutaFuente.c_str())) {
        cout << "No se pudo abrir el archivo fuente: " << rutaFuente << endl;
        return 1;
    }

    bool okLexico = obj.Lexico();

    if (okLexico) {
        bool okSintactico = obj.Sintactico();
        obj.MostrarTablaSimbolos();

        if (okSintactico) {
            cout << "\n----------------------------------------------------------------------\n";
            cout << "\nAnalisis completo correcto: el programa cumple la gramatica del DSL\n";
            cout << "y no presenta errores semanticos bloqueantes.\n";
            cout << "\n----------------------------------------------------------------------\n";
        } else {
            cout << "\n----------------------------------------------------------------------\n";
            cout << "\nEl programa no supero el analisis sintactico-semantico completo.\n";
            cout << "Revise los errores reportados arriba.\n";
            cout << "\n----------------------------------------------------------------------\n";
        }
    }

    return 0;
}