/**
 * MC558 - Teste 06
 * Tiago de Paula - RA 187679
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __GNUC__
// Atributos do GCC
#define attribute(...) __attribute__((__VA_ARGS__))
// Marcador de branch improvável (usado para erros)
#define unlikely(x)    (__builtin_expect((x), 0))

#else
// Fora do GCC, as macros não fazem nada.
#define attribute(...)
#define unlikely(x)      (x)
#endif


/* Identificador de um nó (projeto). */
typedef uint16_t id_t;
/* Tempo de execução do projeto. */
typedef uint16_t tempo_t;

/**
 *  Representação do nó por lista de adjacências.
 */
typedef struct no {
    // se o nó é uma fonte
    bool fonte;
    // tempo de conclusão
    tempo_t tempo;
    // qtde de nós adjacentes
    unsigned tam;
    // vértices adjacentes
    id_t *restrict adj;
} no_t;

/**
 *  Grafo representado por lista de  vértices
 * com suas respectivas listas de adjacências.
 */
typedef struct grafo {
    unsigned tam;
    no_t no[];
} grafo_t;


/* * * * * * * * * *
 * ENTRADA E SAÍDA *
 * * * * * * * * * */

static attribute(cold, nonnull, nothrow)
/**
 * Libera a memória usada para representar o grafo.
 */
void free_grafo(grafo_t *grafo) {
    for (unsigned i = 0; i < grafo->tam; i++) {
        if (grafo->no[i].adj != NULL) {
            free(grafo->no[i].adj);
        }
    }
    free(grafo);
}

static inline attribute(malloc, hot, nothrow)
/**
 *  Alocação do grafo de 'N' vértices na memória.
 * Os tempos são inicializados como 0.
 *
 * Retorna NULL em caso de erro.
 */
grafo_t *alloc_grafo(unsigned N) {
    // memória principal do grafo
    size_t fixo = offsetof(grafo_t, no);
    grafo_t *g = malloc(fixo + N * sizeof(no_t));
    if unlikely(g == NULL) return NULL;
    g->tam = N;

    // marca os nós (ainda sem aresta) como fontes
    for (unsigned i = 0; i < N; i++) {
        g->no[i] = (no_t) {
            .fonte = true,
            .tempo = 0,
            .tam = 0,
            .adj = NULL
        };
    }
    return g;
}

static inline attribute(const, hot, nothrow)
/**
 * Checa se um nó está cheio (tamanho é uma potência de 2).
 */
bool no_cheio(unsigned tam) {
    return tam == 0 || __builtin_popcount(tam) == 1;
}

static inline attribute(nonnull, hot, nothrow)
/**
 * Insere 'adj' na lista de adjacências de 'no'.
 *
 *  Retorna 'true' em caso de sucesso e
 * 'false' em erro de alocação.
 */
bool insere_adj(no_t *no, id_t adj) {
    // se a lista estiver cheia
    if unlikely(no_cheio(no->tam)) {
        id_t *novo;
        // aloca nova
        if unlikely(no->adj == NULL) {
            novo = malloc(sizeof(id_t));
        // ou realoca
        } else {
            novo = realloc(no->adj, 2 * no->tam * sizeof(id_t));
        }
        if unlikely(novo == NULL) return false;

        no->adj = novo;
    }

    no->adj[no->tam++] = adj;
    return true;
}

static inline attribute(malloc, hot, nothrow)
/**
 *  Construção de um grafo de 'N' vértices
 * e leitura dos suas 'M' arestas.
 *
 *  Retorna NULL em caso de erro de alocação
 * ou de leitura.
 */
grafo_t *ler_grafo(void) {
    // dimensões do grafo
    unsigned N, M;
    int rv = scanf("%u %u", &N, &M);
    if unlikely(rv < 2 || N >= UINT16_MAX || N == 0) return NULL;
    // o grafo propriamente
    grafo_t *g = alloc_grafo(N);
    if unlikely(g == NULL) NULL;

    // leitura dos tempos de conclusão
    for (unsigned i = 0; i < N; i++) {
        tempo_t tempo;
        rv = scanf("%"SCNu16, &tempo);
        if unlikely(rv < 1) {
            free_grafo(g);
            return NULL;
        }
        g->no[i].tempo = tempo;
    }

    // leitura das dependências
    for (unsigned i = 0; i < M; i++) {
        unsigned u, v;
        int rv = scanf("%u %u", &u, &v);
        // os nós devem ser válidos e a aresta também
        if unlikely(rv < 2 || u >= N || u >= N) {
            free_grafo(g);
            return NULL;
        }
        // insere a nova aresta
        bool ok = insere_adj(&g->no[u], v);
        if unlikely(!ok) {
            free_grafo(g);
            return NULL;
        }
        // e marca o nó adjacente como não-fonte
        g->no[v].fonte = false;
    }
    return g;
}

static inline
/**
 * Cálculo do tempo total dos projetos.
 *
 *  Retorna UINT64_MAX em erro de alocação
 * ou se o grafo contém ciclos.
 */
uint64_t tempo_total(const grafo_t *g)
attribute(pure, hot, nonnull, nothrow);

int main(void) {
    // leitura do grafo
    grafo_t *grafo = ler_grafo();
    if unlikely(grafo == NULL) return EXIT_FAILURE;

    // cálculo do tempo
    uint64_t tempo = tempo_total(grafo);
    free_grafo(grafo);
    if unlikely(tempo == UINT64_MAX) return EXIT_FAILURE;

    // exibição do resultado
    printf("%"PRIu64"\n", tempo);
    return EXIT_SUCCESS;
}


/* * * * * * * *
 * TEMPO TOTAL *
 * * * * * * * */


// Status de visitação do vértice.
typedef enum status {
    NAOVISTADO = 0,
    VISITANDO  = 1,
    FINALIZADO = 2
} attribute(packed) status_t;


static inline attribute(hot, nonnull, nothrow)
/**
 *  Parte recursiva do DFS, marcando a ordem de
 * finalização de cada nó.
 *
 *  Retorna o novo tamanho da lista 'ordem' ou
 * 0 se alguma aresta de retorno (ou seja, um
 * ciclo) for encontrada.
 */
unsigned dfs(const grafo_t *restrict g, id_t no, status_t *restrict status, id_t *restrict ordem, unsigned tam) {
    // inicia etapa de visitação
    status[no] = VISITANDO;

    // visita os adjacentes
    for (unsigned i = g->no[no].tam; i > 0; i--) {
        id_t adj = g->no[no].adj[i-1];
        // mas só os não visitados
        if (status[adj] == NAOVISTADO) {
            tam = dfs(g, adj, status, ordem, tam);
            if unlikely(tam == 0) return 0;
        // aresta de retorno, grafo contém ciclo
        } else if (status[adj] == VISITANDO) {
            return 0;
        }
    }
    // finaliza o vértice
    status[no] = FINALIZADO;
    // e insere em ordem decrescente
    ordem[g->tam - ++tam] = no;
    return tam;
}

static inline attribute(malloc, hot, nonnull, nothrow)
/**
 *  Encontra a ordem topológica do grafo.
 *
 * Retorna NULL em erro de alocação ou
 * se o grafo contém ciclos.
 */
id_t *ordem_topologica(const grafo_t *g) {
    unsigned N = g->tam;
    // status de visitação de cada nó
    status_t *status = calloc(N, sizeof(status_t));
    if unlikely(status == NULL) return NULL;
    // vértices em ordem topológica
    id_t *ordem = malloc(N * sizeof(id_t));
    if unlikely(ordem == NULL) {
        free(status);
        return NULL;
    }

    unsigned tam = 0;
    for (unsigned i = 0; i < N; i++) {
        // visita apenas as fontes
        if (g->no[i].fonte) {
            tam = dfs(g, i, status, ordem, tam);
            if unlikely(tam == 0) break;
        }
    }
    free(status);
    // ciclo encontrado
    if unlikely(tam == 0) {
        free(ordem);
        return NULL;
    }
    return ordem;
}

static inline attribute(pure, hot, nonnull, nothrow)
/* Tempo total dos projetos. */
uint64_t tempo_total(const grafo_t *g) {
    unsigned N = g->tam;
    // ordena os vértice
    id_t *ordem = ordem_topologica(g);
    if unlikely(ordem == NULL) return UINT64_MAX;
    // tempo de finalização de cada projeto
    uint64_t *tempo = malloc(N * sizeof(uint64_t));
    if unlikely(tempo == NULL) {
        free(ordem);
        return UINT64_MAX;
    }
    uint64_t max = 0;
    // iniciliza desconsiderando as dependências
    for (unsigned i = 0; i < N; i++) {
        tempo[i] = g->no[i].tempo;
        if (tempo[i] > max) {
            max = tempo[i];
        }
    }

    // para cada nó em ordem topológica
    for (unsigned i = 0; i < N; i++) {
        id_t no = ordem[i];
        // tempo do projeto 'no'
        unsigned cur = tempo[no];

        // para cada nó dependente
        for (unsigned j = g->no[no].tam; j > 0; j--) {
            id_t adj = g->no[no].adj[j-1];
            // tempo considerando a dependencia 'no -> adj'
            uint64_t temp = cur + g->no[adj].tempo;
            if (temp > tempo[adj]) {
                // aumenta o tempo de 'adj'
                tempo[adj] = temp;
                // e calcula novo máximo
                if (temp > max) {
                    max = temp;
                }
            }
        }
    }

    free(ordem);
    free(tempo);
    // só retorna o máximo
    return max;
}
